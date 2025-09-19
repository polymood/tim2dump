#include "table_formatter.h"
#include <vector>

namespace tim2 {

void TableFormatter::displayFileHeader(const FileHeader& header) {
    printHeader("FILE HEADER");
    printRow("File ID", std::string(header.fileId, 4));
    printRow("Format Version", formatHex(header.formatVersion, 2));
    printRow("Format ID", header.formatId == TIM2_ALIGN_128 ? "128-byte alignment" : "16-byte alignment");
    printRow("Number of Pictures", std::to_string(header.pictures));
    printSeparator(60);
}

void TableFormatter::displayPictureHeader(const PictureHeader& header, size_t index) {
    printHeader("PICTURE #" + std::to_string(index));

    printRow("Total Size", formatSize(header.totalSize));
    printRow("Image Size", formatSize(header.imageSize));
    printRow("CLUT Size", formatSize(header.clutSize));
    printRow("Header Size", formatSize(header.headerSize));
    printRow("Image Dimensions", std::to_string(header.imageWidth) + " x " + std::to_string(header.imageHeight));
    printRow("Image Format", pixelFormatToString(header.getImagePixelFormat()));

    if (header.hasClut()) {
        printRow("CLUT Format", pixelFormatToString(header.getClutPixelFormat()));
        printRow("CLUT Colors", std::to_string(header.clutColors));
        printRow("CLUT Mode", header.isClutCSM2() ? "CSM2" : "CSM1");
        if (header.isClutCompound()) {
            printRow("CLUT Compound", "Yes");
        }
    }

    if (header.hasMipMaps()) {
        printRow("MipMap Textures", std::to_string(header.mipMapTextures));
    }

    printSeparator(60);
}

void TableFormatter::displayMipMapHeader(const MipMapHeader& header) {
    printHeader("MIPMAP HEADER");
    printRow("MIPTBP1", formatHex(header.gsMiptbp1, 16));
    printRow("MIPTBP2", formatHex(header.gsMiptbp2, 16));

    for (size_t i = 0; i < header.sizes.size(); ++i) {
        printRow("Level " + std::to_string(i) + " Size", formatSize(header.sizes[i]));
    }

    printSeparator(60);
}

void TableFormatter::displayExtendedHeader(const ExtendedHeader& header) {
    printHeader("EXTENDED HEADER");
    printRow("Header ID", std::string(header.exHeaderId, 4));
    printRow("User Space Size", formatSize(header.userSpaceSize));
    printRow("User Data Size", formatSize(header.userDataSize));
    printSeparator(60);
}

void TableFormatter::displayGsRegisters(const PictureHeader& header) {
    printHeader("GS REGISTERS");

    // TEX0 fields
    auto tex0 = GsTex0Fields::parse(header.gsTex0);
    printRow("TEX0", formatHex(header.gsTex0, 16));
    printRow("  TBP0 (Texture Base)", formatHex(tex0.tbp0, 4));
    printRow("  TBW (Buffer Width)", std::to_string(tex0.tbw));
    printRow("  PSM (Pixel Storage)", formatHex(tex0.psm, 2));
    printRow("  TW (Width 2^n)", std::to_string(tex0.tw));
    printRow("  TH (Height 2^n)", std::to_string(tex0.th));
    printRow("  TCC (Color Component)", tex0.tcc ? "RGBA" : "RGB");

    if (header.hasClut()) {
        printRow("  CBP (CLUT Base)", formatHex(tex0.cbp, 4));
        printRow("  CPSM (CLUT Format)", formatHex(tex0.cpsm, 2));
        printRow("  CSM (CLUT Mode)", tex0.csm ? "CSM2" : "CSM1");
        printRow("  CSA (CLUT Offset)", std::to_string(tex0.csa));
    }

    // TEX1 fields
    if (header.hasMipMaps()) {
        auto tex1 = GsTex1Fields::parse(header.gsTex1);
        printRow("TEX1", formatHex(header.gsTex1, 16));
        printRow("  MXL (Max MIP Level)", std::to_string(tex1.mxl));
        printRow("  MMAG (Mag Filter)", tex1.mmag ? "Linear" : "Nearest");
        printRow("  MMIN (Min Filter)", std::to_string(tex1.mmin));
    }

    // TEXA/FBA/PABE
    printRow("TEXA/FBA/PABE", formatHex(header.gsTexaFbaPabe, 8));
    uint8_t ta0 = header.gsTexaFbaPabe & 0xFF;
    uint8_t ta1 = (header.gsTexaFbaPabe >> 16) & 0xFF;
    bool aem = (header.gsTexaFbaPabe >> 15) & 0x01;
    bool fba = (header.gsTexaFbaPabe >> 31) & 0x01;
    bool pabe = (header.gsTexaFbaPabe >> 30) & 0x01;

    printRow("  TA0 (Alpha 0)", std::to_string(ta0));
    printRow("  TA1 (Alpha 1)", std::to_string(ta1));
    printRow("  AEM", aem ? "Enabled" : "Disabled");
    printRow("  FBA", fba ? "Enabled" : "Disabled");
    printRow("  PABE", pabe ? "Enabled" : "Disabled");

    // TEXCLUT (CSM2 only)
    if (header.isClutCSM2()) {
        printRow("TEXCLUT", formatHex(header.gsTexClut, 8));
        uint8_t cbw = header.gsTexClut & 0x3F;
        uint8_t cou = (header.gsTexClut >> 6) & 0x3F;
        uint16_t cov = (header.gsTexClut >> 12) & 0x3FF;
        printRow("  CBW (Buffer Width)", std::to_string(cbw));
        printRow("  COU (Offset U)", std::to_string(cou));
        printRow("  COV (Offset V)", std::to_string(cov));
    }

    printSeparator(60);
}

void TableFormatter::displaySummary(const TIM2Parser& parser) {
    printHeader("TIM2 FILE SUMMARY");

    const auto& header = parser.getFileHeader();
    printRow("Format Version", formatHex(header.formatVersion, 2));
    printRow("Alignment", header.formatId == TIM2_ALIGN_128 ? "128 bytes" : "16 bytes");
    printRow("Total Pictures", std::to_string(parser.getPictureCount()));

    std::cout << "\n";

    for (size_t i = 0; i < parser.getPictureCount(); ++i) {
        const auto* pic = parser.getPicture(i);
        if (!pic) continue;

        std::cout << "Picture " << i << ": ";
        std::cout << pic->header.imageWidth << "x" << pic->header.imageHeight;
        std::cout << " (" << pixelFormatToString(pic->header.getImagePixelFormat()) << ")";

        if (pic->header.hasClut()) {
            std::cout << " [" << pic->header.clutColors << " colors]";
        }

        if (pic->header.hasMipMaps()) {
            std::cout << " [" << (int)pic->header.mipMapTextures << " MIP levels]";
        }

        if (!pic->comment.empty()) {
            std::cout << "\n  Comment: \"" << pic->comment << "\"";
        }

        std::cout << "\n";
    }

    printSeparator(60);
}

void TableFormatter::printSeparator(size_t width) {
    std::cout << std::string(width, '-') << "\n";
}

void TableFormatter::printRow(const std::string& label, const std::string& value) {
    std::cout << std::left << std::setw(30) << label << ": " << value << "\n";
}

void TableFormatter::printHeader(const std::string& title) {
    std::cout << "\n";
    printSeparator(60);
    std::cout << "  " << title << "\n";
    printSeparator(60);
}

std::string TableFormatter::formatHex(uint64_t value, int width) {
    std::ostringstream ss;
    ss << "0x" << std::uppercase << std::hex;
    if (width > 0) {
        ss << std::setfill('0') << std::setw(width);
    }
    ss << value;
    return ss.str();
}

std::string TableFormatter::formatSize(size_t bytes) {
    std::ostringstream ss;
    ss << bytes << " bytes";

    if (bytes >= 1024 * 1024) {
        ss << " (" << std::fixed << std::setprecision(2)
           << (double)bytes / (1024.0 * 1024.0) << " MB)";
    } else if (bytes >= 1024) {
        ss << " (" << std::fixed << std::setprecision(2)
           << (double)bytes / 1024.0 << " KB)";
    }

    return ss.str();
}

} // namespace tim2