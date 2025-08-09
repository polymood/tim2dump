#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <map>
#include <algorithm>
#include <cctype>
#include "tim2_parser.h"
#include "table_formatter.h"
#include "image_converter.h"

namespace fs = std::filesystem;

void printUsage(const char* programName) {
    std::cout << "TIM2 Tool v1.0 - PlayStation 2 TIM2 Image Format Utility\n";
    std::cout << "Usage: " << programName << " <command> <file> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  info <file>           Display detailed information about TIM2 file\n";
    std::cout << "  export <file> [fmt]   Export images (fmt: bmp or png, default: bmp)\n";
    std::cout << "  viewc <file> [pic]    Display image with colors (ANSI terminal)\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -v, --verbose         Show detailed information\n";
    std::cout << "  -g, --gs-registers    Display GS register details\n";
    std::cout << "  -o, --output <name>   Output base filename for export\n";
    std::cout << "  -p, --picture <n>     Select specific picture (0-based index)\n";
    std::cout << "  -m, --miplevel <n>    Select MIP level (default: 0)\n";
    std::cout << "  -w, --width <n>       Max width for terminal display (default: 80)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << " info texture.tim2\n";
    std::cout << "  " << programName << " export texture.tim2 png\n";
    std::cout << "  " << programName << " view texture.tim2 -p 0 -w 120\n";
}

struct Options {
    std::string command;
    std::string inputPath;  // Can be file or folder
    std::string format = "bmp";
    std::string outputFolder;  // For batch processing
    bool verbose = false;
    bool showGsRegisters = false;
    int pictureIndex = -1;
    int mipLevel = 0;
    int maxWidth = 80;
};

Options parseArguments(int argc, char* argv[]) {
    Options opts;

    if (argc < 3) {
        return opts;
    }

    opts.command = argv[1];
    opts.inputPath = argv[2];

    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-v" || arg == "--verbose") {
            opts.verbose = true;
        } else if (arg == "-g" || arg == "--gs-registers") {
            opts.showGsRegisters = true;
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            opts.outputFolder = argv[++i];
        } else if ((arg == "-p" || arg == "--picture") && i + 1 < argc) {
            opts.pictureIndex = std::stoi(argv[++i]);
        } else if ((arg == "-m" || arg == "--miplevel") && i + 1 < argc) {
            opts.mipLevel = std::stoi(argv[++i]);
        } else if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            opts.maxWidth = std::stoi(argv[++i]);
        } else if ((opts.command == "export" || opts.command == "batch") && i == 3) {
            opts.format = arg;
        }
    }

    return opts;
}

// Find all TIM2 files recursively
std::vector<fs::path> findTIM2Files(const fs::path& rootPath) {
    std::vector<fs::path> tim2Files;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
            if (entry.is_regular_file()) {
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(),
                             [](unsigned char c){ return std::tolower(c); });

                if (ext == ".tim2" || ext == ".tm2") {
                    tim2Files.push_back(entry.path());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << "\n";
    }

    return tim2Files;
}

// Generate output filename handling conflicts
std::string generateOutputFilename(const fs::path& outputDir, const fs::path& sourceFile,
                                  const std::string& format, std::map<std::string, int>& nameCounter) {
    std::string baseName = sourceFile.stem().string();
    fs::path outputPath = outputDir / (baseName + "." + format);

    // Handle filename conflicts
    if (fs::exists(outputPath) || nameCounter[baseName] > 0) {
        int counter = ++nameCounter[baseName];
        outputPath = outputDir / (baseName + "_" + std::to_string(counter) + "." + format);
    }

    return outputPath.string();
}

