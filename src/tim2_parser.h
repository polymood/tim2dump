#pragma once

#include "tim2_types.h"
#include <memory>
#include <fstream>
#include <optional>

namespace tim2 {

class Picture {
public:
    PictureHeader header;
    std::optional<MipMapHeader> mipMapHeader;
    std::vector<uint8_t> userData;
    std::vector<uint8_t> imageData;
    std::vector<uint8_t> clutData;
    std::optional<ExtendedHeader> extHeader;
    std::string comment;

    // Get decoded image as RGBA
    std::vector<Color32> decodeImage(size_t mipLevel = 0) const;

    // Get CLUT colors
    std::vector<Color32> getClutColors() const;

private:
    Color32 getPixelColor(size_t x, size_t y, size_t mipLevel = 0) const;
    size_t getImageOffset(size_t mipLevel) const;
    size_t getMipMapWidth(size_t level) const;
    size_t getMipMapHeight(size_t level) const;
};

class TIM2Parser {
public:
    TIM2Parser() = default;
    ~TIM2Parser() = default;

    // Load TIM2 file
    bool loadFile(const std::string& filename);

    // Check if file is loaded and valid
    bool isValid() const { return m_valid; }

    // Get file header
    const FileHeader& getFileHeader() const { return m_fileHeader; }

    // Get pictures
    const std::vector<Picture>& getPictures() const { return m_pictures; }

    // Get specific picture
    const Picture* getPicture(size_t index) const {
        if (index >= m_pictures.size()) return nullptr;
        return &m_pictures[index];
    }

    // Get number of pictures
    size_t getPictureCount() const { return m_pictures.size(); }

    // Get last error message
    const std::string& getLastError() const { return m_lastError; }

private:
    FileHeader m_fileHeader;
    std::vector<Picture> m_pictures;
    bool m_valid = false;
    std::string m_lastError;

    // Helper methods
    bool parseFileHeader(std::ifstream& file);
    bool parsePicture(std::ifstream& file, Picture& pic, size_t alignment);
    bool parseMipMapHeader(std::ifstream& file, Picture& pic);
    bool parseUserSpace(std::ifstream& file, Picture& pic);
    bool parseImageData(std::ifstream& file, Picture& pic);
    bool parseClutData(std::ifstream& file, Picture& pic);

    size_t alignOffset(size_t offset, size_t alignment);
    void skipAlignment(std::ifstream& file, size_t alignment);
};

} // namespace tim2