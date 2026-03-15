// TiledCompressor - Tiled Map Tileset Optimizer
// Copyright (c) 2026 TiledCompressor Contributors
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include "SDL_compat.h"
#include <tmxlite/Map.hpp>
#include "TilesetCompressor.h"

namespace TiledCompressor {

/**
 * Handles loading and converting TMX maps
 */
class MapConverter {
public:
    MapConverter();
    ~MapConverter();

    /**
     * Load a TMX map file
     * @param tmxPath Path to the .tmx file
     * @return true on success
     */
    bool LoadMap(const std::string& tmxPath);

    /**
     * Process the map: render it and track used tiles
     * @return true on success
     */
    bool ProcessMap();

    /**
     * Save the converted map and tileset
     * @param outputDir Directory where to save output files
     * @return true on success
     */
    bool SaveConvertedMap(const std::string& outputDir);

    /**
     * Get the map name (without extension)
     */
    std::string GetMapName() const { return mapName; }

    /**
     * Get the working directory (directory containing the TMX file)
     */
    std::string GetWorkingDirectory() const { return workingDirectory; }

private:
    /**
     * Load tileset images referenced in the map
     */
    bool LoadTilesets();

    /**
     * Render all map layers and track used tiles
     */
    bool RenderAndTrackTiles();
    
    /**
     * Process object layers and track used object tiles
     */
    bool ProcessObjectLayers();

    /**
     * Generate the new TMX file with remapped GIDs
     */
    bool GenerateNewTMX(const std::string& outputPath);

    std::string tmxPath;
    std::string workingDirectory;
    std::string mapName;
    
    tmx::Map map;
    std::vector<SDL_Surface*> tilesetSurfaces;
    std::unique_ptr<TilesetCompressor> compressor;
    std::unique_ptr<TilesetCompressor> objectCompressor; // Separate compressor for objects
    
    // Track object tiles with their sizes (GID -> {width, height})
    std::map<uint32_t, std::pair<int, int>> objectTileSizes;
    int maxObjectTileWidth;   // Maximum width among all object tiles
    int maxObjectTileHeight;  // Maximum height among all object tiles
    
    SDL_Surface* renderSurface;
    int tileWidth;
    int tileHeight;
    int roundedMapWidth;   // Map width rounded up to multiple of 16
    int roundedMapHeight;  // Map height rounded up to multiple of 16
};

} // namespace TiledCompressor