int handleBatch(const Options& opts) {
    fs::path inputPath(opts.inputPath);

    if (!fs::exists(inputPath)) {
        std::cerr << "Error: Input path does not exist: " << opts.inputPath << "\n";
        return 1;
    }

    if (!fs::is_directory(inputPath)) {
        std::cerr << "Error: Input path is not a directory: " << opts.inputPath << "\n";
        return 1;
    }

    // Find all TIM2 files
    auto tim2Files = findTIM2Files(inputPath);

    if (tim2Files.empty()) {
        std::cout << "No TIM2 files found in " << opts.inputPath << "\n";
        return 0;
    }

    std::cout << "Found " << tim2Files.size() << " TIM2 file(s) to process.\n\n";

    // Prepare output directory if specified
    fs::path outputRoot;
    bool useOutputFolder = !opts.outputFolder.empty();

    if (useOutputFolder) {
        outputRoot = fs::path(opts.outputFolder);
        try {
            fs::create_directories(outputRoot);
        } catch (const std::exception& e) {
            std::cerr << "Error creating output directory: " << e.what() << "\n";
            return 1;
        }
    }

    // Process each file
    int successCount = 0;
    int failCount = 0;
    std::map<std::string, int> nameCounter;  // Track filename conflicts

    for (const auto& tim2Path : tim2Files) {
        std::cout << "Processing: " << tim2Path.string() << "\n";

        tim2::TIM2Parser parser;
        if (!parser.loadFile(tim2Path.string())) {
            std::cerr << "  Error: " << parser.getLastError() << "\n";
            failCount++;
            continue;
        }

        // Determine output directory
        fs::path outputDir;
        if (useOutputFolder) {
            // Preserve relative directory structure in output folder
            auto relativePath = fs::relative(tim2Path.parent_path(), inputPath);
            outputDir = outputRoot / relativePath;

            try {
                fs::create_directories(outputDir);
            } catch (const std::exception& e) {
                std::cerr << "  Error creating directory: " << e.what() << "\n";
                failCount++;
                continue;
            }
        } else {
            // Save alongside source file
            outputDir = tim2Path.parent_path();
        }

        // Export all pictures from this TIM2 file
        bool fileSuccess = true;
        for (size_t i = 0; i < parser.getPictureCount(); ++i) {
            const auto* pic = parser.getPicture(i);
            if (!pic) continue;

            for (size_t mip = 0; mip < pic->header.mipMapTextures; ++mip) {
                std::string outputFilename;

                if (useOutputFolder) {
                    // Generate unique filename in output folder for pic and mip level
                    std::string baseName = tim2Path.stem().string();
                    if (parser.getPictureCount() > 1) {
                        baseName += "_pic" + std::to_string(i);
                    }
                    if (pic->header.mipMapTextures > 1) {
                        baseName += "_mip" + std::to_string(mip);
                    }

                    outputFilename = (outputDir / (baseName + "." + opts.format)).string();

                    // Handle conflicts in case different files happen to have the same name
                    if (fs::exists(outputFilename)) {
                        int counter = 1;
                        do {
                            outputFilename = (outputDir / (baseName + "_" + std::to_string(counter++) + "." + opts.format)).string();
                        } while (fs::exists(outputFilename));
                    }
                } else {
                    // Save alongside source with standard naming
                    std::string baseName = tim2Path.stem().string();
                    if (parser.getPictureCount() > 1) {
                        baseName += "_pic" + std::to_string(i);
                    }
                    if (pic->header.mipMapTextures > 1) {
                        baseName += "_mip" + std::to_string(mip);
                    }
                    outputFilename = (outputDir / (baseName + "." + opts.format)).string();
                }

                bool result = false;
                if (opts.format == "png") {
                    result = tim2::ImageConverter::exportPNG(*pic, outputFilename, mip);
                } else {
                    result = tim2::ImageConverter::exportBMP(*pic, outputFilename, mip);
                }

                if (result) {
                    std::cout << "  -> " << outputFilename << "\n";
                } else {
                    std::cerr << "  Failed to export: " << outputFilename << "\n";
                    fileSuccess = false;
                }
            }
        }

        if (fileSuccess) {
            successCount++;
        } else {
            failCount++;
        }
    }

    // Summary
    std::cout << "\n" << std::string(60, '-') << "\n";
    std::cout << "Batch conversion complete!\n";
    std::cout << "  Processed: " << tim2Files.size() << " file(s)\n";
    std::cout << "  Success: " << successCount << "\n";
    std::cout << "  Failed: " << failCount << "\n";

    if (useOutputFolder) {
        std::cout << "  Output directory: " << outputRoot.string() << "\n";
    } else {
        std::cout << "  Files saved alongside source files\n";
    }

    return (failCount > 0) ? 1 : 0;
}

