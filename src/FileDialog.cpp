// TiledCompressor - Tiled Map Tileset Optimizer
// Copyright (c) 2026 TiledCompressor Contributors
// Licensed under the MIT License - see LICENSE file for details

#include "FileDialog.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <vector>

namespace TiledCompressor {

std::string OpenFileDialog() {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "TMX Files (*.tmx)\0*.tmx\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = "Select TMX Map File";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }

    return "";
}

} // namespace TiledCompressor

#else
// Linux/Mac implementation using zenity or kdialog
#include <cstdlib>
#include <array>
#include <memory>

namespace TiledCompressor {

std::string OpenFileDialog() {
    // Try using zenity first
    std::array<char, 512> buffer;
    std::string result;
    
    std::unique_ptr<FILE, decltype(&pclose)> pipe(
        popen("zenity --file-selection --file-filter='TMX files (tmx) | *.tmx' --title='Select TMX Map File' 2>/dev/null", "r"),
        pclose
    );
    
    if (pipe) {
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
    }
    
    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    
    return result;
}

} // namespace TiledCompressor

#endif
