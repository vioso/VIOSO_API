# Eye point provider (EyePointProvider plugin)

For dynamic eye warping (3D mappings) the library can take the eye position and viewing direction from a plugin instead of from
the arguments of `VWB_getViewProj`, `VWB_render` and friends. `EyePointProvider` (built with the examples) is the sample plugin:
it receives the eye point as UDP datagrams, or produces a test signal. The interface is `Include/EyePointProvider.h`
(`CreateEyePointReceiver`, `ReceiveEyePoint`, `DeleteEyePointReceiver`); you can write your own plugin against it.

## Use it

In the .ini file:

```
eyePointProvider=EyePointProvider64
eyePointProviderParam=listen 1919
```
`.dll` (Windows) or `.so` (Linux) is appended to the name, a relative path is resolved against the folder of the VIOSOWarpBlend
library on Windows and against the working directory on Linux. If the key is missing nothing is loaded and the eye comes from your calls. With a provider, `eye` and `rot` that you
pass in are ignored.

| `eyePointProviderParam` | Meaning |
|---|---|
| `sinewave` | test signal, no network (see below) |
| `listen <port> [<ip>]` | receive UDP datagrams on `<port>`. Without `<ip>` (or with `0.0.0.0`) on all interfaces. |

**Port.** The plugin's default is **1919** if `listen` is given without a port (or with 0). Older sample .ini files and the former
TrackerSim tool used **1998**, and that is what they wrote: `listen 1998`. There is no automatic switch, give the port you send to.

**`<ip>` limits what is received.** The plugin binds to that address only. A sender that broadcasts (TrackerSim did, to the broadcast
address) is not received if you give a specific interface address here, so leave `<ip>` out.

The listener thread is started once; further warpers that use the plugin in the same process read the same values.

## Datagram format

One datagram is one text line, six numbers separated by white space, in this order. A point is the decimal separator.
Anything after the sixth number is ignored, a datagram with fewer than six numbers is dropped (the last good values stay).
Keep it below 1024 bytes.

| # | Field | Unit | Meaning |
|---|---|---|---|
| 1 | x | millimeters | eye position, first axis of your eye point (as `pEye[0]`) |
| 2 | y | millimeters | eye position, second axis (`pEye[1]`) |
| 3 | z | millimeters | eye position, third axis (`pEye[2]`) |
| 4 | roll | degrees | rotation around the viewing direction; positive is clockwise |
| 5 | pitch | degrees | rotation around the horizontal axis; positive looks up |
| 6 | yaw | degrees | rotation around the vertical axis; positive looks to the right |

Example: `1000 2000 3000 10 20 30` is x = 1 m, y = 2 m, z = 3 m, roll 10, pitch 20, yaw 30 degrees.

The plugin divides the position by 1000 and converts the angles to radians; the library then uses them like the `pEye` (x, y, z) and
`pRot` (pitch, yaw, roll) arguments of `VWB_getViewProj`, see the comment there for the rotation order (y, x, z) and the signs. The plugin does not change any sign.
The library takes the values as they come (no rescaling in between), so the positions arrive in meters.

The datagram above, sent to a `listen` receiver, returns x, y, z = 1, 2, 3 and
roll, pitch, yaw = 0.1745, 0.3491, 0.5236 (radians).

**Note on names.** The field names in the old TrackerSim dialog (`x/y/z dir` for fields 4 to 6) differ from the plugin's roll, pitch, yaw. The
plugin's reading is the table above. Axes and signs of the rotations are those of the API, not of your tracker: convert on the sending side.

## `sinewave`

`sinewave` sweeps one value after the other, each one a full sine period: x, y, z (amplitude 1, i.e. one meter), then yaw, pitch and roll
(amplitude 1 radian), in this order, after a phase at zero. The time base mixes milliseconds and microseconds, so the phases pass in a
fraction of a second (observed: all six fields moved within a second) instead of the five seconds the constant says; it is a signal to see that the chain works, not a timing reference.

## Sending from your own tool

Any tool that sends the text line above to the port works, e.g. on Windows PowerShell:

```
$u = New-Object System.Net.Sockets.UdpClient
$b = [Text.Encoding]::ASCII.GetBytes("0 0 0 0 0 15")
$u.Send($b, $b.Length, "127.0.0.1", 1919)
```
The former TrackerSim (an MFC dialog that sent a sliding eye point as broadcast, default port 1998) has been removed; this
description replaces it.
