# Integration guide

For developers who add the VIOSO WarpBlend API to an image generator or media server. It follows the code of version 1.7. The
[README](../README.md) has the quickstart, [ini-reference.md](ini-reference.md) every setting, [vwf-format.md](vwf-format.md) the calibration file,
[eyepoint-provider.md](eyepoint-provider.md) the eye point plugin.

## The pitfalls, first

These cost the most time in integrations. Read them before you start.

1. **Always set `base` (or `trans`) in the .ini file, and match it to your world.** `base` tells the API whether your coordinate system
   is left handed (Direct3D: `base=[1,0,0,0; 0,1,0,0; 0,0,-1,0; 0,0,0,1]`) or right handed (OpenGL: the identity matrix).
   The handedness decides the projection `VWB_getViewProj` returns. If you set neither `base` nor `trans`, 1.7 uses the left handed default (z = -1) and writes a warning to the log at
   every log level. **That also happens under OpenGL, where it gives you the wrong projection.** Up to 1.6 the matrix was all zero then: 2D mappings were silently
   left handed, 3D mappings did not work at all. Without an .ini file at all, `trans` is the identity matrix (right handed).
   The warning is the only sign; look into the log (see "Where things go wrong"). The public field `VWB_Warper::trans` is overwritten in the degenerate case.
2. **Relative paths are relative to the folder of the VIOSOWarpBlend library, not to the working directory** (Windows). That is where the log is, where `calibFile`
   and the .ini are looked for. On Linux the working directory is used. If something is "not found", the error message shows the entry as read, in quotes, and the
   library folder.
3. **The .ini parser is the library's own, and strict.** Write `key=value` without blanks around `=`. Put comments on lines of their own; text after a value is part of it
   (`calibFile=a.vwf ;main` looks for a file with that whole name). Numbers ignore trailing text. See `ini-reference.md`.
4. **Bind your render target before `VWB_Init` (Direct3D 10, 11).** The warper reads the size of the output and the viewport from the render target that is bound at that moment.
   Without one the log warns "Render target not set"; with one of the wrong size it warns "Render target size does not match mapping size" and the output is resampled.
   Best make the render target the size of the calibration (`width` and `height` in the header of the channel; `MediaServerExampleDX11.exe file.vwf --list` shows it). The window may be smaller: the
   swap chain scales when it presents. The channel of the sample calibration `2D_plane_FHD.vwf` is exported for a second monitor to the right of the main monitor (offset 1920,0).
5. **`VWB_Create` and `VWB_Init` do different things.** Create reads the .ini file; you may change every field of `VWB_Warper` afterwards. Init loads the calibration file and builds the GPU
   resources. `calibFile`, `calibIndex`, `bPartialInput` and `trans` have to be set before Init.
6. **State.** `VWB_render` restores vertex buffer, input layout, primitive topology and raster state by default. Shaders, shader resources, samplers and constant buffers are *not*
   restored, set them again before your next draw call. The viewport is only touched with `VWB_STATEMASK_VIEWPORT`; without it the viewport must already cover the output. In Direct3D 12 all
   flags except `VWB_STATEMASK_CLEARBACKBUFFER` are ignored.
7. **`calibIndex` counts over all data sets of all files.** It is `-1` without an .ini file and **0** if an .ini file is read and does not say otherwise. With `-1` the library looks for the data set by name; `calibAdapterOrdinal=n`
   searches for the display `D<n> ...` and fails with an error that lists the names it found if there is none. It never falls back to another data set.
8. **The C++ wrapper throws.** `VWB` in `VIOSOWarpBlend.hpp` throws a `VWB_ERROR` (an enumeration value, not a `std::exception`) from its constructor when creating the warper or loading the library fails;
   catch it by that type. `Init` and `Render` return the error.
9. **A Direct3D 11 texture is at most 16384 pixels wide.** If your content space is larger, use `bPartialInput` and give each output only its part (below).
10. **OpenGL: the context has to be current at `VWB_Init` and `VWB_render`.** The API does no multi-threading. Direct3D 12: pass the `ID3D12CommandQueue` as device and a `VWB_D3D12_RENDERINPUT` to render;
    the pipeline is made for the render target format `D3D12RTVF` (default `R8G8B8A8_UNORM`).
