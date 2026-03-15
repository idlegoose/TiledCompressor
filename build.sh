#!/bin/bash

echo "======================================"
echo "TiledCompressor - Quick Build"
echo "======================================"
echo

# Download stb_image_write.h if not present
if [ ! -f "third_party/stb_image_write.h" ]; then
    echo "Downloading stb_image_write.h..."
    wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h -O third_party/stb_image_write.h
    if [ $? -ne 0 ]; then
        echo "Failed to download stb_image_write.h"
        echo "Please download manually from: https://github.com/nothings/stb"
        exit 1
    fi
    echo "Downloaded successfully!"
    echo
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring project..."
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    echo "Make sure CMake and all dependencies are installed."
    exit 1
fi

# Build
echo
echo "Building project..."
cmake --build .
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo
echo "======================================"
echo "Build complete!"
echo "Executable: build/TiledCompressor"
echo "======================================"
echo
