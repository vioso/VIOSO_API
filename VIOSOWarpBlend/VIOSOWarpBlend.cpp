// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "common.h"
#include "WarperBase.h"
#include "PathHelper.h"

#include <locale.h>
#include <time.h>
#include <limits.h>
#include <float.h>
#include <list>
#include <fstream>
#include <atomic>
#include <cfloat>
#include <span>
#include <chrono>
#include <format>
#include <numeric>
#include <array>
#include <span>
#include <sstream>

#define _USE_MATH_DEFINES
#include <math.h>
#include <limits.h>

#ifdef WIN32
// windows specific includes
#include <crtdbg.h>
#include <shlwapi.h>
#pragma comment( lib, "shlwapi" )
// DX warper includes
#include "DX/DX9WarpBlend.h"
#include "DX/DX9EXWarpBlend.h"
#include "DX/DX11WarpBlend.h"
#include "DX/DX10WarpBlend.h"
#include "GL/GLWarpBlendXPL.h"
#ifndef VWB_WIN7_COMPAT
#include "DX/DX12WarpBlend.h"
#endif //ndef VWB_WIN7COMPAT
#else // def WIN32
// X11 includes
#include <dlfcn.h>
#endif // def WIN32

// the other warper includes
#include "DummyWarper.h"
#include "GL/GLWarpBlend.h"

// some global includes
#include "VWF.h"
#include "dpXML.h"

using namespace std;
using namespace VWBUtil;
namespace fs = std::filesystem;

uint8_t const* g_cryptoKey = nullptr;

#ifdef WIN32
HMODULE g_hModDll = 0;
HCURSOR g_hCur = 0;
SIZE	g_dimCur = { 0 };
POINT   g_hotCur = { 0 };
bool    g_bCurEnabled = true;
FPtrInt_BOOL ShowSystemCursor = NULL;
#endif

/////////////////////////////////////////////////////////////////////////////////////
// internal create function
VWB_ERROR create( void* pDxDevice, char const* szChannelName, VWB_Warper** ppWarper ) {
//#ifdef WIN32
//	MessageBoxA( NULL, "BREAK", "DEBUG", MB_OK );
//#else
//#endif
	try {
		if( VWB_DUMMYDEVICE == pDxDevice ) {
			*ppWarper = new Dummywarper();
		} else if( pDxDevice ) {
			IUnknown* pUK = (IUnknown*)pDxDevice;
			if( pUK ) {
				IUnknown* pUK2 = NULL;
				// destinguish between DX flavours
				do {
				#ifdef WIN32
					if( SUCCEEDED( pUK->QueryInterface( __uuidof( IXPlaneRef ), (void**)&pUK2 ) ) ) {
						pUK2->Release();
						if( pUK == pUK2 ) {
							*ppWarper = new GLWarpBlendXPL( (IXPlaneRef*)pDxDevice );
							break;
						}
					}

				#ifndef VWB_WIN7_COMPAT
					if( SUCCEEDED( pUK->QueryInterface( __uuidof( ID3D12CommandQueue ), (void**)&pUK2 ) ) ) {
						pUK2->Release();
						if( pUK == pUK2 ) {
							*ppWarper = new DX12WarpBlend( (ID3D12CommandQueue*)pDxDevice );
							break;
						}
					}
				#endif //ndef VWB_WIN7_COMPAT

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( ID3D11Device ), (void**)&pUK2 ) ) ) {
						pUK2->Release();
						if( pUK == pUK2 ) {
							*ppWarper = new DX11WarpBlend( (ID3D11Device*)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( ID3D10Device1 ), (void**)&pUK2 ) ) ) {
						pUK2->Release();
						if( pUK == pUK2 ) {
							*ppWarper = new DX10WarpBlend( (ID3D10Device*)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( ID3D10Device ), (void**)&pUK2 ) ) ) {
						pUK2->Release();
						if( pUK == pUK2 ) {
							*ppWarper = new DX10WarpBlend( (ID3D10Device*)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( IDirect3DDevice9Ex ), (void**)&pUK2 ) ) ) {
						pUK2->Release();
						if( pUK == pUK2 ) {
							*ppWarper = new DX9EXWarpBlend( (LPDIRECT3DDEVICE9EX)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( IDirect3DDevice9 ), (void**)&pUK2 ) ) ) {
						pUK2->Release();
						if( pUK == pUK2 ) {
							*ppWarper = new DX9WarpBlend( (LPDIRECT3DDEVICE9)pDxDevice );
							break;
						}
					}

				#endif //def WIN32
				} while( 0 );
			}
		} else {
			*ppWarper = new GLWarpBlend();
		}
		if( !*ppWarper )
			throw (VWB_int)VWB_ERROR_GENERIC;
	} catch( VWB_int e ) {
		logStr( 0, "FATAL: Error %d creating warper \"%s\".\n", e, szChannelName ? szChannelName : ( *ppWarper )->channel );
		return (VWB_ERROR)e;
	}
	return VWB_ERROR_NONE;
}

