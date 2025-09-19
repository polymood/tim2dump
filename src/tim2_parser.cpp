#include "tim2_parser.h"
#include <iostream>
#include <algorithm>

namespace tim2 {

// ─────────────────────────────────────────────────────────────
// Picture implementation
// ─────────────────────────────────────────────────────────────

/**
 * Turn one TIM2 picture into a flat RGBA8 buffer.
 *
 * - mipLevel 0 is the largest (top) level. If the picture has no mipmaps,
 *   mipLevel must be 0.
 * - We decode according to header.imageType. For indexed formats we fetch the CLUT,
 *   apply the right ordering rules (CSM1/compound), and output Color32.
 * - On invalid input (e.g., mipLevel out of range), we return an empty vector.
 *
 * NOTE: This method reads pixels one by one via getPixelColor(). That’s simple,
 * but not the fastest. If you need speed, consider a scanline decoder.
 */
std::vector<Color32> Picture::decodeImage(size_t mipLevel) const {
    if (mipLevel >= header.mipMapTextures) {
        return {};
    }

    const size_t width  = getMipMapWidth(mipLevel);
    const size_t height = getMipMapHeight(mipLevel);

    std::vector<Color32> result(width * height);

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            result[y * width + x] = getPixelColor(x, y, mipLevel);
        }
    }

    return result;
}

/**
 * Decode the CLUT (palette) into RGBA colors.
 *
 * - Returns empty if there is no CLUT.
 * - Handles CSM1 “compound” index reordering (TIM2 spec §4.5).
 * - Converts RGB16/24/32 entries to Color32.
 *
 * IMPORTANT (spec detail):
 *   For CSM1 + RGB16/24/32, the spec describes additional intra-block byte swaps
 *   (every 64/96/128 bytes). This implementation currently reorders indices for
 *   compound mode but does NOT perform those byte-block swaps. If you encounter
 *   incorrect colors for certain CSM1 CLUTs, implement that swap step here.
 *   (See TIM2 spec §4.5 for the exact byte swap rules.)
 */
std::vector<Color32> Picture::getClutColors() const {
    std::vector<Color32> colors;
    if (!header.hasClut()) return colors;

    colors.reserve(header.clutColors);
    const uint8_t* data = clutData.data();

    const PixelFormat fmt = header.getClutPixelFormat();
    const bool isCompound = header.isClutCompound();

    for (size_t i = 0; i < header.clutColors; ++i) {
        size_t index = i;

        // CSM1 “compound” mode shuffles palette indices inside 32-color blocks.
        // This implements the index remap table described in §4.5 (the 0..31 matrix).
        if (!header.isClutCSM2() && isCompound) {
            const size_t block    = i / 32;
            size_t       localIdx = i % 32;

            // This is the minimal reordering that matches the 32-entry table.
            // (It swaps [8..15] with [16..23] by +8 / -8.)
            if (localIdx >= 8 && localIdx < 16) {
                localIdx += 8;
            } else if (localIdx >= 16 && localIdx < 24) {
                localIdx -= 8;
            }

            index = block * 32 + localIdx;
        }

        Color32 color{};

        switch (fmt) {
            case TIM2_RGB16: {
                const size_t byteIdx = index * 2;
                const uint16_t val = *reinterpret_cast<const uint16_t*>(&data[byteIdx]);
                Color16 c16{val};
                color = c16.toColor32();
                break;
            }
            case TIM2_RGB24: {
                const size_t byteIdx = index * 3;
                color.r = data[byteIdx + 0];
                color.g = data[byteIdx + 1];
                color.b = data[byteIdx + 2];
                color.a = 255;
                break;
            }
            case TIM2_RGB32: {
                const size_t byteIdx = index * 4;
                color.r = data[byteIdx + 0];
                color.g = data[byteIdx + 1];
                color.b = data[byteIdx + 2];
                color.a = data[byteIdx + 3];
                break;
            }
            default:
                // Unknown CLUT format: leave default (transparent black)
                break;
        }

        colors.push_back(color);
    }

    return colors;
}

