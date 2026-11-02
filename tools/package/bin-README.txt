Quickest start: run VWFTesterDX11.exe and choose 2D_plane_FHD.vwf in this folder (or: VWFTesterDX11.exe 2D_plane_FHD.vwf). Esc ends it.
D3D12Example.exe and OpenGLExample.exe start without arguments; MediaServerExampleDX11.exe 2D_plane_FHD.vwf shows a coordinate grid.
Arguments, keys, what you should see: ..\docs\examples.md. Building the API into your program: ..\docs\integration-guide.md.
earth.png is a test picture (PNG, 2048 x 1025): VWFTesterDX11.exe 2D_plane_FHD.vwf -p earth.png, D3D12Example.exe -image earth.png. Show the same picture in your program to compare it with the tester.
view.export.template.viosoAPI.txt is an export template of the VIOSO Calibrator that writes an API .ini file (see ..\docs\ini-reference.md).
Relative paths (.ini, .vwf, log) are resolved against the folder of VIOSOWarpBlend64.dll, which is this one.
