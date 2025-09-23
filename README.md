# VIOSO WarpBlend API

This library is meant to be used in image generators, to do warping and blending. It takes a VIOSO Warp File (.vwf) export and a texture buffer 
to sample from. If no texture buffer is given, it uses a copy of the current back buffer. It will render to the currently set back buffer.
It provides image based warping, suitable for most cases and, if a 3D map is provided, dynamc eye warping.

You need a vwf mapping to use. Please download a mapping package here:

A simple distortion map
https://vioso.sharepoint.com/:u:/g/EaWsoifHb-1JgadipyWWOIgB9bP3OwNHKRFluPWAaXlvvw?e=4BJQki

cylinder 5x1 landscape
https://vioso.sharepoint.com/:u:/g/EYC0Ko4UxHZPiZh43h5QNR4BgjN1EoV94UwPEW1M4xZbAw?e=8yVmc3

curved 2x1 landscape
https://vioso.sharepoint.com/:u:/g/EdZipE3afB5Io1kahdrrSbQBukJ8c2SUd6IFfmacX2_RPA?e=nbXGYV

panadome 3x1 portrait
https://vioso.sharepoint.com/:u:/g/Eeev8Gzt3EBNoXb-z_g3kscBHoF57Hzk_UbxHg9g4rg3Vg?e=Of4Z7b

or create your own by using VIOSO Integrate plus as trial, download here:
https://vioso.com/download/vioso-integrate-plus

## Dependencies

We use STB and TinyXML2 in VIOSOWarpBlend.
Additionally we need ZLIB, GLM and GLFW in the examples. On systems other than Windows, we need pkg-config, to find tinyxml2.
Make sure, you have a c\+\+20 and a c17 compiler handy, like Visual Studio 2022 or gcc/g\+\+ 13

### vcpkg (Windows)

``` cmd
vcpkg install stb
vcpkg install tinyxml2
vcpkg install zlib
vcpkg install GLM
vcpkg install glfw3
```

### apt (Ubuntu/Debian))

``` bash
sudo apt install libstb-dev 
sudo apt install libtinyxml2-dev
sudo apt install zlib1g-dev
sudo apt install libglm-dev
sudo apt install libglfw3-dev
sudo apt install pkg-config
```

### brew (macOS)


``` bash
brew install stb
brew install tinyxml2
brew install zlib
brew install glm
brew install glfw
brew install pkg-config
```

## Usage 

### 1 Dynamic binding  (preferred)

Use this if you want to avoid linking against the VIOSOWarpBlend library. The library will be loaded at runtime and does version checks for compatibility.
You can catch and evaluate errors to disable warping if needed.

declaration:

``` c++
#include "../../Include/VIOSOWarpBlend.hpp"
const char* s_configFile = "VIOSOWarpBlendGL.ini";
std::shared_ptr<VWB> pWarper;
```

initialization: (where channel is a string containing the channel name)
``` c++
try {
	pWarper = std::make_shared<VWB>( "", nullptr, s_configFile, channel.c_str(), 1, "" );
}
catch( VWB_ERROR )
{
	return FALSE;
}
if( VWB_ERROR_NONE != pWarper->Init() )
	return FALSE;
```

pre-render:
```	c++
float view[16], proj[16];
	float eye[3] = { 0,0,0 };
	float rot[3] = { 0,0,0 };
 
	pWarper->GetViewProj( eye, rot, view, proj ); // call some of the get frustum functions there are others serving clip coordinates or angles
```

render:
    render your scene into FBO / Offscreen RT, attached to texUnwarped
    
post-render:
```	c++
pWarper->Render( texUnwarped, VWB_STATEMASK_PIXEL_SHADER | VWB_STATEMASK_SHADER_RESOURCE );
```

### 2 Static binding

Put VIOSOWarpBlend next to your executable or in add to %path%,
then link against VIOSOWarpBlend.lib and, in your header. Use the interface C-Style:

``` c++
#include "VIOSOWarpBlend.h"
VWB_Warper* warper = nullptr;
```
initialization: (where channel is a string containing the channel name)
```
	if( VWB_ERROR_NONE != VWB_Create( pD3DDevice, configFile, channel, &warper, 0, NULL ) ||
	    VWB_ERROR_NONE != VWB_Init( warper ) )
			return -1;
```

render loop
``` c++
    D3DXMATRIX proj;
    D3DXMATRIX view;
	VWB_getViewProj( warper, NULL, NULL, (VWB_float*)&view, (VWB_float*)&proj );

    HRESULT res;
    LPDIRECT3DDEVICE9 d3d = renderer->getDevice();

    res = pD3DDevice->BeginScene();

    // draw

	VWB_render( warper, renderer->getOutput(), VWB_STATEMASK_ALL );

    res = d3d->EndScene();

	// present
```

uninitialization:100:
``` c++
	if( warper )
		VWB_Destroy( warper );
```

## Build

Open the cloned directory in your CMake compatible IDE and build + install.
Use the build-tree in your install folder to compile and test the examples.

## Help

For configuring and usage see https://helpdesk.vioso.com/documentation/api/
