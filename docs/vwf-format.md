# The .vwf file (VIOSO warp file)

A .vwf file holds the calibration of one or more outputs ("channels"): where each pixel of an output looks into the content (**warp**), how the images of overlapping projectors are blended (**blend**), and optionally a
black level correction. This page tells what an integrator needs to know and describes what the code of version 1.7 does (`LoadVWF` in [`VIOSOWarpBlend/VWF.cpp`](https://github.com/vioso/VIOSO_API/blob/v1.7.0/VIOSOWarpBlend/VWF.cpp), the structures in `Include/VWBTypes.h`, the writing rules in
[`VIOSOWarpBlend/VWF.h`](https://github.com/vioso/VIOSO_API/blob/v1.7.0/VIOSOWarpBlend/VWF.h)). Statements about the exports concern the files of the VIOSO calibration software.

## Layout and channels

```
set header (16 bytes: "vwf1", number of blocks, offset of the first block = 16, reserved = 0)
then, for every channel, in this order:
    warp chunk    VWB_WarpFileHeader5 ("vwf0") followed by width x height VWB_WarpRecord
    blend chunk   a Windows bitmap, may be 0 x 0 for "no blend map"
    black chunk   a Windows bitmap, optional (black level correction)
    white chunk   a Windows bitmap, ambient correction, read, not used
```

The loader also reads the warp header magic numbers `vwf2` and `vwf3`. A warp chunk is recognised by its magic number, a bitmap by `BM`.

- **One output of your host is one channel.** A file can hold many; `calibFile` can name several files, separated by `,`. `calibIndex` is the position in the list of all channels of all files, counted from 0
  (`ini-reference.md`). With `calibIndex >= 0` only the warp and blend data of that channel are loaded; the headers of all channels are always read.
- The header of a channel has, among others: `name`, `hostname` (the computer that drives the output), `offsetX`/`offsetY` (the position of the display on the desktop at export), `width` and `height` (the number of warp
  records, equal to the resolution of the output), `compoundID` (outputs with the same `compoundID` share one content space) and the content fields below.
  All of them are in `VWB_WarpFileHeader5` in `Include/VWBTypes.h`; fields that an exporter does not fill are 0.

## The warp: uv over the whole compound

The warp holds `width x height` records of four floats; record `(x, y)` is the position, where the center of the output pixel at column `x`, row `y` looks into the content.

- **2D map:** `(u, v, 1, 0)`. `u` and `v` are normalised to the **whole content space** of the compound (all channels that belong together), **not** to the part of this channel. `z = 1` marks a valid pixel, 0 an invalid one
  (the VIOSO exports write all four components as 0 for invalid pixels).
- **3D map** (flag `3D` in the header): `(X, Y, Z, w)`, world coordinates of the point the pixel looks at, in the calibration's units, `w = 1` for a valid pixel. The 3D code is unchanged in 1.7, see "Known limits" in the integration guide.

## Content size and part: `optimalRes`, `optimalRect`, `bPartialInput`

- `vCntDispPx[4]`, `vCntDispPx[5]` hold the **normalised content extent of one display pixel**, in x and y. `1 / value` is the content size in pixels at which one display pixel covers one content pixel. The library
  cuts it off to a whole number and gives it to you as `optimalRes`. A value that is only float noise below a whole number counts as that number (up to 1.6 a 1920 pixel wide channel gave 1919; see the changelog).
  Before 1.7 the comment in the header called the field a "ratio", which is not what the code does.
- `vPartialCnt[0..3]` is the part of the content space that this channel uses, normalised: left, top, right, bottom, as the positions of **pixel centers**. The library derives `optimalRect` from it: the content pixels that the channel
  samples, `left` and `top` inclusive, `right` and `bottom` exclusive, rounded outwards with a tolerance of 0.001 pixel against float noise (since 1.7).
- With **`bPartialInput`** your source texture holds exactly `optimalRect` of the content space, texel (0,0) is content pixel `(left, top)`. Use it when the content space does not fit into one texture or when the host sends only a
  part to each output (integration guide, "2D: content space, `optimalRes`, `bPartialInput`").

**Check the files you get.** In the VIOSO exports, `vCntDispPx` can differ from channel to channel of one compound. Then `optimalRes` differs per channel, and a content texture that is shared by all channels
and sized by one of them does not fit the others. This is a property of these exports, not of the format. Look at the content sizes of all channels (below) before you use `bPartialInput` with a shared texture.

## Precision

- Up to 1.6 the Direct3D warp texture of a 2D map was `R16G16_UNORM` (16 bit normalised). Because the records are normalised to the whole content space, one 16 bit step is `content width / 65535` pixels: 2.34 pixels at a content
  space of 153600 pixels, which showed as steps in the image. OpenGL has always used float.
- Since 1.7 it is `R32G32_FLOAT` in Direct3D 10, 11 and 12 (Direct3D 10 has the same change, see "Known limits" in the integration guide), still clamped to 0 to 1. Measured with [`tools/WarpPrecision`](https://github.com/vioso/VIOSO_API/blob/v1.7.0/tools/WarpPrecision.cpp) (one channel of 3840 x 2160 in a content space of
  153600 x 2160, `bPartialInput`): Direct3D 11, 12 and OpenGL are identical now, the largest error is 0.0039 pixel. The warp texture needs twice the memory (a 4K channel about 66 instead of about 33 MB).
- The 3D warp has always been float.
- At the last pixel of an output, which samples on the edge of the last source texel, the GPU mixes in the border color (black) with a weight of 1/256: an image value there is 0.4 % too dark. That is no error of the mapping.

## Blend and black level

- The **blend map** is a Windows bitmap of the same size as the warp (0 x 0 means "no blend map"). 24 or 32 bit per pixel is 8 bit per channel, 48 or 64 bit is 16 bit, 96 or 128 bit is float. Red, green and blue are the factor, the alpha is
  a mask that is multiplied to all. For the GPU the map is converted to 16 bit normalised.
- The blend maps of the exporters contain the gamma correction of the projector; the library multiplies them as they are. The `gamma` setting of the .ini file is an additional correction on top (`integration-guide.md`, "Colour and gamma").
- The optional **black level map** is a second bitmap (RGB, 8 bit) with the parameters `blackScale`, `blackDark` and `blackBright` in the header. The correction is done in the shader; `bDoNoBlack` switches it off.
  The formulas are in [`VIOSOWarpBlend/VWF.h`](https://github.com/vioso/VIOSO_API/blob/v1.7.0/VIOSOWarpBlend/VWF.h).

## Looking into a file

- `MediaServerExampleDX11.exe file.vwf --list` lists the channels: number, name, host, position, size and content size (`docs/examples.md`).
- `VWB_vwfInfo` / `VWB_vwfInfoC` give your application the headers of all channels (integration guide, "Choose the channel").
- `MakeTestVwf info file.vwf` prints the headers in full. It is in `bin/` of the release archive (`docs/examples.md`). `MakeTestVwf make` writes synthetic 2D files through the library's own `SaveVWF`, also
  files in which one channel describes a small part of a very wide content space.

## What is not described here

The proprietary parts of the exporters (how the maps are made, the XML in `ident`, encrypted files), the black level and blend data layout beyond the structures named above, and everything about 3D maps beyond the flag and the record layout.
