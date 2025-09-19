# TIM2dump

A comprehensive utility for extracting, converting, and analyzing PlayStation 2 TIM2 (Texture Image Map 2) format files. Supports all TIM2 pixel formats, CLUT palettes, mipmaps, and batch processing. Export textures to BMP/PNG for game modding, preservation, or analysis.

## Overview

TIM2dump provides comprehensive support for the TIM2 (Texture Image Map 2) format used by PlayStation 2 games and development tools. This utility enables developers, modders, and digital preservationists to extract, convert, and analyze texture data from PS2 game assets.

## Key Features

### Core Functionality
- **Full TIM2 Format Support**
  - All pixel formats: RGB32, RGB24, RGB16, IDTEX8 (8-bit indexed), IDTEX4 (4-bit indexed)
  - Complete CLUT (Color Look-Up Table) support with CSM1/CSM2 modes
  - Multi-level mipmap texture support
  - Extended header and user data parsing
  - Comment extraction from TIM2 files

### Export Capabilities
- **BMP Export** - Native implementation with no external dependencies
- **PNG Export** - High-quality PNG output via stb_image_write
- **Batch Processing** - Process entire directories recursively
- **Flexible Output** - Customizable output paths and naming conventions

### Analysis Tools
- **Detailed Information Display** - Complete header and metadata analysis
- **GS Register Inspection** - View raw Graphics Synthesizer register values
- **Terminal Visualization** - ANSI 256-color preview directly in console
- **Format Validation** - Automatic detection of format inconsistencies

## Requirements

### Build Requirements
- **C++ Compiler** with C++14 support or later:
  - GCC 5.0+
  - Clang 3.4+
  - MSVC 2015 (19.0)+
  - Apple Clang 6.0+
- **CMake** 3.10 or newer
- **Standard C++ Library** with filesystem support

### Runtime Requirements
- No special runtime dependencies for basic operation
- Terminal with ANSI color support for visualization features (optional)

## Installation

### Building from Source

#### Linux/macOS

```bash
# Clone the repository
git clone https://github.com/yourusername/tim2dump.git
cd tim2dump

# Download stb_image_write.h (for PNG support)
mkdir -p third_party
wget -O third_party/stb_image_write.h \
  https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h

# Build with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Install (optional)
sudo cmake --install build
```

#### Windows (Visual Studio)

```powershell
# Clone the repository
git clone https://github.com/yourusername/tim2dump.git
cd tim2dump

# Download stb_image_write.h
New-Item -ItemType Directory -Force -Path third_party
Invoke-WebRequest -Uri "https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h" `
  -OutFile "third_party\stb_image_write.h"

# Generate Visual Studio project
cmake -B build -G "Visual Studio 16 2019"

# Build
cmake --build build --config Release
```

#### Windows (MinGW)

```bash
# Similar to Linux, but specify MinGW generator
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Usage

### Basic Commands

```bash
# Display information about a TIM2 file
tim2dump info texture.tim2

# Export all images as BMP (default)
tim2dump export texture.tim2

# Export as PNG with custom output name
tim2dump export texture.tim2 png -o output/mytexture

# View image in terminal (ANSI colors)
tim2dump viewc texture.tim2
```

### Command Reference

#### `info` - Display file information

```bash
tim2dump info <file> [options]

Options:
  -v, --verbose       Show detailed header information
  -g, --gs-registers  Display raw GS register values

Examples:
  tim2dump info game_texture.tim2
  tim2dump info boss.tim2 --verbose --gs-registers
```

#### `export` - Export images

```bash
tim2dump export <file> [format] [options]

Formats:
  bmp  - Bitmap format (default)
  png  - PNG format

Options:
  -o, --output <path>  Output base filename
  -p, --picture <n>    Export specific picture only (0-based)
  -m, --miplevel <n>   Export specific mip level (default: 0)

Examples:
  # Export all pictures and mip levels as BMP
  tim2dump export character.tim2
  
  # Export as PNG to specific directory
  tim2dump export weapon.tim2 png -o extracted/weapon
  
  # Export only picture 2, mip level 1
  tim2dump export atlas.tim2 png -p 2 -m 1
```

#### `batch` - Process multiple files

```bash
tim2dump batch <directory> [format] [options]

Options:
  -o, --output <dir>   Output directory (preserves structure)

Examples:
  # Convert all TIM2 files in a directory tree to PNG
  tim2dump batch game_data/ png -o converted/
  
  # Export all textures, saving alongside originals
  tim2dump batch textures/ bmp
```

#### `viewc` - Terminal preview

```bash
tim2dump viewc <file> [options]

Options:
  -p, --picture <n>    View specific picture (default: 0)
  -m, --miplevel <n>   View specific mip level (default: 0)
  -w, --width <n>      Maximum display width in characters (default: 80)

Examples:
  # Basic preview
  tim2dump viewc icon.tim2
  
  # View picture 1 at 120 character width
  tim2dump viewc menu.tim2 -p 1 -w 120
  
  # View mipmap level 2
  tim2dump viewc texture.tim2 -m 2
```

