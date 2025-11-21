// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#define SAFE_RELEASE( x ) if( x ){ x->Release(); x = NULL; }

#if !defined( VWB_common_h )
#define VWB_common_h

// C RunTime Header Files
#include <memory.h>
#include <stdio.h>

#include "../Include/VIOSOWarpBlend.h"
#include "logging.h"
#include "resource.h"
#include "StringConversions.h"
#define STRINGIFY(s) #s
#define STRVER( ma, mi, x ) STRINGIFY(ma.mi.x)
#define VWB_VERSTR STRVER(VWB_Version_MAJ,VWB_Version_MIN,VWB_Version_MAI)
#include "mmath.h"

#include "../Include/EyePointProvider.h"
#include "licCM.h"

#if defined( WIN32 )
extern VWB_int g_error;
extern bool g_bCurEnabled;
extern HCURSOR g_hCur;
extern SIZE	g_dimCur;
extern POINT g_hotCur;
typedef int (WINAPI *FPtrInt_BOOL)(BOOL bShow);
extern FPtrInt_BOOL ShowSystemCursor;

size_t copyCursorBitmapToMappedTexture( HBITMAP hbmMask, HBITMAP hbmColor, BITMAP& bmMask, BITMAP& bmColor, void* data, int pitch );
#endif

#ifndef MIN
	#define MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
	#define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

#if defined( __GNU__ )
  void __attribute__ ((constructor)) my_init(void);
  void __attribute__ ((destructor)) my_fini(void);
#endif

#endif //!defined( VWB_common_h )

