# Changelog

## 1.7.1

Drop-in replacement for 1.7.0: the interface, the ABI and the .vwf format are unchanged. Known limits: docs/integration-guide.md.

### Fixed

- Direct3D 12: several frames in flight no longer corrupt the output. `Render()` used one descriptor for the content texture, one constant buffer and, without a command list from the caller, one command allocator for all calls; a second call before the GPU had finished the first one showed the wrong texture or lost the draw commands (debug layer: `CreateShaderResourceView ... bound as STATIC`, `CommandAllocator::Reset ... being reset before previous executions ... have completed`). The warper now uses a ring of 3 slots (own descriptor for the content texture, own constant buffer range, own allocator and list), tracked by a fence on the command queue, and waits on that fence if a slot is still in use. Assumption: the caller submits the command list it passes in `VWB_D3D12_RENDERINPUT::commandList` to the queue before it calls `VWB_render` again for the same warper. Not supported: `VWB_render` of one warper from several threads at the same time.
- Direct3D 12: the destructor waits for the GPU and releases the command allocator, the command queue reference and the synchronisation objects (they were leaked); the event of the upload fence is closed.

### Changed

- Direct3D 12: `Render()` holds up to three references to content textures (one per slot) until a slot gets another texture or the warper is destroyed. It can wait up to 10 seconds for the GPU; after that it logs an error and returns `VWB_ERROR_GENERIC`. A failing reset of the command list is logged and returned as `VWB_ERROR_GENERIC`, and `Render()` before `Init` returns `VWB_ERROR_GENERIC`.
- Line endings: our own text files use CRLF; `.gitattributes` switches the conversion off (`* -text`), third-party code keeps its line endings.
- Release package: `bin/` has the test picture `earth.png` and `view.export.template.viosoAPI.txt` (export template of the VIOSO Calibrator) as well; the build copies `earth.png` next to the executables like the other sample files.

### Added

- Documentation: integration guide pitfall 13 (Direct3D 12, frames in flight) and the section "A host without access to the device: texture interop" (tested with UNIGINE 2.20); the check with moving content and a frame counter in `docs/agent-integration.md`; the Calibrator export template in `docs/ini-reference.md`; `earth.png` as the picture to compare the tester and a host.

### Removed

- `data/aa.html` (an unused test form), `data/earth.jpg` (replaced by `data/earth.png`: the tester reads PNG only).

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
