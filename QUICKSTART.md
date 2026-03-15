# Quick Start Guide - TiledCompressor

## Easy Installation (Recommended)

### Step 1: Install vcpkg and dependencies

Run this PowerShell script (as Administrator recommended):

```powershell
.\setup-vcpkg.ps1
```

This will:
- Install vcpkg to `C:\vcpkg`
- Install SDL2/SDL3, SDL2_image/SDL3_image, and tmxlite
- Configure vcpkg integration

**Note:** This may take 15-30 minutes depending on your system.

### Step 2: Configure CMake

```powershell
.\configure-cmake.ps1
```

### Step 3: Build

```powershell
.\build.bat
```

### Step 4: Run

```powershell
.\build\Release\TiledCompressor.exe
```

---

## Manual Installation (Alternative)

If you prefer manual installation or the scripts don't work:

### Option A: Download Pre-built Libraries

1. **SDL2**: Download development libraries from https://www.libsdl.org/download-2.0.php
   - Extract to `C:\SDL2`
   - Add `C:\SDL2\lib\x64` to PATH

2. **SDL2_image**: Download from https://www.libsdl.org/projects/SDL_image/
   - Extract to `C:\SDL2_image`
   - Add to PATH

3. **tmxlite**: 
   ```powershell
   git clone https://github.com/fallahn/tmxlite.git
   cd tmxlite/tmxlite
   mkdir build; cd build
   cmake ..
   cmake --build . --config Release
   cmake --install .
   ```

4. **Build TiledCompressor**:
   ```powershell
   mkdir build; cd build
   cmake ..
   cmake --build . --config Release
   ```

### Option B: Install vcpkg manually

1. Install vcpkg:
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```

2. Install dependencies:
   ```powershell
   .\vcpkg install sdl2:x64-windows sdl2-image:x64-windows tmxlite:x64-windows
   ```

3. Build with vcpkg toolchain:
   ```powershell
   cd D:\Repos\TiledCompressor
   mkdir build; cd build
   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
   cmake --build . --config Release
   ```

---

## Troubleshooting

### "vcpkg not found"
- Run `.\setup-vcpkg.ps1` to install it
- Or install manually and update PATH

### "SDL3 not found"
- The project automatically falls back to SDL2
- Both are supported

### "tmxlite not found"
- CMake will automatically download it via FetchContent
- Or install manually via vcpkg

### Build errors
- Make sure Visual Studio 2019 or newer is installed
- Ensure CMake 3.20+ is available
- Check that all DLLs are copied to build directory

---

## Quick Test

After building, test with a sample TMX file:

```powershell
.\build\Release\TiledCompressor.exe path\to\your\map.tmx
```

Or run without arguments to open a file dialog.