11. **The eye point provider's parameter string must stay valid while the receiver runs.** The library keeps it in `VWB_Warper::eyeProviderParam`; a host that writes its own plugin must not free it.
12. **`VWB__logString` uses the text as a format string** (printf style). Double every `%` in text that you pass, for example a path or a message from the user: `%%`. Otherwise the text is interpreted (a `%s` reads an argument that was not passed).
13. **Direct3D 12: several frames in flight.** From 1.7.1 the warper keeps a ring of three slots. Every `VWB_render` takes the next slot (its own descriptor for the content texture, its own range in the
    constant buffer and, without your command list, its own allocator and list) and waits on a fence on the queue you gave `VWB_Create` if the GPU has not finished the render three calls ago. You may record and
    submit up to three frames before the GPU is done; more costs a wait, not a wrong picture. Two rules. First, with your own command list in `VWB_D3D12_RENDERINPUT::commandList`, close it and submit it to that queue
    before you call `VWB_render` for the same warper again; the warper counts on that. Second, call `VWB_render` of one warper from one thread at a time. Up to 1.7.0 the output was wrong with more than one frame in flight (see `CHANGELOG.md`, 1.7.1). The Direct3D 12 debug layer reports such errors (`CreateShaderResourceView ... bound as STATIC`, `CommandAllocator::Reset ... being reset before previous executions ... have completed`)
    only while the executions overlap, so a test with one frame in flight finds nothing. Test with moving content and two or three frames in flight (agent integration, "Check", step 4).

## How it works

A **channel** is one output of a projector or display. A **.vwf file** holds the calibration of one or more channels (`vwf-format.md`): for each pixel of the output where it looks into
the content (**warp**), how bright it is where projectors overlap (**blend**), and optionally a black level correction. The API takes the content as a texture and draws the warped and blended image
into the render target you bound.

```
VWB_Create ( device, ini file, channel name )  -> a VWB_Warper, filled with the settings of the .ini file
  [ change fields: calibFile, calibIndex, trans, bPartialInput ... ]
VWB_Init                                         -> loads the .vwf, builds the GPU resources
every frame:
   draw your content into a texture
   bind the output's render target
   VWB_render( warper, content texture, state mask )
VWB_Destroy
```

You can use the functions directly (`VIOSOWarpBlend.h`, static or dynamic binding, see the README) or the header-only wrapper class `VWB` in `VIOSOWarpBlend.hpp`, which loads the library and
holds the instance. The calls are the same.

### Direct3D 11, without an .ini file

```cpp
#include "VIOSOWarpBlend.hpp"

// device and the output's render target are yours; bind the render target before Init
VWB warper( nullptr, device, nullptr, "Display1" );      // throws VWB_ERROR
VWB_Warper& w = warper.get();
strcpy_s( w.calibFile, "2D_plane_FHD.vwf" );
w.calibIndex = 0;                                         // the first data set
float const lh[16]{ 1,0,0,0, 0,1,0,0, 0,0,-1,0, 0,0,0,1 };
memcpy( w.trans, lh, sizeof( lh ) );                      // base for Direct3D: left handed
if( VWB_ERROR err = warper.Init(); VWB_ERROR_NONE != err )
    /* VWB::GetErrorCStr( err ), and look into VIOSOWarpBlend.log */;

// every frame: contentTexture is an ID3D11Texture2D with your picture
warper.Render( contentTexture, VWB_STATEMASK_DEFAULT | VWB_STATEMASK_VIEWPORT );
```

`Examples/DirectX11/MediaServerExample.cpp` is a complete example (several outputs on one device, optional partial input). `Examples/DirectX11/DX11Example.cpp` is the tester.

### OpenGL

