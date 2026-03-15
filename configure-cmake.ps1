# Configure CMake with vcpkg

Write-Host "=====================================" -ForegroundColor Cyan
Write-Host "TiledCompressor - CMake Configuration" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""

$vcpkgPath = "C:\vcpkg"

# Find vcpkg
if (-not (Test-Path $vcpkgPath)) {
    Write-Host "vcpkg not found at $vcpkgPath" -ForegroundColor Yellow
    $vcpkgPath = Read-Host "Enter vcpkg path (or press Enter to skip)"
    
    if ([string]::IsNullOrWhiteSpace($vcpkgPath) -or -not (Test-Path $vcpkgPath)) {
        Write-Host "Configuring without vcpkg..." -ForegroundColor Yellow
        $vcpkgPath = $null
    }
}

# Create build directory
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

Set-Location build

# Configure
Write-Host "Running CMake configuration..." -ForegroundColor Cyan

if ($vcpkgPath) {
    $toolchainFile = Join-Path $vcpkgPath "scripts\buildsystems\vcpkg.cmake"
    Write-Host "Using vcpkg toolchain: $toolchainFile" -ForegroundColor Green
    cmake .. -DCMAKE_TOOLCHAIN_FILE="$toolchainFile" -DCMAKE_BUILD_TYPE=Release
} else {
    cmake .. -DCMAKE_BUILD_TYPE=Release
}

Set-Location ..

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "=====================================" -ForegroundColor Green
    Write-Host "Configuration successful!" -ForegroundColor Green
    Write-Host "=====================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Next step: Run .\build.bat to compile" -ForegroundColor Cyan
} else {
    Write-Host ""
    Write-Host "Configuration failed!" -ForegroundColor Red
    Write-Host "Please check the error messages above." -ForegroundColor Red
}

Write-Host ""
pause
