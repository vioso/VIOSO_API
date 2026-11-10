# Integrating VIOSO WarpBlend into a host program (for agents)

This is a procedure for an agent that builds the VIOSO WarpBlend API into the project of an integrator (an image generator, a media server, a game). The facts are in
[integration-guide.md](integration-guide.md) (pitfalls first), [ini-reference.md](ini-reference.md) and [vwf-format.md](vwf-format.md); this page does not repeat them, it tells you in which order to work and
how to check. Read the pitfalls at the top of the integration guide before you write code.

## Hard rules

- **Do not change the `.vwf` file, the VIOSOWarpBlend DLL, the headers or the library sources.** The calibration belongs to the installation. An integration uses the public interface only (`VIOSOWarpBlend.h`,
  or the wrapper `VIOSOWarpBlend.hpp`).
- **Do not invent settings.** Every `.ini` key and its default is in `ini-reference.md`. A field of `VWB_Warper` that is not in `VWBTypes.h` does not exist.
- Do not pull code from the examples that needs their private helpers (the tester uses a model loader, the media server a PNG library); take the calls to the API, not the scaffolding.
- If something is unclear or fails, **read the log** (below) before you change anything. If you cannot get it to work, report what you tried and what the log says. Do not hide a failure with a fallback.

## 1. What you need

- The release archive (or a build of this repository): `VIOSOWarpBlend64.dll` (x64) and `Include/`. For a check also the tester `VWFTesterDX11.exe`, the sample calibration `2D_plane_FHD.vwf` and the test picture `earth.png`.
- The calibration file (`.vwf`) of the installation, or the sample file for a first test.
- The host project, to read and build.

## 2. Find out the graphics API of the host

Search the host for what it renders with, then take the matching example as a template:

| Host uses | First argument of `VWB_Create` | Template (in the repository) |
|---|---|---|
| Direct3D 11 | the `ID3D11Device*` | `Examples/DirectX11/MediaServerExample.cpp` (several outputs), `Examples/DirectX11/DX11Example.cpp` |
| Direct3D 12 | the `ID3D12CommandQueue*`; `VWB_render` gets a `VWB_D3D12_RENDERINPUT` | `Examples/DirectX12/D3D12HelloTexture.cpp` |
| OpenGL | `nullptr`, with the context current | `Examples/OpenGL_4/gl3_2.cpp` |
| Direct3D 12, but the host gives no device or queue (an engine such as UNIGINE) | the `ID3D12CommandQueue*` of a device **of your own** on the same adapter, with textures shared by the host | integration guide, "A host without access to the device: texture interop" |
| Direct3D 9, 10 | the device | no example; the render paths exist, check on your side |
| Vulkan, Metal, anything else | - | **not supported. Stop and tell the integrator.** |

If the host has several graphics back ends, do one at a time.

## 3. Load the library

Put `VIOSOWarpBlend64.dll` next to the host executable. Use the wrapper `VIOSOWarpBlend.hpp` (it loads the library at runtime) or link `VIOSOWarpBlend64.lib`; see the README, "Usage". Relative paths
(`.ini`, `calibFile`, log) are resolved against the **library folder**.

## 4. Choose the channels

Look into the file first: `MediaServerExampleDX11.exe file.vwf --list` or `VWB_vwfInfo` (integration guide, "Choose the channel"). One output of the host gets one warper with one data set
(`calibIndex`). Write down which channel belongs to which output of the host.

## 5. Write the integration

In this order (integration guide, "How it works" and the code samples):

1. Bind the output's **render target before `VWB_Init`** (Direct3D 10 and 11).
2. `VWB_Create` with the device; then set `calibFile`, `calibIndex` and **`trans` (or `base` in the `.ini`) for the handedness of the host**: left handed (z = -1) for Direct3D, the identity matrix for OpenGL.
   Never leave it out.
3. `VWB_Init`. Check the result; on an error write the text (`VWB::GetErrorCStr`) and the entry `calibFile` as read to the host's log.
4. After `Init`, read `optimalRes` and `optimalRect`. `optimalRes` is the content size for a 1:1 assignment, but every content size works: a host with a given content resolution, such as a media server, keeps it. If the content space is larger than the graphics API allows (Direct3D 11: 16384 pixels) or the host sends
   only a part to each output, use `bPartialInput` as described in the integration guide.
