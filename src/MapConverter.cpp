// TiledCompressor - Tiled Map Tileset Optimizer
// Copyright (c) 2026 TiledCompressor Contributors
// Licensed under the MIT License - see LICENSE file for details

#include "MapConverter.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <map>
#include <vector>
#include "SDL_compat.h"
#include <tmxlite/Layer.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <tmxlite/Object.hpp>
#include <tmxlite/Property.hpp>
#include <tmxlite/Tileset.hpp>
#include "../third_party/stb_image_write.h"

namespace fs = std::filesystem;

namespace TiledCompressor {

MapConverter::MapConverter() 
    : renderSurface(nullptr), tileWidth(0), tileHeight(0), 
      roundedMapWidth(0), roundedMapHeight(0),
      maxObjectTileWidth(0), maxObjectTileHeight(0) {
    compressor = std::make_unique<TilesetCompressor>();
    objectCompressor = std::make_unique<TilesetCompressor>();
}

MapConverter::~MapConverter() {
    // Clean up surfaces
    for (auto surface : tilesetSurfaces) {
        if (surface) {
            SDL_DestroySurface(surface);
        }
    }
    
    if (renderSurface) {
        SDL_DestroySurface(renderSurface);
    }
}

bool MapConverter::LoadMap(const std::string& tmxPath) {
    this->tmxPath = tmxPath;
    
    // Extract working directory and map name
    fs::path path(tmxPath);
    workingDirectory = path.parent_path().string();
    mapName = path.stem().string();
    
    std::cout << "Loading map: " << tmxPath << std::endl;
    std::cout << "Working directory: " << workingDirectory << std::endl;
    std::cout << "Map name: " << mapName << std::endl;
    
    // Load the TMX map
    if (!map.load(tmxPath)) {
        std::cerr << "Failed to load TMX file: " << tmxPath << std::endl;
        return false;
    }
    
    auto& mapTileSize = map.getTileSize();
    tileWidth = static_cast<int>(mapTileSize.x);
    tileHeight = static_cast<int>(mapTileSize.y);
    
    // Round up map dimensions to nearest multiple of 16 for chunk alignment
    auto mapSize = map.getTileCount();
    roundedMapWidth = ((mapSize.x + 15) / 16) * 16;
    roundedMapHeight = ((mapSize.y + 15) / 16) * 16;
    
    std::cout << "Map size: " << mapSize.x << "x" << mapSize.y << std::endl;
    std::cout << "Rounded map size: " << roundedMapWidth << "x" << roundedMapHeight << " (for 16x16 chunks)" << std::endl;
    std::cout << "Tile size: " << tileWidth << "x" << tileHeight << std::endl;
    
    return true;
}

bool MapConverter::LoadTilesets() {
    const auto& tilesets = map.getTilesets();
    
    if (tilesets.empty()) {
        std::cerr << "No tilesets found in map!" << std::endl;
        return false;
    }
    
    std::cout << "\n=== Loading Tilesets ===" << std::endl;
    std::cout << "Total tilesets to load: " << tilesets.size() << std::endl;
    std::cout.flush();
    
    size_t tilesetIndex = 0;
    for (const auto& tileset : tilesets) {
        tilesetIndex++;
        std::string imagePath = tileset.getImagePath();
        
        // Skip tilesets without images
        if (imagePath.empty()) {
            std::cout << "Skipping tileset with no image path" << std::endl;
            tilesetSurfaces.push_back(nullptr);
            continue;
        }
        
        // Make path relative to working directory
        fs::path fullPath = fs::path(workingDirectory) / imagePath;
        std::string fullPathStr = fullPath.string();
        
        std::cout << "\n[Tileset " << tilesetIndex << "/" << tilesets.size() << "] Loading: " << fullPathStr << std::endl;
        
        // Check if file exists before attempting to load
        if (!fs::exists(fullPath)) {
            std::cerr << "  ERROR: File does not exist!" << std::endl;
            tilesetSurfaces.push_back(nullptr);
            continue;
        }
        
        SDL_Surface* surface = IMG_Load(fullPathStr.c_str());
        if (!surface) {
            std::cerr << "  Failed to load image: " << SDL_GetError() << std::endl;
            tilesetSurfaces.push_back(nullptr);
            continue;
        }
        
        std::cout << "  SUCCESS! Loaded " << surface->w << "x" << surface->h << " surface" << std::endl;
        
        tilesetSurfaces.push_back(surface);
        
        std::cout << "  Tile size: " << tileset.getTileSize().x << "x" << tileset.getTileSize().y << std::endl;
        std::cout << "  First GID: " << tileset.getFirstGID() << std::endl;
        std::cout << "  Tile count: " << tileset.getTileCount() << std::endl;
        std::cout.flush();
    }
    
    // Check if we loaded at least one tileset
    bool hasValidTileset = false;
    for (auto surface : tilesetSurfaces) {
        if (surface != nullptr) {
            hasValidTileset = true;
            break;
        }
    }
    
    if (!hasValidTileset) {
        std::cerr << "\n*** ERROR: No tilesets could be loaded! ***" << std::endl;
        std::cerr << "All " << tilesets.size() << " tileset images failed to load." << std::endl;
        std::cerr << "Please check that:" << std::endl;
        std::cerr << "  1. The PNG files are valid and not corrupted" << std::endl;
        std::cerr << "  2. The PNG format is supported (standard PNG, not exotic variants)" << std::endl;
        std::cerr << "  3. Required codec DLLs are present (libpng16.dll, etc.)" << std::endl;
        return false;
    }
    
    int loadedCount = 0;
    for (auto surface : tilesetSurfaces) {
        if (surface != nullptr) loadedCount++;
    }
    std::cout << "\n=== Tileset Loading Complete ===" << std::endl;
    std::cout << "Successfully loaded: " << loadedCount << " / " << tilesets.size() << " tilesets" << std::endl;
    std::cout.flush();
    return true;
}

bool MapConverter::RenderAndTrackTiles() {
    std::cout << "\n=== Rendering Map and Tracking Tiles ===" << std::endl;
    std::cout.flush();
    
    // Create render surface
    auto mapSize = map.getTileCount();
    int mapWidth = mapSize.x * tileWidth;
    int mapHeight = mapSize.y * tileHeight;
    
    renderSurface = SDL_CreateSurface(mapWidth, mapHeight, SDL_PIXELFORMAT_RGBA32);
    if (!renderSurface) {
        std::cerr << "Failed to create render surface: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Fill with transparent background
    SDL_FillSurfaceRect(renderSurface, nullptr, SDL_MapSurfaceRGBA(renderSurface, 0, 0, 0, 0));
    
    std::cout << "\nProcessing layers..." << std::endl;
    
    const auto& layers = map.getLayers();
    const auto& tilesets = map.getTilesets();
    
    // Process each layer from bottom to top
    for (const auto& layer : layers) {
        if (layer->getType() != tmx::Layer::Type::Tile) {
            std::cout << "Skipping non-tile layer: " << layer->getName() << std::endl;
            continue;
        }
        
        const auto* tileLayer = dynamic_cast<const tmx::TileLayer*>(layer.get());
        if (!tileLayer) {
            std::cout << "WARNING: Failed to cast to TileLayer!" << std::endl;
            continue;
        }
        
        std::cout << "Processing layer: " << layer->getName() << " (" << mapSize.x << "x" << mapSize.y << " tiles)" << std::endl;
        std::cout.flush();
        
        const auto& tiles = tileLayer->getTiles();
        const auto& chunks = tileLayer->getChunks();
        int nonZeroCount = 0;
        
        // Handle chunked layers (for infinite maps)
        if (!chunks.empty()) {
            for (const auto& chunk : chunks) {
                
                for (int y = 0; y < chunk.size.y; ++y) {
                    for (int x = 0; x < chunk.size.x; ++x) {
                        size_t index = y * chunk.size.x + x;
                        if (index >= chunk.tiles.size()) continue;
                        
                        uint32_t gid = chunk.tiles[index].ID;
                        if (gid == 0) continue; // Empty tile
                        nonZeroCount++;
                        
                        // Find which tileset this GID belongs to
                        const tmx::Tileset* tileset = nullptr;
                        size_t tilesetIndex = 0;
                        
                        for (size_t i = 0; i < tilesets.size(); ++i) {
                            if (gid >= tilesets[i].getFirstGID()) {
                                if (i + 1 < tilesets.size()) {
                                    if (gid < tilesets[i + 1].getFirstGID()) {
                                        tileset = &tilesets[i];
                                        tilesetIndex = i;
                                        break;
                                    }
                                } else {
                                    tileset = &tilesets[i];
                                    tilesetIndex = i;
                                    break;
                                }
                            }
                        }
                        
                        if (!tileset || tilesetIndex >= tilesetSurfaces.size()) {
                            continue;
                        }
                        
                        // Skip if tileset surface failed to load
                        if (tilesetSurfaces[tilesetIndex] == nullptr) {
                            continue;
                        }
                        
                        // Calculate tile position in tileset
                        uint32_t localID = gid - tileset->getFirstGID();
                        int tilesetTileWidth = tileset->getTileSize().x;
                        int tilesetTileHeight = tileset->getTileSize().y;
                        int tilesPerRow = tilesetSurfaces[tilesetIndex]->w / tilesetTileWidth;
                        
                        int srcX = (localID % tilesPerRow) * tilesetTileWidth;
                        int srcY = (localID / tilesPerRow) * tilesetTileHeight;
                        
                        // Register this tile
                        compressor->RegisterUsedTile(gid, tilesetSurfaces[tilesetIndex],
                                                    srcX, srcY, tilesetTileWidth, tilesetTileHeight);
                        
                        // Render to surface (for visualization, optional)
                        int mapX = chunk.position.x + x;
                        int mapY = chunk.position.y + y;
                        
                        SDL_Rect srcRect = { srcX, srcY, tilesetTileWidth, tilesetTileHeight };
                        SDL_Rect dstRect = { 
                            mapX * tileWidth, 
                            mapY * tileHeight, 
                            tileWidth, 
                            tileHeight 
                        };
                        
                        SDL_BlitSurface(tilesetSurfaces[tilesetIndex], &srcRect, renderSurface, &dstRect);
                    }
                }
            }
        }
        // Handle flat tile arrays (for non-infinite maps)
        else if (!tiles.empty()) {
            for (size_t y = 0; y < mapSize.y; ++y) {
                for (size_t x = 0; x < mapSize.x; ++x) {
                    size_t index = y * mapSize.x + x;
                    uint32_t gid = tiles[index].ID;
                    
                    if (gid == 0) continue; // Empty tile
                    nonZeroCount++;
                
                // Find which tileset this GID belongs to
                const tmx::Tileset* tileset = nullptr;
                size_t tilesetIndex = 0;
                
                for (size_t i = 0; i < tilesets.size(); ++i) {
                    if (gid >= tilesets[i].getFirstGID()) {
                        if (i + 1 < tilesets.size()) {
                            if (gid < tilesets[i + 1].getFirstGID()) {
                                tileset = &tilesets[i];
                                tilesetIndex = i;
                                break;
                            }
                        } else {
                            tileset = &tilesets[i];
                            tilesetIndex = i;
                            break;
                        }
                    }
                }
                
                if (!tileset || tilesetIndex >= tilesetSurfaces.size()) {
                    continue;
                }
                
                // Skip if tileset surface failed to load
                if (tilesetSurfaces[tilesetIndex] == nullptr) {
                    continue;
                }
                
                // Calculate tile position in tileset
                uint32_t localID = gid - tileset->getFirstGID();
                int tilesetTileWidth = tileset->getTileSize().x;
                int tilesetTileHeight = tileset->getTileSize().y;
                int tilesPerRow = tilesetSurfaces[tilesetIndex]->w / tilesetTileWidth;
                
                int srcX = (localID % tilesPerRow) * tilesetTileWidth;
                int srcY = (localID / tilesPerRow) * tilesetTileHeight;
                
                // Register this tile
                compressor->RegisterUsedTile(gid, tilesetSurfaces[tilesetIndex],
                                            srcX, srcY, tilesetTileWidth, tilesetTileHeight);
                
                // Render to surface (for visualization, optional)
                SDL_Rect srcRect = { srcX, srcY, tilesetTileWidth, tilesetTileHeight };
                SDL_Rect dstRect = { 
                    static_cast<int>(x * tileWidth), 
                    static_cast<int>(y * tileHeight), 
                    tileWidth, 
                    tileHeight 
                };
                
                SDL_BlitSurface(tilesetSurfaces[tilesetIndex], &srcRect, renderSurface, &dstRect);
                }
            }
        }
    }
    
    std::cout << "Map rendering complete!" << std::endl;
    std::cout.flush();
    return true;
}

bool MapConverter::ProcessObjectLayers() {
    std::cout << "\n=== Processing Object Layers ===" << std::endl;
    
    const auto& layers = map.getLayers();
    const auto& tilesets = map.getTilesets();
    int objectLayerCount = 0;
    int totalObjects = 0;
    
    for (const auto& layer : layers) {
        if (layer->getType() != tmx::Layer::Type::Object) {
            continue;
        }
        
        objectLayerCount++;
        const auto* objectGroup = dynamic_cast<const tmx::ObjectGroup*>(layer.get());
        if (!objectGroup) {
            std::cout << "WARNING: Failed to cast to ObjectGroup!" << std::endl;
            continue;
        }
        
        std::cout << "Processing object layer: " << layer->getName() << std::endl;
        const auto& objects = objectGroup->getObjects();
        std::cout << "  Objects in layer: " << objects.size() << std::endl;
        
        for (const auto& obj : objects) {
            uint32_t tileID = obj.getTileID();
            
            // Only process objects that reference tiles (tileID > 0)
            if (tileID == 0) {
                continue;
            }
            
            totalObjects++;
            
            // Find which tileset this GID belongs to
            const tmx::Tileset* tileset = nullptr;
            size_t tilesetIndex = 0;
            
            for (size_t i = 0; i < tilesets.size(); ++i) {
                if (tileID >= tilesets[i].getFirstGID()) {
                    if (i + 1 < tilesets.size()) {
                        if (tileID < tilesets[i + 1].getFirstGID()) {
                            tileset = &tilesets[i];
                            tilesetIndex = i;
                            break;
                        }
                    } else {
                        tileset = &tilesets[i];
                        tilesetIndex = i;
                        break;
                    }
                }
            }
            
            if (!tileset || tilesetIndex >= tilesetSurfaces.size()) {
                continue;
            }
            
            if (tilesetSurfaces[tilesetIndex] == nullptr) {
                continue;
            }
            
            // Get tile dimensions from the source tileset (not AABB)
            // This ensures we use the actual tile size defined in the tileset
            int tilesetTileWidth = tileset->getTileSize().x;
            int tilesetTileHeight = tileset->getTileSize().y;
            
            // Store object tile size and track maximum dimensions
            objectTileSizes[tileID] = {tilesetTileWidth, tilesetTileHeight};
            maxObjectTileWidth = std::max(maxObjectTileWidth, tilesetTileWidth);
            maxObjectTileHeight = std::max(maxObjectTileHeight, tilesetTileHeight);
            
            // Calculate tile position in tileset
            uint32_t localID = tileID - tileset->getFirstGID();
            int tilesPerRow = tilesetSurfaces[tilesetIndex]->w / tilesetTileWidth;
            
            int srcX = (localID % tilesPerRow) * tilesetTileWidth;
            int srcY = (localID / tilesPerRow) * tilesetTileHeight;
            
            // Register with object compressor using tileset's tile dimensions
            objectCompressor->RegisterUsedTile(tileID, tilesetSurfaces[tilesetIndex],
                                              srcX, srcY, tilesetTileWidth, tilesetTileHeight);
        }
    }
    
    std::cout << "\nProcessed " << objectLayerCount << " object layer(s) with " 
              << totalObjects << " tile-based objects" << std::endl;
    
    // Print object tile statistics if any were found
    if (totalObjects > 0) {
        objectCompressor->PrintTileStatistics();
    }
    
    return true;
}

bool MapConverter::ProcessMap() {
    std::cout << "\n=== Processing Map ===" << std::endl;
    std::cout.flush();
    
    if (!LoadTilesets()) {
        return false;
    }
    
    if (!RenderAndTrackTiles()) {
        return false;
    }
    
    if (!ProcessObjectLayers()) {
        return false;
    }
    
    // Print tile statistics
    compressor->PrintTileStatistics();
    
    return true;
}

bool MapConverter::GenerateNewTMX(const std::string& outputPath) {
    std::cout << "Generating new TMX file: " << outputPath << std::endl;
    
    std::ofstream out(outputPath);
    if (!out.is_open()) {
        std::cerr << "Failed to open output file: " << outputPath << std::endl;
        return false;
    }
    
    auto mapSize = map.getTileCount();
    
    // Write TMX header with infinite="1" for chunked format
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<map version=\"1.10\" tiledversion=\"1.10.0\" orientation=\"" 
        << (map.getOrientation() == tmx::Orientation::Orthogonal ? "orthogonal" : "isometric")
        << "\" renderorder=\"right-down\" width=\"" << roundedMapWidth
        << "\" height=\"" << roundedMapHeight
        << "\" tilewidth=\"" << tileWidth
        << "\" tileheight=\"" << tileHeight << "\" infinite=\"1\">\n";
    
    // Write new tileset reference
    std::string tilesetFilename = mapName + "_tileset_compressed.png";
    size_t uniqueTiles = compressor->GetUniqueTileCount();
    int tilesPerRow = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(uniqueTiles))));
    int numRows = static_cast<int>(std::ceil(static_cast<double>(uniqueTiles) / tilesPerRow));
    
    out << " <tileset firstgid=\"1\" name=\"compressed_tileset\" tilewidth=\"" << tileWidth
        << "\" tileheight=\"" << tileHeight << "\" tilecount=\"" << uniqueTiles
        << "\" columns=\"" << tilesPerRow << "\">\n";
    out << "  <image source=\"" << tilesetFilename << "\" width=\""
        << tilesPerRow * tileWidth
        << "\" height=\""
        << numRows * tileHeight
        << "\"/>\n";
    out << " </tileset>\n";
    
    // Write object tileset reference if objects exist
    if (objectCompressor->GetUniqueTileCount() > 0) {
        std::string objectTilesetFilename = mapName + "_objects_compressed.png";
        size_t uniqueObjectTiles = objectCompressor->GetUniqueTileCount();
        int objTilesPerRow = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(uniqueObjectTiles))));
        int objNumRows = static_cast<int>(std::ceil(static_cast<double>(uniqueObjectTiles) / objTilesPerRow));
        
        // Object tileset gets first GID after tile tileset
        uint32_t objectFirstGid = compressor->GetUniqueTileCount() + 1;
        
        // Use actual object tile dimensions
        int objTileWidth = (maxObjectTileWidth > 0) ? maxObjectTileWidth : tileWidth;
        int objTileHeight = (maxObjectTileHeight > 0) ? maxObjectTileHeight : tileHeight;
        
        out << " <tileset firstgid=\"" << objectFirstGid << "\" name=\"objects_compressed\" tilewidth=\"" << objTileWidth
            << "\" tileheight=\"" << objTileHeight << "\" tilecount=\"" << uniqueObjectTiles
            << "\" columns=\"" << objTilesPerRow << "\">\n";
        out << "  <image source=\"" << objectTilesetFilename << "\" width=\""
            << objTilesPerRow * objTileWidth
            << "\" height=\""
            << objNumRows * objTileHeight
            << "\"/>\n";
        out << " </tileset>\n";
    }
    
    // Write layers with remapped GIDs in 16x16 chunks
    const auto& layers = map.getLayers();
    const auto& tilesets = map.getTilesets();
    
    for (const auto& layer : layers) {
        if (layer->getType() != tmx::Layer::Type::Tile) {
            continue;
        }
        
        const auto* tileLayer = dynamic_cast<const tmx::TileLayer*>(layer.get());
        if (!tileLayer) {
            continue;
        }
        
        out << " <layer id=\"" << layer->getName() << "\" name=\"" << layer->getName()
            << "\" width=\"" << roundedMapWidth << "\" height=\"" << roundedMapHeight << "\">\n";
        out << "  <data encoding=\"csv\">\n";
        
        const auto& tiles = tileLayer->getTiles();
        const auto& chunks = tileLayer->getChunks();
        
        // Create a map to store all tile GIDs by position
        std::map<std::pair<int, int>, uint32_t> tileMap;
        
        // Read from chunks if available, otherwise from flat array
        if (!chunks.empty()) {
            for (const auto& chunk : chunks) {
                for (int cy = 0; cy < chunk.size.y; ++cy) {
                    for (int cx = 0; cx < chunk.size.x; ++cx) {
                        size_t index = cy * chunk.size.x + cx;
                        if (index < chunk.tiles.size()) {
                            int worldX = chunk.position.x + cx;
                            int worldY = chunk.position.y + cy;
                            tileMap[{worldX, worldY}] = chunk.tiles[index].ID;
                        }
                    }
                }
            }
        } else if (!tiles.empty()) {
            for (size_t y = 0; y < mapSize.y; ++y) {
                for (size_t x = 0; x < mapSize.x; ++x) {
                    size_t index = y * mapSize.x + x;
                    if (index < tiles.size()) {
                        tileMap[{static_cast<int>(x), static_cast<int>(y)}] = tiles[index].ID;
                    }
                }
            }
        }
        
        // Write output in 16x16 chunks
        for (int chunkY = 0; chunkY < roundedMapHeight; chunkY += 16) {
            for (int chunkX = 0; chunkX < roundedMapWidth; chunkX += 16) {
                out << "   <chunk x=\"" << chunkX << "\" y=\"" << chunkY << "\" width=\"16\" height=\"16\">\n";
                
                // Write 16x16 tile GIDs for this chunk
                for (int y = 0; y < 16; ++y) {
                    for (int x = 0; x < 16; ++x) {
                        int worldX = chunkX + x;
                        int worldY = chunkY + y;
                        
                        uint32_t oldGID = 0;
                        auto it = tileMap.find({worldX, worldY});
                        if (it != tileMap.end()) {
                            oldGID = it->second;
                        }
                        
                        uint32_t newGID = compressor->GetNewIndex(oldGID);
                        out << newGID;
                        
                        if (x < 15 || y < 15) {
                            out << ",";
                        }
                    }
                    if (y < 15) {
                        out << "\n";
                    }
                }
                
                out << "\n   </chunk>\n";
            }
        }
        
        out << "  </data>\n";
        out << " </layer>\n";
    }
    
    // Write object layers
    for (const auto& layer : layers) {
        if (layer->getType() != tmx::Layer::Type::Object) {
            continue;
        }
        
        const auto* objectGroup = dynamic_cast<const tmx::ObjectGroup*>(layer.get());
        if (!objectGroup) {
            continue;
        }
        
        out << " <objectgroup id=\"" << layer->getName() << "\" name=\"" << layer->getName() << "\">\n";
        
        const auto& objects = objectGroup->getObjects();
        uint32_t objectFirstGid = compressor->GetUniqueTileCount() + 1;
        
        for (const auto& obj : objects) {
            uint32_t oldTileID = obj.getTileID();
            const auto& pos = obj.getPosition();
            const auto& aabb = obj.getAABB();
            float rotation = obj.getRotation();
            const std::string& objType = obj.getClass();
            
            // Start object tag
            out << "  <object id=\"" << obj.getUID() << "\"";
            
            // Add name if present
            if (!obj.getName().empty()) {
                out << " name=\"" << obj.getName() << "\"";
            }
            
            // Add type/class if present
            if (!objType.empty()) {
                out << " type=\"" << objType << "\"";
            }
            
            // Add position
            out << " x=\"" << pos.x << "\" y=\"" << pos.y << "\"";
            
            // Add dimensions
            out << " width=\"" << aabb.width << "\" height=\"" << aabb.height << "\"";
            
            // Add rotation if non-zero
            if (rotation != 0.0f) {
                out << " rotation=\"" << rotation << "\"";
            }
            
            // Add GID for tile-based objects
            if (oldTileID > 0) {
                uint32_t newLocalID = objectCompressor->GetNewIndex(oldTileID);
                uint32_t newGID = (newLocalID > 0) ? (objectFirstGid + newLocalID - 1) : 0;
                out << " gid=\"" << newGID << "\"";
            }
            
            // Check if object has properties or shape data
            const auto& properties = obj.getProperties();
            const auto& points = obj.getPoints();
            auto shape = obj.getShape();
            bool hasContent = !properties.empty() || !points.empty() || 
                            shape == tmx::Object::Shape::Ellipse ||
                            shape == tmx::Object::Shape::Text;
            
            if (hasContent) {
                out << ">\n";
                
                // Output shape information for non-rectangle shapes
                if (shape == tmx::Object::Shape::Ellipse) {
                    out << "   <ellipse/>\n";
                }
                else if (shape == tmx::Object::Shape::Point) {
                    out << "   <point/>\n";
                }
                else if (shape == tmx::Object::Shape::Polygon && !points.empty()) {
                    out << "   <polygon points=\"";
                    for (size_t i = 0; i < points.size(); ++i) {
                        if (i > 0) out << " ";
                        out << points[i].x << "," << points[i].y;
                    }
                    out << "\"/>\n";
                }
                else if (shape == tmx::Object::Shape::Polyline && !points.empty()) {
                    out << "   <polyline points=\"";
                    for (size_t i = 0; i < points.size(); ++i) {
                        if (i > 0) out << " ";
                        out << points[i].x << "," << points[i].y;
                    }
                    out << "\"/>\n";
                }
                else if (shape == tmx::Object::Shape::Text) {
                    const auto& text = obj.getText();
                    out << "   <text";
                    if (!text.fontFamily.empty()) {
                        out << " fontfamily=\"" << text.fontFamily << "\"";
                    }
                    if (text.pixelSize > 0) {
                        out << " pixelsize=\"" << text.pixelSize << "\"";
                    }
                    if (text.wrap) {
                        out << " wrap=\"1\"";
                    }
                    if (text.bold) {
                        out << " bold=\"1\"";
                    }
                    if (text.italic) {
                        out << " italic=\"1\"";
                    }
                    if (text.underline) {
                        out << " underline=\"1\"";
                    }
                    if (text.strikethough) {
                        out << " strikeout=\"1\"";
                    }
                    // Output color if not default (black)
                    if (text.colour.r != 0 || text.colour.g != 0 || text.colour.b != 0 || text.colour.a != 255) {
                        out << " color=\"#" << std::hex << std::setfill('0') 
                            << std::setw(2) << (int)text.colour.r
                            << std::setw(2) << (int)text.colour.g
                            << std::setw(2) << (int)text.colour.b
                            << std::setw(2) << (int)text.colour.a
                            << std::dec << "\"";
                    }
                    out << ">" << text.content << "</text>\n";
                }
                
                // Output properties
                if (!properties.empty()) {
                    out << "   <properties>\n";
                    for (const auto& prop : properties) {
                        out << "    <property name=\"" << prop.getName() << "\"";
                        
                        switch (prop.getType()) {
                            case tmx::Property::Type::Boolean:
                                out << " type=\"bool\" value=\"" 
                                    << (prop.getBoolValue() ? "true" : "false") << "\"";
                                break;
                            case tmx::Property::Type::Float:
                                out << " type=\"float\" value=\"" 
                                    << prop.getFloatValue() << "\"";
                                break;
                            case tmx::Property::Type::Int:
                                out << " type=\"int\" value=\"" 
                                    << prop.getIntValue() << "\"";
                                break;
                            case tmx::Property::Type::String:
                                out << " type=\"string\" value=\"" 
                                    << prop.getStringValue() << "\"";
                                break;
                            case tmx::Property::Type::Colour:
                                {
                                    const auto& col = prop.getColourValue();
                                    out << " type=\"color\" value=\"#" << std::hex << std::setfill('0')
                                        << std::setw(2) << (int)col.r
                                        << std::setw(2) << (int)col.g
                                        << std::setw(2) << (int)col.b
                                        << std::setw(2) << (int)col.a
                                        << std::dec << "\"";
                                }
                                break;
                            case tmx::Property::Type::File:
                                out << " type=\"file\" value=\"" 
                                    << prop.getFileValue() << "\"";
                                break;
                            case tmx::Property::Type::Object:
                                out << " type=\"object\" value=\"" 
                                    << prop.getObjectValue() << "\"";
                                break;
                            default:
                                break;
                        }
                        
                        out << "/>\n";
                    }
                    out << "   </properties>\n";
                }
                
                out << "  </object>\n";
            }
            else {
                // Self-closing tag for simple objects
                out << "/>\n";
            }
        }
        
        out << " </objectgroup>\n";
    }
    
    out << "</map>\n";
    out.close();
    
    std::cout << "New TMX file generated successfully!" << std::endl;
    return true;
}