/**
 * Return the color of a single pixel at (x, y) for a given mip level.
 *
 * - True-color formats read directly from imageData.
 * - Indexed formats first read the index, then look up into the decoded CLUT.
 *
 * NOTE on RGB24:
 *   The GS stores RGB24 in a packed layout. The code below treats pixels as tightly
 *   packed 3-byte triplets laid sequentially. If you run into RGB24 TIM2s that look
 *   scrambled, you likely need to implement the exact GS swizzle/packing for 24-bit
 *   textures as described in the spec (§4.6). Consider that a future optimization.
 */
Color32 Picture::getPixelColor(size_t x, size_t y, size_t mipLevel) const {
    const size_t offset = getImageOffset(mipLevel);
    const size_t width  = getMipMapWidth(mipLevel);
    const uint8_t* data = imageData.data() + offset;

    Color32 result{};

    switch (header.getImagePixelFormat()) {
        case TIM2_RGB32: {
            const size_t idx = (y * width + x) * 4;
            result.r = data[idx + 0];
            result.g = data[idx + 1];
            result.b = data[idx + 2];
            result.a = data[idx + 3];
            break;
        }
        case TIM2_RGB24: {
            // Packed 3 bytes per pixel, no padding between pixels here.
            // See note above if you encounter layout mismatches.
            const size_t idx = (y * width + x) * 3;
            result.r = data[idx + 0];
            result.g = data[idx + 1];
            result.b = data[idx + 2];
            result.a = 255;
            break;
        }
        case TIM2_RGB16: {
            const size_t idx = (y * width + x) * 2;
            const uint16_t val = *reinterpret_cast<const uint16_t*>(&data[idx]);
            Color16 c16{val};
            result = c16.toColor32();
            break;
        }
        case TIM2_IDTEX8: {
            if (header.hasClut()) {
                const size_t idx = y * width + x;
                const uint8_t colorIdx = data[idx];
                const auto colors = getClutColors();
                if (colorIdx < colors.size()) {
                    result = colors[colorIdx];
                }
            }
            break;
        }
        case TIM2_IDTEX4: {
            if (header.hasClut()) {
                const size_t pixelIdx = y * width + x;
                const size_t byteIdx  = pixelIdx / 2;
                // Even pixel = low nibble, odd pixel = high nibble.
                const uint8_t packed  = data[byteIdx];
                const uint8_t colorIdx = (pixelIdx & 1) ? (packed >> 4) : (packed & 0x0F);
                const auto colors = getClutColors();
                if (colorIdx < colors.size()) {
                    result = colors[colorIdx];
                }
            }
            break;
        }
        default:
            // Unknown / unsupported format: return transparent black
            break;
    }

    return result;
}

/**
 * Compute the byte offset to the start of a given mip level within imageData.
 *
 * - If there’s no MIPMAP header (i.e., mipMapTextures == 1), level 0 always starts at 0.
 * - If present, mipMapHeader->sizes holds each level’s byte length (already 16-byte aligned).
 */
size_t Picture::getImageOffset(size_t mipLevel) const {
    if (!mipMapHeader || mipLevel == 0) return 0;

    size_t offset = 0;
    for (size_t i = 0; i < mipLevel && i < mipMapHeader->sizes.size(); ++i) {
        offset += mipMapHeader->sizes[i];
    }
    return offset;
}

/**
 * Width of a given mip level. We shift-right per level and clamp to at least 1.
 */
size_t Picture::getMipMapWidth(size_t level) const {
    return std::max<size_t>(1, header.imageWidth >> level);
}

/**
 * Height of a given mip level. We shift-right per level and clamp to at least 1.
 */
size_t Picture::getMipMapHeight(size_t level) const {
    return std::max<size_t>(1, header.imageHeight >> level);
}

// ─────────────────────────────────────────────────────────────
// TIM2Parser implementation
// ─────────────────────────────────────────────────────────────

/**
 * Load a TIM2 file and parse all pictures inside it.
 *
 * Steps:
 *   1) Read and validate the 16-byte FileHeader.
 *   2) Align to the file’s declared alignment (16 or 128).
 *   3) For each picture:
 *      - Read PictureHeader
 *      - If needed, read MipMapHeader
 *      - Read User Space (and parse ExtendedHeader/comment if present)
 *      - Align, then read Image data
 *      - Align, then read CLUT data
 */