void report( VWB_Warper** ppWarper, bool withHeader = true, char const* message = "Parameters" ) {
	if( withHeader ) {
		auto t = std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>( std::chrono::system_clock::now() ) };
		logStr( 1, "%04d/%02d/%02d VIOSOWarpBlend API %d.%d.%d.%d.\n", int( t.year() ), unsigned( t.month() ), unsigned( t.day() ), VWB_Version_MAJ, VWB_Version_MIN, VWB_Version_MAI, VWB_Version_REV );
	#ifdef _DEBUG
		logStr( 1, "DEBUG" );
	#endif

		filesystem::path modPath;
		filesystem::path procPath;
	#ifdef WIN32
		wchar_t wszProcPath[MAX_PATH]{ 0 };
		if( ::GetModuleFileNameW( 0, wszProcPath, MAX_PATH ) )
			procPath = wszProcPath;
		wchar_t wszModPath[MAX_PATH]{ 0 };
		if( ::GetModuleFileNameW( g_hModDll, wszModPath, MAX_PATH ) )
			modPath = wszModPath;
	#else
		Dl_info dl_info{};
		auto res = dladdr( (void*)VWB_CreateA, &dl_info );
		if( res )
			modPath = dl_info.dli_fname;
		char path[PATH_MAX];
		ssize_t len = readlink( "/proc/self/exe", path, sizeof( path ) - 1 );
		if( len != -1 ) {
			path[len] = '\0';
			procPath = path;
		}

	#endif //def WIN32
		if( !modPath.empty() ) {
			if( ( (VWB_Warper_base*)( *ppWarper ) )->isUTF8() )
				logStr( 1, "lib path: \"%s\".", modPath.u8string().c_str() );
			else
				logStr( 1, "lib path: \"%s\".", modPath.string().c_str() );
		}
		if( !procPath.empty() ) {
			if( ( (VWB_Warper_base*)( *ppWarper ) )->isUTF8() )
				logStr( 1, "process path: \"%s\".", procPath.u8string().c_str() );
			else
				logStr( 1, "process path: \"%s\".", procPath.string().c_str() );
		}

		if( ( (VWB_Warper_base*)( *ppWarper ) )->isDP() )
			logStr( 1, "Config mode XML." );
		else
			logStr( 1, "Config mode VWF." );

		if( ( (VWB_Warper_base*)( *ppWarper ) )->isUTF8() )
			logStr( 1, "string mode: UTF-8." );
		else
			logStr( 1, "string mode: ASCII." );

		logStr( 1, "LogLevel=%d.", g_logLevel );
	}
	logStr( 2,
			"%.4s-Warper \"%s\":\n%s:\n"
			"calibFile=%s\n"
			"calibIndex=%d\n"
			"calibSplit=[%d,%d,%d,%d]\n"
			"bTurnWithView=%d\n"
			"bDoNotBlend=%d\n"
			"eyePointProvider=%s\n"
			"eyePointProviderParam=%s\n"
			"eye=[%.5f, %.5f, %.5f]\n"
			"near=%.5f\n"
			"far=%.5f\n"
			"bBicubic=%d\n"
			"bUseGL110=%d\n"
			"bPartialInput=%d\n"
			"splice=%u\n"
			"trans=[%.5f, %.5f, %.5f, %.5f; %.5f, %.5f, %.5f, %.5f; %.5f, %.5f, %.5f, %.5f; %.5f, %.5f, %.5f, %.5f]\n"
			"autoViewC=%.5f\n"
			"bAutoView=%d\n"
			"dir=[%.5f, %.5f, %.5f]\n"
			"fov=[%.5f, %.5f, %.5f, %.5f]\n"
			"screen=%.5f\n"
			"optimalRes=[%d, %d]\n"
			"optimalRect=[%d, %d, %d, %d]\n"
			"gamma=%.5f\n"
			"port=%d\n"
			"heartbeatPort=%d\n"
			"address=%s\n"
			"mouseMode=%d\n"
			"bDoNoBlack=%d\n"
			"overrideStatemask=%d\n"
			"bFixWraparound=%d\n"
			"D3D12RTVF=%d\n"
			"blackScale=%f\n"
			"inputGamma=%f\n"
			"blackDarkAdjust=%f\n"
			"blackBrightAdjust=%f\n"
			"outputGamma=%f\n"
			"bFlipWarpmeshTexcoords=%d\n"
			"bFlipWarpmeshVertices=%d\n"
			"pluginId=%d\n"
			"hMonitor=%d\n"
			"\n",
			( (VWB_Warper_base*)*ppWarper )->GetType(), ( *ppWarper )->channel,
			message,
			( *ppWarper )->calibFile,
			( *ppWarper )->calibIndex,
			( *ppWarper )->calibSplit[0], ( *ppWarper )->calibSplit[1], ( *ppWarper )->calibSplit[2], ( *ppWarper )->calibSplit[3],
			( *ppWarper )->bTurnWithView ? 1 : 0,
			( *ppWarper )->bDoNotBlend ? 1 : 0,
			( *ppWarper )->eyeProvider,
			( *ppWarper )->eyeProviderParam,
			( *ppWarper )->eye[0], ( *ppWarper )->eye[1], ( *ppWarper )->eye[2],
			( *ppWarper )->nearDist,
			( *ppWarper )->farDist,
			( *ppWarper )->bBicubic ? 1 : 0,
			( *ppWarper )->bUseGL110 ? 1 : 0,
			( *ppWarper )->bPartialInput ? 1 : 0,
			( *ppWarper )->splice,
			( *ppWarper )->trans[0], ( *ppWarper )->trans[1], ( *ppWarper )->trans[2], ( *ppWarper )->trans[3],
			( *ppWarper )->trans[4], ( *ppWarper )->trans[5], ( *ppWarper )->trans[6], ( *ppWarper )->trans[7],
			( *ppWarper )->trans[8], ( *ppWarper )->trans[9], ( *ppWarper )->trans[10], ( *ppWarper )->trans[11],
			( *ppWarper )->trans[12], ( *ppWarper )->trans[13], ( *ppWarper )->trans[14], ( *ppWarper )->trans[15],
			( *ppWarper )->autoViewC,
			( *ppWarper )->bAutoView ? 1 : 0,
			( *ppWarper )->dir[0], ( *ppWarper )->dir[1], ( *ppWarper )->dir[2],
			( *ppWarper )->fov[0], ( *ppWarper )->fov[1], ( *ppWarper )->fov[2], ( *ppWarper )->fov[3],
			( *ppWarper )->screenDist,
			( *ppWarper )->optimalRes.cx, ( *ppWarper )->optimalRes.cy,
			( *ppWarper )->optimalRect.left, ( *ppWarper )->optimalRect.top, ( *ppWarper )->optimalRect.right, ( *ppWarper )->optimalRect.bottom,
			( *ppWarper )->gamma,
			( *ppWarper )->port,
			( *ppWarper )->heartBeatPort,
			( *ppWarper )->addr,
			( *ppWarper )->mouseMode,
			( *ppWarper )->bDoNoBlack,
			( *ppWarper )->overrideStatemask,
			( *ppWarper )->bFixWraparound,
			( *ppWarper )->D3D12RTVF,
			( *ppWarper )->blackScale,
			( *ppWarper )->inputGamma,
			( *ppWarper )->blackDarkAdjust,
			( *ppWarper )->blackBrightAdjust,
			( *ppWarper )->outputGamma,
			( *ppWarper )->bFlipWarpmeshTexcoords ? 1 : 0,
			( *ppWarper )->bFlipWarpmeshVertices ? 1 : 0,
			( *ppWarper )->pluginId,
			( *ppWarper )->hMonitor
	);
};

