# VIOSO WarpBlend API

This library is meant to be used in image generators, to do warping and blending. It takes a VIOSO Warp File (.vwf) export and a texture buffer 
to sample from. If no texture buffer is given, it uses a copy of the current back buffer. It will render to the currently set back buffer.
It provides image based warping, suitable for most cases and, if a 3D map is provided, dynamc eye warping.

- You need a vwf mapping to use. This the native VIOSO Warp Format for calibration files. Please download a sample file from here:
https://vioso-my.sharepoint.com/:f:/p/jerbi_ahmed/IgCYy0VoCTVzS53ECvtF8bvwAec5J1zaHThyrX7Npy1hT8k?e=h2gba5
Or generate an export from our Calibration software:
https://vioso.com/calibration-software/vioso6

- You need a configuration (VIOSOWarpBlend.ini) to initialize the plugin. Reference and example here: https://docs.vioso.com/viosowarpblend.ini-reference
  

## Usage 

### 1 Static binding
having VIOSOWarpBlend next to your executable or in added to %path%,
link against VIOSOWarpBlend.lib and, in your header.
```
#include "VIOSOWarpBlend.h"
```

### 2 Dynamic binding  

### 2a use wrapper from VIOSOWarpBlend
declaration:

```
#include "../../Include/VIOSOWarpBlend.hpp"
const char* s_configFile = "VIOSOWarpBlendGL.ini";
std::shared_ptr<VWB> pWarper;
```

initialization: (where channel is a string containing the channel name)
```
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
```	
float view[16], proj[16];
	float eye[3] = { 0,0,0 };
	float rot[3] = { 0,0,0 };
 
	pWarper->GetViewProj( eye, rot, view, proj ); // call some of the get frustum functions there are others serving clip coordinates or angles
```

render:
    render your scene into FBO / Offscreen RT, attached to texUnwarped
    
post-render:
```	
pWarper->Render( texUnwarped, VWB_STATEMASK_PIXEL_SHADER | VWB_STATEMASK_SHADER_RESOURCE );
```


### 2b via [precompiled] header:

in header declare functions and types:
```
#define VIOSOWARPBLEND_DYNAMIC_DEFINE
#include "VIOSOWarpBlend.h"
```
in one file on top, to implement the actual functions/objects
```
#define VIOSOWARPBLEND_DYNAMIC_IMPLEMENT
#include "VIOSOWarpBlend.h"
```
in module initialization, this loads function pointers from library
```
#define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
#include "VIOSOWarpBlend.h"
```
### 2c Single file:

in file on top, to declare and implement functions/objects,
```
#define VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT
#include "VIOSOWarpBlend.h"
```
in module initialization, this loads function pointers from library
```
#define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
#include "VIOSOWarpBlend.h"
```
Always make sure to have your platform headers loaded before!

## Build on Windows
Clone the repositiory, use MS Visual Studio(R) and include VIOSOWarpBlend/VIOSOWarpBlend.vcxproj project file to your solution. Compile along with your projects.

## Build on Linux:
There is a separate linux branch which is work in progress. Master branch is constantly converging, worth a try.
Check it out via the command 
 ```git clone -b linux_test https://bitbucket.org/VIOSO/VIOSO_api.git```
 ```git clone -b master https://bitbucket.org/VIOSO/VIOSO_api.git```
(be sure to have installed git lfs or do ```sudo apt-get install git-lfs```  if you get an error related to extracting vioso2d.zip or vioso3D.zip you may have force a git lfs checkout: ```git lfs pull```)

Build it (as a shared library) using cmake: in the root folder execute the follwing command (having cmake installed - sudo apt-get install cmake):
```mkdir build && cd build && cmake .. && make```
and
```make install``` 
to make it available system wide.

Link it the usual way:  
```g++  ... -L/usr/local/lib -lVIOSOWarpBlend``` 
(adjust -L/usr/local/lib to the place you installed it to)

In order to build the example project you'll need glfw + dependencies. use this command: ```sudo apt-get install libglfw3-dev libxcursor-dev libxi-dev libxinerama-dev freeglut3-dev```

Use dynamic load or dynamic linked lirary like this:
```
#define VIOSOWARPBLEND_DYNAMIC_IMPLEMENT
#include "../../Include/VIOSOWarpBlend.h"
LPCTSTR s_configFile = _T( "VIOSOWarpBlend.ini" );
LPCTSTR s_channel = _T( "IG1" );
VWB_Warper* g_warper = nullptr;

void Init()
{
    #define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
	#include "../../Include/VIOSOWarpBlend.h"

    if(
        nullptr == VWB_Create ||
        nullptr == VWB_Init ||
        VWB_ERROR_NONE != VWB_Create( device, s_configFile, s_channel, &g_warper, 0, NULL ) ||
        VWB_ERROR_NONE != VWB_Init( g_warper )
        )
        throw std::runtime_error( "Failed to initialize VIOSO Warper" );
}

void Destroy()
{
    if( nullptr != VWB_Destroy && )
        VWB_Destroy( g_warper );
}

void PreRender( float* trackerPos, float* trackerDir, float* mView, float* mProjection )
{
    // NOTE: VWB_getViewClip or VWB_getPosDirClip can also be used to request optimal frustum
    //       This step is mandatory to update the warper's data
    if( nullptr != VWB_getViewProj )
       VWB_getViewProj( m_warper, trackerPos, trackerDir, mView, mProjection );
}

void PostRender( TexHandle inTex )
{
    if( nullptr != VWB_render )
        VWB_render( m_warper, (VWB_param)inTex, VWB_STATEMASK_STANDARD )
}
```


For configuring and usage see [https://helpdesk.vioso.com/documentation/api/](https://docs.vioso.com/vioso-warpblend-api)
