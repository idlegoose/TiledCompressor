// TiledCompressor - Tiled Map Tileset Optimizer
// Copyright (c) 2026 TiledCompressor Contributors
// Licensed under the MIT License - see LICENSE file for details

#pragma once

#include <string>

namespace TiledCompressor {

/**
 * Opens a native file dialog for selecting a TMX file
 * @return Path to selected file, or empty string if canceled
 */
std::string OpenFileDialog();

} // namespace TiledCompressor