```cpp
// the OpenGL context is current
pWarper = std::make_shared<VWB>( "", nullptr, "VIOSOWarpBlendGL.ini", "Display1", 1, "" );
if( VWB_ERROR_NONE != pWarper->Init() ) /* ... */;

// every frame: draw your scene into the back buffer, then
pWarper->Render( VWB_UNDEFINED_GL_TEXTURE, VWB_STATEMASK_PIXEL_SHADER | VWB_STATEMASK_SHADER_RESOURCE );
```

With `VWB_UNDEFINED_GL_TEXTURE` (-1, not NULL and not 0) the warper copies the back buffer first; pass an OpenGL texture name instead, if you render into a texture. From 1.7.2 `VWB_render` returns
`VWB_ERROR_PARAMETER` for a name that is not a texture of the current context (the frame is not drawn) and `VWB_ERROR_SHADER` if its shader program cannot be made current; each is logged once, so check the result.
The default path (`bUseGL110=0`, shader version 330) runs in a Core profile context (3.3 or newer) and in a Compatibility context; `bUseGL110=1` uses the fixed pipeline and needs a Compatibility context.
A GL error that the host leaves behind does not stop the warper; errors pending at its first calls are cleared and logged once as a warning. `Examples/OpenGL_4/gl3_2.cpp` is the
example (the OpenGL one and the Linux one are in the same file). The sample .ini sets `base` to the identity (right handed) for OpenGL.

### Direct3D 12

`Examples/DirectX12/D3D12HelloTexture.cpp`: `VWB_Create` gets the command queue; `VWB_render` gets a `VWB_D3D12_RENDERINPUT` (source resource in `PIXEL_SHADER_RESOURCE` state,
render target resource in `RENDER_TARGET` state, its descriptor handle, optionally your command list).

## A host without access to the device: texture interop

Some engines give a plug-in neither the graphics device nor the queue (tested: UNIGINE 2.20 with Direct3D 12). The warper can still run, on a device of your own on the same adapter, with textures that the engine shares:

1. Create an `ID3D12Device` on the same adapter as the engine (compare the LUID, skip software adapters) and a direct queue. Pass that queue to `VWB_Create`.
2. Let the engine create the content texture and the output texture as shared resources (UNIGINE: `Texture::FORMAT_USAGE_SHARED`, `getResourceExternalMemory()->getWin32Handle()`) and open them with
   `ID3D12Device::OpenSharedHandle`. They start in state `COMMON`; transition them to what `VWB_render` needs and back to `COMMON`.
3. Each frame, on your queue and in one command list of your own: copy the content into a texture of your own that never changes (the source of the warper), call `VWB_render` for every channel into the output
   texture, submit. Use a command list and an allocator per frame slot and reset them only after the queue is done with them (pitfall 13).
4. The engine shows the output texture itself. If the engine renders its own camera into the window as well, replace that render (UNIGINE: the custom render callback of the main window), or it costs most of the frame.

**Synchronisation is the hard part.** If the engine can only signal a fence to the outside and cannot wait for a fence of yours, you cannot make the GPU work of the engine wait for the warper. UNIGINE's `ResourceFence` is of this kind:
the engine signals it, `waitGPU()` is a wait of the CPU, and a value set from your queue neither makes the engine wait nor stall it. What works is a ring of several content and several output textures, with the fences only polled
(`GetCompletedValue`), never waited for: the warper reads a content slot that the engine has finished, the engine shows the newest output that the warper has finished. The price is a delay of a few frames, because of the polling.
The fences of an engine can be unreliable in two places: in the first frames after the start, and just after a signal (a value that is reached does not always mean that the copy before it is visible). The ring therefore needs a start-up period
in which it does nothing, and a margin before it trusts a value. Both depend on the engine; find them for yours with the frame counter test (agent integration, "Check", step 4) and choose them generously: a value that is enough in one run can be
too small in the next, for example after a slow start.

More with UNIGINE: the engine pauses without keyboard focus (set the background update to "always render" before the world loads); render hooks also fire in the nested render of an offscreen viewport (do not draw your output there); switch on the
Direct3D 12 debug layer before the engine creates its device, later it removes the device; an offscreen scene render with shadows and post effects is expensive, without them it is cheap.

## Choose the channel

