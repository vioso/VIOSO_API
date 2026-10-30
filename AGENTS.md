# AGENTS.md

For agents that work **in this repository** (build, fix, extend the VIOSO WarpBlend API). If you are asked to build VIOSO into someone else's project, read
[docs/agent-integration.md](docs/agent-integration.md) instead.

## What this is

A library that warps and blends content for projectors and displays. It takes a calibration (`.vwf`) and a texture and draws the warped and blended image into the
render target of the application (Direct3D 9 to 12, OpenGL). The public C interface is `Include/VIOSOWarpBlend.h`; types are in `Include/VWBTypes.h`.

## Layout

| Folder | Content |
|---|---|
| `Include/` | the public headers (`VIOSOWarpBlend.h`, `.hpp` wrapper, `.cs` wrapper, `VWBTypes.h`, `VWBDef.h`, `EyePointProvider.h`) |
| `VIOSOWarpBlend/` | the library: `VIOSOWarpBlend.cpp` (settings, .ini parser calls, init), `VWF.cpp` (.vwf reader and writer), `DX/`, `GL/` (render paths) |
| `EyePointProvider/` | sample plugin that receives eye points as UDP |
| `Examples/` | `DirectX11` (tester `VWFTesterDX11`, `MediaServerExampleDX11`), `DirectX12`, `OpenGL_4` |
| `tools/` | development tools: `MakeTestVwf` (always built, in the release package), `WarpPrecision` (`-DVIOSO_BUILD_TOOLS=ON`), `make-release-package.ps1` |
| `data/` | sample `.ini` files and the sample calibration `2D_plane_FHD.vwf` |
| `docs/` | documentation, see below |

## Build and run

CMake only; presets are in `CMakePresets.json`: `windows-x64-debug`, `windows-x64-release`, `linux-debug`, `linux-release`.

```
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release
```
(Windows: from a Developer Command Prompt.) The binaries end up in `out/build/<preset>/bin`, the sample `.ini` files and `2D_plane_FHD.vwf` are copied next to them.

- See a picture: start `VWFTesterDX11.exe` (a file dialog opens) or `VWFTesterDX11.exe 2D_plane_FHD.vwf`. `MediaServerExampleDX11.exe file.vwf --list` lists the channels.
- The log is `VIOSOWarpBlend.log` next to the library; `logLevel=3` in the `.ini` shows more.
- A change of the warp, the content size or the sampling is checked with `WarpPrecision` (`docs/vwf-format.md`, "Precision").

## Documentation

`README.md` (quickstart), `docs/integration-guide.md` (pitfalls first), `docs/ini-reference.md` (every setting, how the parser reads), `docs/vwf-format.md` (the calibration file),
`docs/eyepoint-provider.md` (the plugin), `CHANGELOG.md`.

## Rules

- **The library is a drop-in.** Keep the interface (function signatures, struct layouts of `VWB_Warper`), the `.vwf` format and the meaning of `.ini` keys. A new behaviour needs a reason and a line in
  `CHANGELOG.md`; a changed `.ini` behaviour is also described in `docs/ini-reference.md`.
- **The code is the reference.** If a comment or a document and the code differ, check the code, then fix the text (or ask).
- **Do not guess in documents.** Write what you checked in the code or by running it, as plain facts, without telling how you found out (that belongs in the plan). What a reader has to know about a limit goes under "Known limits" in `docs/integration-guide.md`.
- **No binaries and nothing outside the repository** in the build. Sample data live in `data/`.
- **Files:** CRLF line endings. `Include/VIOSOWarpBlend.h` and `Include/VWBTypes.h` are Latin-1, keep their encoding. Comments and documents are in English, in the style of the surrounding code.
- Do not commit or push unless you were asked to.
- For **an integration** into a host program do not change the library; use the public interface and read `docs/agent-integration.md`.