VWB_ERROR VWB_CreateU( void* pDxDevice, char8_t const* szConfigFile, char8_t const* szChannelName, VWB_Warper** ppWarper, VWB_int logLevel, char8_t const* szLogFile ) {
	if( NULL == ppWarper || !szChannelName || !szChannelName[0] )
		return VWB_ERROR_PARAMETER;

	*ppWarper = NULL;
	g_logLevel = logLevel;
	if( szLogFile && szLogFile[0] )
		g_logFilePath = szLogFile;
	else
		g_logFilePath = "VIOSOWarpBlend";

	g_logFilePath = MkPath( g_logFilePath, ".log" );

	auto res = create( pDxDevice, (char const*)szChannelName, ppWarper );
	if( VWB_ERROR_NONE != res )
		return res;

	( (VWB_Warper_base*)( *ppWarper ) )->setUTF8( true );

	std::string reportMsg;
	if( NULL != szConfigFile && 0 != szConfigFile[0] ) {
		if( VWB_ERROR_NONE != ( (VWB_Warper_base*)*ppWarper )->ReadConfigFile( szConfigFile, (char const*)szChannelName ) ) {
			logStr( 0, "FATAL: .ini file (%s) parsing error.\n", szConfigFile );
			delete ( (VWB_Warper_base*)*ppWarper );
			*ppWarper = NULL;
			return VWB_ERROR_INI_LOAD;
		}
		reportMsg = "Parameters from \"" + std::string( (char const*)szConfigFile ) + "\"";
	} else {
		if( NULL != szChannelName && 0 != szChannelName[0] )
			VWBUtil::copy( ( (VWB_Warper_base*)*ppWarper )->channel, (char const*)szChannelName );
		else
			VWBUtil::copy( ( (VWB_Warper_base*)*ppWarper )->channel, "default" );
		reportMsg = "Parameters (no .ini file set, using defaults)";
	}

	report( ppWarper, true, reportMsg.c_str() );

	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_CreateA( void* pDxDevice, char const* szConfigFile, char const* szChannelName, VWB_Warper** ppWarper, VWB_int logLevel, char const* szLogFile ) {
	if( NULL == ppWarper || !szChannelName || !szChannelName[0] )
		return VWB_ERROR_PARAMETER;

	*ppWarper = NULL;
	g_logLevel = logLevel;
	if( szLogFile && szLogFile[0] )
		g_logFilePath = szLogFile;
	else
		g_logFilePath = "VIOSOWarpBlend";

	if( g_logFilePath != "CON" && g_logFilePath != "NUL" && g_logFilePath != "PRN" && g_logFilePath != "stdout" && g_logFilePath != "stderr")
	g_logFilePath = MkPath( g_logFilePath, ".log" );

	auto res = create( pDxDevice, (char const*)szChannelName, ppWarper );
	if( VWB_ERROR_NONE != res )
		return res;

	std::string reportMsg;
	if( NULL != szConfigFile && 0 != szConfigFile[0] ) {
		if( VWB_ERROR_NONE != ( (VWB_Warper_base*)*ppWarper )->ReadConfigFile( szConfigFile, szChannelName ) ) {
			logStr( 0, "FATAL: config file (%s) parsing error.\n", szConfigFile );
			delete ( (VWB_Warper_base*)*ppWarper );
			*ppWarper = NULL;
			return VWB_ERROR_INI_LOAD;
		}
		reportMsg = "Parameters from \"" + std::string( szConfigFile ) + "\"";
	} else {
		if( NULL != szChannelName && 0 != szChannelName[0] )
			VWBUtil::copy( ( (VWB_Warper_base*)*ppWarper )->channel, szChannelName );
		else
			VWBUtil::copy( ( (VWB_Warper_base*)*ppWarper )->channel, "default" );
		reportMsg = "Parameters (no .ini file set, using defaults)";
	}

	report( ppWarper, true, reportMsg.c_str() );

	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_CreateW( void* pDxDevice, wchar_t const* szConfigFile, wchar_t const* szChannelName, VWB_Warper** ppWarper, VWB_int logLevel, wchar_t const* szLogFile ) {
	return VWB_CreateU( pDxDevice, std::filesystem::path( szConfigFile ? szConfigFile : L"" ).u8string().c_str(), VWBUtil::to_u8string( szChannelName ).c_str(), ppWarper, logLevel, std::filesystem::path( szLogFile ? szLogFile : L"" ).u8string().c_str() );
}

VWB_ERROR VWB_CreateA2( void* pDxDevice, int pluginId, char const* szConfigFile, char const* szChannelName, VWB_Warper** ppWarper, VWB_int logLevel, char const* szLogFile ) {
	auto ret = VWB_CreateA( pDxDevice, szConfigFile, szChannelName, ppWarper, logLevel, szLogFile );
	if( VWB_ERROR_NONE == ret && pluginId != 0  ) {
		( *ppWarper )->pluginId = pluginId;
	}
	return ret;
}
VWB_ERROR VWB_CreateW2( void* pDxDevice, int pluginId, wchar_t const* szConfigFile, wchar_t const* szChannelName, VWB_Warper** ppWarper, VWB_int logLevel, wchar_t const* szLogFile ) {
	auto ret = VWB_CreateW( pDxDevice, szConfigFile, szChannelName, ppWarper, logLevel, szLogFile );
	if( VWB_ERROR_NONE == ret && pluginId != 0  ) {
		( *ppWarper )->pluginId = pluginId;
	}
	return ret;
}
VWB_ERROR VWB_CreateU2( void* pDxDevice, int pluginId, char8_t const* szConfigFile, char8_t const* szChannelName, VWB_Warper** ppWarper, VWB_int logLevel, char8_t const* szLogFile ) {
	auto ret = VWB_CreateU( pDxDevice, szConfigFile, szChannelName, ppWarper, logLevel, szLogFile );
	if( VWB_ERROR_NONE == ret && pluginId != 0 ) {
		( *ppWarper )->pluginId = pluginId;
	}
	return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////

VWB_ERROR VWB_Init( VWB_Warper* pWarper ) {
	return VWB_InitExt( pWarper, nullptr );
}

VWB_ERROR VWB_InitExt( VWB_Warper* pWarper, VWB_WarpBlendSet* extSet ) {
	if( NULL == pWarper )
		return VWB_ERROR_PARAMETER;

	VWB_ERROR err = VWB_ERROR_NONE;

	if( nullptr != extSet ) {
		if( ( VWB_ERROR_NONE != err ) || !VerifySet( *extSet ) ) {
			logStr( 0, "ERROR: Failed to verify given set.\n" );
			return err;
		} else
			err = VWB_ERROR_NONE;

		// call virtual init method
		err = ((VWB_Warper_base*)pWarper)->Init( *extSet );
	} else {
		VWB_WarpBlendSet set;

		size_t sBegin = 0;

		fs::path path;
		if(( (VWB_Warper_base*)pWarper )->isUTF8() )
			path = (char8_t*)( (VWB_Warper_base*)pWarper )->calibFile;
		else
			path = ( (VWB_Warper_base*)pWarper )->calibFile;
		if( ( (VWB_Warper_base*)pWarper )->isDP() ) {
			if( path.empty() ) {
				logStr( 0, "ERROR: No mapping files in calibFile.\n" );
				return VWB_ERROR_PARAMETER;
			}
			// convert to absolute path
			auto path_views = split( path.native(), fs::path::string_type::value_type( ',' ) );
			std::vector<fs::path> paths( path_views.begin(), path_views.end() );
			err = LoadDPXML( set, paths, pWarper->bFlipWarpmeshVertices, pWarper->bFlipWarpmeshTexcoords );
			if( VWB_ERROR_NONE != err ) {
				logStr( 0, "ERROR: LoadDPXML: Failed to load mapping files.\n" );
				return err;
			}
			// update trans matrix, it scales Meter to Millimeter, swaps y and z axis, and moves to pos
			// todo: add a handedness hint to config.xml
			bool bLH = false;
			if( ( (VWB_Warper_base*)pWarper )->GetType()[0] == 'D' &&
				( (VWB_Warper_base*)pWarper )->GetType()[1] == 'X' )
				bLH = true;
			// HACK: we check on Windows for Unreal engine process, to make a dummy warper left-handed, this is ONLY used in Editor to create the screen previews
		#ifdef WIN32
			if( ( (VWB_Warper_base*)pWarper )->GetType()[0] == 'D' &&
				( (VWB_Warper_base*)pWarper )->GetType()[1] == 'U' ) {
				wchar_t procName[MAX_PATH]{ 0 };
				if( GetModuleFileNameW( NULL, procName, MAX_PATH ) ) {
					auto wsProcName = fs::path( procName ).filename().native();
					std::transform( wsProcName.begin(), wsProcName.end(), wsProcName.begin(), ::towlower );
					if( wsProcName.find( L"unrealeditor" ) != std::wstring::npos ) {
						bLH = true;
					}
				}
			}
		#endif
			if( bLH ) { // go left handed for Direct 3D
				VWB_MAT44f(
					 1000.f,    0.f,    0.f, -set.back()->header.pos[0],
						0.f,    0.f, 1000.f, -set.back()->header.pos[1],
						0.f, 1000.f,    0.f, set.back()->header.pos[2],
						0.f,    0.f,    0.f,                        1.f
					).SetPtr( pWarper->trans );
			} else { // go right handed otherwise
				VWB_MAT44f(
					 1000.f,    0.f,     0.f, -set.back()->header.pos[0],
						0.f,    0.f, -1000.f, -set.back()->header.pos[1],
						0.f, 1000.f,     0.f, -set.back()->header.pos[2],
						0.f,    0.f,     0.f,                        1.f
					).SetPtr( pWarper->trans );
			}
			// update fov and dir
			VWB_VEC3f( set.back()->header.dir ).SetPtr( pWarper->dir );
			VWB_VEC4f( set.back()->header.fov ).SetPtr( pWarper->fov );
			pWarper->screenDist = set.back()->header.screen;
			set.back()->header.hMonitor = pWarper->hMonitor; // retrofit the monitor handle, verifySet will check for validity
			if( std::numeric_limits<float>::max() == pWarper->screenDist ) { // no target or frustum loaded; we enable autoView
				pWarper->bAutoView = true;
				// and set a default screen distance for domeprojection setups
				pWarper->screenDist = 1000.f;
			}
		} else do {
			auto sEnd = path.native().find( ',', sBegin );
			std::filesystem::path pp;
			pp = MkPath( path.native().substr( sBegin, sEnd ), ".vwf", ((VWB_Warper_base*)pWarper)->getConfigPath() );

			err = LoadVWF( set, pp, false, pWarper->calibIndex, g_cryptoKey, ((VWB_Warper_base*)pWarper)->isUTF8() );
			set.back()->header.hMonitor = pWarper->hMonitor;

			if( sEnd == std::filesystem::path::string_type::npos )
				break;
			sBegin = sEnd + 1;
		} while( 1 );

		if( VWB_ERROR_NONE != err ) {
			logStr( 0, "ERROR: LoadVWF: Failed to load set.\n" );
			return err;
		}
		if( !VerifySet( set, pWarper->calibIndex ) ) {
			logStr( 0, "ERROR: LoadVWF: Failed to verify set.\n" );
			return VWB_ERROR_GENERIC;
		} else
			err = VWB_ERROR_NONE;

		// call virtual init method
		err = ((VWB_Warper_base*)pWarper)->Init( set );

		// clean up
		DeleteVWF( set );
	}

	if( VWB_ERROR_NONE != err ) {
		logStr( 0, "ERROR: Warper Init failed.\n" );
		return err;
	}
	( (VWB_Warper_base*)pWarper )->GetViewProjection( NULL, NULL, NULL, NULL );

	auto infoFlags = ( (VWB_Warper_base*)pWarper )->getInfoFlags();
	logStr( 2, "%.4s-Warper \"%s\" initialized.\n"
			" Eyepoint: %s\n Border: %s\n Handedness: %s\n Stringmode: %s\n Shadermode: %s\n"
			, ( (VWB_Warper_base*)pWarper )->GetType(), pWarper->channel, 
			( infoFlags& VWB_WARPER_INFO_FLAGS_DYNAMIC_EYEPOINT ) ? "DYNAMIC" : "STATIC",
			( infoFlags& VWB_WARPER_INFO_FLAGS_BORDER ) ? "ON" : "OFF",
			( infoFlags& VWB_WARPER_INFO_FLAGS_RIGHT_HANDED ) ? "RIGHT" : "LEFT",
		#if defined( WIN32 ) || defined( WIN64 )
			( infoFlags& VWB_WARPER_INFO_FLAGS_UTF8 ) ? "UTF-8" : "ANSI",
		#else
			"UTF-8",
		#endif
			( infoFlags & VWB_WARPER_INFO_FLAGS_DOMEPROJECTION ) ? "SHAPE" : "PIXELMAP"
	);
	report( &pWarper, false );
	return err;
}

void VWB_Destroy( VWB_Warper* pWarper ) {
	if( pWarper )
		delete (VWB_Warper_base*)pWarper;
}


VWB_ERROR VWB_getViewProj( VWB_Warper* pWarper, VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pProj ) {

	if( NULL != pEye && NULL != pRot ) {
		logStr( 4, "VWB_getViewProj\n IN:   eye: (%f, %f, %f)\n"
				"       rot: (%f, %f, %f)\n"
				, pEye[0], pEye[1], pEye[2]
				, pRot[0], pRot[1], pRot[2]
		);
	}
	if( pWarper ) {
		VWB_ERROR err = ( (VWB_Warper_base*)pWarper )->GetViewProjection( pEye, pRot, pView, pProj );
		if( NULL != pView && NULL != pProj ) {
			logStr( 4,
					" OUT: view: (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"      proj: (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					, pView[0], pView[1], pView[2], pView[3]
					, pView[4], pView[5], pView[6], pView[7]
					, pView[8], pView[9], pView[10], pView[11]
					, pView[12], pView[13], pView[14], pView[15]
					, pProj[0], pProj[1], pProj[2], pProj[3]
					, pProj[4], pProj[5], pProj[6], pProj[7]
					, pProj[8], pProj[9], pProj[10], pProj[11]
					, pProj[12], pProj[13], pProj[14], pProj[15]
			);
		}
		return err;
	}
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getViewClip( VWB_Warper* pWarper, VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pClip ) {
	if( NULL != pEye && NULL != pRot ) {
		logStr( 4, "VWB_getViewClip\n IN:   eye: (%f, %f, %f)\n"
				"       rot: (%f, %f, %f)\n"
				, pEye[0], pEye[1], pEye[2]
				, pRot[0], pRot[1], pRot[2]
		);
	}
	if( pWarper ) {
		VWB_ERROR err = ( (VWB_Warper_base*)pWarper )->GetViewClip( pEye, pRot, pView, pClip );
		if( NULL != pView && NULL != pClip ) {
			logStr( 4,
					" OUT: view: (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"            (%f, %f, %f, %f)\n"
					"      clip: (%f, %f, %f, %f, %f, %f)\n"
					, pView[0], pView[1], pView[2], pView[3]
					, pView[4], pView[5], pView[6], pView[7]
					, pView[8], pView[9], pView[10], pView[11]
					, pView[12], pView[13], pView[14], pView[15]
					, pClip[0], pClip[1], pClip[2], pClip[3], pClip[4], pClip[5]
			);
		}
		return err;
	}
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getPosDirClip( VWB_Warper* pWarper, VWB_float* pEye, VWB_float* pRot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric, float aspect ) {
	if( NULL != pEye && NULL != pRot ) {
		logStr( 4, "VWB_getPosDirClip\n IN:   eye: (%f, %f, %f)\n"
				"       rot: (%f, %f, %f)\n"
				, pEye[0], pEye[1], pEye[2]
				, pRot[0], pRot[1], pRot[2]
		);
	}
	if( pWarper ) {
		VWB_ERROR err = ( (VWB_Warper_base*)pWarper )->GetPosDirClip( pEye, pRot, pPos, pDir, pClip, symmetric, aspect );
		if( NULL != pPos && NULL != pDir && NULL != pClip ) {
			logStr( 4,
					" OUT: pos: (%f, %f, %f)\n"
					"      dir: (%f, %f, %f)\n"
					"      clip: (%f, %f, %f, %f, %f, %f)\n"
					, pPos[0], pPos[1], pPos[2]
					, pDir[0], pDir[1], pDir[2]
					, pClip[0], pClip[1], pClip[2], pClip[3]
			);
		}
		return err;
	}
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getScreenplane( VWB_Warper* pWarper, VWB_float* pTL, VWB_float* pTR, VWB_float* pBL, VWB_float* pBR ) {
	if( pWarper )
		return ( (VWB_Warper_base*)pWarper )->GetScreenplane( pTL, pTR, pBL, pBR );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_setViewProj( VWB_Warper* pWarper, VWB_float* pView, VWB_float* pProj ) {
	if( pWarper )
		return ( (VWB_Warper_base*)pWarper )->SetViewProjection( pView, pProj );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_render( VWB_Warper* pWarper, VWB_param inputTexture, VWB_uint restoreMask ) {
	logStr( 4, "VWB_Render" );
	if( NULL == pWarper )
		return VWB_ERROR_PARAMETER;
	VWB_ERROR err = ( (VWB_Warper_base*)pWarper )->Render( inputTexture, restoreMask );
	return err;
}

VWB_ERROR VWB_render2( VWB_Warper* pWarper, VWB_param inputTexture, VWB_uint restoreMask ) {
	logStr( 4, "VWB_Render" );
	if( NULL == pWarper )
		return VWB_ERROR_PARAMETER;
	VWB_ERROR err = ( (VWB_Warper_base*)pWarper )->Render2( inputTexture, restoreMask );
	return err;
}

VWB_ERROR VWB_vwfInfo( char const* path, VWB_WarpBlendHeaderSet* set ) {
	return ScanVWF( path, set );
}
VWB_ERROR VWB_vwfInfoW( wchar_t const* path, VWB_WarpBlendHeaderSet* set ) {
	return ScanVWF( path, set );
}
VWB_ERROR VWB_vwfInfoU( char8_t const* path, VWB_WarpBlendHeaderSet* set ) {
	return ScanVWF( path, set );
}

VWB_ERROR VWB_vwfInfoC( char const* path, VWB_WarpBlendHeader* headers, VWB_uint* count ) {
	if( nullptr == count )
		return VWB_ERROR_PARAMETER;
	VWB_WarpBlendHeaderSet set;
	VWB_ERROR res = ScanVWF( path, &set );
	if( VWB_ERROR_NONE == res ) {
		VWB_uint c = (VWB_uint)set.size();
		if( nullptr == headers )
			*count = c;
		else {
			if( *count < c )
				res = VWB_ERROR_FALSE;
			else {
				*count = c;
				for( VWB_uint i = 0; i != c; i++ )
					headers[i] = *set[i];
			}
		}
	}
	return res;
}

VWB_ERROR VWB_vwfInfoCW( wchar_t const* path, VWB_WarpBlendHeader* headers, VWB_uint* count ) {
	if( nullptr == count )
		return VWB_ERROR_PARAMETER;
	VWB_WarpBlendHeaderSet set;
	VWB_ERROR res = ScanVWF( path, &set, true );
	if( VWB_ERROR_NONE == res ) {
		VWB_uint c = (VWB_uint)set.size();
		if( nullptr == headers )
			*count = c;
		else {
			if( *count < c )
				res = VWB_ERROR_FALSE;
			else {
				*count = c;
				for( VWB_uint i = 0; i != c; i++ )
					headers[i] = *set[i];
			}
		}
	}
	return res;
}

VWB_ERROR VWB_vwfInfoCU( char8_t const* path, VWB_WarpBlendHeader* headers, VWB_uint* count ) {
	if( nullptr == count )
		return VWB_ERROR_PARAMETER;
	VWB_WarpBlendHeaderSet set;
	VWB_ERROR res = ScanVWF( path, &set, true );
	if( VWB_ERROR_NONE == res ) {
		VWB_uint c = (VWB_uint)set.size();
		if( nullptr == headers )
			*count = c;
		else {
			if( *count < c )
				res = VWB_ERROR_FALSE;
			else {
				*count = c;
				for( VWB_uint i = 0; i != c; i++ )
					headers[i] = *set[i];
			}
		}
	}
	return res;
}

VWB_ERROR VWB__logString( VWB_int level, char const* str ) {
	if( NULL == str || 0 == str[0] )
		return VWB_ERROR_PARAMETER;
	logStr( level, str );
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_logClear() {
	logClear();
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_getWarpBlend( VWB_Warper* pWarper, VWB_WarpBlend const*& wb ) {
	if( pWarper )
		return ( (VWB_Warper_base*)pWarper )->getWarpBlend( wb );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getWarpBlendC( VWB_Warper* pWarper, VWB_WarpBlend const** wb ) {
	if( nullptr == wb )
		return VWB_ERROR_PARAMETER;
	if( pWarper )
		return ( (VWB_Warper_base*)pWarper )->getWarpBlend( *wb );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getShaderVPMatrix( VWB_Warper* pWarper, VWB_float* pMPV ) {
	if( pWarper )
		return ( (VWB_Warper_base*)pWarper )->getShaderVPMatrix( pMPV );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getWarpBlendMesh( VWB_Warper* pWarper, VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh ) {
	if( pWarper )
		return ( (VWB_Warper_base*)pWarper )->getWarpMesh( cols, rows, mesh );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getWarpBlendMeshC( VWB_Warper* pWarper, VWB_int cols, VWB_int rows, VWB_WarpBlendMesh* mesh ) {
	if( mesh && pWarper )
		return ( (VWB_Warper_base*)pWarper )->getWarpMesh( cols, rows, *mesh );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_destroyWarpBlendMesh( VWB_Warper* pWarper, VWB_WarpBlendMesh& mesh ) {
	if( pWarper ) {
		if( mesh.idx )
			delete[] mesh.idx;
		if( mesh.vtx )
			delete[] mesh.vtx;
		mesh = VWB_WarpBlendMesh{ 0 };
		return VWB_ERROR_NONE;
	}
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_destroyWarpBlendMeshC( VWB_Warper* pWarper, VWB_WarpBlendMesh* mesh ) {
	if( mesh )
		return VWB_destroyWarpBlendMesh( pWarper, *mesh );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getNativeGPUResources( VWB_Warper* pWarper, void** pResources, VWB_uint nResources ) {
	if( nullptr == pResources || 0 == nResources )
		return VWB_ERROR_PARAMETER;
	for( auto p : std::span( pResources, pResources + nResources ) ) {
		p = ( (void*)-1 );
	}
	return VWB_ERROR_NOT_IMPLEMENTED;
}

VWB_ERROR VWB_getWarperInfoFlags( VWB_Warper* pWarper, int* pFlags ) {
	if( nullptr == pFlags || nullptr == pFlags)
		return VWB_ERROR_PARAMETER;
	*pFlags = ((VWB_Warper_base*)pWarper)->getInfoFlags();
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_getVersion( VWB_int* major, VWB_int* minor, VWB_int* maintenance, VWB_int* build ) {
	if( !major || !minor || !maintenance || !build )
		return VWB_ERROR_PARAMETER;
	*major = VWB_Version_MAJ;
	*minor = VWB_Version_MIN;
	*maintenance = VWB_Version_MAI;
	*build = VWB_Version_REV;
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_setCryptoKey( uint8_t const* key ) {
	g_cryptoKey = key;
	return VWB_ERROR_NONE;
}

#ifdef WIN32
size_t copyCursorBitmapToMappedTexture( HBITMAP hbmMask, HBITMAP hbmColor, BITMAP& bmMask, BITMAP& bmColor, void* bits, int pitch ) {
	size_t r = 0;

	size_t szMask = ptrdiff_t( bmMask.bmWidthBytes ) * abs( bmMask.bmHeight );

	bmMask.bmBits = malloc( szMask );
	BYTE* pData = reinterpret_cast<BYTE*>( malloc( ptrdiff_t( abs( bmMask.bmHeight ) ) * pitch ) );
	BYTE* pMask = reinterpret_cast<BYTE*>( malloc( szMask ) );
	BYTE* pM = pMask;
	LONG cb = LONG( szMask );
	if( cb != szMask )
		throw exception( "arithmetic overflow in copyCursorBitmapToMappedTexture" );
	r = ::GetBitmapBits( hbmMask, cb, pMask );
	if( hbmColor ) {
		LONG szColor = bmColor.bmWidthBytes * abs( bmColor.bmHeight );
		r = ::GetBitmapBits( hbmColor, szColor, pData );
	} else {
		bmColor.bmWidth = bmMask.bmWidth;
		bmColor.bmWidthBytes = bmMask.bmWidth * 4;
		bmColor.bmHeight = bmMask.bmHeight / 2;
		bmColor.bmBitsPixel = 32;
		bmColor.bmPlanes = bmMask.bmPlanes;

		BYTE* data = pData;
		for( int y = 0; y != abs( bmMask.bmHeight / 2 ); y++, pM += bmMask.bmWidthBytes, data += pitch ) {
			BYTE* byteAND = pM;
			BYTE maskAND = *byteAND;
			BYTE* byteXOR = pM + ( bmMask.bmHeight * bmMask.bmWidthBytes / 2 );
			BYTE maskXOR = *byteXOR;
			BYTE* pixel = data;

			for( int x = 0; x != bmMask.bmWidth; x++, pixel += 4 ) {
				if( maskXOR & maskAND & 128 ) // -> reverse screen ??, we take green
				{
					pixel[0] = 0;
					pixel[1] = 0;
					pixel[2] = 0;
					pixel[3] = 255;
				} else if( maskXOR & 128 ) // -> white
				{
					pixel[0] = 255;
					pixel[1] = 255;
					pixel[2] = 255;
					pixel[3] = 255;
				} else if( maskAND & 128 ) // -> screen
				{
					pixel[0] = 0;
					pixel[1] = 0;
					pixel[2] = 0;
					pixel[3] = 0;
				} else // -> black
				{
					pixel[0] = 0;
					pixel[1] = 0;
					pixel[2] = 0;
					pixel[3] = 255;
				}

				if( 7 == x % 8 ) {
					byteAND++;
					maskAND = *byteAND;
					byteXOR++;
					maskXOR = *byteXOR;
				} else {
					maskAND <<= 1;
					maskXOR <<= 1;
				}
			}
		}
	}
	// copy image to texture#
	r = abs( bmColor.bmHeight ) * pitch;
	LONG paddBm = bmColor.bmWidthBytes - 4 * bmColor.bmWidth;
	LONG paddTx = pitch - 4 * bmColor.bmWidth;
	// swivel to argb
	for( BYTE* p = pData, *pE = pData + r, *pS = (BYTE*)bits; p != pE; p += paddBm, pS += paddTx ) {
		for( BYTE const* pLE = p + 4 * bmColor.bmWidth; p != pLE; p += 4, pS += 4 ) {
			pS[0] = p[3];
			pS[1] = p[0];
			pS[2] = p[1];
			pS[3] = p[2];
		}
	}
	free( pMask );
	free( pData );
	logStr( 4, "VERBOSE: Cursor image updated.\n" );
	return r;
}
#endif //def WIN32

////////////////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved ) {
	switch( ul_reason_for_call ) {
	case DLL_PROCESS_ATTACH:
		g_hModDll = hModule;
		//g_logLevel = 2;
		//strcpy_s( g_logFilePath, "VWB.log" );
		//logStr( 2, "bla" );
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		if( nullptr != ShowSystemCursor &&
			!g_bCurEnabled ) {
			ShowSystemCursor( TRUE );
		}
		break;
	}
	return TRUE;
}

#elif defined __GNU__
void __attribute__( ( constructor ) ) my_init( void ) {}

void __attribute__( ( destructor ) ) my_fini( void ) {}
#endif //def WIN32
