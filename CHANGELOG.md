# Changelog

## 1.7.0

Drop-in replacement for 1.6.x: the interface and the .vwf format are unchanged. Known limits: docs/integration-guide.md.

### Changed

- 2D warp texture is `R32G32_FLOAT` in Direct3D 10, 11 and 12 (was 16 bit normalised): no steps with wide content spaces, twice the GPU memory (about 66 instead of 33 MB per 4K channel).
- `base` / `trans` missing in the .ini: the left handed default is used and a warning is logged (was an all-zero matrix); it is written to `VWB_Warper::trans`.
- Error messages show the `calibFile` and .ini entries as read.
- CMake is the only build (presets for Windows and Linux); ATL is no longer needed.
- Tester is `VWFTesterDX11.exe`: file dialog without argument, `-F` needs no .ini, windows outside the desktop are shown as a scaled preview, unhandled exceptions are logged.
- Release package: `bin/`, `lib/`, `Include/`, `docs/`, `Examples/` sources; debug symbols in a separate `-symbols.zip`.

### Fixed

- `calibAdapterOrdinal` >= 2 did not find the right display; it finds `D2`, `D3` ... now. A missing display fails `VWB_Init` with the names found, without a fallback.
- `optimalRes` was 1 pixel too small when float noise left it below a whole number.
- `optimalRect` (`bPartialInput`) was up to 1 pixel too small right and bottom; rounded outwards.
- `FixWraparound` used `vCntDispPx[4]` twice instead of `[4]` and `[5]`.
- DLL version strings (FileVersion, ProductVersion, ProductName ...) were empty in the file properties.
- Log: "Mapping Info" printed the split values wrong (0 on Windows, shifted values on Linux).
- Log: wrong formats in `VWB_getPosDirClip`, "Blend map size mismatch", "Unknown file type", AutoView coverage, wrap-around regions, GL "No blend texture".
- Linux: missing includes, library names in the loader, log path overflow, only the API is exported.
- Direct3D 11 tester crashed with an access violation (white window left behind) when `VWB_Create` or `VWB_Init` failed, for example with a missing .ini; it shows the error now.
- Examples show the error of `VWB_Create` / `VWB_Init` instead of ending silently; the OpenGL example checks the view and projection result.
- Direct3D 12 example reports an unreadable `-image` file instead of crashing.
- `MakeTestVwf`: `--overlap` is only limited where there are neighbours; a rejected argument is explained.
- Comments in `VWBTypes.h`, `VIOSOWarpBlend.h`, `VWF.h` follow the code.

### Added

- `MediaServerExampleDX11` (several outputs on one Direct3D 11 device, `--partial`, `--list`) and `D3D12Example` in the CMake build.
- `MakeTestVwf` (synthetic 2D .vwf files, `info`) and `WarpPrecision` (`-DVIOSO_BUILD_TOOLS=ON`).
- `data/`: sample `.ini` files and the sample calibration `2D_plane_FHD.vwf`.
- Log format check (`printf` attribute) on GCC and Clang.
- Documentation in `docs/`, `AGENTS.md`.

### Removed

- Visual Studio projects and solution; use the CMake targets, or the DLL and the headers.
- Binaries in the repository and the Vulkan SDK extract.
- Examples: DirectX 9, 9Ex, 9_render, Mesh, OpenGL 1, Vulkan, OpenGL_4_vwfInfo, MapingLoader, TrackerSim (datagram: docs/eyepoint-provider.md), module test. The Direct3D 9 render paths of the library stay.
