# Quick Start Guide - TiledCompressor

## Easy Installation (Recommended)

### Step 1: Install vcpkg and dependencies

**Option A: Using PowerShell Script**

If PowerShell scripts are enabled on your system:

```powershell
.\setup-vcpkg.ps1
```

**Option B: Manual Installation (Recommended for first-time users)**

```powershell
# Install vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install

# Install dependencies (SDL3 is preferred, but SDL2 works too)
.\vcpkg install sdl3:x64-windows sdl3-image[png]:x64-windows tmxlite:x64-windows
```

**Note:** This may take 15-30 minutes depending on your system.

### Step 2: Configure CMake

**For VS Code Users (Recommended):**

The project includes `.vscode` settings that automatically configure CMake with vcpkg. Just:
1. Open the project in VS Code
2. When prompted, select the **"Visual Studio - x64 (vcpkg)"** kit
3. CMake will configure automatically

**For Command Line:**

```powershell
# Clean build directory if it exists
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cd ..
```

### Step 3: Build

**In VS Code:** Click the "Build" button in the status bar, or press F7

**Command Line:**

```powershell
cmake --build build --config Release
```

### Step 4: Run

```powershell
.\build\Release\TiledCompressor.exe
```

Or run without arguments to open a file dialog and select a TMX file.

---

## Important Notes

### SDL3 vs SDL2

The project automatically detects which SDL version you have installed:
- **SDL3** is preferred and will be used if available
- **SDL2** works as a fallback if SDL3 is not found
- Both versions are fully supported

If you installed via vcpkg as shown above, you'll have **SDL3** by default.

### VS Code Configuration

The project includes `.vscode/settings.json` that automatically configures:
- CMake toolchain file for vcpkg
- Build configuration
- Debug launch settings

**First-time VS Code setup:**
1. Install the "CMake Tools" extension
2. Open the project folder
3. Select the "Visual Studio - x64 (vcpkg)" kit when prompted
4. Build using F7 or the Build button

---

## Alternative: Manual SDL Installation (Not Recommended)

If you cannot use vcpkg, you can manually install SDL libraries:

1. **SDL3**: Download from https://github.com/libsdl-org/SDL/releases
   - Extract to `C:\SDL3`
   - Add to PATH and set `SDL3_DIR` environment variable

2. **tmxlite**: 
   ```powershell
   git clone https://github.com/fallahn/tmxlite.git
   cd tmxlite/tmxlite
   mkdir build; cd build
   cmake ..
   cmake --build . --config Release
   cmake --install .
   ```

3. **Build TiledCompressor**:
   ```powershell
   mkdir build; cd build
   cmake ..
   cmake --build . --config Release
   ```

**Note:** Manual installation is more complex and error-prone. vcpkg is strongly recommended.

---

## Troubleshooting

### "Cannot load PowerShell script"
PowerShell execution policy is restricted. Either:
- Use the manual vcpkg installation steps above (recommended)
- Or run: `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned`

### "Could not find SDL2/SDL3"
The CMake toolchain file is missing. Solutions:
1. **VS Code**: Reload the window and select "Visual Studio - x64 (vcpkg)" kit
2. **Command line**: Always use `-DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake`
3. **Clean rebuild**: Delete the `build` folder and reconfigure from scratch

### "CMake configuration failed"
- Delete the `build/CMakeCache.txt` file or entire `build` folder
- Ensure vcpkg is at `C:\vcpkg` (or update the toolchain path)
- Check that vcpkg packages are installed: `C:\vcpkg\vcpkg list | Select-String "sdl"`

### Wrong SDL version being used
- CMake prefers SDL3 if both are installed
- To force SDL2: uninstall SDL3 from vcpkg or manually set `SDL3_FOUND=FALSE`
- Check build output: "Found SDL3" vs "Using SDL2..."

### DLL not found when running
All required DLLs should be copied automatically. If not:
- Rebuild the project (CMake copies DLLs during build)
- Check `build/Release/` contains: SDL3.dll (or SDL2.dll), SDL3_image.dll, tmxlite.dll, libpng16.dll

### "vcpkg not found"
Install vcpkg manually following Step 1, Option B above.

### Build errors with Visual Studio
- Ensure Visual Studio 2019 or newer is installed with C++ desktop development
- CMake requires version 3.20 or higher
- Use x64 architecture (not x86 or ARM)

---

## Quick Test

After building, test with a sample TMX file:

```powershell
# Run with file dialog (recommended for first test)
.\build\Release\TiledCompressor.exe

# Or specify a TMX file directly
.\build\Release\TiledCompressor.exe path\to\your\map.tmx
```

The tool will create a new folder next to your TMX file named `<mapname>_compressed/` containing:
- Compressed tileset PNG (only tiles actually used in the map)
- New TMX file with remapped tile indices
- Tileset TSX file

---

## Development with VS Code

Once configured, your workflow is:
1. **Edit code** in VS Code
2. **Build**: Press `F7` or click "Build" in status bar
3. **Debug**: Press `F5` to launch with debugger
4. **Clean**: Delete `build` folder and reconfigure if needed

The CMake Tools extension will show configuration status in the status bar.