## Advanced Examples

### Batch Processing with Organization

```bash
# Extract all textures from a game, organizing by type
for dir in characters enemies environments; do
  tim2dump batch "game_files/$dir" png -o "extracted/$dir"
done
```

### Information Extraction Pipeline

```bash
# Generate a report of all TIM2 files
find game_data -name "*.tim2" | while read file; do
  echo "=== $file ===" >> report.txt
  tim2dump info "$file" --verbose >> report.txt
  echo "" >> report.txt
done
```

### Automated Conversion Script

```bash
#!/bin/bash
# convert_tim2.sh - Convert TIM2 files with error handling

INPUT_DIR="$1"
OUTPUT_DIR="$2"

if [ -z "$INPUT_DIR" ] || [ -z "$OUTPUT_DIR" ]; then
  echo "Usage: $0 <input_dir> <output_dir>"
  exit 1
fi

mkdir -p "$OUTPUT_DIR"

find "$INPUT_DIR" -name "*.tim2" -o -name "*.TM2" | while read file; do
  base=$(basename "$file" .tim2)
  base=$(basename "$base" .TM2)
  
  if tim2dump export "$file" png -o "$OUTPUT_DIR/$base"; then
    echo "✓ Converted: $file"
  else
    echo "✗ Failed: $file" >&2
  fi
done
```

## Supported Formats

### Image Formats
| Format | Bits/Pixel | Description | Export Support |
|--------|------------|-------------|----------------|
| RGB32 | 32 | True color with alpha | ✓ Full |
| RGB24 | 24 | True color | ✓ Full |
| RGB16 | 16 | 5:5:5:1 color | ✓ Full |
| IDTEX8 | 8 | 256-color indexed | ✓ Full |
| IDTEX4 | 4 | 16-color indexed | ✓ Full |

### CLUT Modes
| Mode | Description | Support |
|------|-------------|---------|
| CSM1 | Sequential/compound layout | ✓ Full |
| CSM2 | Direct GS memory layout | ✓ Full |

### Special Features
- **Mipmaps**: Full support for multi-level textures
- **Extended Headers**: User data and comment extraction
- **Alignment**: Both 16-byte and 128-byte alignment modes

## Troubleshooting

### Common Issues

**Issue**: "Invalid TIM2 file signature"
- **Solution**: Verify the file is actually a TIM2 format file (should start with "TIM2" magic bytes)

**Issue**: Colors look incorrect in exported images
- **Cause**: Some games use non-standard CLUT arrangements
- **Solution**: Try using the `--verbose` flag to inspect CLUT mode and GS registers

**Issue**: Build fails with filesystem errors
- **Solution**: Ensure your compiler supports C++14 filesystem (may need to link `-lstdc++fs` on older GCC)

**Issue**: PNG export not working
- **Solution**: Verify `stb_image_write.h` is in the `third_party/` directory

### Debug Options

Enable verbose output for troubleshooting:
```bash
# Maximum verbosity
tim2dump info problematic.tim2 -v -g

# Check specific picture
tim2dump export problematic.tim2 bmp -p 0 -v
```

## Project Structure

```
tim2dump/
├── src/
│   ├── main.cpp              # Command-line interface
│   ├── tim2_parser.cpp        # Core TIM2 parsing logic
│   ├── tim2_parser.h          # Parser class definitions
│   ├── tim2_types.h           # TIM2 format structures
│   ├── image_converter.cpp    # Image export functionality
│   ├── image_converter.h      # Converter interfaces
│   ├── table_formatter.cpp    # Information display
│   ├── table_formatter.h      # Formatting utilities
│   └── utils.h                # Helper functions
├── third_party/
│   └── stb_image_write.h      # PNG export library
├── CMakeLists.txt             # Build configuration
├── LICENSE                    # MIT License
└── README.md                  # This file
```

## Technical Details

### Memory Layout
TIM2 files follow a specific memory layout optimized for PlayStation 2's Graphics Synthesizer:
- File header (16 bytes)
- Picture blocks (aligned to 16 or 128 bytes)
- Each picture contains: header, optional mipmap data, image data, optional CLUT data

### GS Register Mapping
The tool preserves original GS register values for advanced users who need to understand the exact texture configuration used by the game.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **stb_image_write** by Sean Barrett - Public domain image writing library
- **PlayStation 2 Linux Community** - TIM2 format documentation

## References

- [TIM2 File Format Specification](https://github.com/GirianSeed/tim2)
- [PlayStation 2 Graphics Synthesizer Documentation](https://psi-rockin.github.io/ps2tek/)
- [stb Libraries](https://github.com/nothings/stb)

## Contact

For bug reports and feature requests, please use the [GitHub issue tracker](https://github.com/yourusername/tim2dump/issues).
