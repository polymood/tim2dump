#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace tim2 {

#pragma pack(push, 1)

// TIM2 Format constants (from TIM2 spec)
constexpr uint8_t TIM2_FORMAT_VERSION = 0x04; // Official TIM2 version (0x04 per spec)
constexpr uint8_t TIM2_ALIGN_16  = 0x00;      // 16-byte alignment mode
constexpr uint8_t TIM2_ALIGN_128 = 0x01;      // 128-byte alignment mode

// Pixel format types (ImageType or ClutType low 6 bits)
enum PixelFormat : uint8_t {
    TIM2_NONE   = 0x00, // No image / no CLUT
    TIM2_RGB16  = 0x01, // 16-bit RGB (5:5:5:1)
    TIM2_RGB24  = 0x02, // 24-bit RGB
    TIM2_RGB32  = 0x03, // 32-bit RGBA
    TIM2_IDTEX4 = 0x04, // 4-bit indexed texture
    TIM2_IDTEX8 = 0x05  // 8-bit indexed texture
};

// CLUT storage mode flags (bit 7 of ClutType)
enum ClutMode : uint8_t {
    CLUT_CSM1 = 0x00, // CSM1 (sequential or compound layout)
    CLUT_CSM2 = 0x80  // CSM2 (direct layout in GS)
};

// File Header (16 bytes, fixed size, always at file start)
struct FileHeader {
    char     fileId[4];       // Must be 'T','I','M','2' to identify TIM2 file
    uint8_t  formatVersion;   // Format version; official spec uses 0x04
    uint8_t  formatId;        // Alignment: 0x00 = 16B, 0x01 = 128B
    uint16_t pictures;        // Number of picture data blocks in file
    char     reserved[8];     // Must be all 0x00 (padding for 16-byte header)

    bool isValid() const {
        return std::memcmp(fileId, "TIM2", 4) == 0;
    }

    size_t getAlignment() const {
        return (formatId == TIM2_ALIGN_128) ? 128 : 16;
    }
};

// Picture Header (48 bytes, aligned to 16 or 128 bytes)
struct PictureHeader {
    uint32_t totalSize;       // Total bytes for this picture (headers + image + CLUT)
    uint32_t clutSize;        // Size of CLUT data (may be 0 if no CLUT)
    uint32_t imageSize;       // Size of image data (sum of MIPMAP levels)
    uint16_t headerSize;      // Size of headers (picture + optional MIPMAP + user space)
    uint16_t clutColors;      // Actual number of colors in CLUT
    uint8_t  pictFormat;      // Must be 0 for TIM2 v0x04
    uint8_t  mipMapTextures;  // Number of MIPMAP textures (0x01 = LV0 only)
    uint8_t  clutType;        // Bit7=CSM2, Bit6=compound flag, Bits0-5=pixel fmt
    uint8_t  imageType;       // Pixel format of image data (see PixelFormat)
    uint16_t imageWidth;      // Width in pixels (restrictions per format/levels)
    uint16_t imageHeight;     // Height in pixels (no power-of-two requirement)
    uint64_t gsTex0;          // Raw GS TEX0 register value
    uint64_t gsTex1;          // Raw GS TEX1 register value
    uint32_t gsTexaFbaPabe;   // Packed TEXA, FBA, PABE register bits
    uint32_t gsTexClut;       // TEXCLUT register (only valid if CSM2 mode)

    // Helper methods for interpretation
    PixelFormat getImagePixelFormat() const {
        return static_cast<PixelFormat>(imageType);
    }

    PixelFormat getClutPixelFormat() const {
        return static_cast<PixelFormat>(clutType & 0x3F);
    }

    bool isClutCSM2() const { return (clutType & 0x80) != 0; }
    bool isClutCompound() const { return (clutType & 0x40) != 0; }
    bool hasClut() const { return clutSize > 0 && getClutPixelFormat() != TIM2_NONE; }
    bool hasMipMaps() const { return mipMapTextures > 1; }
};

// MIPMAP Header (variable size, only if mipMapTextures > 1)
struct MipMapHeader {
    uint64_t gsMiptbp1;       // GS MIPTBP1 register (LV1–LV3 buffer base/width)
    uint64_t gsMiptbp2;       // GS MIPTBP2 register (LV4–LV6 buffer base/width)
    std::vector<uint32_t> sizes;  // Size (bytes) for each MIPMAP level (aligned to 16B)
};

// Extended Header (optional, 16 bytes, starts user space)
struct ExtendedHeader {
    char     exHeaderId[4];   // 'e','X','t','\0' identifier
    uint32_t userSpaceSize;   // Valid total size of user space (incl. this header)
    uint32_t userDataSize;    // Bytes of user data before comment string
    uint32_t reserved;        // Must be 0x00000000

    bool isValid() const {
        return std::memcmp(exHeaderId, "eXt\0", 4) == 0;
    }
};