bool TIM2Parser::loadFile(const std::string& filename) {
    m_valid = false;
    m_pictures.clear();
    m_lastError.clear();

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        m_lastError = "Failed to open file: " + filename;
        return false;
    }

    // (1) File header
    if (!parseFileHeader(file)) {
        return false;
    }

    const size_t alignment = m_fileHeader.getAlignment();

    // (2) Alignment after file header (TIM2 aligns picture blocks)
    skipAlignment(file, alignment);

    // (3) Pictures
    m_pictures.reserve(m_fileHeader.pictures);
    for (uint16_t i = 0; i < m_fileHeader.pictures; ++i) {
        Picture pic;
        if (!parsePicture(file, pic, alignment)) {
            m_lastError = "Failed to parse picture " + std::to_string(i);
            return false;
        }
        m_pictures.push_back(std::move(pic));
    }

    m_valid = true;
    return true;
}

/**
 * Read and sanity-check the 16-byte FileHeader.
 * We accept version != 0x04 with a warning, but continue anyway (many tools do).
 */
bool TIM2Parser::parseFileHeader(std::ifstream& file) {
    file.read(reinterpret_cast<char*>(&m_fileHeader), sizeof(FileHeader));

    if (!file) {
        m_lastError = "Failed to read file header";
        return false;
    }

    if (!m_fileHeader.isValid()) {
        m_lastError = "Invalid TIM2 file signature";
        return false;
    }

    if (m_fileHeader.formatVersion != TIM2_FORMAT_VERSION) {
        std::cerr << "Warning: Unknown TIM2 format version: 0x"
                  << std::hex << static_cast<int>(m_fileHeader.formatVersion)
                  << std::dec << "\n";
    }

    return true;
}

/**
 * Parse a single picture block:
 *   - PictureHeader
 *   - optional MipMapHeader
 *   - optional User Space (may start with ExtendedHeader)
 *   - aligned Image data
 *   - aligned CLUT data
 *
 * The “alignment” parameter comes from the file header (16 or 128).
 */
bool TIM2Parser::parsePicture(std::ifstream& file, Picture& pic, size_t alignment) {
    // Picture header (fixed 48 bytes)
    file.read(reinterpret_cast<char*>(&pic.header), sizeof(PictureHeader));
    if (!file) {
        m_lastError = "Failed to read picture header";
        return false;
    }

    // MIP map header (only if more than one level)
    if (pic.header.mipMapTextures > 1) {
        if (!parseMipMapHeader(file, pic)) {
            return false;
        }
    }

    // If headerSize is bigger than what we already consumed, the remainder is “user space”.
    size_t headerDataSize = sizeof(PictureHeader);
    if (pic.mipMapHeader) {
        size_t mipHeaderSize = 16 + pic.header.mipMapTextures * 4; // 16 = two u64 fields
        mipHeaderSize = alignOffset(mipHeaderSize, 16);
        headerDataSize += mipHeaderSize;
    }

    if (pic.header.headerSize > headerDataSize) {
        if (!parseUserSpace(file, pic)) {
            return false;
        }
    }

    // Jump to image data start (aligned)
    {
        const size_t currentPos = static_cast<size_t>(file.tellg());
        const size_t imageStart = alignOffset(currentPos, alignment);
        if (imageStart > currentPos) file.seekg(imageStart);
    }

    // Image data (may be 0 for CLUT-only pictures)
    if (pic.header.imageSize > 0) {
        if (!parseImageData(file, pic)) {
            return false;
        }
    }

    // Jump to CLUT data start (aligned)
    {
        const size_t currentPos = static_cast<size_t>(file.tellg());
        const size_t clutStart  = alignOffset(currentPos, alignment);
        if (clutStart > currentPos) file.seekg(clutStart);
    }

    // CLUT data (only for indexed formats; size may still be 0)
    if (pic.header.clutSize > 0) {
        if (!parseClutData(file, pic)) {
            return false;
        }
    }

    return true;
}

/**
 * Read the MIPMAP header:
 * - Two 64-bit GS registers (MIPTBP1/MIPTBP2)
 * - An array of level sizes (LV0..LVn), then pad to 16 bytes.
 */
