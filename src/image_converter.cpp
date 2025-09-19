#include "image_converter.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "stb_image_write.h"

namespace tim2 {

bool ImageConverter::exportBMP(const Picture& pic, const std::string& filename, size_t mipLevel) {
    if (mipLevel >= pic.header.mipMapTextures) {
        std::cerr << "Invalid MIP level\n";
        return false;
    }

    auto imageData = pic.decodeImage(mipLevel);
    if (imageData.empty()) {
        std::cerr << "Failed to decode image\n";
        return false;
    }

    size_t width = std::max<size_t>(1, pic.header.imageWidth >> mipLevel);
    size_t height = std::max<size_t>(1, pic.header.imageHeight >> mipLevel);

    // BMP row size must be multiple of 4 bytes
    size_t rowSize = ((width * 3 + 3) / 4) * 4;
    size_t imageSize = rowSize * height;

    BMPHeader bmpHeader;
    bmpHeader.fileSize = sizeof(BMPHeader) + sizeof(BMPInfoHeader) + imageSize;

    BMPInfoHeader infoHeader;
    infoHeader.width = width;
    infoHeader.height = height;
    infoHeader.imageSize = imageSize;

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to create file: " << filename << "\n";
        return false;
    }

    // Write headers
    file.write(reinterpret_cast<const char*>(&bmpHeader), sizeof(bmpHeader));
    file.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));

    // Write pixel data (BMP stores bottom-to-top, BGR format)
    std::vector<uint8_t> row(rowSize, 0);
    for (int y = height - 1; y >= 0; --y) {
        for (size_t x = 0; x < width; ++x) {
            const Color32& pixel = imageData[y * width + x];
            row[x * 3 + 0] = pixel.b;
            row[x * 3 + 1] = pixel.g;
            row[x * 3 + 2] = pixel.r;
        }
        file.write(reinterpret_cast<const char*>(row.data()), rowSize);
    }

    return file.good();
}

bool ImageConverter::exportPNG(const Picture& pic, const std::string& filename, size_t mipLevel) {
    if (mipLevel >= pic.header.mipMapTextures) {
        std::cerr << "Invalid MIP level\n";
        return false;
    }

    auto imageData = pic.decodeImage(mipLevel);
    if (imageData.empty()) {
        std::cerr << "Failed to decode image\n";
        return false;
    }

    size_t width = std::max<size_t>(1, pic.header.imageWidth >> mipLevel);
    size_t height = std::max<size_t>(1, pic.header.imageHeight >> mipLevel);

    // Convert to RGBA format for stb_image_write
    std::vector<uint8_t> rgbaData(width * height * 4);
    for (size_t i = 0; i < imageData.size(); ++i) {
        rgbaData[i * 4 + 0] = imageData[i].r;
        rgbaData[i * 4 + 1] = imageData[i].g;
        rgbaData[i * 4 + 2] = imageData[i].b;
        rgbaData[i * 4 + 3] = imageData[i].a;
    }

    int result = stbi_write_png(filename.c_str(), width, height, 4,
                                rgbaData.data(), width * 4);

    return result != 0;
}

bool ImageConverter::exportAll(const TIM2Parser& parser, const std::string& baseFilename,
                              const std::string& format) {
    bool success = true;

    for (size_t i = 0; i < parser.getPictureCount(); ++i) {
        const auto* pic = parser.getPicture(i);
        if (!pic) continue;

        for (size_t mip = 0; mip < pic->header.mipMapTextures; ++mip) {
            std::string filename = baseFilename + "_pic" + std::to_string(i);
            if (pic->header.mipMapTextures > 1) {
                filename += "_mip" + std::to_string(mip);
            }
            filename += "." + format;

            bool result = false;
            if (format == "png") {
                result = exportPNG(*pic, filename, mip);
            } else {
                result = exportBMP(*pic, filename, mip);
            }

            if (result) {
                std::cout << "Exported: " << filename << "\n";
            } else {
                std::cerr << "Failed to export: " << filename << "\n";
                success = false;
            }
        }
    }

    return success;
}

void ImageConverter::displayANSI(const Picture& pic, size_t maxWidth, size_t mipLevel) {
    auto imageData = pic.decodeImage(mipLevel);
    if (imageData.empty()) return;

    size_t width = std::max<size_t>(1, pic.header.imageWidth >> mipLevel);
    size_t height = std::max<size_t>(1, pic.header.imageHeight >> mipLevel);

    // Calculate scaling factor
    double scale = 1.0;
    if (width > maxWidth / 2) {  // Each pixel takes 2 chars
        scale = (double)(maxWidth / 2) / width;
    }

    size_t displayWidth = width * scale;
    size_t displayHeight = height * scale * 0.5;

    for (size_t dy = 0; dy < displayHeight; ++dy) {
        for (size_t dx = 0; dx < displayWidth; ++dx) {
            size_t sx = dx / scale;
            size_t sy = dy / scale / 0.5;

            if (sx < width && sy < height) {
                const Color32& pixel = imageData[sy * width + sx];

                // Convert to 256-color ANSI
                int r = pixel.r * 5 / 255;
                int g = pixel.g * 5 / 255;
                int b = pixel.b * 5 / 255;
                int color = 16 + 36 * r + 6 * g + b;

                std::cout << "\033[48;5;" << color << "m  \033[0m";
            }
        }
        std::cout << "\n";
    }
}

} // namespace tim2