A file can hold many channels. Look into it first: `VWB_vwfInfo( path, &set )` (C++) or `VWB_vwfInfoC` fills the headers (`name`, `hostname`, `offsetX/Y`, size ...); release the
memory with `VWB_vwfInfo( NULL, set )`. Then give each warper its data set:

- `calibIndex=n` in the .ini or the field, position in the list of all data sets of all files in `calibFile` (separated by `,`).
- `calibIndex=-1`: the library searches the data set by name; with `calibAdapterOrdinal=n` it is the display `D<n> ...` (channel section only).
- `calibSplit=[columns, rows, column index, row index]`: one part of an evenly divided map.

`MediaServerExampleDX11 file.vwf --list` prints the table, `MakeTestVwf info file.vwf` the full headers.

## 2D: content space, `optimalRes`, `bPartialInput`

The warp of a 2D map holds, for every output pixel, a position in the **content space** of the whole installation, normalised to that space. After `Init` the warper tells you what it needs:

- `optimalRes` (`VWB_Warper::optimalRes`): the size of the content in pixels, at which one display pixel covers one content pixel. It is the size for a 1:1 assignment,
  but every content size works (the texture is sampled with filtering); a host with a given content resolution, such as a media server, keeps it.
- `optimalRect`: the pixels of the content that this channel uses (left and top inclusive, right and bottom exclusive).

If the whole content space fits into a texture, render it all and pass it; leave `bPartialInput` off. If it does not (a media server with 50 projectors), set
`bPartialInput=1` **before Init** and give each output a texture of exactly the size of its `optimalRect`, holding that part of the content: texel (0,0) is pixel
`(optimalRect.left, optimalRect.top)` of the content space. The warper corrects for the offset. Read `optimalRes` and `optimalRect` after Init, then create the textures.

```cpp
w.bPartialInput = true;                 // before Init
warper.Init();
int cw = w.optimalRect.right - w.optimalRect.left, ch = w.optimalRect.bottom - w.optimalRect.top;
// create a texture cw x ch, fill it with content pixels [left, right) x [top, bottom) of the space optimalRes.cx x optimalRes.cy
```

Notes:
- Up to 1.6 `optimalRes` was one pixel too small when float noise left it just below a whole number (a 1920 pixel wide channel gave 1919), and `optimalRect` was up to 1 pixel too small on the right and at the bottom. Texture sizes that you
  derived from them were one pixel short. Both are fixed in 1.7 (`optimalRes` is still cut off if it is not a whole number, e.g. 938.9 gives 938); if you worked around it, remove the workaround.
- **The VIOSO exports can have different `vCntDispPx` per channel** of one compound. Then `optimalRes` differs per channel. With `bPartialInput` each warper
  scales and offsets by its own `optimalRes`, so a texture that you share between channels and size by one of them does not fit the rest. Check the `content size` column of `--list`.
- The Direct3D 11 limit of 16384 pixels per texture side applies to the partial texture, too.

## Colour and gamma

The blend maps of the VIOSO exports already contain the gamma of the projector; the API multiplies them as they are and uses no `*_SRGB` views. The examples
render plain `UNORM` textures with the values you want to see on the display. `gamma` in the .ini is an additional correction (blend values `^(1/gamma)`) for
a projector whose gamma was changed after calibration; leave it at 1 otherwise. Black level correction can be switched off with `bDoNoBlack`, blending with `bDoNotBlend`. Details: `vwf-format.md`.

## Precision

