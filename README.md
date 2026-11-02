# VIOSO WarpBlend API

This library is meant to be used in image generators, to do warping and blending. It takes a VIOSO Warp File (.vwf) export and a texture buffer 
to sample from. If no texture buffer is given, it uses a copy of the current back buffer. It will render to the currently set back buffer.
It provides image based warping, suitable for most cases and, if a 3D map is provided, dynamic eye warping.

- You need a vwf mapping to use. This is the native VIOSO Warp Format for calibration files. Example calibrations (.vwf) to download:
https://vioso-my.sharepoint.com/:f:/p/jerbi_ahmed/IgCYy0VoCTVzS53ECvtF8bvwAec5J1zaHThyrX7Npy1hT8k?e=h2gba5
Or generate an export from our Calibration software:
https://vioso.com/calibration-software/vioso7/

- You need a configuration (VIOSOWarpBlend.ini) to initialize the plugin. The online documentation describes the .ini: https://docs.vioso.com/viosowarpblend.ini-reference
  [docs/ini-reference.md](docs/ini-reference.md) is the same reference, in the package, matching this version.
  

## Quickstart

1. **Get the programs**, in one of two ways:
   - **The release package** (`VIOSO_API-<version>-win-x64.zip`): unpack it and start `bin\VWFTesterDX11.exe`. The sample calibration `2D_plane_FHD.vwf`,
     the test picture `earth.png` and the sample `.ini` files are in the same folder.
   - **Build from the sources** (Windows, Visual Studio 2022 or 2026 and CMake): see [Build](#build). The build copies the sample calibration and
     the sample configuration next to the executables.
2. **See a picture.** Start `VWFTesterDX11.exe` without arguments and choose a .vwf file (or drop the file on the executable, or
   `VWFTesterDX11.exe file.vwf`). It opens every channel of the file in its original place, or as a preview that fits on your desktop, and shows a test scene through the warp.
   `VWFTesterDX11.exe file.vwf -p picture.png` shows a picture instead. `MediaServerExampleDX11.exe file.vwf --list` lists the channels of a file. All examples, their arguments and keys: [docs/examples.md](docs/examples.md).
3. **Put it into your application.** The short version, Direct3D 11, without an .ini file:

   ```cpp
   #include "VIOSOWarpBlend.hpp"                       // loads the library at runtime

   // bind your output's render target, then:
   VWB warper( nullptr, d3d11Device, nullptr, "Display1" );   // throws VWB_ERROR
   VWB_Warper& w = warper.get();
   strcpy_s( w.calibFile, "2D_plane_FHD.vwf" );
   w.calibIndex = 0;                                          // data set in the file
   float const lh[16]{ 1,0,0,0, 0,1,0,0, 0,0,-1,0, 0,0,0,1 };
   memcpy( w.trans, lh, sizeof( lh ) );                       // left handed, Direct3D
   warper.Init();

   // every frame: render your content into a texture, bind the output's render target, then
   warper.Render( contentTexture, VWB_STATEMASK_DEFAULT | VWB_STATEMASK_VIEWPORT );
   ```

   OpenGL: create the warper with the OpenGL context current, `base` is the identity matrix (right handed), and
   `warper.Render( VWB_UNDEFINED_GL_TEXTURE, VWB_STATEMASK_PIXEL_SHADER | VWB_STATEMASK_SHADER_RESOURCE )` after your scene. See
   the [integration guide](docs/integration-guide.md), which also lists the pitfalls: **set `base` always**, relative paths are relative to the library folder, and more.

More: [integration guide](docs/integration-guide.md), [.ini reference](docs/ini-reference.md), [.vwf format](docs/vwf-format.md),
[examples](docs/examples.md), [eye point provider](docs/eyepoint-provider.md).

## Binding the library

Use the wrapper class `VWB` (see the quickstart), or the C functions of `VIOSOWarpBlend.h`, linked statically or loaded at run time. The calls and their
order are described in the [integration guide](docs/integration-guide.md).

### Static binding

Link against `VIOSOWarpBlend64.lib` and have `VIOSOWarpBlend64.dll` next to your executable or in the `PATH`.

```cpp
#include "VIOSOWarpBlend.h"

VWB_Warper* warper = nullptr;
if( VWB_ERROR_NONE != VWB_CreateA( device, "VIOSOWarpBlend.ini", "Display1", &warper, 0, nullptr ) ||
    VWB_ERROR_NONE != VWB_Init( warper ) )
{
    /* report the error, look into VIOSOWarpBlend.log */
}

// every frame, with the output's render target bound
// (3D maps: call VWB_getViewProj( warper, eye, rot, view, proj ) first)
VWB_render( warper, (VWB_param)contentTexture, VWB_STATEMASK_DEFAULT | VWB_STATEMASK_VIEWPORT );

VWB_Destroy( warper );   // before you release the device
```

`VWB_Create` is `VWB_CreateW` in a Unicode build; `VWB_CreateA` takes `char` strings in any build.

### Dynamic binding

The header can declare the function pointers and load the library for you. Have your platform headers (`windows.h`, `d3d11.h` ...) included before.

```cpp
#include <windows.h>
#define VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT   // declares and defines the function pointers
#include "VIOSOWarpBlend.h"

VWB_Warper* g_warper = nullptr;

bool Init( void* device )
{
#define VIOSOWARPBLEND_DYNAMIC_INITIALIZE         // loads the library and the functions
#include "VIOSOWarpBlend.h"
    return nullptr != VWB_CreateA && nullptr != VWB_Init && nullptr != VWB_render && nullptr != VWB_Destroy &&
        VWB_ERROR_NONE == VWB_CreateA( device, "VIOSOWarpBlend.ini", "Display1", &g_warper, 0, nullptr ) &&
        VWB_ERROR_NONE == VWB_Init( g_warper );
}

void Render( void* contentTexture )
{
    VWB_render( g_warper, (VWB_param)contentTexture, VWB_STATEMASK_DEFAULT | VWB_STATEMASK_VIEWPORT );
}

void Destroy()
{
    if( g_warper )
        VWB_Destroy( g_warper );
    g_warper = nullptr;
#define VIOSOWARPBLEND_DYNAMIC_UNINITIALIZE       // sets the pointers to NULL and frees the library
#include "VIOSOWarpBlend.h"
}
```

The library is loaded as `ViosoWarpBlend64` from the folder of your executable (the usual DLL search); define `VIOSOWARPBLEND_FILE` before the `INITIALIZE` include to name another file.
To keep the declarations in a header, use `VIOSOWARPBLEND_DYNAMIC_DEFINE` there and `VIOSOWARPBLEND_DYNAMIC_IMPLEMENT` in exactly one source file instead of `..._DEFINE_IMPLEMENT`.

## Examples
The build produces these examples (Windows unless noted), all loading the library at runtime. Arguments, keys and what you should see: [docs/examples.md](docs/examples.md).

- `VWFTesterDX11` (Direct3D 11): shows the outputs of one or more .vwf files, in their original places or as a preview; start it without arguments to choose a file.
- `MediaServerExampleDX11` (Direct3D 11): several outputs on one device, content as one image, optionally only the part each channel needs (`--partial`).
- `D3D12Example` (Direct3D 12): a scene rendered through the warper, starts with the sample calibration.
- `OpenGLExample` (OpenGL 4, Windows and Linux).
- `EyePointProvider`: a plugin for eye points, see [docs/eyepoint-provider.md](docs/eyepoint-provider.md).

`Include/VIOSOWarpBlend.cs` is a C# wrapper (not built here). The Direct3D 9, 9Ex and 10 render paths are still in the library; the
former DirectX 9, Mesh, OpenGL 1, Vulkan, `OpenGL_4_vwfInfo`, `MapingLoader`, `TrackerSim` and `Test` example projects have been removed.

## Build
The build uses CMake (3.21 or newer) only. Presets for Windows x64 and Linux are in [`CMakePresets.json`](https://github.com/vioso/VIOSO_API/blob/v1.7.1/CMakePresets.json).

Windows (Visual Studio 2022 or 2026, workload "Desktop development with C++"; no ATL needed):
open the repository folder in Visual Studio ("Open a local folder") and pick a preset, or from a Developer Command Prompt

```
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release
```

Linux (tested with GCC 13 on Ubuntu 24.04):

```
sudo apt-get install cmake ninja-build g++ libgl-dev libglu1-mesa-dev libx11-dev
cmake --preset linux-release
cmake --build --preset linux-release
```

The binaries end up in `out/build/<preset>/bin`: `VIOSOWarpBlend64.dll` (Windows) or `libVIOSOWarpBlend.so` (Linux),
the EyePointProvider plugin and the examples. The examples load the library at runtime from their own directory.
The build copies the sample configuration files (`VIOSOWarpBlend.ini`, `VIOSOWarpBlendGL.ini`, from [`data/`](https://github.com/vioso/VIOSO_API/tree/v1.7.1/data)) and the sample calibration
`2D_plane_FHD.vwf` and the test picture `earth.png` next to the example executables, unless files of these names are already there. Relative paths in the .ini
file (calibration file, log file) are resolved against the folder of the VIOSOWarpBlend library on Windows (the working directory on Linux).
More example calibration files (.vwf) are available via the download link at the top.

Online documentation (configuring and usage): [https://docs.vioso.com/vioso-warpblend-api](https://docs.vioso.com/vioso-warpblend-api)
