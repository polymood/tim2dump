# tim2dump

**tim2dump** is a simple command-line utility for working with PlayStation 2 **TIM2** image format files.

It can parse TIM2 files, display detailed information, export images, and even visualize them directly in your terminal.

## Features

- **Detailed TIM2 parsing** – View complete header and image metadata.
- **Image export**:
  - BMP (built-in, no external dependencies)
  - PNG (via [`stb_image_write.h`](https://github.com/nothings/stb))
- **Full TIM2 format support**:
  - Pixel formats: `RGB32`, `RGB24`, `RGB16`, `IDTEX8`, `IDTEX4`
  - CLUT (palette) support with `CSM1` / `CSM2` modes
  - MipMap support for multi-level textures
- **Terminal visualization**:
  - ANSI 256-color block rendering

## Building

### Prerequisites

- **C++20** compatible compiler:
  - GCC 10+
  - Clang 10+
  - MSVC 2019+
- **CMake** 3.20 or newer
- *(Optional)* CLion or any other CMake-based IDE

### Setup

1. Clone the repository:
   ```bash
   git clone https://github.com/polymood/tim2dump.git
   cd tim2dump
    ```

2. Download [`stb_image_write.h`](https://github.com/nothings/stb/blob/master/stb_image_write.h)
   Place it inside a `third_party/` directory in the project root:

   ```
   tim2dump/
   ├── src/
   ├── third_party/
   │   └── stb_image_write.h
   └── CMakeLists.txt
   ```

3. Build with CMake:

   ```bash
   cmake -B build
   cmake --build build
   ```

4. The resulting binary will be in:

   ```
   build/tim2dump
   ```

## Usage

```bash
tim2dump <input.tim2> [options]
```

**Options**:

* `-v, --verbose` – Show detailed information.
* `-g, --gs-registers` – Display GS register details.
* `-o, --output <name>` – Output base filename for export.
* `-p, --picture <n>` – Select specific picture (0-based index).
* `-m, --miplevel <n>` – Select MIP level (default: 0).
* `-w, --width <n>` – Max width for terminal display (default: 80).


Example:

```bash
tim2dump example.tim2 --export png --out out/image
```

## Credits

* **PNG export** uses [`stb_image_write.h`](https://github.com/nothings/stb)
  by [Sean Barrett](https://nothings.org/), licensed under **public domain** / **MIT**.
* **TIM2 specifications** at https://github.com/GirianSeed/tim2
## License

This project is licensed under the MIT License – see the [LICENSE](LICENSE) file for details.