// GS Register field extraction helpers (TEX0/TEX1 parsing)
struct GsTex0Fields {
    uint16_t tbp0;   // Texture base pointer
    uint8_t  tbw;    // Texture buffer width
    uint8_t  psm;    // Pixel storage mode
    uint8_t  tw;     // Texture width log2
    uint8_t  th;     // Texture height log2
    uint8_t  tcc;    // Texture color component
    uint8_t  tfx;    // Texture function
    uint16_t cbp;    // CLUT buffer base
    uint8_t  cpsm;   // CLUT pixel storage mode
    uint8_t  csm;    // CLUT storage mode (0=CSM1, 1=CSM2)
    uint8_t  csa;    // CLUT entry offset
    uint8_t  cld;    // CLUT buffer load control

    static GsTex0Fields parse(uint64_t tex0) {
        GsTex0Fields f;
        f.tbp0 = static_cast<uint16_t>(tex0 & 0x3FFF);
        f.tbw  = static_cast<uint8_t>((tex0 >> 14) & 0x3F);
        f.psm  = static_cast<uint8_t>((tex0 >> 20) & 0x3F);
        f.tw   = static_cast<uint8_t>((tex0 >> 26) & 0x0F);
        f.th   = static_cast<uint8_t>((tex0 >> 30) & 0x0F);
        f.tcc  = static_cast<uint8_t>((tex0 >> 34) & 0x01);
        f.tfx  = static_cast<uint8_t>((tex0 >> 35) & 0x03);
        f.cbp  = static_cast<uint16_t>((tex0 >> 37) & 0x3FFF);
        f.cpsm = static_cast<uint8_t>((tex0 >> 51) & 0x0F);
        f.csm  = static_cast<uint8_t>((tex0 >> 55) & 0x01);
        f.csa  = static_cast<uint8_t>((tex0 >> 56) & 0x1F);
        f.cld  = static_cast<uint8_t>((tex0 >> 61) & 0x07);
        return f;
    }
};

struct GsTex1Fields {
    uint8_t  lcm;    // LOD calculation method
    uint8_t  mxl;    // Maximum MIP level
    uint8_t  mmag;   // Filter when texture expanded
    uint8_t  mmin;   // Filter when texture reduced
    uint8_t  mtba;   // MIPMAP base address spec
    uint16_t l;      // LOD parameter L
    uint16_t k;      // LOD parameter K

    static GsTex1Fields parse(uint64_t tex1) {
        GsTex1Fields f;
        f.lcm  = static_cast<uint8_t>(tex1 & 0x01);
        f.mxl  = static_cast<uint8_t>((tex1 >> 2) & 0x07);
        f.mmag = static_cast<uint8_t>((tex1 >> 5) & 0x01);
        f.mmin = static_cast<uint8_t>((tex1 >> 6) & 0x07);
        f.mtba = static_cast<uint8_t>((tex1 >> 9) & 0x01);
        f.l    = static_cast<uint16_t>((tex1 >> 19) & 0x03);
        f.k    = static_cast<uint16_t>((tex1 >> 32) & 0xFFF);
        return f;
    }
};

#pragma pack(pop)


// Color conversion helpers
struct Color32 {
    uint8_t r, g, b, a;
    Color32() : r(0), g(0), b(0), a(255) {}
    Color32(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}
};

struct Color16 {
    uint16_t value;
    Color32 toColor32() const {
        uint8_t r = static_cast<uint8_t>(((value & 0x001F) << 3) | ((value & 0x001F) >> 2));
        uint8_t g = static_cast<uint8_t>(((value & 0x03E0) >> 2) | ((value & 0x03E0) >> 7));
        uint8_t b = static_cast<uint8_t>(((value & 0x7C00) >> 7) | ((value & 0x7C00) >> 12));
        uint8_t a = (value & 0x8000) ? 255 : 0;
        return Color32(r, g, b, a);
    }
};

// Utility functions
inline std::string pixelFormatToString(PixelFormat fmt) {
    switch(fmt) {
        case TIM2_NONE:   return "None";
        case TIM2_RGB16:  return "RGB16";
        case TIM2_RGB24:  return "RGB24";
        case TIM2_RGB32:  return "RGB32";
        case TIM2_IDTEX4: return "IDTEX4 (4-bit indexed)";
        case TIM2_IDTEX8: return "IDTEX8 (8-bit indexed)";
        default:          return "Unknown";
    }
}

inline size_t getBitsPerPixel(PixelFormat fmt) {
    switch(fmt) {
        case TIM2_IDTEX4: return 4;
        case TIM2_IDTEX8: return 8;
        case TIM2_RGB16:  return 16;
        case TIM2_RGB24:  return 24;
        case TIM2_RGB32:  return 32;
        default:          return 0;
    }
}

} // namespace tim2