The 2D warp was a 16 bit normalised texture in Direct3D up to 1.6; one step was `content width / 65535` pixels: 2.3 pixels at a content width of 153600. It is float since 1.7. OpenGL was always float.
Measurement and method: `vwf-format.md`, [`tools/WarpPrecision.cpp`](https://github.com/vioso/VIOSO_API/blob/v1.7.2/tools/WarpPrecision.cpp). The float texture needs twice the memory (about 66 instead of 33 MB for a 4K channel).

## 3D mappings and eye points

A map with the flag `3D` holds world coordinates instead of uv. With it the API can render the view through a moving eye point: `VWB_getViewProj`, `VWB_getViewClip` and `VWB_getPosDirClip` give the
view and projection for the eye and rotation you pass in (or that the eye point provider delivers), and your image generator renders its scene with them into the content texture. Rotation order is y, x, z,
positive means turning right, up and clockwise; the handedness of the projection follows `trans`/`base` (pitfall 1). The `autoViewC`, `splice`, `mode` and `bTurnWithView` settings
shape the view (`ini-reference.md`).

The 3D code and the eye point receiver are unchanged in 1.7; the sample calibration is 2D and there is no 3D test data in this repository (see "Known limits"). Use them as documented in `VIOSOWarpBlend.h`.
The sample plugin for eye points is described in `eyepoint-provider.md`.

## Where things go wrong

- **The log.** `VIOSOWarpBlend.log` in the folder of the library (set `logFile` and `logLevel` in the .ini; 2 shows the parameters, 3 more, 4 is a trace that also dumps debug textures).
  `bLogClear=1` starts a new file. Warnings that matter are written at every level.
- **Error codes** (`VWB_ERROR`): -1 generic (the log says more), -2 parameter (OpenGL: the source texture is not a texture of the current context), -3 .ini file could not be loaded, -4 blend, -5 warp (could not be loaded to the GPU),
  -6 shader (usually unsupported hardware; OpenGL: the shader program cannot be made current), -7 .vwf broken or wrong version, -8 .vwf not found, -9 not implemented, -10 network, -11 NDI, -16 nothing done (no error). `VWB::GetErrorCStr` gives text.
- **Blank output, not black:** check `calibFile` (the error shows it as read), the render target at `Init`, that your content texture is not empty, `calibIndex`.
- **Wrong or flipped projection from `VWB_getViewProj`:** `base` (pitfall 1). `bFlipDXVs` is a field only, it is *not* read from the .ini.
- **A dark line at the outermost pixel column:** the sampler uses the border color black; at the exact edge of the content the GPU mixes in 1/256 of it. Not an error of the mapping. Since 1.7.2 the same rim can appear in Direct3D 10, 11 and 12 where the warp lies just outside the content (before, the edge pixel was stretched there), as in OpenGL.
- **Light areas outside the picture:** two different things. The *image* is black where the warp points outside the content and where the blend map is 0; since 1.7.2 in all backends (up to 1.7.1 Direct3D 10, 11 and 12 repeated
  the edge pixel of the content outside it, which showed as grey). The *black level correction* is separate: a calibration with a black level map raises the output by the black level of the projector everywhere, also outside the mesh, whatever the
  content shows. That is intended; `bDoNoBlack=1` switches it off. A file without a black level map adds nothing.

## Known limits of 1.7

- Direct3D 10 with `bPartialInput`: with a very wide content space ([`tools/WarpPrecision`](https://github.com/vioso/VIOSO_API/blob/v1.7.2/tools/WarpPrecision.cpp): content space 153600 px, channel 3840 px) the output in Direct3D 10 is almost empty, while Direct3D 11, 12 and OpenGL are
  correct; with a small file Direct3D 10 is correct. If you use Direct3D 10 with `bPartialInput`, check the output.
- 3D mappings and eye points: unchanged in 1.7, without test data in this repository.
- OpenGL Core profile: tested with a Core 3.3 context on Windows (GLFW, debug output on, one Intel GPU) and with a Core 4.1 context on Linux (Mesa 23.2.1 llvmpipe under Xvfb), without a message from the warper; not tested on other GPUs and drivers or on macOS. `bUseGL110=1` and the X-Plane variant of the OpenGL path (`GLWarpBlendXPL.cpp`) are not changed for Core.
- Linux: the OpenGL example builds and runs (a synthetic two-channel .vwf), so far under Xvfb with Mesa software rendering, not on graphics hardware. Windows is the main platform of 1.7.
- Texture interop (above): the output arrives a few frames late because the fences are only polled, and the start-up period and the margin of the ring depend on the engine; tested with UNIGINE 2.20. Determine them again with the frame counter test for another engine or version.
- The Direct3D 9 render paths are still in the library; the examples for them are gone.
