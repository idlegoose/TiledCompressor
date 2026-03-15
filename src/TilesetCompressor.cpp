// TiledCompressor - Tiled Map Tileset Optimizer
// Copyright (c) 2026 TiledCompressor Contributors
// Licensed under the MIT License - see LICENSE file for details

#include "TilesetCompressor.h"
#include <iostream>
#include <algorithm>
#include <cmath>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb_image_write.h"

namespace TiledCompressor {

TilesetCompressor::TilesetCompressor() {
}

TilesetCompressor::~TilesetCompressor() {
}

void TilesetCompressor::RegisterUsedTile(uint32_t gid, SDL_Surface* sourceSurface,
                                         int sourceX, int sourceY, int tileWidth, int tileHeight) {
    if (gid == 0) return; // GID 0 is empty tile
    
    // Track usage count
    tileUsageCount[gid]++;
    
    // If we haven't seen this tile before, store its info
    if (usedTiles.find(gid) == usedTiles.end()) {
        TileInfo info;
        info.originalGID = gid;
        info.sourceX = sourceX;
        info.sourceY = sourceY;
        info.width = tileWidth;
        info.height = tileHeight;
        info.sourceSurface = sourceSurface;
        
        usedTiles[gid] = info;
    }
}

bool TilesetCompressor::GenerateCompressedTileset(const std::string& outputPath, 
                                                   int tileWidth, int tileHeight) {
    if (usedTiles.empty()) {
        std::cerr << "No tiles to compress!" << std::endl;
        return false;
    }

    // Sort GIDs by usage count (most used first) for optimal cache performance
    std::vector<std::pair<uint32_t, int>> gidCountPairs;
    gidCountPairs.reserve(usedTiles.size());
    for (const auto& pair : usedTiles) {
        uint32_t gid = pair.first;
        int count = tileUsageCount[gid];
        gidCountPairs.push_back({gid, count});
    }
    
    // Sort by count (descending), then by GID (ascending) for ties
    std::sort(gidCountPairs.begin(), gidCountPairs.end(), 
        [](const std::pair<uint32_t, int>& a, const std::pair<uint32_t, int>& b) {
            if (a.second != b.second) {
                return a.second > b.second; // Higher count first
            }
            return a.first < b.first; // Lower GID first for ties
        });
    
    // Extract sorted GIDs for processing
    std::vector<uint32_t> sortedGIDs;
    sortedGIDs.reserve(gidCountPairs.size());
    for (const auto& pair : gidCountPairs) {
        sortedGIDs.push_back(pair.first);
    }

    // Calculate tileset dimensions (pack into roughly square texture)
    size_t numTiles = sortedGIDs.size();
    int tilesPerRow = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(numTiles))));
    int numRows = static_cast<int>(std::ceil(static_cast<double>(numTiles) / tilesPerRow));

    int outputWidth = tilesPerRow * tileWidth;
    int outputHeight = numRows * tileHeight;

    std::cout << "Generating compressed tileset: " << outputWidth << "x" << outputHeight 
              << " (" << numTiles << " tiles, " << tilesPerRow << " tiles per row)" << std::endl;

    // Create output surface
    SDL_Surface* outputSurface = SDL_CreateSurface(outputWidth, outputHeight, SDL_PIXELFORMAT_RGBA32);
    if (!outputSurface) {
        std::cerr << "Failed to create output surface: " << SDL_GetError() << std::endl;
        return false;
    }

    // Fill with transparent pixels
    SDL_FillSurfaceRect(outputSurface, nullptr, SDL_MapSurfaceRGBA(outputSurface, 0, 0, 0, 0));

    // Copy each tile to the output surface
    uint32_t newIndex = 1; // Start from 1 (0 is reserved for empty)
    for (size_t i = 0; i < sortedGIDs.size(); ++i) {
        uint32_t gid = sortedGIDs[i];
        TileInfo& info = usedTiles[gid];
        
        // Calculate position in output tileset
        int col = i % tilesPerRow;
        int row = i / tilesPerRow;
        int destX = col * tileWidth;
        int destY = row * tileHeight;

        // Source rectangle
        SDL_Rect srcRect = { info.sourceX, info.sourceY, tileWidth, tileHeight };
        SDL_Rect dstRect = { destX, destY, tileWidth, tileHeight };

        // Convert source surface to same format if needed
        SDL_Surface* convertedSrc = info.sourceSurface;
        bool needsFreeing = false;
        
        if (info.sourceSurface->format != outputSurface->format) {
            convertedSrc = SDL_ConvertSurface(info.sourceSurface, outputSurface->format);
            needsFreeing = true;
        }

        // Blit the tile
        if (SDL_BlitSurface(convertedSrc, &srcRect, outputSurface, &dstRect) < 0) {
            std::cerr << "Failed to blit tile " << gid << ": " << SDL_GetError() << std::endl;
        }

        if (needsFreeing) {
            SDL_DestroySurface(convertedSrc);
        }

        // Store new index
        info.newIndex = newIndex;
        gidToNewIndex[gid] = newIndex;
        newIndex++;
    }

    // Save to PNG using stb_image_write
    // Convert SDL surface to raw pixel data
    std::vector<uint8_t> pixels(outputWidth * outputHeight * 4);
    
    if (SDL_MUSTLOCK(outputSurface)) {
        SDL_LockSurface(outputSurface);
    }
    
    memcpy(pixels.data(), outputSurface->pixels, pixels.size());
    
    if (SDL_MUSTLOCK(outputSurface)) {
        SDL_UnlockSurface(outputSurface);
    }

    // Write PNG
    int result = stbi_write_png(outputPath.c_str(), outputWidth, outputHeight, 4, 
                               pixels.data(), outputWidth * 4);
    
    SDL_DestroySurface(outputSurface);

    if (!result) {
        std::cerr << "Failed to write PNG file: " << outputPath << std::endl;
        return false;
    }

    std::cout << "Compressed tileset saved to: " << outputPath << std::endl;
    return true;
}

uint32_t TilesetCompressor::GetNewIndex(uint32_t originalGID) const {
    auto it = gidToNewIndex.find(originalGID);
    if (it != gidToNewIndex.end()) {
        return it->second;
    }
    return 0; // Empty tile
}

void TilesetCompressor::PrintTileStatistics() const {
    std::cout << "\n=== Tile Usage Statistics ===" << std::endl;
    std::cout << "Total unique tiles used: " << usedTiles.size() << std::endl;
    std::cout << "\nTile GID usage counts (sorted by frequency, matching tileset order):" << std::endl;
    
    // Sort by usage count (descending), then by GID (ascending) for ties
    std::vector<std::pair<uint32_t, int>> sortedCounts(tileUsageCount.begin(), tileUsageCount.end());
    std::sort(sortedCounts.begin(), sortedCounts.end(),
        [](const std::pair<uint32_t, int>& a, const std::pair<uint32_t, int>& b) {
            if (a.second != b.second) {
                return a.second > b.second; // Higher count first
            }
            return a.first < b.first; // Lower GID first for ties
        });
    
    for (const auto& pair : sortedCounts) {
        if (pair.second > 0) {
            std::cout << "  GID " << pair.first << ": " << pair.second << " times" << std::endl;
        }
    }
    std::cout << "============================\n" << std::endl;
}

} // namespace TiledCompressor