int handleInfo(const Options& opts) {
    tim2::TIM2Parser parser;

    if (!parser.loadFile(opts.inputPath)) {
        std::cerr << "Error: " << parser.getLastError() << "\n";
        return 1;
    }

    // Display summary
    tim2::TableFormatter::displaySummary(parser);

    if (opts.verbose) {
        // Display file header
        tim2::TableFormatter::displayFileHeader(parser.getFileHeader());

        // Display each picture's details
        for (size_t i = 0; i < parser.getPictureCount(); ++i) {
            const auto* pic = parser.getPicture(i);
            if (!pic) continue;

            tim2::TableFormatter::displayPictureHeader(pic->header, i);

            if (pic->mipMapHeader) {
                tim2::TableFormatter::displayMipMapHeader(*pic->mipMapHeader);
            }

            if (pic->extHeader) {
                tim2::TableFormatter::displayExtendedHeader(*pic->extHeader);
            }

            if (opts.showGsRegisters) {
                tim2::TableFormatter::displayGsRegisters(pic->header);
            }
        }
    }

    return 0;
}

int handleExport(const Options& opts) {
    tim2::TIM2Parser parser;

    if (!parser.loadFile(opts.inputPath)) {
        std::cerr << "Error: " << parser.getLastError() << "\n";
        return 1;
    }

    std::cout << "Exporting images from " << opts.inputPath << "...\n";

    // Generate output base name from input filename
    fs::path inputFile(opts.inputPath);
    std::string outputBase = inputFile.stem().string();

    if (opts.pictureIndex >= 0) {
        // Export specific picture
        const auto* pic = parser.getPicture(opts.pictureIndex);
        if (!pic) {
            std::cerr << "Error: Picture index " << opts.pictureIndex << " not found\n";
            return 1;
        }

        std::string filename = outputBase + "." + opts.format;
        bool success = false;

        if (opts.format == "png") {
            success = tim2::ImageConverter::exportPNG(*pic, filename, opts.mipLevel);
        } else {
            success = tim2::ImageConverter::exportBMP(*pic, filename, opts.mipLevel);
        }

        if (success) {
            std::cout << "Exported: " << filename << "\n";
        } else {
            std::cerr << "Failed to export: " << filename << "\n";
            return 1;
        }
    } else {
        // Export all pictures
        if (!tim2::ImageConverter::exportAll(parser, outputBase, opts.format)) {
            return 1;
        }
    }

    std::cout << "Export complete!\n";
    return 0;
}

int handleView(const Options& opts, bool useColor) {
    tim2::TIM2Parser parser;

    if (!parser.loadFile(opts.inputPath)) {
        std::cerr << "Error: " << parser.getLastError() << "\n";
        return 1;
    }

    size_t picIndex = (opts.pictureIndex >= 0) ? opts.pictureIndex : 0;
    const auto* pic = parser.getPicture(picIndex);

    if (!pic) {
        std::cerr << "Error: Picture index " << picIndex << " not found\n";
        return 1;
    }

    std::cout << "Displaying picture " << picIndex;
    if (pic->header.mipMapTextures > 1) {
        std::cout << " (MIP level " << opts.mipLevel << ")";
    }
    std::cout << ":\n";
    std::cout << "Dimensions: " << pic->header.imageWidth << "x" << pic->header.imageHeight;
    std::cout << " (" << tim2::pixelFormatToString(pic->header.getImagePixelFormat()) << ")\n\n";

    if (useColor) {
        tim2::ImageConverter::displayANSI(*pic, opts.maxWidth, opts.mipLevel);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    Options opts = parseArguments(argc, argv);

    // Check if input exists (file or directory depending on command)
    if (!fs::exists(opts.inputPath)) {
        std::cerr << "Error: Path not found: " << opts.inputPath << "\n";
        return 1;
    }

    // Handle commands
    if (opts.command == "info") {
        if (!fs::is_regular_file(opts.inputPath)) {
            std::cerr << "Error: 'info' command requires a file, not a directory\n";
            return 1;
        }
        return handleInfo(opts);
    } else if (opts.command == "export") {
        if (!fs::is_regular_file(opts.inputPath)) {
            std::cerr << "Error: 'export' command requires a file, not a directory\n";
            return 1;
        }
        return handleExport(opts);
    } else if (opts.command == "batch") {
        if (!fs::is_directory(opts.inputPath)) {
            std::cerr << "Error: 'batch' command requires a directory, not a file\n";
            return 1;
        }
        return handleBatch(opts);
    } else if (opts.command == "viewc") {
        if (!fs::is_regular_file(opts.inputPath)) {
            std::cerr << "Error: 'viewc' command requires a file, not a directory\n";
            return 1;
        }
        return handleView(opts, true);
    } else {
        std::cerr << "Error: Unknown command: " << opts.command << "\n";
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}