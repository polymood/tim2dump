#pragma once

#include <cstdint>
#include <algorithm>
#include <bit>

namespace tim2 {
namespace utils {

// Endianness helpers (TIM2 is little-endian)
template<typename T>
inline T swapEndian(T value) {
    if constexpr (sizeof(T) == 1) {
        return value;
    } else if constexpr (sizeof(T) == 2) {
        return (value >> 8) | (value << 8);
    } else if constexpr (sizeof(T) == 4) {
        return ((value >> 24) & 0xFF) |
               ((value >> 8) & 0xFF00) |
               ((value << 8) & 0xFF0000) |
               ((value << 24) & 0xFF000000);
    } else if constexpr (sizeof(T) == 8) {
        return ((value >> 56) & 0xFF) |
               ((value >> 40) & 0xFF00) |
               ((value >> 24) & 0xFF0000) |
               ((value >> 8) & 0xFF000000) |
               ((value << 8) & 0xFF00000000) |
               ((value << 24) & 0xFF0000000000) |
               ((value << 40) & 0xFF000000000000) |
               ((value << 56) & 0xFF00000000000000);
    }
    return value;
}

// Check if system is big-endian
inline bool isBigEndian() {
    if constexpr (std::endian::native == std::endian::big) {
        return true;
    }
    return false;
}

// Convert from little-endian if needed
template<typename T>
inline T fromLittleEndian(T value) {
    if (isBigEndian()) {
        return swapEndian(value);
    }
    return value;
}

// Alignment helpers
inline size_t alignUp(size_t value, size_t alignment) {
    return ((value + alignment - 1) / alignment) * alignment;
}

inline bool isAligned(size_t value, size_t alignment) {
    return (value % alignment) == 0;
}

// Bit manipulation helpers
inline uint32_t extractBits(uint64_t value, int start, int count) {
    return (value >> start) & ((1ULL << count) - 1);
}

inline void setBits(uint64_t& value, int start, int count, uint32_t bits) {
    uint64_t mask = ((1ULL << count) - 1) << start;
    value = (value & ~mask) | ((uint64_t(bits) << start) & mask);
}

// Color conversion helpers
inline uint8_t expand5to8(uint8_t value) {
    // Expand 5-bit color to 8-bit by replicating top bits
    return (value << 3) | (value >> 2);
}

inline uint8_t expand6to8(uint8_t value) {
    // Expand 6-bit color to 8-bit
    return (value << 2) | (value >> 4);
}

inline uint8_t contract8to5(uint8_t value) {
    return value >> 3;
}

inline uint8_t contract8to6(uint8_t value) {
    return value >> 2;
}

// GS memory address calculation
inline size_t calculateGSAddress(int tbp, int tbw, int x, int y) {
    // Simplified GS memory address calculation
    // Actual GS uses complex swizzling, but this gives logical address
    return tbp * 256 + y * tbw * 64 + x;
}

// Texture size calculation
inline size_t calculateTextureSize(int width, int height, int bpp) {
    size_t pixelCount = width * height;
    size_t bitCount = pixelCount * bpp;
    return (bitCount + 7) / 8;  // Round up to nearest byte
}

// MipMap dimension calculation
inline int getMipDimension(int baseDimension, int mipLevel) {
    return std::max(1, baseDimension >> mipLevel);
}

// Validate dimensions for specific formats
inline bool isValidDimension(int dimension, int pixelFormat, int mipLevels) {
    // Check dimension requirements based on format and MIP levels
    // as specified in the TIM2 documentation

    int requiredMultiple = 1;

    if (pixelFormat == 0x04) {  // IDTEX4
        requiredMultiple = 1 << (mipLevels + 1);  // 4, 8, 16, 32...
    } else if (pixelFormat == 0x05) {  // IDTEX8
        requiredMultiple = 1 << mipLevels;  // 2, 4, 8, 16...
    } else {  // RGB formats
        if (mipLevels > 1) {
            requiredMultiple = 1 << (mipLevels - 1);
        }
    }

    return (dimension % requiredMultiple) == 0;
}

// Debug output helper
inline void hexDump(const uint8_t* data, size_t size, size_t bytesPerLine = 16) {
    for (size_t i = 0; i < size; i += bytesPerLine) {
        // Print offset
        printf("%08zx: ", i);

        // Print hex bytes
        for (size_t j = 0; j < bytesPerLine; ++j) {
            if (i + j < size) {
                printf("%02x ", data[i + j]);
            } else {
                printf("   ");
            }
        }

        // Print ASCII
        printf(" |");
        for (size_t j = 0; j < bytesPerLine && i + j < size; ++j) {
            uint8_t c = data[i + j];
            printf("%c", (c >= 32 && c < 127) ? c : '.');
        }
        printf("|\n");
    }
}

} // namespace utils
} // namespace tim2