bool TIM2Parser::parseMipMapHeader(std::ifstream& file, Picture& pic) {
    pic.mipMapHeader = MipMapHeader{};
    auto& mipmap = *pic.mipMapHeader;

    file.read(reinterpret_cast<char*>(&mipmap.gsMiptbp1), sizeof(uint64_t));
    file.read(reinterpret_cast<char*>(&mipmap.gsMiptbp2), sizeof(uint64_t));

    mipmap.sizes.resize(pic.header.mipMapTextures);
    for (uint8_t i = 0; i < pic.header.mipMapTextures; ++i) {
        file.read(reinterpret_cast<char*>(&mipmap.sizes[i]), sizeof(uint32_t));
    }

    // Header must end on a 16-byte boundary (TIM2 spec). Skip any padding.
    const size_t mipHeaderSize = 16 + pic.header.mipMapTextures * 4;
    const size_t paddedSize    = alignOffset(mipHeaderSize, 16);
    if (paddedSize > mipHeaderSize) {
        file.seekg(static_cast<std::streamoff>(paddedSize - mipHeaderSize), std::ios::cur);
    }

    return file.good();
}

/**
 * Read the “user space” bytes between headers and image data.
 *
 * If an ExtendedHeader is present at the start:
 *   - extHeader describes the valid user-space size (excluding alignment padding)
 *   - userData is the first extHeader.userDataSize bytes after the header
 *   - comment (if present) follows userData and is null-terminated ASCII
 *
 * If not present, we keep the whole blob as opaque userData.
 */
bool TIM2Parser::parseUserSpace(std::ifstream& file, Picture& pic) {
    size_t headerDataSize = sizeof(PictureHeader);
    if (pic.mipMapHeader) {
        size_t mipHeaderSize = 16 + pic.header.mipMapTextures * 4;
        mipHeaderSize = alignOffset(mipHeaderSize, 16);
        headerDataSize += mipHeaderSize;
    }

    const size_t userSpaceSize = pic.header.headerSize - headerDataSize;
    if (userSpaceSize == 0) return true;

    pic.userData.resize(userSpaceSize);
    file.read(reinterpret_cast<char*>(pic.userData.data()), userSpaceSize);

    // Probe for ExtendedHeader signature
    if (userSpaceSize >= sizeof(ExtendedHeader)) {
        ExtendedHeader extHdr{};
        std::memcpy(&extHdr, pic.userData.data(), sizeof(ExtendedHeader));

        if (extHdr.isValid()) {
            pic.extHeader = extHdr;

            // If there is a comment, it starts right after the user data
            const size_t commentOffset = sizeof(ExtendedHeader) + extHdr.userDataSize;

            // Both userSpaceSize and extHdr.userSpaceSize can exist; the latter
            // is the “valid” size without trailing alignment padding.
            const size_t endLimit = std::min(userSpaceSize, static_cast<size_t>(extHdr.userSpaceSize));

            if (commentOffset < endLimit) {
                const char* commentPtr = reinterpret_cast<const char*>(pic.userData.data() + commentOffset);
                const size_t maxLen    = endLimit - commentOffset;
                const size_t commentLen = strnlen(commentPtr, maxLen);
                pic.comment.assign(commentPtr, commentLen);
            }
        }
    }

    return file.good();
}

/**
 * Read raw image bytes (GS layout, not decoded).
 */
bool TIM2Parser::parseImageData(std::ifstream& file, Picture& pic) {
    pic.imageData.resize(pic.header.imageSize);
    file.read(reinterpret_cast<char*>(pic.imageData.data()), pic.header.imageSize);
    return file.good();
}

/**
 * Read raw CLUT bytes (not decoded).
 */
bool TIM2Parser::parseClutData(std::ifstream& file, Picture& pic) {
    pic.clutData.resize(pic.header.clutSize);
    file.read(reinterpret_cast<char*>(pic.clutData.data()), pic.header.clutSize);
    return file.good();
}

/**
 * Round “offset” up to the next multiple of “alignment”.
 * e.g., alignOffset(17, 16) == 32.
 */
size_t TIM2Parser::alignOffset(size_t offset, size_t alignment) {
    return ((offset + alignment - 1) / alignment) * alignment;
}

/**
 * Move the file cursor forward to the next aligned position.
 * Safe to call even if already aligned.
 */
void TIM2Parser::skipAlignment(std::ifstream& file, size_t alignment) {
    const size_t currentPos = static_cast<size_t>(file.tellg());
    const size_t alignedPos = alignOffset(currentPos, alignment);
    if (alignedPos > currentPos) {
        file.seekg(static_cast<std::streamoff>(alignedPos), std::ios::beg);
    }
}

} // namespace tim2
