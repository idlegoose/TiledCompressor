# TiledCompressor

A command-line tool that compresses Tiled map files (.tmx) by optimizing their tilesets into a single compressed tileset containing only the tiles actually used in the map.

## Features

- **File Dialog Support**: Select TMX files using a native file dialog (Windows/Linux)
- **Automatic Tileset Loading**: Loads all referenced tileset images
- **Layer Rendering**: Renders all map layers from bottom to top
- **Tile Usage Tracking**: Tracks which tiles are actually used in the map
- **Statistics Output**: Displays tile usage counts to the command line
- **Compressed Tileset Generation**: Creates a new PNG tileset with only used tiles
- **Map Remapping**: Generates a new TMX file with updated tile indices

## Requirements

### Libraries

- **SDL3**: Graphics and image loading
- **tmxlite**: TMX/TSX file parsing
- **stb_image_write**: PNG image writing (header-only)

### Build Tools

- CMake 3.20 or higher
- C++17 compatible compiler (MSVC 2019+, GCC 7+, Clang 5+)

## Building

### Windows

1. **Install Dependencies**

   **Option A: Using vcpkg (Recommended)**
   ```powershell
   # Install vcpkg if you haven't
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   
   # Install dependencies
   # IMPORTANT: sdl3-image needs [png] feature for PNG support
   .\vcpkg install sdl3 sdl3-image[png] tmxlite
   ```

   **Option B: Manual Installation**
   - Download and install SDL3 from [libsdl.org](https://www.libsdl.org/)
   - Clone and build tmxlite from [GitHub](https://github.com/fallahn/tmxlite)

2. **Download stb_image_write.h**
   
   Download the complete `stb_image_write.h` from the [stb repository](https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h) and place it in `third_party/stb_image_write.h` (replacing the stub file).

3. **Configure and Build**
   ```powershell
   mkdir build
   cd build
   
   # If using vcpkg:
   cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
   
   # Or without vcpkg:
   cmake ..
   
   # Build
   cmake --build . --config Release
   ```

### Linux

1. **Install Dependencies**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install cmake g++ libsdl3-dev libsdl3-image-dev zenity
   
   # Clone and build tmxlite (if not in package manager)
   git clone https://github.com/fallahn/tmxlite.git
   cd tmxlite/tmxlite
   mkdir build && cd build
   cmake ..
   make
   sudo make install
   ```

2. **Download stb_image_write.h**
   ```bash
   wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h -O third_party/stb_image_write.h
   ```

3. **Build**
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

## Usage

### Interactive Mode (File Dialog)

Simply run the executable without arguments:

```bash
./TiledCompressor
```

A file dialog will appear allowing you to select a TMX file.

### Command Line Mode

Provide the TMX file path as an argument:

```bash
./TiledCompressor path/to/your/map.tmx
```

### Output

The program will:

1. Load the selected TMX map and all referenced tilesets
2. Process all layers from bottom to top
3. Track which tiles are actually used
4. Display tile usage statistics in the console:
   ```
   === Tile Usage Statistics ===
   Total unique tiles used: 42
   
   Tile GID usage counts (non-zero only):
     GID 1: 150 times
     GID 5: 80 times
     GID 12: 45 times
     ...
   ```
5. Create a new directory: `[mapname]_converted/`
6. Generate `[mapname]_tileset_compressed.png` (optimized tileset)
7. Generate `[mapname]_converted.tmx` (new map file with remapped indices)

### Example

```
Input:  maps/level1.tmx
        maps/tileset1.png
        maps/tileset2.png

Output: maps/level1_converted/
        maps/level1_converted/level1_tileset_compressed.png
        maps/level1_converted/level1_converted.tmx
```

## How It Works

1. **File Selection**: User selects a TMX file via dialog or command line
2. **Working Directory**: The directory containing the TMX file becomes the working directory
3. **Map Loading**: tmxlite parses the TMX file and extracts map structure
4. **Tileset Loading**: SDL3_image loads all tileset images referenced in the map
5. **Layer Processing**: Each layer is processed sequentially, bottom to top
6. **Tile Tracking**: Each non-zero tile GID is recorded with its source location
7. **Usage Counting**: Tile usage is tallied for statistics
8. **Tileset Compression**: Used tiles are packed into a new square-ish tileset
9. **Map Remapping**: Original GIDs are replaced with new sequential indices
10. **File Output**: New tileset PNG and TMX file are saved

## Project Structure

```
TiledCompressor/
├── CMakeLists.txt           # Build configuration
├── README.md                # This file
├── .gitignore              # Git ignore rules
├── third_party/
│   └── stb_image_write.h   # PNG writing library
└── src/
    ├── main.cpp            # Entry point
    ├── FileDialog.h/cpp    # Native file dialog
    ├── MapConverter.h/cpp  # Main conversion logic
    └── TilesetCompressor.h/cpp  # Tileset optimization
```

## Notes

- Tile GID 0 represents an empty tile and is not included in the compressed tileset
- The compressed tileset is arranged in a roughly square layout for optimal texture use
- All tile usage statistics exclude zero-count tiles
- The tool preserves layer structure but only processes tile layers (not object layers)

## Troubleshooting

### "Failed to load tileset image"
- Ensure tileset image paths in the TMX file are relative to the TMX file location
- Verify that all referenced image files exist

### "Failed to initialize SDL"
- Ensure SDL3 is properly installed and in your PATH (Windows) or library path (Linux)

### "No file selected"
- On Linux, ensure either `zenity` or `kdialog` is installed for the file dialog

### Link Errors
- If using vcpkg, ensure the toolchain file is specified in CMake
- Verify all dependencies are installed correctly

## License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

### Third-Party Dependencies

This project uses the following open-source libraries, all with permissive licenses:

- **SDL3** - zlib License - Image loading and processing
- **SDL3_image** - zlib License - PNG image support
- **tmxlite** - zlib License - TMX/TSX file parsing
- **stb_image_write** - MIT/Public Domain - PNG writing

For complete license texts and attribution, see [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).

### Tiled Map Format

This tool reads and writes TMX (Tiled Map XML) files, which is an open format created by the [Tiled Map Editor](https://www.mapeditor.org/). The file format itself is freely usable. Note that while Tiled (the editor software) is GPL-licensed, using the TMX format does not impose GPL requirements on your project.

## Contributing

Contributions are welcome! Feel free to submit issues or pull requests for improvements.
