$ErrorActionPreference = 'Stop'

$distro = 'Ubuntu-20.04'
$wslRepo = '/home/xcrane/epass_dev/drm_app_neo'
$repo = Split-Path -Parent $PSScriptRoot
$wslShare = "\\wsl.localhost\$distro\home\xcrane\epass_dev\drm_app_neo"

if (-not (Test-Path -LiteralPath $wslShare)) {
    throw "WSL build mirror not found: $wslShare"
}

Write-Host 'Syncing simulator and EEZ-generated UI...'
Copy-Item -Path (Join-Path $PSScriptRoot '*') `
    -Destination (Join-Path $wslShare 'simulator') -Recurse -Force
Copy-Item -Path (Join-Path $repo 'eez_design\src\ui\*') `
    -Destination (Join-Path $wslShare 'eez_design\src\ui') -Recurse -Force
Copy-Item -LiteralPath (Join-Path $repo 'src\lv_conf.h') `
    -Destination (Join-Path $wslShare 'src\lv_conf.h') -Force
Copy-Item -Path (Join-Path $repo 'src\ui\cat_pet.*') `
    -Destination (Join-Path $wslShare 'src\ui') -Force
Copy-Item -Path (Join-Path $repo 'src\ui\theme.*') `
    -Destination (Join-Path $wslShare 'src\ui') -Force

Write-Host 'Configuring and building...'
& wsl.exe -d $distro --cd $wslRepo -- `
    cmake -S simulator -B build/simulator -DCMAKE_BUILD_TYPE=Debug
if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed with exit code $LASTEXITCODE"
}

& wsl.exe -d $distro --cd $wslRepo -- `
    cmake --build build/simulator --parallel 4
if ($LASTEXITCODE -ne 0) {
    throw "Simulator build failed with exit code $LASTEXITCODE"
}

Write-Host 'Starting the 360 x 640 desktop window...'
& wsl.exe -d $distro -- pkill -f 'build/simulator/epass_ui_simulator'

$arguments = @(
    '-d', $distro,
    '--cd', $wslRepo,
    '--', './build/simulator/epass_ui_simulator'
)
Start-Process -FilePath 'wsl.exe' -ArgumentList $arguments -WindowStyle Hidden
