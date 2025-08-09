#pragma once

#include "tim2_parser.h"
#include <string>

namespace tim2 {

    class ImageConverter {
    public:
        // Export picture to BMP (no external dependencies)
        static bool exportBMP(const Picture& pic, const std::string& filename, size_t mipLevel = 0);

        // Export picture to PNG (requires stb_image_write.h)
        static bool exportPNG(const Picture& pic, const std::string& filename, size_t mipLevel = 0);

        // Export all pictures from a TIM2 file
        static bool exportAll(const TIM2Parser& parser, const std::string& baseFilename,
                             const std::string& format = "bmp");

        // Display image with ANSI colors (for terminals that support it)
        static void displayANSI(const Picture& pic, size_t maxWidth = 80, size_t mipLevel = 0);

    private:
        // BMP file structures
        struct BMPHeader {
            uint16_t type = 0x4D42;  // "BM"
            uint32_t fileSize;
            uint32_t reserved = 0;
            uint32_t dataOffset = 54;
        } __attribute__((packed));

        struct BMPInfoHeader {
            uint32_t size = 40;
            int32_t width;
            int32_t height;
            uint16_t planes = 1;
            uint16_t bitCount = 24;
            uint32_t compression = 0;
            uint32_t imageSize = 0;
            int32_t xPelsPerMeter = 2835;
            int32_t yPelsPerMeter = 2835;
            uint32_t clrUsed = 0;
            uint32_t clrImportant = 0;
        } __attribute__((packed));
    };

} // namespace tim2