bool MapConverter::SaveConvertedMap(const std::string& outputDir) {
    // Create output directory if it doesn't exist
    try {
        fs::create_directories(outputDir);
        std::cout << "\n=== Saving Output ===" << std::endl;
        std::cout << "Created output directory: " << outputDir << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create output directory: " << e.what() << std::endl;
        return false;
    }
    
    // Generate compressed tileset
    std::string tilesetPath = (fs::path(outputDir) / (mapName + "_tileset_compressed.png")).string();
    if (!compressor->GenerateCompressedTileset(tilesetPath, tileWidth, tileHeight)) {
        return false;
    }
    
    // Generate object tileset if objects exist
    std::string objectTilesetPath;
    if (objectCompressor->GetUniqueTileCount() > 0) {
        objectTilesetPath = (fs::path(outputDir) / (mapName + "_objects_compressed.png")).string();
        std::cout << "\n=== Generating Object Tileset ===" << std::endl;
        
        // Use the actual maximum object tile dimensions
        int objTileWidth = (maxObjectTileWidth > 0) ? maxObjectTileWidth : tileWidth;
        int objTileHeight = (maxObjectTileHeight > 0) ? maxObjectTileHeight : tileHeight;
        
        std::cout << "Using object tile size: " << objTileWidth << "x" << objTileHeight << std::endl;
        
        if (!objectCompressor->GenerateCompressedTileset(objectTilesetPath, objTileWidth, objTileHeight)) {
            std::cout << "Warning: Failed to generate object tileset" << std::endl;
        }
    }
    
    // Generate new TMX file
    std::string newTmxPath = (fs::path(outputDir) / (mapName + "_converted.tmx")).string();
    if (!GenerateNewTMX(newTmxPath)) {
        return false;
    }
    
    // Save render surface as PNG for visualization
    if (renderSurface) {
        std::string renderPath = (fs::path(outputDir) / (mapName + "_render.png")).string();
        std::cout << "\nSaving render preview..." << std::endl;
        
        // Copy surface data to buffer
        std::vector<uint8_t> pixels(renderSurface->w * renderSurface->h * 4);
        
        if (SDL_MUSTLOCK(renderSurface)) {
            SDL_LockSurface(renderSurface);
        }
        
        memcpy(pixels.data(), renderSurface->pixels, pixels.size());
        
        if (SDL_MUSTLOCK(renderSurface)) {
            SDL_UnlockSurface(renderSurface);
        }
        
        // Write PNG
        int result = stbi_write_png(renderPath.c_str(), renderSurface->w, renderSurface->h, 4,
                                   pixels.data(), renderSurface->w * 4);
        
        if (result) {
            std::cout << "Render preview saved to: " << renderPath << std::endl;
        } else {
            std::cout << "Warning: Failed to save render preview" << std::endl;
        }
    }
    
    std::cout << "\n=== Conversion Complete! ===" << std::endl;
    std::cout << "Output directory: " << outputDir << std::endl;
    std::cout << "Tileset: " << tilesetPath << std::endl;
    std::cout << "Map: " << newTmxPath << std::endl;
    
    return true;
}

} // namespace TiledCompressor
