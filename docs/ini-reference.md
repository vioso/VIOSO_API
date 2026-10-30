# VIOSOWarpBlend.ini reference

This is the reference for the settings of `VIOSOWarpBlend.ini`, version 1.7.

## How the file is read

- **Location.** A relative path (the .ini, `calibFile`, `logFile`, `eyePointProvider`) is resolved against the folder of the
  loaded VIOSOWarpBlend library on Windows, **not** against the working directory. On Linux it is the working directory.
  `.ini` is appended, if the name has no extension.
- **Sections.** `[default]` applies to all channels. A section named like the channel (the `channelName` passed to
  `VWB_Create`) overrides it. A key that is in neither section keeps the built-in default.
- **Syntax.** Section and key names are not case sensitive. Write `key=value` without blanks around `=`; there is no trimming
  there. Put comments on lines of their own, starting with `;`. Text after a value is part of the value: numbers ignore it
  (`calibIndex=2 ;left` is 2), but file names and other strings do not. If a calibration file or .ini can't be opened,
  the log (and the examples) show the entry in quotes exactly as it was read, so a comment that went into the name is visible.
  These rules are the same on Windows and Linux: the library has its own parser (`GetIniString` in `PathHelper.cpp`, no
  `#ifdef`), it does not use `GetPrivateProfileString`.
- **Numbers and lists.** Integers and floats as usual. Matrices and vectors in brackets, separated by `,` or `;`:
  `[1,0,0,0; 0,1,0,0; 0,0,1,0; 0,0,0,1]`.
- **Not read from the .ini:** `bFlipDXVs` (set the field before `VWB_Init`), and `ndiCalibStream`, which appeared in old sample files
  and is ignored.

## Keys

"Built-in" is the value without an .ini file, or if the key is missing.

| Key | Meaning | Built-in |
|---|---|---|
| `logFile` | log file; `stdout` and `stderr` are accepted | the value passed to `VWB_Create`, else `VIOSOWarpBlend.log` |
| `logLevel` | 0 errors and important warnings, 1 plus warnings, 2 plus parameters and progress, 3 more details, 4 trace (also dumps debug textures as .bmp next to the log) | the value passed to `VWB_Create` |
| `bLogClear` | 1: the first warper of the process starts a new log (the old one is renamed to `.bak`) | 0 |
| `calibFile` | calibration file(s), several files separated by `,` | `vioso.vwf` |
| `calibIndex` | data set to use, counted over all data sets of all files in `calibFile`, from 0. Only the warp and blend data of that one is loaded. | -1 without .ini, **0** with an .ini |
| `calibAdapterOrdinal` | `n`: use the data set whose name starts with `D<n> ` (Windows display number). Works with the channel section only and only if `calibIndex` is -1 or missing. If there is none, `VWB_Init` fails and the log lists the names found. | - |
| `calibSplit` | `[columns, rows, column index, row index]`: use one part of an evenly divided map; the size of the map must be divisible | `[0,0,0,0]` (off) |
| `base` | transformation from your world coordinates to the calibration coordinates (usually millimeters), **transposed** as written; its handedness decides which projection the `VWB_getViewProj` family returns. Identity: right-handed (OpenGL). z = -1: left-handed (Direct3D). **Always set it.** | identity without .ini; see "degenerate base" |
| `trans` | same as `base`, but **not** transposed. If both are set, `trans` wins. | identity without .ini |
| `eye` | eye offset `[x,y,z]` relative to the pivot | `[0,0,0]` |
| `dir` | view direction `[pitch,yaw,roll]` in degrees; only used without `bAutoView` | `[0,0,0]` |
| `fov` | `[left,top,right,bottom]` in degrees; only used without `bAutoView` | `[35,30,35,30]` |
| `screen` | distance of the screen plane; only used without `bAutoView` | 1 |
| `near`, `far` | clip planes | 0.125, 20000 |
| `bAutoView` | calculate `dir`, `fov` and `screen` (3D) or the optimal content size and rect (2D) from the mapping | **1** |
| `autoViewC` | widening of the view for moving eye points (3D) | 1 |
| `bTurnWithView` | the world turns and moves with the view direction (motion platforms) | 0 |
| `bDoNotBlend`, `bDoNoBlack` | switch off blending, switch off black level correction | 0 |
| `bBicubic` (or `bicubic`) | bicubic sampling of the source texture; `bBicubic` wins | 0 |
| `bUseGL110` | OpenGL shader version 1.1 with fixed pipeline | 0 |
| `bPartialInput` | your source texture contains only `optimalRect` | 0 |
| `optimalRes`, `optimalRect` | `[w,h]`, `[l,t,r,b]`; calculated by `bAutoView` | `[0,0]`, `[0,0,0,0]` |
| `bFixWraparound` | one coherent content space on wrap-arounds (360 degree panoramas) | 0 |
| `gamma` | gamma correction on top of the gamma that is part of the blend map | 1 |
| `inputGamma` | gamma for the black level compensation | 2.2 |
| `blackScale`, `blackDarkAdjust`, `blackBrightAdjust` | black level adjustments; 0.5 keeps the value from the vwf | 1, 0.5, 0.5 |
| `mode` | preset for `splice`: 1 is `0x122`, 2 is `0x133`, other values 0. Without `mode`, `splice` is used. | not set |
| `splice` | swivel and negate eye input, bit field, see `VWBTypes.h` | 0 |
| `mouseMode` | Windows: 1 draw the mouse cursor, 2 hide the system cursor over the window | 0 |
| `overrideStatemask` | replaces the `stateMask` of `VWB_render`; DX11 only | 0 |
| `D3D12RTVF` | DXGI format of the render target for D3D12 | 28 (`R8G8B8A8_UNORM`) |
| `eyePointProvider`, `eyePointProviderParam` | library and parameters for an eye point provider (`.dll` or `.so` is appended) | none |
| `port`, `heartBeatPort`, `addr` | network control port, UDP heartbeat port, address to listen on | 0, 0, `0.0.0.0` |
| `debugBreak` | Windows: show a message box at start, to attach a debugger | 0 |

### Degenerate base

If an .ini file has neither `base` nor `trans`, the matrix is all zero (the reader overwrites it with zeros). The inverse of
that is NaN, which made 3D mappings unusable; 2D mappings only noticed "left-handed". `VWB_Init` now replaces a degenerate
matrix with `[1,0,0,0; 0,1,0,0; 0,0,-1,0; 0,0,0,1]` (left-handed, as 2D mappings had before), writes a warning to the log
at every log level and **overwrites the public field `VWB_Warper::trans`**. Set `base` to avoid this.
