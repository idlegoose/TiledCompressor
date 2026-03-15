# Setup script for vcpkg and dependencies

Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "TiledCompressor - Dependency Setup" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

$vcpkgPath = "C:\vcpkg"

# Check if vcpkg already exists
if (Test-Path $vcpkgPath) {
    Write-Host "vcpkg found at $vcpkgPath" -ForegroundColor Green
    $response = Read-Host "Use existing vcpkg installation? (Y/n)"
    if ($response -eq 'n' -or $response -eq 'N') {
        $vcpkgPath = Read-Host "Enter vcpkg path"
    }
} else {
    Write-Host "vcpkg not found. Installing to $vcpkgPath..." -ForegroundColor Yellow
    Write-Host ""
    
    # Clone vcpkg
    Write-Host "Cloning vcpkg..." -ForegroundColor Cyan
    git clone https://github.com/Microsoft/vcpkg.git $vcpkgPath
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to clone vcpkg. Please install git or clone manually." -ForegroundColor Red
        exit 1
    }
    
    # Bootstrap vcpkg
    Write-Host "Bootstrapping vcpkg..." -ForegroundColor Cyan
    Push-Location $vcpkgPath
    .\bootstrap-vcpkg.bat
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to bootstrap vcpkg." -ForegroundColor Red
        Pop-Location
        exit 1
    }
    
    # Integrate with system
    Write-Host "Integrating vcpkg..." -ForegroundColor Cyan
    .\vcpkg integrate install
    Pop-Location
}

# Install dependencies
Write-Host ""
Write-Host "Installing dependencies..." -ForegroundColor Cyan
Write-Host "This may take 15-30 minutes depending on your system..." -ForegroundColor Yellow
Write-Host ""

Push-Location $vcpkgPath

# SDL3 might not be available in vcpkg yet (as of early 2026)
# We'll try SDL2 as fallback or use direct SDL3 download
Write-Host "Checking for SDL3..." -ForegroundColor Cyan
.\vcpkg search sdl3

Write-Host ""
$useSDL2 = Read-Host "If SDL3 is not available, use SDL2 instead? (Y/n)"

if ($useSDL2 -ne 'n' -and $useSDL2 -ne 'N') {
    Write-Host "Installing SDL2 (fallback)..." -ForegroundColor Yellow
    .\vcpkg install sdl2:x64-windows sdl2-image:x64-windows
} else {
    Write-Host "Installing SDL3..." -ForegroundColor Cyan
    .\vcpkg install sdl3:x64-windows sdl3-image:x64-windows
}

Write-Host "Installing tmxlite..." -ForegroundColor Cyan
.\vcpkg install tmxlite:x64-windows

Pop-Location

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "=====================================" -ForegroundColor Green
    Write-Host "Dependencies installed successfully!" -ForegroundColor Green
    Write-Host "=====================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "vcpkg toolchain file location:" -ForegroundColor Cyan
    Write-Host "$vcpkgPath\scripts\buildsystems\vcpkg.cmake" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Cyan
    Write-Host "1. Run: .\configure-cmake.ps1" -ForegroundColor White
    Write-Host "2. Run: .\build.bat" -ForegroundColor White
} else {
    Write-Host ""
    Write-Host "Failed to install some dependencies." -ForegroundColor Red
    Write-Host "Please check the output above for errors." -ForegroundColor Red
}

Write-Host ""
pause
