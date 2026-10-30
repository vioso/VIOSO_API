# tools/make-release-package.ps1
# Builds the Windows x64 release archive from a Release build tree.
#
#   powershell -File tools/make-release-package.ps1 [-BuildDir <dir>] [-OutDir <dir>] [-Label <text>] [-List]
#
# -List only prints what would go into the archives and writes nothing.
# Besides the archive a second one, <name>-symbols.zip, holds the .pdb files of the library, the examples and the tools (bin/<name>.pdb);
#   the main archive has no .pdb.
# -Label makes a test package for the coming release without touching resource.h: the version in the name is then fixed to the
#   coming release (1.7.0) and the label is appended, e.g. -Label rc1 gives VIOSO_API-1.7.0-rc1-win-x64.zip.
# Prerequisite: a Release build of the repository (cmake --preset windows-x64-release, cmake --build ...),
# without -DVIOSO_BUILD_TOOLS (MakeTestVwf is always built and goes into the package, WarpPrecision does not).
param(
    [string]$BuildDir = "",
    [string]$OutDir = "",
    [string]$Label = "",
    [switch]$List
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot

# the version lives in resource.h only
$ver = @{}
foreach ($l in Get-Content "$repo/VIOSOWarpBlend/resource.h") {
    if ($l -match '^#define VWB_Version_(MAJ|MIN|MAI)\s+(\d+)') { $ver[$Matches[1]] = $Matches[2] }
}
$version = "$($ver.MAJ).$($ver.MIN).$($ver.MAI)"
if ($Label) { $version = "1.7.0-$Label" } # the coming release, until resource.h says so

if (-not $BuildDir) { $BuildDir = "$repo/out/build/windows-x64-release" }
if (-not $OutDir) { $OutDir = $repo }
$bin = "$BuildDir/bin"
$lib = "$BuildDir/lib"
$name = "VIOSO_API-$version-win-x64"

# source -> path inside the archive
$files = [ordered]@{
    "$bin/VIOSOWarpBlend64.dll"          = "bin/VIOSOWarpBlend64.dll"
    "$bin/EyePointProvider64.dll"        = "bin/EyePointProvider64.dll"
    "$bin/VWFTesterDX11.exe"             = "bin/VWFTesterDX11.exe"
    "$bin/assimp-vc142-mt.dll"           = "bin/assimp-vc142-mt.dll"
    "$bin/MediaServerExampleDX11.exe"    = "bin/MediaServerExampleDX11.exe"
    "$bin/D3D12Example.exe"              = "bin/D3D12Example.exe"
    "$bin/shaders.hlsl"                  = "bin/shaders.hlsl"
    "$bin/OpenGLExample.exe"             = "bin/OpenGLExample.exe"
    "$bin/MakeTestVwf.exe"               = "bin/MakeTestVwf.exe"
    "$repo/data/VIOSOWarpBlend.ini"      = "bin/VIOSOWarpBlend.ini"
    "$repo/data/VIOSOWarpBlendGL.ini"    = "bin/VIOSOWarpBlendGL.ini"
    "$repo/data/2D_plane_FHD.vwf"        = "bin/2D_plane_FHD.vwf"
    "$repo/tools/package/bin-README.txt" = "bin/README.txt"
    "$lib/VIOSOWarpBlend64.lib"          = "lib/VIOSOWarpBlend64.lib"
    "$lib/EyePointProvider64.lib"        = "lib/EyePointProvider64.lib"
    "$repo/README.md"                    = "README.md"
    "$repo/CHANGELOG.md"                 = "CHANGELOG.md"
    "$repo/LICENSE.md"                   = "LICENSE.md"
}
foreach ($f in 'VIOSOWarpBlend.h','VIOSOWarpBlend.hpp','VIOSOWarpBlend.cs','VWBTypes.h','VWBDef.h','EyePointProvider.h') {
    $files["$repo/Include/$f"] = "Include/$f"
}
foreach ($f in Get-ChildItem "$repo/docs" -File) { $files[$f.FullName] = "docs/$($f.Name)" }
# reference source of the examples: the program sources of each example folder, no build files, no third-party libraries (subfolders)
foreach ($d in 'DirectX11','DirectX12','OpenGL_4') {
    foreach ($f in Get-ChildItem "$repo/Examples/$d" -File | Where-Object { $_.Extension -in '.cpp','.h','.hlsl' }) { $files[$f.FullName] = "Examples/$d/$($f.Name)" }
}

# symbols: the .pdb of every program and library of the archive that has one next to it in the build (not the third-party assimp)
$symbols = [ordered]@{}
foreach ($f in $files.Keys | Where-Object { $_ -like "$bin/*" -and $_ -match '\.(dll|exe)$' -and $_ -notlike '*/assimp-*' }) {
    $symbols[[IO.Path]::ChangeExtension($f, '.pdb')] = "bin/" + [IO.Path]::GetFileNameWithoutExtension($f) + ".pdb"
}

$missing = $files.Keys + $symbols.Keys | Where-Object { -not (Test-Path $_) }
if ($missing) { throw "missing: `n  " + ($missing -join "`n  ") }

if ($List) {
    $files.GetEnumerator() | ForEach-Object { "{0,-34} <- {1}" -f $_.Value, $_.Key }
    "-> $OutDir/$name.zip"
    "symbols:"
    $symbols.GetEnumerator() | ForEach-Object { "{0,-34} <- {1}" -f $_.Value, $_.Key }
    "-> $OutDir/$name-symbols.zip"
    return
}

# Written entry by entry with "/" as the separator: Compress-Archive of Windows PowerShell 5.1 writes "\", which other tools do not read
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
function Write-Zip($zip, $entries) {
    if (Test-Path $zip) { Remove-Item $zip }
    $archive = [IO.Compression.ZipFile]::Open($zip, [IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($e in $entries.GetEnumerator()) {
            $null = [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $e.Key, $e.Value, [IO.Compression.CompressionLevel]::Optimal)
        }
    } finally {
        $archive.Dispose()
    }
    "wrote $zip"
}
Write-Zip (Join-Path $OutDir "$name.zip") $files
Write-Zip (Join-Path $OutDir "$name-symbols.zip") $symbols
