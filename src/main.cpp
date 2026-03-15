// TiledCompressor - Tiled Map Tileset Optimizer
// Copyright (c) 2026 TiledCompressor Contributors
// Licensed under the MIT License - see LICENSE file for details

#include <iostream>
#include <filesystem>
#include <limits>
#include "SDL_compat.h"

// SDL_main.h must only be included in the file with main()
#ifdef USE_SDL3
    #include <SDL3/SDL_main.h>
#elif defined(USE_SDL2)
    #include <SDL2/SDL_main.h>
#endif

#include "FileDialog.h"
#include "MapConverter.h"

namespace fs = std::filesystem;

void WaitForExit() {
    std::cout << "\n\nPress Enter to exit..." << std::endl;
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

int main(int argc, char* argv[]) {
    std::cout << "=== TiledCompressor ===" << std::endl;
    std::cout << "Version 1.0.0\n" << std::endl;
    
    // Show build info
    std::cout << "Build configuration:" << std::endl;
#ifdef USE_SDL3
    std::cout << "  SDL Version: SDL3" << std::endl;
#elif defined(USE_SDL2)
    std::cout << "  SDL Version: SDL2" << std::endl;
#endif
    std::cout << std::endl;
    std::cout.flush();

    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        WaitForExit();
        return 1;
    }

    // Initialize SDL_image
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cerr << "Failed to initialize SDL_image: " << IMG_GetError() << std::endl;
        SDL_Quit();
        WaitForExit();
        return 1;
    }
    
    // Show SDL_image version and supported formats
    std::cout << "SDL_image initialized successfully" << std::endl;
#ifdef USE_SDL3
    std::cout << "SDL3_image version: " << IMG_Version() << std::endl;
#endif
    
    // Test if we can actually load a PNG
    std::cout << "Testing PNG support..." << std::endl;
    std::cout.flush();

    std::string tmxPath;

    // Check if file path was provided as command line argument
    if (argc > 1) {
        tmxPath = argv[1];
        std::cout << "Using TMX file from command line: " << tmxPath << std::endl;
    } else {
        // Open file dialog
        std::cout << "Opening file dialog..." << std::endl;
        tmxPath = TiledCompressor::OpenFileDialog();
        
        if (tmxPath.empty()) {
            std::cout << "No file selected. Exiting." << std::endl;
            IMG_Quit();
            SDL_Quit();
            WaitForExit();
            return 0;
        }
    }

    // Verify file exists
    if (!fs::exists(tmxPath)) {
        std::cerr << "Error: File does not exist: " << tmxPath << std::endl;
        IMG_Quit();
        SDL_Quit();
        WaitForExit();
        return 1;
    }

    // Create converter
    TiledCompressor::MapConverter converter;

    // Load the map
    if (!converter.LoadMap(tmxPath)) {
        std::cerr << "\n*** Failed to load map! ***" << std::endl;
        IMG_Quit();
        SDL_Quit();
        WaitForExit();
        return 1;
    }

    // Process the map (load tilesets, render, track tiles)
    if (!converter.ProcessMap()) {
        std::cerr << "\n*** Failed to process map! ***" << std::endl;
        IMG_Quit();
        SDL_Quit();
        WaitForExit();
        return 1;
    }

    // Create output directory
    std::string outputDir = (fs::path(converter.GetWorkingDirectory()) / 
                             (converter.GetMapName() + "_converted")).string();

    // Save converted map
    if (!converter.SaveConvertedMap(outputDir)) {
        std::cerr << "\n*** Failed to save converted map! ***" << std::endl;
        IMG_Quit();
        SDL_Quit();
        WaitForExit();
        return 1;
    }

    std::cout << "\n=== SUCCESS! ===" << std::endl;

    // Cleanup
    IMG_Quit();
    SDL_Quit();

    WaitForExit();

    return 0;
}
