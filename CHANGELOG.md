# Changelog

## 1.7.2

Drop-in replacement for 1.7.1: the interface, the ABI and the .vwf format are unchanged. Known limits: docs/integration-guide.md. The entries marked **Behaviour change** show in the output or in a return value: read them before you update a host.

### Fixed

- Direct3D 10, 11 and 12: `bFixWraparound=1` works. `FixWraparound` moves the coordinates of one region beyond 1, so that the content space of a 360 degree panorama is coherent; the warp texture was clamped to 0 to 1 on upload, so the wrapped region showed the smeared edge pixel of the content instead of the panorama. Hosts that switched `bFixWraparound` off as a workaround can switch it on again with 1.7.2. OpenGL was not affected.
- OpenGL: a Core profile context works (`bUseGL110=0`, the default). The wrap mode of the warp, blend and black level textures was set to `GL_CLAMP`, which is not valid in a Core profile: every frame raised `GL_INVALID_ENUM` in `glTexParameteri` (18 debug messages in 3 frames). It is `GL_CLAMP_TO_BORDER` with a black border now; the image in a Compatibility context does not change. `bUseGL110=1` (fixed pipeline) stays Compatibility only.
- OpenGL: a GL error that the host left behind no longer makes `VWB_render` skip the frame. `Render()` read the error state of the whole context after `glUseProgram` and drew nothing on any error, so a host that leaves one error per frame got an empty output and `VWB_ERROR_NONE`. It now checks that its own program is the current one (`GL_CURRENT_PROGRAM`). Errors that are pending at the first calls of `VWB_render` (until a frame was drawn) are read away, up to 16 per call, and logged once as a warning (log level 1: "GL error 0x... pending at VWB_render entry (left by the host or a previous call)").

### Changed

- **Behaviour change.** Direct3D 10, 11 and 12 (2D calibrations): an area whose warp coordinate lies outside the content (below 0 or above 1) is black, as in OpenGL. Up to 1.7.1 the warp texture was clamped to 0 to 1 on upload (a leftover from the 16 bit format of 1.5), so such an area repeated the edge pixel of the content and showed grey or coloured areas, for example outside the content in a test file whose blend is 1.0 everywhere. Where the warp lies just outside 0 to 1 at the edge of the content, a thin dark rim can appear instead of a stretched edge pixel, as in OpenGL. NaN and Inf in a warp map are black (they gave undefined pixels). The black level correction is not part of this: with a black level map it raises the output everywhere, outside the mesh too, in all backends, as before.
- **Behaviour change.** OpenGL, return value of `VWB_render`: `VWB_ERROR_PARAMETER` if the source is not a texture of the current context; the frame is not drawn (before, the warper drew with it, which gave a black image, and returned `VWB_ERROR_NONE`). Texture name 0 is not a texture. To read the back buffer pass `VWB_UNDEFINED_GL_TEXTURE` (-1), not NULL and not 0 (`VIOSOWarpBlend.h` says NULL for DirectX and -1 for OpenGL); a host that passes NULL in OpenGL mode gets the error and a log line now. `VWB_ERROR_SHADER` if the shader program cannot be made current (before, the frame was skipped and `VWB_ERROR_NONE` returned). Each of the two is logged once per process.
- **Behaviour change (tool, not the library).** `MakeTestVwf`: a relative path of `make` and `info` is relative to the working directory. Up to 1.7.1 it was relative to the folder of the program on Windows, so `make out.vwf` wrote next to `MakeTestVwf.exe`. A missing folder of the output file is created, `.vwf` is added if the name has no extension, and the messages show the full path.

### Added

- `MakeTestVwf`: `make --edge N` and `--edge-left`, `--edge-right`, `--edge-top`, `--edge-bottom N` write a soft blend edge on the outer sides of the arrangement; a single channel has a blend map of 1 without them. The ramp is the one of the overlaps. Without the options the files are byte-identical to those of 1.7.1.
- Documentation: black outside the picture and the black level correction are two different things (integration guide, "Where things go wrong"; `docs/vwf-format.md`); the OpenGL Core profile, the source texture -1 and the return codes of `VWB_render` in the integration guide, the README, `docs/agent-integration.md` and `VIOSOWarpBlend.h` (comments only); relative paths on Linux are resolved against the working directory (`docs/eyepoint-provider.md`, `PathHelper.h`, as `docs/ini-reference.md` says).

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