5. Each frame: render the content into the texture, bind the output's render target, `VWB_render`. Set the state again that the warper does not restore (integration guide, pitfall 6). In OpenGL pass a texture name, or `VWB_UNDEFINED_GL_TEXTURE` (-1) to copy the back buffer, never NULL or 0;
   check the result of `VWB_render` (from 1.7.2 `VWB_ERROR_PARAMETER` for a name that is no texture, `VWB_ERROR_SHADER` if the shader program cannot be made current). The OpenGL path runs in a Core profile context (3.3 or newer) with the default `bUseGL110=0`; do not ask for a Compatibility context.
6. Destroy the warper before the device.

On Windows build from a Developer Command Prompt, or run `vcvars64.bat` of Visual Studio first.

## 6. Check

1. **Sizes and log, without comparing pictures.** Compare `optimalRes` and `optimalRect` that the warper of the host reports after `Init` with the `content size` column of
   `MediaServerExampleDX11.exe file.vwf --list` and, for `optimalRect`, with the `content WxH, rect [l,t,r,b]` line that `MediaServerExampleDX11.exe file.vwf` prints for the channel. The log (`VIOSOWarpBlend.log`) has no `ERROR` and no "Render target size does not match".
2. **Against the tester, if you can look at the screen.** The tester is the reference: it uses the same library and the same file. Start `VWFTesterDX11.exe file.vwf -p earth.png` (or the OpenGL example; `earth.png` is in the archive next to the sample calibration, 2048 x 1025) with the
   **same `.vwf`**, and note what the channel looks like. Let the host render **the same picture** (`earth.png`) as content and compare the output of the same channel: position, scale, the edges of the picture, where the blend of
   overlapping projectors starts. If the image is mirrored, flipped or distorted in 3D, look at `base` first.
   The tester also runs with one monitor and needs no extra argument: `VWFTesterDX11.exe file.vwf`. A channel whose position (`offsetX`, `offsetY` in the file; (1920,0) for the sample calibration) is not on the desktop
   is moved, or scaled if it does not fit, onto the primary monitor; the window title then ends with `(preview, moved to fit)` or `(preview, scaled to fit, N%)`. `-s` shows a quarter size. The preview on the primary monitor is intended: it is the comparison.
3. **Blend and several channels.** The sample calibration does not test the blend: its blend map is 1.0 everywhere. A statement about the blend needs this test with two channels. `MakeTestVwf.exe make wall.vwf --channels 2x1 --size 1920x1080 --overlap 200` writes a calibration of two channels that overlap by 200 pixels (content space 3640 x 1080, with a
   blend ramp in the overlap). Run the host with it: the content has to continue across the channels without a jump, and the overlap has to blend (`docs/examples.md`, "MakeTestVwf.exe").
4. **Several frames in flight (Direct3D 12) and texture interop.** A still picture hides the errors of this kind. Let the host render **moving content with a frame counter in the picture**
   (for example the frame number as 16 bars of black and white in the top rows) and read back every output, a few frames later without waiting for the GPU. Each output has to show the counter of the content frame it was made from.
   Run it with the Direct3D 12 debug layer on and with two or three frames in flight; the debug layer reports the overlap errors (integration guide, pitfall 13) only while the executions overlap, the readback finds the wrong picture. A host
   with texture interop uses the same test to find the margin and the start-up skip of its ring.
5. **Outside the content.** The image is black where the warp points outside the content, with every backend (1.7.2). `MakeTestVwf.exe make out.vwf --size 1920x1200 --content 1280x720` makes a file with such an area: the content fills the top left 1280 x 720 pixels of the output, the rest has to be black. Grey or coloured areas there are an error. A calibration with a black level map
   raises the output outside the mesh as well: that is the black level correction, not the image (`docs/vwf-format.md`, "Blend and black level").

## 7. Read the log

`VIOSOWarpBlend.log` is by default in the folder of the library; `logFile=` in the `.ini` (or the log file name passed to `VWB_Create`) puts it elsewhere. Set `logLevel=3` in the `.ini` (or pass a log level to `VWB_Create`) while you work. Look for, in this order: `ERROR`, `WARNING`, then the lines
`INFO: AutoView ... yields: optimalRes=... optimalRect=...`. The integration guide ("Where things go wrong") explains the common messages (a default for `base` in use, no render target at `Init`,
`calibIndex` out of range, `.vwf` not found).

## 8. Report

Tell the integrator which API you used, which channel is on which output, the values of `optimalRes` and `optimalRect` you read, and what you compared with the tester. Report only what you checked.
Put everything else in a separate list "Not checked", for example: the blend (the sample calibration cannot show it), the black level (the sample calibration holds a black level map whose parameters are 0, so it
has no effect), 3D mappings and eye points, the Direct3D 10 path with `bPartialInput`, more than one frame in flight (step 4) and texture interop, if you did not run the frame counter test.
