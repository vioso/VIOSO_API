# The examples

What each example program does, how to start it, what you should see and how to end it. The source code of the examples is in `Examples/` of the release package, without the third-party libraries (assimp, libpng, zlib, glm) and without build files.

All programs load `VIOSOWarpBlend64.dll` at run time from their own folder. Relative paths (`.ini`, `.vwf`, log file) are resolved against the **folder of that DLL**, not the working directory
(integration guide, pitfall 2). The build and the release archive put the sample files next to the programs, so every program below starts without arguments from that folder, except the media server
example and the tester, which want a `.vwf` file.

The sample calibration is a 2D map of one channel of 1920 x 1080. It bends the picture into a quadrilateral that is wider at the top than at the bottom, on black.

`earth.png` (PNG, 2048 x 1025) is the test picture next to it: `VWFTesterDX11.exe 2D_plane_FHD.vwf -p earth.png` shows it through the warp, `D3D12Example.exe -image earth.png` too. Render the same picture as content in your host and compare it with the tester (`agent-integration.md`, "Check").

| Program | Graphics API | Purpose |
|---|---|---|
| [`VWFTesterDX11.exe`](#vwftesterdx11exe) | Direct3D 11 | show all channels of one or more .vwf files, with a test scene or a picture |
| [`MediaServerExampleDX11.exe`](#mediaserverexampledx11exe) | Direct3D 11 | several outputs on one device, content as one image, list the channels of a file |
| [`D3D12Example.exe`](#d3d12exampleexe) | Direct3D 12 | the smallest Direct3D 12 integration |
| [`OpenGLExample.exe`](#openglexampleexe) | OpenGL 4 | the OpenGL integration (Windows and Linux) |
| [`MakeTestVwf.exe`](#maketestvwfexe) | none | write synthetic .vwf files for tests, print the headers of a file (Windows and Linux) |

## VWFTesterDX11.exe

The reference to compare an integration against: it uses the library and the file in the same way as your host program should. Source: `Examples/DirectX11/DX11Example.cpp`.

**Start**

```
VWFTesterDX11.exe                      a file dialog opens
VWFTesterDX11.exe file.vwf             all channels of the file, each in a window of its own
VWFTesterDX11.exe file.vwf -p pic.png  the same, showing a PNG picture
```

You can also drop a .vwf file on the executable. Without `-i` a .vwf file needs no `.ini` file. Several files: `VWFTesterDX11.exe a.vwf b.vwf`.

**Arguments** (`VWFTesterDX11.exe -h` shows them in a box)

| Argument | Meaning |
|---|---|
| `-F file [file]...` | `--mode file`: the channels of these files, each in its original place. This is the mode if you give a bare file name |
| `-I [inifile]` | `--mode inifile`: the .vwf file named in the `[default]` section of the .ini file, all its channels |
| `-M [# [Name ...]]` | `--mode monitor`: one window per listed monitor of the Windows display configuration (0 = primary); the warper uses the section `Name` of the .ini file |
| `-A [Name]` | `--mode allmonitors`: a window on every connected monitor |
| `-W Name x y w h [...]` | `--mode window`: windows at the given positions; without arguments one window `Display1` of 800 x 600 |
| `-c` | `--render cubes`: coloured cubes (the default) |
| `-m model` | `--render model`: a 3D model file (read with assimp) |
| `-p file.png` | `--render image`: a PNG picture, RGB or RGBA, 8 or 16 bit |
| `-i inifile` | use this .ini file; `-i ""` uses internal defaults. Without `-i` the modes other than `-F` read `VIOSOWarpBlend.ini` |
| `-n` | `--nowarp`: the same windows without the warper, to compare |
| `-s` | `--small`: a preview at 1/4 size (modes `-F` and `-I`) |
| `-h` | the help |

In the modes `-F` and `-I` the windows are placed at `offsetX`/`offsetY` of each channel. If the arrangement does not fit on your desktop, it is shrunk evenly and centred on the primary monitor; the window titles then say
`(preview ...)`. The windows have no frame and no title bar.

**Keys.** `Esc` ends the program (click a window first, it needs the keyboard focus). The other keys move the 3D scene while you hold them: `W` `S` forward and back, `A` `D` left and right, `R` `F` up and down, the arrow
keys turn it, `,` and `.` roll it. They change the world matrix of the renderers; the cubes and the model use it, the picture (`-p`) does not move.

**What you should see.** With the sample file: a field of coloured cubes (blue to magenta) on a light blue background, inside the quadrilateral of the calibration, black around it. With `-p`: your picture in the same quadrilateral.
With a file of several channels: one window per channel, the cubes continuing across the windows, and a blend where projectors overlap.

**Errors** appear in a message box and on stderr: wrong arguments (exit code -2), an error of the library (-3, with the text of the error and the folder where relative paths are resolved).

## MediaServerExampleDX11.exe

A media server's job: one device, one window and one warper per channel, the content as **one picture of the whole content space**. Optionally every output gets only the part it needs (`bPartialInput`).
Source: `Examples/DirectX11/MediaServerExample.cpp`; it is the template for hosts with several outputs.

**Start**

```
MediaServerExampleDX11.exe file.vwf --list
MediaServerExampleDX11.exe file.vwf
```

**Arguments**

| Argument | Meaning |
|---|---|
| `file.vwf` | the calibration file (required) |
| `--list` | print the table of the channels and end: number, name, host, position, size and content size of each channel |
| `--channels A,B` | only the channels with these names (a name that is not in the file is an error) |
| `--host name` | only the channels whose `hostname` in the file is `name` |
| `--partial` | every output gets a texture of exactly its `optimalRect`: `bPartialInput` is set. Needed when the content space does not fit into one texture (16384 pixels) |
| `--png file` | a PNG picture (RGB or RGBA, 8 or 16 bit) that covers the whole content space; it may have another size than the content |
| `--small` | everything at 1/4 size |

For every channel the program prints `content WxH, rect [l,t,r,b]`: the `optimalRes` and `optimalRect` it read after `Init`. Run it from a console to see them.

**Keys.** `Esc` ends the program (in one of the windows).

**What you should see.** A dark blue grid: a thin line every 100 content pixels, a thick yellow line every 500, labelled with the content coordinates (`0,0`, `500,0`, `0,500` ...). With the sample file the grid lies in the quadrilateral of the calibration.
With several channels the lines have to continue across the windows without a jump; if they do not, look at the `content size` column of `--list` (vwf-format.md, "Content size and part").

## D3D12Example.exe

The Direct3D 12 integration in one small program. It renders a textured quad and warps it. Source: `Examples/DirectX12/D3D12HelloTexture.cpp`.

**Start:** `D3D12Example.exe`. It reads `ViosoWarpBlend.ini` and the channel `Display1`, in the window of 1280 x 720 at the top left of the desktop.

**Arguments** (a `-` or `/` in front; the full word)

| Argument | Meaning |
|---|---|
| `-ini file` | the .ini file (default `ViosoWarpBlend.ini`) |
| `-name channel` | the channel name, section of the .ini file (default `Display1`) |
| `-dll name` | the library to load (default `ViosoWarpBlend64`) |
| `-image file` | a picture for the quad instead of the checkerboard: PNG, JPEG, BMP and GIF; TIFF is not read. A file that cannot be read is reported like the other errors (`Failed to load image: <file>`) |
| `-rect l t r b` | the window rectangle |
| `-warp` | use the WARP software device |
| `-assets folder` | where `shaders.hlsl` is (default: the folder of the program) |

**Keys.** `Esc` ends the program.

**What you should see.** A blue field with a black and white checkerboard strip in the middle, inside the quadrilateral of the calibration, black around. It does not move.

**Errors** appear in a message box and on stderr (exit code 2) with the text of the library error and the paths.

## OpenGLExample.exe

The OpenGL integration, the same source (`Examples/OpenGL_4/gl3_2.cpp`) for Windows and Linux (`OpenGLExample`). It reads `VIOSOWarpBlendGL.ini`; there, `base` is the identity matrix (right handed).

**Start:** `OpenGLExample.exe [channel [x y w h]]`, on Linux `./OpenGLExample [channel [x y w h]]`, for example `OpenGLExample.exe Display1 0 0 1280 720`. The defaults are the channel `Display1` and, without a size, a window that covers the
whole monitor at the position `x y` (`0 0`: the primary monitor).

**Keys.** `Esc` ends the program on Windows. On Linux any key press in the window ends it.

**What you should see.** The same coloured cubes on light blue as in the tester, inside the quadrilateral of the calibration. The window has no frame.

**Errors** appear in a message box (Windows) and on stderr, with the error text and the paths; the exit codes are 2 (window), 3 (`VWB_Create`), 4 (`VWB_Init`).

## MakeTestVwf.exe

A tool, not an example of the API: it prints the headers of a .vwf file and writes synthetic 2D calibration files. It is in `bin/` of the release package (on Linux the program is called `MakeTestVwf`, without `.exe`). It does not load the library; it contains the
reader and writer of the file format (`SaveVWF`, `ScanVWF`), so the files it writes follow the format by construction. Source: [`tools/MakeTestVwf.cpp`](https://github.com/vioso/VIOSO_API/blob/v1.7.2/tools/MakeTestVwf.cpp).

**Start**

```
MakeTestVwf.exe info file.vwf
MakeTestVwf.exe make out.vwf [options]
```

**`info`** prints one entry per channel: number, name, resolution, flags, position on the desktop, and `vCntDispPx` and `vPartialCnt` (the content size and the part of the content space,
see [vwf-format.md](vwf-format.md)). With the sample file it prints one channel of 1920 x 1080 at offset (1920,0); `vCntDispPx` shows `0.00153362 0.00106508` there.

**`make`** writes a file whose mapping is linear: a pixel of the output looks at the same position in the content space. Without options it makes one channel of 256 x 144. The options:

| Option | Meaning (default) |
|---|---|
| `--channels CxR` | tiles of channels, columns x rows (1x1) |
| `--size WxH` | pixels (warp records) of each channel (256x144) |
| `--overlap N` | overlap of neighbouring channels in pixels (0) |
| `--content WxH` | size of the content space in pixels (all channels, minus the overlaps) |
| `--content-offset X,Y` | position of the top left corner of the channels in the content space, in pixels (0,0) |
| `--screen-offset X,Y` | position of the first channel on the desktop (0,0) |
| `--distort A` | sine distortion of the mapping, in content pixels, to make the warp visible (0) |
| `--edge N` | soft edge of N pixels on all outer sides of the arrangement: the blend ramps from black to full, like in an overlap (0, no edge) |
| `--edge-left N`, `--edge-right N`, `--edge-top N`, `--edge-bottom N` | the same for one outer side; a later option wins over `--edge` for its side (0) |
| `--name PREFIX` | channel names are `PREFIX_<column>_<row>` (`Test`) |

The edges are for a channel without a neighbour on that side (a single channel, or the outside of an arrangement): the blend of such a side is otherwise 1. Between channels the overlap ramps are used. An edge must be smaller than the width or height of a channel. A soft edge shows whether the host applies the blend at all, with a single channel.

The output name is a path; a relative path is relative to the working directory (the same for `info`), and a missing folder is created. The program prints the full path it wrote. The file is large for large channels: `--channels 2x1 --size 1920x1080` gave about 79 MB.

**What to test with it**
- **Several channels and overlaps** without a calibration: `make wall.vwf --channels 2x1 --size 1920x1080 --overlap 200` (content space 3640 x 1080). Look at it with `MediaServerExampleDX11.exe wall.vwf --list`
  and in the tester. The tool writes a blend map with linear ramps inside the overlaps, so the blend is visible in the picture.
- **A content space that is larger than a texture, with `bPartialInput`:** with `--content` and `--content-offset` one channel can describe a small part of a very wide content space, for example
  `--size 3840x2160 --content 153600x2160`. This is the file that [`tools/WarpPrecision`](https://github.com/vioso/VIOSO_API/blob/v1.7.2/tools/WarpPrecision.cpp) measured with (vwf-format.md, "Precision"); `MediaServerExampleDX11.exe file.vwf --partial` shows such files, because the content space does not fit into one texture.
  With generated files of 10 channels and a content space of 19328 pixels the grid is continuous across the channels and the blend ramps are visible.
- **The warp itself:** `--distort 20` bends the mapping by up to 20 content pixels.

## EyePointProvider

A plugin for eye points, not a program: see [eyepoint-provider.md](eyepoint-provider.md).
