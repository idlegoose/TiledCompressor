@echo off
echo ======================================
echo TiledCompressor - Quick Build
echo ======================================
echo.

REM Check if vcpkg is available
where vcpkg >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: vcpkg not found in PATH
    echo Please ensure dependencies are installed manually or configure vcpkg
    echo.
)

REM Download stb_image_write.h if not present
if not exist "third_party\stb_image_write.h" (
    echo Downloading stb_image_write.h...
    powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h' -OutFile 'third_party\stb_image_write.h'"
    if %ERRORLEVEL% NEQ 0 (
        echo Failed to download stb_image_write.h
        echo Please download manually from: https://github.com/nothings/stb
        pause
        exit /b 1
    )
    echo Downloaded successfully!
    echo.
)

REM Create build directory
if not exist "build" mkdir build
cd build

REM Configure with CMake
echo Configuring project...
cmake .. -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    echo Make sure CMake and all dependencies are installed.
    cd ..
    pause
    exit /b 1
)

REM Build
echo.
echo Building project...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

echo.
echo ======================================
echo Build complete!
echo Executable: build\Release\TiledCompressor.exe
echo ======================================
echo.

cd ..
pause
