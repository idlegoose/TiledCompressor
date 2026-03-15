// TiledCompressor - Tiled Map Tileset Optimizer
// Copyright (c) 2026 TiledCompressor Contributors
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "SDL_compat.h"

namespace TiledCompressor {

struct TileInfo {
    uint32_t originalGID;
    uint32_t newIndex;
    int sourceX, sourceY;
    int width, height;
    SDL_Surface* sourceSurface;
};

/**
 * Manages tile compression and tileset generation
 */
class TilesetCompressor {
public:
    TilesetCompressor();
    ~TilesetCompressor();

    /**
     * Register a tile that was used in the map
     * @param gid The global tile ID from the original map
     * @param sourceSurface The surface containing the tile
     * @param sourceX X position of the tile in the source surface
     * @param sourceY Y position of the tile in the source surface
     * @param tileWidth Width of the tile
     * @param tileHeight Height of the tile
     */
    void RegisterUsedTile(uint32_t gid, SDL_Surface* sourceSurface, 
                         int sourceX, int sourceY, int tileWidth, int tileHeight);

    /**
     * Generate the compressed tileset image
     * @param outputPath Path where to save the compressed tileset PNG
     * @param tileWidth Width of individual tiles
     * @param tileHeight Height of individual tiles
     * @return true on success
     */
    bool GenerateCompressedTileset(const std::string& outputPath, int tileWidth, int tileHeight);

    /**
     * Get the new index for an original GID
     * @param originalGID The original global tile ID
     * @return The new index in the compressed tileset, or 0 if not found
     */
    uint32_t GetNewIndex(uint32_t originalGID) const;

    /**
     * Print statistics about tile usage to command line
     */
    void PrintTileStatistics() const;

    /**
     * Get the count of unique tiles
     */
    size_t GetUniqueTileCount() const { return usedTiles.size(); }

private:
    std::map<uint32_t, TileInfo> usedTiles;  // GID -> TileInfo
    std::map<uint32_t, uint32_t> gidToNewIndex; // Original GID -> New Index
    std::map<uint32_t, int> tileUsageCount; // GID -> usage count
};

} // namespace TiledCompressor
