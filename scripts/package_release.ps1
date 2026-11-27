# Builds the Windows release: x64, with the Visual C++ runtime linked in so
# nothing else needs installing. Leaves the renamed plugins in
# build-release\stage\.
#
#   powershell -ExecutionPolicy Bypass -File scripts\package_release.ps1
#
# Building with PHYLLOTAXIS_INSTALL_PLUGINS off leaves the installed plugins alone.

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build-release'
$stage = Join-Path $build 'stage'

function Invoke-Checked {
    & $args[0] $args[1..($args.Count - 1)]
    if ($LASTEXITCODE -ne 0) { throw "$($args[0]) failed with exit code $LASTEXITCODE" }
}

Invoke-Checked cmake -S $root -B $build -G 'Visual Studio 17 2022' -A x64 `
    -DYUP_DIR="$root/yup" `
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -DSMTG_USE_STATIC_CRT=ON `
    -DPHYLLOTAXIS_INSTALL_PLUGINS=OFF
Invoke-Checked cmake --build $build --config Release --parallel

if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory $stage | Out-Null

# A VST3 bundle's inner binary must carry the bundle's name
$vst3 = Join-Path $stage 'Phyllotaxis.vst3'
Copy-Item -Recurse (Join-Path $build 'VST3\Release\phyllotaxis_vst3_plugin.vst3') $vst3
Rename-Item (Join-Path $vst3 'Contents\x86_64-win\phyllotaxis_vst3_plugin.vst3') 'Phyllotaxis.vst3'

Copy-Item (Join-Path $build 'Release\phyllotaxis_clap_plugin.clap') (Join-Path $stage 'Phyllotaxis.clap')
Copy-Item (Join-Path $build 'Release\Phyllotaxis.exe') (Join-Path $stage 'Phyllotaxis.exe')

Write-Host "Staged in $stage"
