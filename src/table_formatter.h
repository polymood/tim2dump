#pragma once

#include "tim2_types.h"
#include "tim2_parser.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>


namespace tim2 {

    class TableFormatter {
    public:
        static void displayFileHeader(const FileHeader& header);
        static void displayPictureHeader(const PictureHeader& header, size_t index);
        static void displayMipMapHeader(const MipMapHeader& header);
        static void displayExtendedHeader(const ExtendedHeader& header);
        static void displayGsRegisters(const PictureHeader& header);
        static void displaySummary(const TIM2Parser& parser);

    private:
        static void printSeparator(size_t width);
        static void printRow(const std::string& label, const std::string& value);
        static void printHeader(const std::string& title);
        static std::string formatHex(uint64_t value, int width = 0);
        static std::string formatSize(size_t bytes);
    };

} // namespace tim2