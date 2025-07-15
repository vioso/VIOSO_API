// VIOSO API
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#ifdef WIN32
#include "DX/DX9WarpBlend.h"
#include "DX/DX9EXWarpBlend.h"
#include "DX/DX11WarpBlend.h"
#include "DX/DX10WarpBlend.h"
#include "GL/GLWarpBlendXPL.h"

#ifndef VWB_WIN7_COMPAT
#include "DX/DX12WarpBlend.h"
#endif //ndef VWB_WIN7COMPAT
#pragma comment( lib, "shlwapi" )
#include <shlwapi.h>
#else
#include <dlfcn.h>
#endif
#include "GL/GLWarpBlend.h"
#include <locale.h>
#include <time.h>
#include <limits.h>
#include <float.h>
#include <list>
#include <fstream>
#include <atomic>
#ifdef WIN32
#include <crtdbg.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>
#include <limits.h>

#include "logging.h"
//#include "3rdparty/delauney/DelaunayTriangles.h"

using namespace std;

VWB_size _size0 = { 0,0 };
bool g_bFirstInstance = true;
uint8_t const* g_cryptoKey = nullptr;

#ifdef WIN32
HMODULE g_hModDll = 0;
VWB_int g_error = 0;
HCURSOR g_hCur = 0;
SIZE	g_dimCur = { 0 };
POINT   g_hotCur = { 0 };
bool    g_bCurEnabled = true;
FPtrInt_BOOL ShowSystemCursor = NULL;
#endif

#ifdef WIN32
DWORD VWB_GetError()
{
	return g_error;
}
#endif

VWB_ERROR invertWB( VWB_WarpBlend const& in, VWB_WarpBlend& out );

VWB_ERROR VWB_Warper_base::ReadIniFile( char const* szConfigFile, char const* szChannelName )
{
	if( szConfigFile && szConfigFile[0] )
	{
		Defaults();
		strcpy_s( path, szConfigFile );
		MkPath( path, MAX_PATH, ".ini" );

		if( szChannelName )
			strcpy_s( channel, szChannelName );
		else
			strcpy_s( channel, "default" );

		FILE* f = NULL;
		if( NO_ERROR == fopen_s( &f, path, "r" ) )
			fclose(f);
		else
		{
			logStr(0, "Error open \"%s\"", path);
			return VWB_ERROR_INI_LOAD;
		}

		char s[1024];
		char sDef[MAX_PATH + 1024];
		int iDef;
		float fDef;

		GetIniString( "default", "logFile", g_logFilePath, sDef, MAX_PATH, path );
		GetIniString( channel, "logFile", sDef, g_logFilePath, MAX_PATH, path );

		iDef = GetIniInt( "default", "logLevel", g_logLevel, path );
		g_logLevel = GetIniInt( channel, "logLevel", iDef, path );

		if (g_bFirstInstance)
		{
			iDef = GetIniInt("default", "bLogClear", 0, path);
			if (1 == GetIniInt(channel, "bLogClear", iDef, path))
				logClear();
			else
				logStr(1, "--- BEGIN SESSION --- BEGIN SESSION --- BEGIN SESSION --- BEGIN SESSION ---\n");
			g_bFirstInstance = false;
		}

		iDef = GetIniInt( "default", "bTurnWithView", bTurnWithView, path );
		bTurnWithView = 0 != GetIniInt( channel, "bTurnWithView", iDef, path );

		iDef = GetIniInt( "default", "bDoNotBlend", bDoNotBlend, path );
		bDoNotBlend = 0 != GetIniInt( channel, "bDoNotBlend", iDef, path );

		GetIniString( "default", "eyePointProvider", eyeProvider, sDef, MAX_PATH, path );
		GetIniString( channel, "eyePointProvider", sDef, eyeProvider, MAX_PATH, path );
#if defined( WIN32 )
		char const* ext = ".dll";
#elif defined( __APPLE__ )
        // no eye provider
        char const* ext = "";
        *eyeProvider = 0;
#else
		char const* ext = ".so";
#endif // defined( WIN32 )
        MkPath( eyeProvider, MAX_PATH, ext );

		GetIniString( "default", "eyePointProviderParam", eyeProviderParam, sDef, MAX_PATH, path );
		GetIniString( channel, "eyePointProviderParam", sDef, eyeProviderParam, MAX_PATH, path );

		GetIniString( "default", "calibFile", calibFile, sDef, MAX_PATH, path );
		GetIniString( channel, "calibFile", sDef, calibFile, 12 * MAX_PATH, path );

		iDef = GetIniInt( "default", "calibIndex", calibIndex, path );
		calibIndex = GetIniInt( channel, "calibIndex", iDef, path );
		if( -1 == calibIndex )
		{
			iDef = GetIniInt( channel, "calibAdapterOrdinal", 0, path );
			calibIndex = -1 * ( GetIniInt( channel, "calibAdapterOrdinal", iDef, path ) );
		}
	
		fDef = GetIniFloat( "default", "near", nearDist, path );
		nearDist = GetIniFloat( channel, "near", fDef, path );

		fDef = GetIniFloat( "default", "far", farDist, path );
		farDist = GetIniFloat( channel, "far", fDef, path );

		fDef = GetIniFloat( "default", "screen", screenDist, path );
		screenDist = GetIniFloat( channel, "screen", fDef, path );

		VWB_float vDef[16] = { 0 };

		GetIniMat( "default", "eye", 3, 1, eye, vDef, path );
		GetIniMat( channel, "eye", 3, 1, vDef, eye, path );

		GetIniMat( "default", "dir", 3, 1, dir, vDef, path );
		GetIniMat( channel, "dir", 3, 1, vDef, dir, path );

		GetIniMat( "default", "fov", 4, 1, fov, vDef, path );
		GetIniMat( channel, "fov", 4, 1, vDef, fov, path );

		if( NULL == GetIniMat( "default", "trans", 4, 4, nullptr, vDef, path, false ) )
		{
			if( NULL == GetIniMat( channel, "trans", 4, 4, nullptr, trans, path, false ) )
			{
				GetIniMat( "default", "base", 4, 4, trans, vDef, path, true );
				GetIniMat( channel, "base", 4, 4, vDef, trans, path, true );
			}
		}
		else
		{
			GetIniMat( channel, "trans", 4, 4, vDef, trans, path, false );
		}

		iDef = GetIniInt( "default", "mode", -1, path );
		iDef = GetIniInt( channel, "mode", iDef, path );
		if( -1 != iDef )
		{
			switch( iDef )
			{
			case 1: 
				splice = 0x00000122;
				break;
			case 2:
				splice = 0x00000133;
				break;
			default:
				splice = 0;
			}
		}
		else
		{
			iDef = GetIniInt( "default", "splice", splice, path );
			splice = GetIniInt( channel, "splice", iDef, path );
		}

		iDef = GetIniInt( "default", "bBicubic", -1, path );
		iDef = GetIniInt( channel, "bBicubic", iDef, path );
		if( -1 == iDef )
		{
			iDef = GetIniInt( "default", "bicubic", bBicubic, path );
			bBicubic = 0 != GetIniInt( channel, "bicubic", iDef, path );
		}
		else
			bBicubic = 0 != iDef;

		iDef = GetIniInt( "default", "bUseGL110", bUseGL110, path );
		bUseGL110 = 0 != GetIniInt( channel, "bUseGL110", iDef, path );

		iDef = GetIniInt("default", "bPartialInput", bPartialInput, path);
		bPartialInput = 0 != GetIniInt(channel, "bPartialInput", iDef, path);

		iDef = GetIniInt("default", "mouseMode", mouseMode, path);
		mouseMode = GetIniInt(channel, "mouseMode", iDef, path);

		GetIniString( "default", "autoViewC", "1.0", sDef, 1024, path );
		GetIniString( channel, "autoViewC", sDef, s, 1024, path );
		sscanf_s( s, "%f", &autoViewC );

		iDef = GetIniInt( "default", "bAutoView", bAutoView, path );
		bAutoView = 0 != GetIniInt( channel, "bAutoView", iDef, path );

		GetIniString( "default", "optimalRes", "[0,0]", sDef, 1024, path );
		GetIniString( channel, "optimalRes", sDef, s, 1024, path );
		sscanf_s( s, "[%i,%i]", &optimalRes.cx, &optimalRes.cy );

		GetIniString( "default", "optimalRes", "[0,0]", sDef, 1024, path );
		GetIniString( channel, "optimalRes", sDef, s, 1024, path );
		sscanf_s( s, "[%d,%d]", &optimalRes.cx, &optimalRes.cy );

		GetIniString( "default", "optimalRect", "[0,0,0,0]", sDef, 1024, path );
		GetIniString( channel, "optimalRect", sDef, s, 1024, path );
		sscanf_s( s, "[%d,%d,%d,%d]", &optimalRect.left, &optimalRect.top, &optimalRect.right, &optimalRect.bottom );

		gamma = GetIniFloat( "default", "gamma", gamma, path );
		gamma = GetIniFloat( channel, "gamma", gamma, path );

		iDef = GetIniInt( "default", "port", port, path );
		port = GetIniInt( channel, "port", iDef, path );

		iDef = GetIniInt( "default", "heartBeatPort", heartBeatPort, path );
		heartBeatPort = GetIniInt( channel, "heartBeatPort", iDef, path );

		GetIniString( "default", "addr", addr, sDef, MAX_PATH, path );
		GetIniString( channel, "addr", sDef, addr, MAX_PATH, path );

		iDef = GetIniInt( "default", "bDoNoBlack", bDoNoBlack, path );
		bDoNoBlack = 0 != GetIniInt( channel, "bDoNoBlack", iDef, path );

		VWB_float split[4]{};
		GetIniMat( "default", "calibSplit", 4, 1, nullptr, vDef, path);
		GetIniMat( channel, "calibSplit", 4, 1, vDef, split, path);
		for (int i = 0; i != 4; i++) calibSplit[i] = VWB_word(split[i]);

		iDef = GetIniInt( "default", "overrideStatemask", overrideStatemask, path );
		overrideStatemask = GetIniInt( channel, "overrideStatemask", iDef, path );

		iDef = GetIniInt( "default", "bFixWraparound", bFixWraparound, path );
		bFixWraparound = 0 != GetIniInt( channel, "bFixWraparound", iDef, path );

		D3D12RTVF = GetIniInt( "default", "D3D12RTVF", D3D12RTVF, path );
		D3D12RTVF = GetIniInt( channel, "D3D12RTVF", D3D12RTVF, path );

		fDef = GetIniFloat( "default", "blackScale", blackScale, path );
		blackScale = GetIniFloat( channel, "blackScale", fDef, path );

		fDef = GetIniFloat( "default", "inputGamma", inputGamma, path );
		inputGamma = GetIniFloat( channel, "inputGamma", fDef, path );

		fDef = GetIniFloat( "default", "blackDarkAdjust", blackDarkAdjust, path );
		blackDarkAdjust = GetIniFloat( channel, "blackDarkAdjust", fDef, path );

		fDef = GetIniFloat( "default", "blackBrightAdjust", blackBrightAdjust, path );
		blackBrightAdjust = GetIniFloat( channel, "blackBrightAdjust", fDef, path );

		iDef = GetIniInt( "default", "debugBreak", 0, path );
		if( GetIniInt( channel, "debugBreak", iDef, path ) )
		{
#ifdef WIN32
			MessageBoxA( NULL, "BREAK", "DEBUG", MB_OK );
#else
#endif
		}
		return VWB_ERROR_NONE;
	}
	return VWB_ERROR_FALSE;
}


/////////////////////////////////////////////////////////////////////////////////////
VWB_ERROR VWB_CreateA( void* pDxDevice, char const* szConfigFile, char const* szChannelName, VWB_Warper** ppWarper, VWB_int logLevel, char const* szLogFile )
{
	if( NULL == ppWarper )
		return VWB_ERROR_PARAMETER;

	*ppWarper = NULL;
	g_logLevel = logLevel;
	if( szLogFile && szLogFile[0] )
		strcpy_s( g_logFilePath, szLogFile );
	else
		strcpy_s( g_logFilePath, "VIOSOWarpBlend" );
    MkPath( g_logFilePath, 260, ".log" );

	try {
		if (VWB_DUMMYDEVICE == pDxDevice)
		{
			*ppWarper = new Dummywarper();
		}
		else if(pDxDevice)
		{
			IUnknown* pUK = (IUnknown*)pDxDevice;
			if( pUK )
			{
				IUnknown* pUK2 = NULL;
				// destinguish between DX flavours
				do {
		#ifdef WIN32

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( IXPlaneRef ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new GLWarpBlendXPL( (IXPlaneRef*)pDxDevice );
							break;
						}
					}

				#ifndef VWB_WIN7_COMPAT
					if( SUCCEEDED( pUK->QueryInterface( __uuidof( ID3D12CommandQueue ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new DX12WarpBlend( (ID3D12CommandQueue*)pDxDevice );
							break;
						}
					}
				#endif //ndef VWB_WIN7_COMPAT

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( ID3D11Device ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new DX11WarpBlend( (ID3D11Device*)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( ID3D10Device1 ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new DX10WarpBlend( (ID3D10Device*)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( ID3D10Device ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new DX10WarpBlend( (ID3D10Device*)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( IDirect3DDevice9Ex ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new DX9EXWarpBlend( (LPDIRECT3DDEVICE9EX)pDxDevice );
							break;
						}
					}

					if( SUCCEEDED( pUK->QueryInterface( __uuidof( IDirect3DDevice9 ), (void**)&pUK2 ) ) )
					{
						pUK2->Release();
						if( pUK == pUK2 )
						{
							*ppWarper = new DX9WarpBlend( (LPDIRECT3DDEVICE9)pDxDevice );
							break;
						}
					}

		#endif //def WIN32
				} while( 0 );
			}
		}
		else
		{
			*ppWarper = new GLWarpBlend();
		}
		if( !*ppWarper )
			throw (VWB_int)VWB_ERROR_GENERIC;
	} catch ( VWB_int e )
	{
		logStr( 0, "FATAL: Error %d creating warper \"%s\".\n", e, szChannelName ? szChannelName : (*ppWarper)->channel );
		return (VWB_ERROR)e;
	}
	if( NULL != szConfigFile && 0 != szConfigFile[0] )
	{
		if( VWB_ERROR_NONE != ((VWB_Warper_base*)*ppWarper)->ReadIniFile( szConfigFile, szChannelName ) )
		{
			logStr( 0, "FATAL: .ini file (%s) parsing error.\n", szConfigFile );
			delete ((VWB_Warper_base*)*ppWarper);
			*ppWarper = NULL;
			return VWB_ERROR_INI_LOAD;
		}
	}
	else
	{
		if( NULL != szChannelName && 0 != szChannelName[0] )
			strcpy_s( ((VWB_Warper_base*)*ppWarper)->channel, szChannelName );
		else
			strcpy_s( ((VWB_Warper_base*)*ppWarper)->channel, "default" );
	}

	{
		time_t t;
		time( &t );
		struct tm tm;
		localtime_s( &tm, &t );
		logStr( 1, "%04d/%02d/%02d VIOSOWarpBlend API %d.%d.%d.%d.\n", 1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday, VWB_Version_MAJ, VWB_Version_MIN, VWB_Version_MAI, VWB_Version_REV );
	#ifdef _DEBUG
		logStr( 1, "DEBUG" );
	#endif

		char* szModPath = NULL;
		char* szProcPath = NULL;
#ifdef WIN32
		char procPath[MAX_PATH]{ 0 };
		if( ::GetModuleFileNameA( 0, procPath, MAX_PATH ) )
			szProcPath = procPath;
		char modPath[MAX_PATH]{ 0 };
		szModPath = modPath;
		if( ::GetModuleFileNameA( g_hModDll, modPath, MAX_PATH ) )
#else
		Dl_info dl_info{};
		szModPath = (char*) dl_info.dli_fname;
		if( dladdr((void *)VWB_CreateA, &dl_info) && szModPath )
#endif //def WIN32
			logStr( 1, "lib-path %s.\n", szModPath );
		if( szProcPath )
			logStr( 1, "process path %s.\n", szProcPath );
	}
	logStr( 2,
			"%.4s-Warper \"%s\" created. Logging level is %d\nEvaluated parameters%s%s:\n"
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
			"\n",
			((VWB_Warper_base*)*ppWarper)->GetType(), (*ppWarper)->channel, g_logLevel,
			(*ppWarper)->path[0] ? " from\n" : ", no .ini file set, using defaults",
			(*ppWarper)->path,
			(*ppWarper)->calibFile,
			(*ppWarper)->calibIndex,
			(*ppWarper)->calibSplit[0], (*ppWarper)->calibSplit[1], (*ppWarper)->calibSplit[2], (*ppWarper)->calibSplit[3],
			(*ppWarper)->bTurnWithView ? 1 : 0,
			(*ppWarper)->bDoNotBlend ? 1 : 0,
			(*ppWarper)->eyeProvider,
			(*ppWarper)->eyeProviderParam,
			(*ppWarper)->eye[0],(*ppWarper)->eye[1],(*ppWarper)->eye[2],
			(*ppWarper)->nearDist,
			(*ppWarper)->farDist,
			(*ppWarper)->bBicubic ? 1 : 0,
			(*ppWarper)->bUseGL110 ? 1 : 0,
			(*ppWarper)->bPartialInput ? 1 : 0,
			(*ppWarper)->splice,
			(*ppWarper)->trans[ 0],(*ppWarper)->trans[ 1],(*ppWarper)->trans[ 2],(*ppWarper)->trans[ 3],
			(*ppWarper)->trans[ 4],(*ppWarper)->trans[ 5],(*ppWarper)->trans[ 6],(*ppWarper)->trans[ 7],
			(*ppWarper)->trans[ 8],(*ppWarper)->trans[ 9],(*ppWarper)->trans[10],(*ppWarper)->trans[11],
			(*ppWarper)->trans[12],(*ppWarper)->trans[13],(*ppWarper)->trans[14],(*ppWarper)->trans[15],
			(*ppWarper)->autoViewC,
			(*ppWarper)->bAutoView ? 1 : 0,
			(*ppWarper)->dir[0],(*ppWarper)->dir[1],(*ppWarper)->dir[2],
			(*ppWarper)->fov[0],(*ppWarper)->fov[1],(*ppWarper)->fov[2],(*ppWarper)->fov[3],
			(*ppWarper)->screenDist,
			(*ppWarper)->optimalRes.cx, (*ppWarper)->optimalRes.cy,
			(*ppWarper)->optimalRect.left, (*ppWarper)->optimalRect.top, (*ppWarper)->optimalRect.right, (*ppWarper)->optimalRect.bottom,  
		    (*ppWarper)->gamma,
			(*ppWarper)->port,
			(*ppWarper)->heartBeatPort,
			(*ppWarper)->addr,
			(*ppWarper)->mouseMode,
			(*ppWarper)->bDoNoBlack,
			(*ppWarper)->overrideStatemask,
			( *ppWarper )->bFixWraparound,
			( *ppWarper )->D3D12RTVF,
			( *ppWarper )->blackScale,
			( *ppWarper )->inputGamma,
			( *ppWarper )->blackDarkAdjust,
			( *ppWarper )->blackBrightAdjust
	);

	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_CreateW( void* pDxDevice, wchar_t const* szConfigFile, wchar_t const* szChannelName, VWB_Warper** ppWarper, VWB_int logLevel, wchar_t const* szLogFile )
{
	try
	{
		char szCF[MAX_PATH];
		char* pszCF = NULL;
		char szCN[MAX_PATH];
		char* pszCN = NULL;
		char szLF[MAX_PATH];
		char* pszLF = NULL;
		if( szConfigFile )
		{
			wcstombs_s( NULL, szCF, szConfigFile, MAX_PATH );
			pszCF = szCF;
		}
		if( szChannelName )
		{
			wcstombs_s( NULL, szCN, szChannelName, MAX_PATH );
			pszCN = szCN;
		}
		if( szLogFile )
		{
			wcstombs_s( NULL, szLF, szLogFile, MAX_PATH );
			pszLF = szLF;
		}
		return VWB_CreateA( pDxDevice, pszCF, pszCN, ppWarper, logLevel, pszLF );
	} catch(...)
	{
		return VWB_ERROR_PARAMETER;
	}
}

VWB_ERROR VWB_Init( VWB_Warper* pWarper )
{
	return VWB_InitExt( pWarper, nullptr );
}

VWB_ERROR VWB_InitExt( VWB_Warper* pWarper, VWB_WarpBlendSet* extSet )
{
	if( NULL == pWarper )
		return VWB_ERROR_PARAMETER;

	VWB_ERROR err = VWB_ERROR_NONE;

	if( nullptr != extSet )
	{
		if ((VWB_ERROR_NONE != err) || !VerifySet(*extSet))
		{
			logStr(0, "ERROR: Failed to verify given set.\n");
			return err;
		}
		else
			err = VWB_ERROR_NONE;

		VWB_Warper_base* p = (VWB_Warper_base*)pWarper;
		err = p->Init( *extSet );
	}
	else
	{
		VWB_WarpBlendSet set;
		err = LoadVWF( set, ( (VWB_Warper_base*)pWarper )->calibFile, false, pWarper->calibIndex, g_cryptoKey );
		//if( ( VWB_ERROR_VWF_LOAD == err ) ||
		//	( VWB_ERROR_VWF_FILE_NOT_FOUND == err && VWB_ERROR_NONE != CreateDummyVWF( set, ((VWB_Warper_base*)pWarper)->calibFile ) ) ||
		//    ( VWB_ERROR_NONE == err  && !VerifySet( set ) ) )

		if (VWB_ERROR_NONE != err)
		{
			logStr(0, "ERROR: LoadVWF: Failed to load set.\n");
			return err;
		}
		if (!VerifySet(set, pWarper->calibIndex) )
		{
			logStr(0, "ERROR: LoadVWF: Failed to verify set.\n");
			return VWB_ERROR_GENERIC;
		}
		else
			err = VWB_ERROR_NONE;

		VWB_Warper_base* p = (VWB_Warper_base*)pWarper;
		err = p->Init(set);
		DeleteVWF(set);
	}	

	((VWB_Warper_base*)pWarper)->GetViewProjection( NULL, NULL, NULL, NULL );

	return err;
}

void VWB_Destroy( VWB_Warper* pWarper )
{
	if( pWarper )
		delete ( VWB_Warper_base* )pWarper;
}


VWB_ERROR VWB_getViewProj( VWB_Warper* pWarper, VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pProj )
{

	if( NULL != pEye && NULL != pRot )
	{
		logStr( 4, "VWB_getViewProj\n IN:   eye: (%f, %f, %f)\n"
				"       rot: (%f, %f, %f)\n"
				, pEye[0], pEye[1], pEye[2]
				, pRot[0], pRot[1], pRot[2]
		);
	}
	if( pWarper )
	{
		VWB_ERROR err = ( (VWB_Warper_base*)pWarper )->GetViewProjection( pEye, pRot, pView, pProj );
		if( NULL != pView && NULL != pProj )
		{
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

VWB_ERROR VWB_getViewClip( VWB_Warper* pWarper, VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pClip )
{
	if( NULL != pEye && NULL != pRot )
	{
		logStr( 4, "VWB_getViewClip\n IN:   eye: (%f, %f, %f)\n"
				"       rot: (%f, %f, %f)\n"
				, pEye[0], pEye[1], pEye[2]
				, pRot[0], pRot[1], pRot[2]
		);
	}
	if( pWarper )
	{
		VWB_ERROR err = ( (VWB_Warper_base*)pWarper )->GetViewClip( pEye, pRot, pView, pClip );
		if( NULL != pView && NULL != pClip )
		{
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

VWB_ERROR VWB_getPosDirClip( VWB_Warper* pWarper, VWB_float* pEye, VWB_float* pRot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric, float aspect )
{
	if( NULL != pEye && NULL != pRot )
	{
		logStr( 4, "VWB_getPosDirClip\n IN:   eye: (%f, %f, %f)\n"
				"       rot: (%f, %f, %f)\n"
				, pEye[0], pEye[1], pEye[2]
				, pRot[0], pRot[1], pRot[2]
		);
	}
	if( pWarper )
	{
		VWB_ERROR err = ( (VWB_Warper_base*)pWarper )->GetPosDirClip( pEye, pRot, pPos, pDir, pClip, symmetric, aspect );
		if( NULL != pPos && NULL != pDir && NULL != pClip )
		{
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

VWB_ERROR VWB_getScreenplane( VWB_Warper* pWarper, VWB_float* pTL, VWB_float* pTR, VWB_float* pBL, VWB_float* pBR )
{
	if( pWarper )
		return ( (VWB_Warper_base*)pWarper )->GetScreenplane( pTL, pTR, pBL, pBR );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_setViewProj( VWB_Warper* pWarper, VWB_float* pView,  VWB_float* pProj )
{
	if( pWarper )
		return ((VWB_Warper_base*)pWarper)->SetViewProjection( pView, pProj );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_render( VWB_Warper* pWarper, VWB_param inputTexture, VWB_uint restoreMask )
{
	logStr( 4, "VWB_Render" );
	if( NULL == pWarper )
		return VWB_ERROR_PARAMETER;
	VWB_ERROR err = ((VWB_Warper_base*)pWarper)->Render( inputTexture, restoreMask );
	return err;
}

VWB_ERROR VWB_vwfInfo( char const* path, VWB_WarpBlendHeaderSet* set )
{
	return ScanVWF( path, set );
}

VWB_ERROR VWB_vwfInfoC( char const* path, VWB_WarpBlendHeader* headers, VWB_uint* count )
{
	if( nullptr == count )
		return VWB_ERROR_PARAMETER;
	VWB_WarpBlendHeaderSet set;
	VWB_ERROR res = ScanVWF( path, &set );
	if( VWB_ERROR_NONE == res )
	{
		VWB_uint c = (VWB_uint)set.size();
		if( nullptr == headers )
			*count = c;
		else
		{
			if( *count < c )
				res = VWB_ERROR_FALSE;
			else
			{
				for( VWB_uint i = 0; i != c; i++ )
					headers[i] = *set[i];
			}
		}
	}
	return res;
}

VWB_ERROR VWB__logString( VWB_int level, char const* str )
{
	if( NULL == str || 0 == str[0] )
		return VWB_ERROR_PARAMETER;
	logStr( level, str );
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_getWarpBlend( VWB_Warper* pWarper, VWB_WarpBlend const*& wb )
{
	if( pWarper )
		return ( (VWB_Warper_base*)pWarper )->getWarpBlend( wb );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getWarpBlendC( VWB_Warper* pWarper, VWB_WarpBlend const** wb )
{
	if( nullptr == wb )
		return VWB_ERROR_PARAMETER;
	if( pWarper )
		return ( (VWB_Warper_base*)pWarper )->getWarpBlend( *wb );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getShaderVPMatrix( VWB_Warper* pWarper, VWB_float* pMPV )
{
	if( pWarper )
		return ( (VWB_Warper_base*)pWarper )->getShaderVPMatrix( pMPV );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getWarpBlendMesh( VWB_Warper* pWarper, VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh )
{
	if( pWarper )
		return ( (VWB_Warper_base*)pWarper )->getWarpMesh( cols, rows, mesh );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getWarpBlendMeshC( VWB_Warper* pWarper, VWB_int cols, VWB_int rows, VWB_WarpBlendMesh* mesh )
{
	if( mesh && pWarper )
		return ( (VWB_Warper_base*)pWarper )->getWarpMesh( cols, rows, *mesh );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_destroyWarpBlendMesh( VWB_Warper* pWarper, VWB_WarpBlendMesh& mesh )
{
	if( pWarper )
	{
		if( mesh.idx )
			delete[] mesh.idx;
		if( mesh.vtx )
			delete[] mesh.vtx;
		mesh = VWB_WarpBlendMesh{ 0 };
		return VWB_ERROR_NONE;
	}
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_destroyWarpBlendMeshC( VWB_Warper* pWarper, VWB_WarpBlendMesh* mesh )
{
	if( mesh )
		return VWB_destroyWarpBlendMesh( pWarper, *mesh );
	return VWB_ERROR_PARAMETER;
}

VWB_ERROR VWB_getVersion( VWB_int* major, VWB_int* minor, VWB_int* maintenance, VWB_int* build )
{
	if( !major || !minor || !maintenance || !build )
		return VWB_ERROR_PARAMETER;
	*major = VWB_Version_MAJ;
	*minor = VWB_Version_MIN;
	*maintenance = VWB_Version_MAI;
	*build = VWB_Version_REV;
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_setCryptoKey( uint8_t const* key )
{
	g_cryptoKey = key;
	return VWB_ERROR_NONE;
}

///////////////////////////////////////////////////////////////
#include "mmath.h"

VWB_Warper_base::VWB_Warper_base()
: m_type4cc( 0 )
, m_sizeMap( _size0 )
, m_sizeIn( _size0 )
, m_bDynamicEye( false )
, m_bBorder( false )
, m_mBaseI( VWB_MAT44f::I() )
, m_mViewIG( VWB_MAT44f::I() )
, m_bRH(true)
, m_mVP( VWB_MAT44f::I() )
, m_hmEPP( 0 )
, m_hEPP( NULL )
, m_fnEPPCreate( NULL )
, m_fnEPPGet( NULL )
, m_fnEPPRelease( NULL )
{
	Defaults();
	memset( &m_ep, 0, sizeof( m_ep ) );
	m_viewSizes.x = 1;
	m_viewSizes.y = 1;
	m_viewSizes.z = 1;
	m_viewSizes.w = 1;
	m_blackBias.x = 0;
	m_blackBias.y = 1;
	m_blackBias.z = 1;
	m_blackBias.w = 0;
}

VWB_Warper_base::~VWB_Warper_base()
{
	if( m_hmEPP )
	{
		if( m_hEPP && m_fnEPPRelease )
			m_fnEPPRelease( m_hEPP );
        #ifdef WIN32
		::FreeLibrary( m_hmEPP );
        #endif
	}
}

VWB_ERROR VWB_Warper_base::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR ret = VWB_ERROR_NONE;
	// determine screen
	if( 0 > calibIndex )
	{
		int iScreen = -calibIndex;
		char szScreen[18];
		sprintf_s( szScreen, "D%u ", iScreen );
		int l = (int)strlen( szScreen );
		int i = 0;
		for( ; i != (int)wbs.size(); i++ )
		{
			char const* si = wbs[i]->header.name;
			if( 0 == strncmp( si, szScreen, l ) )
			{
				logStr( 2, "AdapterOrdinal %d (%s) resolved to calibIndex %d.\n", -calibIndex, si, i );
				calibIndex = i;
				break;
			}
		}
		if( i == (int)wbs.size() )
		{
			logStr( 0, "ERROR: Could not find mapping of display D%u.", iScreen );
			return VWB_ERROR_GENERIC;
		}
	}

#ifdef WIN32
	ShowSystemCursor = reinterpret_cast<FPtrInt_BOOL>(::GetProcAddress(::GetModuleHandleA("user32.dll"), "ShowSystemCursor"));
	if (nullptr == ShowSystemCursor)
	{
		logStr(1, "WARNING: Could not find ShowSystemCursor function.");
	}
#endif //def WIN32


	if( (VWB_int)wbs.size() <= calibIndex )
	{
		logStr( 0, "ERROR: calibIndex out of range. There is(are) only %u dataset(s).", (VWB_uint)wbs.size() );
		return VWB_ERROR_GENERIC;
	}

	VWB_WarpBlend& wb = *wbs[calibIndex];
	int oRes[2] = {
		int( 1.0f / wb.header.vCntDispPx[4] ),
		int( 1.0f / wb.header.vCntDispPx[5] )
	};
	int oRect[4] = {
		int( wb.header.vPartialCnt[0] * oRes[0] ),
		int( wb.header.vPartialCnt[1] * oRes[1] ),
		int( wb.header.vPartialCnt[2] * oRes[0] ),
		int( wb.header.vPartialCnt[3] * oRes[1] )
	};

	if( 0 >= oRect[2] - oRect[0] ||
		0 >= oRect[3] - oRect[1] ||
		0 >= oRes[0] || 0 >= oRes[1] ) {
		logStr( 1, "Warning: Invalid content bounds. Recalculating..." );
		CalculateBounds( wb, oRes[0], oRes[1], oRect[0], oRect[1], oRect[2], oRect[3] );
	}

	logStr( 2, "Mapping Info:\n"
			"    Hostname: \"%s\"\n"
			"  Devicename: \"%s\"\n"
			"   SplitInfo: (%i,%i) of (%i,%i)\n"
			"  Resolution: %dx%d\n"
			"    Position: %d,%d\n"
			"  optimalRes: [%d,%d]\n"
			" optimalRect: [%d,%d,%d,%d]\n",
			wb.header.hostname,
			wb.header.name,
			wb.header.splitColumnIndex, wb.header.splitRowIndex, wb.header.splitColumns, wb.header.splitRows,
			wb.header.width, wb.header.height,
			(int)wb.header.offsetX, (int)wb.header.offsetY,
			oRes[0], oRes[1],
			oRect[0], oRect[1], oRect[2], oRect[3]
	);

	CalculateBounds( wb, oRes[0], oRes[1], oRect[0], oRect[1], oRect[2], oRect[3] );
	logStr( 2, "Recalculated Bounds:\n"
			"  optimalRes: [%d,%d]\n"
			" optimalRect: [%d,%d,%d,%d]\n",
			oRes[0], oRes[1],
			oRect[0], oRect[1], oRect[2], oRect[3]
	);

	if (calibSplit[0])
	{
		ret = SplitVWF(wb, calibSplit);
		if (ret != VWB_ERROR_NONE && ret != VWB_ERROR_FALSE)
		{
			logStr(0, "ERROR: Failed to split mapping");
			return ret;
		}
		logStr(2, "Splitted map [%i,%i,%i,%i], new resolution (%i,%i), new position %d,%d.",
			   calibSplit[0], calibSplit[1], calibSplit[2], calibSplit[3],
			   wb.header.width, wb.header.height,
			   (int)wb.header.offsetX, (int)wb.header.offsetY);
	}


	m_bBorder = 0 != ( FLAG_WARPFILE_HEADER_BORDER & wb.header.flags );
	m_bDynamicEye = 0 != ( FLAG_WARPFILE_HEADER_3D & wb.header.flags );
	if( m_bDynamicEye )
		logStr( 2, "3D data found in mapping. Using DYNAMIC EYE settings.\n" );
	else
		logStr( 2, "2D data found in mapping. Using FIXED EYE settings.\n" );

	m_sizeMap.cx = wbs[calibIndex]->header.width;
	m_sizeMap.cy = wbs[calibIndex]->header.height;
	m_blackBias.x = wbs[calibIndex]->header.blackScale;
	m_blackBias.y = wbs[calibIndex]->header.blackDark;
	m_blackBias.z = wbs[calibIndex]->header.blackBright;

	VWB_MAT44f B( trans );
	m_mBaseI = B.Inverted();

	m_bRH = 0 < VWB_VEC3f(B.Z()).dot( VWB_VEC3f(B.X()) * VWB_VEC3f(B.Y()) );
	logStr( 2, "%s-handedness detected.\n", m_bRH ? "right" : "left" );

	ret = PrepareForUse( wb, gamma );
	if( VWB_ERROR_NONE != ret )
	{
		return ret;
	}

	if( 0 == ( wb.header.flags & FLAG_WARPFILE_HEADER_3D ) )
	{
		// in case of 2D mapping
		// calculate bounds, init with opposite extremum
		wb.header.vPartialCnt[0] = 1; // left
		wb.header.vPartialCnt[1] = 1; // top
		wb.header.vPartialCnt[2] = 0; // right
		wb.header.vPartialCnt[3] = 0; // bottom
		VWB_BlendRecord2* pB = wb.pBlend2;
		for( VWB_WarpRecord* pW = wb.pWarp, *pWE = wb.pWarp + ptrdiff_t( m_sizeMap.cx ) * m_sizeMap.cy; pW != pWE; pW++, pB++ )
		{
			if(0.5f <= pW->z && ( 0 < pB->r || 0 < pB->g || 0 < pB->b))
			{
				if (wb.header.vPartialCnt[0] > pW->x)
					wb.header.vPartialCnt[0] = pW->x;
				if (wb.header.vPartialCnt[1] > pW->y)
					wb.header.vPartialCnt[1] = pW->y;
				if (wb.header.vPartialCnt[2] < pW->x)
					wb.header.vPartialCnt[2] = pW->x;
				if (wb.header.vPartialCnt[3] < pW->y)
					wb.header.vPartialCnt[3] = pW->y;
			}
		}
		if (wb.header.vPartialCnt[2] <= wb.header.vPartialCnt[0] ||
			wb.header.vPartialCnt[3] <= wb.header.vPartialCnt[1])
		{
			wb.header.vPartialCnt[0] = 0;
			wb.header.vPartialCnt[1] = 0;
			wb.header.vPartialCnt[2] = 0;
			wb.header.vPartialCnt[3] = 0;
			logStr( 1, "WARNING: Could not calculate content bounds." );
		}
		else
		{
			wb.header.vPartialCnt[4] = (wb.header.vPartialCnt[2] - wb.header.vPartialCnt[0]) / (wb.header.vPartialCnt[3] - wb.header.vPartialCnt[1]);
		}
	}

	// initialize eye provider, in case it is stated
	if( eyeProvider[0] )
	{
		char p[MAX_PATH] = {0};
		strcpy_s( p, eyeProvider );
#ifdef WIN32
		m_hmEPP = LoadLibraryA( p );
		if( 0 == m_hmEPP )
		{
			logStr( 0, "ERROR: Could not find eye point provider module: \"%s\".\n", p );
			return VWB_ERROR_PARAMETER;
		}
		m_fnEPPCreate = (pfn_CreateEyePointReceiver)GetProcAddress( m_hmEPP, "CreateEyePointReceiver" );
		m_fnEPPGet = (pfn_ReceiveEyePoint)GetProcAddress( m_hmEPP, "ReceiveEyePoint" );
		m_fnEPPRelease  = (pfn_DeleteEyePointReceiver)GetProcAddress( m_hmEPP, "DeleteEyePointReceiver" );
		if( NULL == m_fnEPPCreate || NULL == m_fnEPPGet || NULL == m_fnEPPRelease )
		{
			logStr( 0, "ERROR: Could not load eye point provider methods from \"%s\".\n", p );
			return VWB_ERROR_GENERIC;
		}
#else
//#error implement!
		logStr( 0, "ERROR: Not implemented. Could not load eye point provider methods from \"%s\".\n", p );
		return VWB_ERROR_NOT_IMPLEMENTED;
#endif //def WIN32

		m_hEPP = m_fnEPPCreate( eyeProviderParam );
		if( NULL == m_hEPP )
		{
			logStr( 0, "ERROR: Could not initialize eye point provider \"%s\".\n", p );
			return VWB_ERROR_GENERIC;
		}
		logStr( 1, "Eye point provider \"%s\" successfully initialized.\n", p );
	}

	// do auto calculations
	if( m_bDynamicEye )
	{
		if( bAutoView )
		{
			VWB_ERROR res = AutoView( *wbs[calibIndex] );		// viewport calculation
			if( VWB_ERROR_NONE != res )
				logStr( 0, "ERROR: Autoview returns error %d.\n", res );
		}	
		else
		{
			optimalRes.cx = wb.header.width;
			optimalRes.cy = wb.header.height;
			optimalRect.left =	0;
			optimalRect.top =	0;
			optimalRect.right = wb.header.width;
			optimalRect.bottom =wb.header.height;
		}
#ifdef _DEBUG
		// collect dimensions
		VWB_BOXf b = VWB_BOXf::M();
		VWB_WarpRecord* pW = wb.pWarp;
		if (wb.pBlend2)
		{
			for (VWB_BlendRecord2* p = wb.pBlend2, *pE = wb.pBlend2 + ptrdiff_t(m_sizeMap.cx) * m_sizeMap.cy; p != pE; p++, pW++)
			{
				if (0.5 <= pW->w && ( p->r || p->g || p->b ) )
				{
					b += VWB_VEC3f::ptr(&pW->x);
				}
			}
		}
		else
		{
			for (VWB_WarpRecord* pWE = pW + ptrdiff_t(m_sizeMap.cx) * m_sizeMap.cy; pW != pWE; pW++)
			{
				if (0.5 <= pW->w)
				{
					b += VWB_VEC3f::ptr(&pW->x);
				}
			}
		}
#endif //def _DEBUG
	}
	else
	{
		if( bAutoView )
		{
			if( bFixWraparound )
			{
				VWB_ERROR res = FixWraparound( *wbs[calibIndex] );
				if( VWB_ERROR_NONE != res )
					logStr( 0, "ERROR: FixWraparound returns error %d.\n", res );
			}

			if( 0 != wb.header.vCntDispPx[4] && 0 != wb.header.vCntDispPx[5] )
			{
				optimalRes.cx = ( VWB_int )( 1.0f / wb.header.vCntDispPx[4] );
				optimalRes.cy = ( VWB_int )( 1.0f / wb.header.vCntDispPx[5] );
			}
			else
			{
				optimalRes.cx = wb.header.width;
				optimalRes.cy = wb.header.height;
				logStr( 2, "WARNING: AutoView could not calculate optimal resolution due to missing information in warp file\n" );
			}
			if( 0 != ( wb.header.vPartialCnt[2] - wb.header.vPartialCnt[0] ) &&
				0 != ( wb.header.vPartialCnt[3] - wb.header.vPartialCnt[1] ) )
			{
				optimalRect.left = ( VWB_int )( wb.header.vPartialCnt[0] * optimalRes.cx );
				optimalRect.top = ( VWB_int )( wb.header.vPartialCnt[1] * optimalRes.cy );
				optimalRect.right = ( VWB_int )( wb.header.vPartialCnt[2] * optimalRes.cx );
				optimalRect.bottom = ( VWB_int )( wb.header.vPartialCnt[3] * optimalRes.cy );
			}
			else
			{
				optimalRect.left = 0;
				optimalRect.top = 0;
				optimalRect.right = wb.header.width;
				optimalRect.bottom = wb.header.height;
				logStr( 2, "WARNING: AutoView could not calculate optimal partial rect due to missing information in warp file\n" );
			}
			logStr( 1, "INFO: AutoView for channel [%s] yields:\n"
				"optimalRes=[%d, %d]\n"
				"optimalRect=[%d, %d, %d, %d]\n",
				channel,
				optimalRes.cx, optimalRes.cy,
				optimalRect.left, optimalRect.top, optimalRect.right, optimalRect.bottom );
		}
		else
		{
			if( bFixWraparound )
			{
				VWB_ERROR res = FixWraparound( *wbs[calibIndex] );
				if( VWB_ERROR_NONE != res )
					logStr( 0, "ERROR: FixWraparound returns error %d.\n", res );
			}

			if( optimalRes.cx <= 0 || optimalRes.cy <= 0 ) {
				optimalRes.cx = wb.header.width;
				optimalRes.cy = wb.header.height;
			}
			if( optimalRect.right - optimalRect.left <= 0 ||
				optimalRect.bottom - optimalRect.top <= 0 ) {
				// if no rect is given, use the whole image
				// this is the case for 2D mappings, where no partial rects are defined
				optimalRect.left = 0;
				optimalRect.top = 0;
				optimalRect.right = wb.header.width;
				optimalRect.bottom = wb.header.height;
			}
		}
	}

	// translate world to local IG's coordinates
	// translation/rotation to VIOSO's eye point is done via base matrix later

	// we have to inverse rotate and translate
	m_mViewIG = m_bRH ? VWB_MAT44f::R( VWB_VEC3f::ptr( dir ) * VWB_float( M_PI / 180.0 ) ) : VWB_MAT44f::R_LH( VWB_VEC3f::ptr( dir ) * VWB_float( M_PI / 180.0 ) );

	// Remember the sizes of the frustum on screenDist
	m_viewSizes.x = tan( DEG2RAD(fov[0] ) ) * screenDist; // left
	m_viewSizes.y = tan( DEG2RAD(fov[1] ) ) * screenDist; // top
	m_viewSizes.z = tan( DEG2RAD(fov[2] ) ) * screenDist; // right
	m_viewSizes.w = tan( DEG2RAD(fov[3] ) ) * screenDist; // bottom

	return ret;
}

VWB_ERROR VWB_Warper_base::UpdateEye( VWB_float* eye, VWB_float* rot )
{
	// receive the current car parameters
	if( m_hEPP && m_fnEPPGet ) // try eye porvider
	{
		m_fnEPPGet( m_hEPP, &m_ep );
		if( eye )
		{
			eye[0] = (float)m_ep.x;
			eye[1] = (float)m_ep.y;
			eye[2] = (float)m_ep.z;
		}
		if( rot )
		{
			rot[0] = (float)m_ep.pitch;
			rot[1] = (float)m_ep.yaw;
			rot[2] = (float)m_ep.roll;
		}

		logStr( 4, "INFO: EyePointReceiver %s input for channel [%s]: pos=[%01.3f,%01.3f,%01.3f] dir=[%01.3f,%01.3f,%01.3f].\n",
				eyeProviderParam, channel,
				m_ep.x, m_ep.y, m_ep.z,
				m_ep.pitch, m_ep.yaw, m_ep.roll );
	}
	else
	{ // use given
		if( eye )
			spliceVec( m_ep.x, m_ep.y, m_ep.z, eye[0], eye[1], eye[2], ( splice >> 16 ) );
		else
		{
			m_ep.x = 0;
			m_ep.y = 0;
			m_ep.z = 0;
		}

		if( rot )
			spliceVec( m_ep.pitch, m_ep.yaw, m_ep.roll, rot[0], rot[1], rot[2], splice );
		else
		{
			m_ep.yaw = 0;
			m_ep.pitch = 0;
			m_ep.roll = 0;
		}

		if( NULL != eye && NULL != rot )
			logStr( 4, "INFO: Eyepoint from call for channel [%s]: pos=[%01.3f,%01.3f,%01.3f] dir=[%01.3f,%01.3f,%01.3f].\n",
					channel,
					eye[0], eye[1], eye[2],
					rot[0], rot[1], rot[2] );
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj )
{
	return VWB_ERROR_NOT_IMPLEMENTED;
}

VWB_ERROR VWB_Warper_base::GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip )
{
	return VWB_ERROR_NOT_IMPLEMENTED;
}

VWB_ERROR VWB_Warper_base::GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric, VWB_float aspect )
{
	return VWB_ERROR_NOT_IMPLEMENTED;
}

VWB_ERROR VWB_Warper_base::GetScreenplane( VWB_float* pTL, VWB_float* pTR, VWB_float* pBL, VWB_float* pBR )
{
	if( !(pTL && pTR && pBL && pBR ) )
		return VWB_ERROR_PARAMETER;

	pBL[0] = pTL[0] = -m_viewSizes[0];
	pTR[1] = pTL[1] = m_viewSizes[1];
	pBR[0] = pTR[0] = m_viewSizes[2];
	pBR[1] = pBL[1] = -m_viewSizes[3];
	pTL[2] = pTR[2] = pBL[2] = pBR[2] = m_bRH ? -screenDist : screenDist;
	VWB_MAT44f V = m_mViewIG.Inverted();
	VWB_VEC3f::ptr( pTL ) = V * VWB_VEC3f::ptr( pTL );
	VWB_VEC3f::ptr( pTR ) = V * VWB_VEC3f::ptr( pTR );
	VWB_VEC3f::ptr( pBL ) = V * VWB_VEC3f::ptr( pBL );
	VWB_VEC3f::ptr( pBR ) = V * VWB_VEC3f::ptr( pBR );
	return VWB_ERROR_NONE;
}

void VWB_Warper_base::getClip( VWB_VEC3f const& e, VWB_float * pClip )
{
	// x points right and y points up in NDC, same goes for input value "e"
	// z is depending on handedness, LH z points forward, RH rearward
	// screenDist is always positive, so we need to take handedness into account
	VWB_float dd = nearDist / ( screenDist + ( m_bRH ? -e.z : e.z ) ); 
	pClip[0] = ( m_viewSizes[0] - e.x ) * dd; // left
	pClip[1] = ( m_viewSizes[1] + e.y ) * dd; // top
	pClip[2] = ( m_viewSizes[2] + e.x ) * dd; // right
	pClip[3] = ( m_viewSizes[3] - e.y ) * dd; // bottom
	pClip[4] = nearDist;
	pClip[5] = farDist;
}

void VWB_Warper_base::Defaults()
{
	size_t sz1 = sizeof( VWB_Warper );
	size_t sz2 = sizeof( VWB_Warper_base );
	::memset( &this->path, 0, sz1 );
	strcpy_s( calibFile, "vioso.vwf" );
	strcpy_s( channel, "channel 1" );
	strcpy_s( addr, "0.0.0.0" );
	nearDist = 0.125f;
	farDist = 20000.0f;
	fov[0]=35;
	fov[1]=30;
	fov[2]=35;
	fov[3]=30;
	trans[0] = 1;
	trans[5] = 1;
	trans[10] = 1;
	trans[15] = 1;
	screenDist = 1;
	autoViewC = 1;
	calibIndex = -1;
	bAutoView = true;
	gamma = 1;
	D3D12RTVF = 28;
	blackScale = 1.0f;
	inputGamma = 2.2f;
	blackDarkAdjust = 0.5f;
	blackBrightAdjust = 0.5f;
}

VWB_ERROR VWB_Warper_base::AutoView( VWB_WarpBlend const& wb )
{
	// test if blend2
	if( !( FLAG_WARPFILE_HEADER_BLENDV2 & wb.header.flags ) )
		return VWB_ERROR_PARAMETER;

	// check base matrix, if left or right handed...
	// global base matrix
	VWB_MAT44d B = VWB_MAT44d( VWB_MAT44f::ptr(trans) );
	VWB_MAT44d Bi = B.Inverted();

	// find x and y axis from scan
	// find corners
	VWB_WarpRecord* pW = wb.pWarp;
	VWB_BlendRecord2* pB = wb.pBlend2;
	int wh = wb.header.width / 2;
	int hh = wb.header.height / 2;
	VWB_WarpRecord *ptl = NULL, *ptr = NULL, *pbl = NULL, *pbr = NULL; // the extremal corners
	int ltl = INT_MAX, ltr = INT_MIN, lbl = INT_MAX, lbr = INT_MIN; // the extremal corner's distance to projector centre on projector
	#ifdef _DEBUG
	struct ImageCorners
	{
		struct px {
			int x, y;
		} tl, tr, bl, br;
	} imgCrn;
	#endif
	for( int y = hh - wb.header.height; y != hh; y++ )
	{
		for( int x = wh - wb.header.width; x != wh; x++, pW++, pB++ )
		{
			if( 1 == pW->w && // map contains valid value
				0 != pB->a && // not masked
				0 != ( pB->r + pB->g + pB->b ) && // not entirely blended black
				( 0 != pW->x || 0 != pW->y || 0 != pW->z ) ) // we skip (0,0,0)
			{
				//int sq = x * x + y * y;
				int d1 = x + y;
				int d2 = x - y;

				// top left
				if( d1 < ltl )
				{
					ltl = d1;
					ptl = pW;
					#ifdef _DEBUG
					imgCrn.tl.x = x;
					imgCrn.tl.y = y;
					#endif
				}

				// top right
				if( d2 > ltr )
				{
					ltr = d2;
					ptr = pW;
					#ifdef _DEBUG
					imgCrn.tr.x = x;
					imgCrn.tr.y = y;
					#endif
				}

				// bottom left
				if( d2 < lbl )
				{
					lbl = d2;
					pbl = pW;
					#ifdef _DEBUG
					imgCrn.bl.x = x;
					imgCrn.bl.y = y;
					#endif
				}

				//bottom right
				if( d1 > lbr )
				{
					lbr = d1;
					pbr = pW;
					#ifdef _DEBUG
					imgCrn.br.x = x;
					imgCrn.br.y = y;
					#endif
				}
			}
		}
	}


	//TODO reduce the chance of picking an outlier by using corrected average.
	// but the chance of an outlier is really low, because the data is adjusted by the Calibrator
	if( NULL == ptl || NULL == ptr || NULL == pbl || NULL == pbr )
	{
		logStr( 1, "WARINIG: AutoView cannot find corners of screen.\n" );
		return VWB_ERROR_GENERIC;
	}
	else
	{
		logStr( 2, "INFO: AutoView mapping display corners:\n tl(%.4f,%.4f,%.4f)\n tr(%.4f,%.4f,%.4f)\n bl(%.4f,%.4f,%.4f)\n br(%.4f,%.4f,%.4f).\n", 
				ptl->x, ptl->y, ptl->z,
				ptr->x, ptr->y, ptr->z,
				pbl->x, pbl->y, pbl->z,
				pbr->x, pbr->y, pbr->z );
	}

	// use corners to calculate 
	// use mid points and transform to IG coordinates
	VWB_VEC3d dx,dy,dtl,dtr,dbl,dbr; // the main coordinate axes to-be.

	// calculate the display corners in host coordinates
	dtl = VWB_VEC3d( VWB_VEC3f::ptr((VWB_float*)ptl) );
	dtr = VWB_VEC3d( VWB_VEC3f::ptr((VWB_float*)ptr) );
	dbl = VWB_VEC3d( VWB_VEC3f::ptr((VWB_float*)pbl) );
	dbr = VWB_VEC3d( VWB_VEC3f::ptr((VWB_float*)pbr) );
	dtl = Bi * dtl;
	dtr = Bi * dtr;
	dbr = Bi * dbr;
	dbl = Bi * dbl;

	dx = dtr + dbr - dtl - dbl;
	dy = dtl + dtr - dbl - dbr;
	
	// calculate display local base matrix to IG coordinates, this is the rotation from (0,0,0) looking to +/-z (depending on handedness) with y up, to look at the display, as it would be in the virtual world 
	VWB_MAT33d M = VWB_MAT33d::Base( dx, dy ); // this will always create a right-handed base with normalized vectors
	VWB_MAT44d T = VWB_MAT44d(M) * Bi; // T contains now a transformation from VIOSO coordinates to a display local coordinate system
	{
		VWB_VEC3d c = ( dtl + dtr + dbl + dbr ) / 4; // the centre of the view plane in IG coordinates
		VWB_VEC3d cL = M * c; // centre in display local coordinates
		screenDist = VWB_float( cL.z ); // IG (0,0,0) to plane distance
		if( m_bRH ) // turn screenDist positive, in case we are right-handed, as z points backwards
			screenDist *= -1;
	}

	logStr( 3,
			"View:\n"
			"[%.6f, %.6f, %.6f, %.6f ]\n"
			"[%.6f, %.6f, %.6f, %.6f]\n"
			"[%.6f, %.6f, %.6f, %.6f]\n"
			"[%.6f, %.6f, %.6f, %.6f]\n",
			M._11, M._12, M._13, 0.0,
			M._21, M._22, M._23, 0.0,
			M._31, M._32, M._33, 0.0,
			0.0, 0.0, 0.0, 1.0 );

	VWB_VEC3f::ptr( dir ) = VWB_VEC3f( ( m_bRH ? M.GetR() : -M.GetR() ) * ( 180.0 / M_PI ) );

	// caclulate FoVs
	double minDx = FLT_MAX, minDy = FLT_MAX;  // minimal horizontal and vertical projected distance on render plane, for quality purposes
	double maxL = FLT_MAX, maxT = -FLT_MAX, maxR = -FLT_MAX, maxB = FLT_MAX;  // maximum horizontal and vertical view size, left top right bottom
	double maxEL = FLT_MAX, maxET = -FLT_MAX, maxER = -FLT_MAX, maxEB = FLT_MAX;  // maximum horizontal and vertical view size, left top right bottom throughout moving space

	// now we get the corners of a box in that coordinate system
	VWB_BOXd b( VWB_VEC3d(DBL_MAX, DBL_MAX, DBL_MAX), VWB_VEC3d(-DBL_MAX, -DBL_MAX, -DBL_MAX) );

	pW = wb.pWarp;
	pB = wb.pBlend2;
	size_t sz = ptrdiff_t( wb.header.width ) * wb.header.height;
	// double l = autoViewC * screenDist / 4; optimized (we devide everything by screenDist and multiply the final result):
	double l = autoViewC / 4;

	// the distance maps and iterators
	vector<double> distanceMapX( wb.header.width* wb.header.height, 0 );
	auto distX = distanceMapX.begin();
	vector<double> distanceMapY( wb.header.width* wb.header.height, 0 );
	auto distY = distanceMapY.begin();

	// find the clip planes
	vector<VWB_VEC3d> prevL(ptrdiff_t( wb.header.width )); // for optimization, we keep the transformed values of the previous line of the mapping
	for( auto& v : prevL )
		v = VWB_VEC3d::O(); // set to all zero
	// start iterating through mapping
	for( VWB_WarpRecord const* pWE = pW + sz; pW != pWE; )
	{
		VWB_VEC3d prev = VWB_VEC3d::O(); // for optimization, we also keep previous transformed value
		auto prevT = prevL.begin(); // reset to start
		// start iterating through line
		for( VWB_WarpRecord const* pWLE = pW + wb.header.width; pW != pWLE; pW++, pB++, prevT++, distX++, distY++ )
		{
			if( 1 == pW->w && // map contains valid value
				0 != pB->a && // not masked
				0 != ( pB->r + pB->g + pB->b ) && // not entirely blended black
				( 0 != pW->x || 0 != pW->y || 0 != pW->z ) ) // we skip (0,0,0)
			{
				VWB_VEC3d v( pW->x, pW->y, pW->z); // this is the 3D VIOSO coordinate
				VWB_VEC3d vTT = T * v; // transform to display local coordinate

				// in case we have right-handed coordinate system
				// it will mirror forward direction thus toggle the sign on z to turn it positive
				if( m_bRH )
					vTT.z *= -1;

				// calculate deviation value depending on distance to screen plane
				// if a point sticks out from the screen plane, it can potentionally
				// move out of the minimal frustum, so we need to widen it.
				// using 1 as autoViewC allows for a moving volume of half the
				// screen (plane) distance
				//double dd = abs( l * screenDist / vTT.z - l );
				double dd = abs( l / vTT.z - l / screenDist );

				// project v to the local display's screen plane
				// this yields the a coordinate on a plane at z = screenDist, x right, y up
				//double vx = vTT.x * screenDist / vTT.z;
				//double vy = vTT.y * screenDist / vTT.z; optimized
				double vx = vTT.x / vTT.z;
				double vy = vTT.y / vTT.z;
				b += VWB_VEC3d( vx, vy, 1 ); // add to AABB

				if( maxL > vx ) // left, minimal x (x points right)
					maxL = vx;
				if( maxT < vy ) // top, maximal y (y points up)
					maxT = vy;
				if( maxR < vx ) // right, maximal x
					maxR = vx;
				if( maxB > vy ) // bottom, minimal y
					maxB = vy;

				if( maxEL > vx - dd ) // left, minimal x (x points right)
					maxEL = vx - dd;
				if( maxET < vy + dd ) // top, maximal y (y points up)
					maxET = vy + dd;
				if( maxER < vx + dd) // right, maximal x
					maxER = vx + dd;
				if( maxEB > vy - dd) // bottom, minimal y
					maxEB = vy - dd;

				// sum up for x, if prev is valid
				if( 1 == prev.z )
				{ // previous value valid, go for horizontal
					*distX= abs( vx - prev.x );
				}
				if( 1 == prevT->z )
				{ // upper value is valid
					*distY = abs( vy - prevT->y );
				}
				*prevT = prev = VWB_VEC3d( vx, vy, 1 ); 
			}
			else
			{
				prev.z = prevT->z = 0; // mark invalid
			}
		}
	}

	if( FLT_MAX == maxEL || FLT_MAX == maxET || -FLT_MAX == maxER || -FLT_MAX == maxEB )
	{
		logStr( 1, "WARNING: AutoView cannot calculate FoVs.\n" );
		return VWB_ERROR_GENERIC;
	}

	// calculate FoVs
	double left =	b.vMin.x;
	double top =	b.vMax.y;
	double right =	b.vMax.x;
	double bottom = b.vMin.y;
	screenDist = abs( screenDist );

	fov[0] = VWB_float( RAD2DEG( atan( -maxEL ) ) );
	fov[1] = VWB_float( RAD2DEG( atan( maxET ) ) );
	fov[2] = VWB_float( RAD2DEG( atan( maxER ) ) );
	fov[3] = VWB_float( RAD2DEG( atan( -maxEB ) ) );

	// the border fit gives a hint about the quality of the screen plane; it shows how much the movement affects the FoVs.
	VWB_VEC4d borderFit( ( left - maxEL ) / ( maxER - maxEL ), ( maxET - top ) / ( maxET - maxEB ), ( maxER - right ) / ( maxER - maxEL ), ( bottom - maxEB ) / ( maxET - maxEB ) );

	{ // calculate optimal resolution
		
		double sum = 0; // sum of all valid projected mapping distances
		size_t num = 0; // counter
		
		for( auto x : distanceMapX )
		{
			if( 0 < x ) 
			{
				sum += x;
				num++;
			}
		}
		double avgX = sum / num;
		sum = 0; num = 0;
		for( auto y : distanceMapY )
		{
			if( 0 < y )
			{
				sum += y;
				num++;
			}
		}
		double avgY = sum / num;

		optimalRes.cx = VWB_int( ( maxR - maxL ) / avgX );
		optimalRes.cy = VWB_int( ( maxT - maxB ) / avgY );
		logStr( 1, "INFO: AutoView average resolution is %ix%i.", optimalRes.cx, optimalRes.cy );

		// now we search for lowest, but ignoring values, that would result in more than double resolution
		// 2 * width = ( maxR - maxL ) / capX
		// <=> capX = 0.5 * ( maxR - maxL ) / width
		double minX = DBL_MAX;
		double cap = 0.5 * ( maxR - maxL ) / wb.header.width;
		size_t droppedX = 0;
		for( auto x : distanceMapX )
		{
			if( cap <= x )
			{
				if( x < minX )
				{
					minX = x;
				}
			}
			else
				droppedX++;
		}

		double minY = DBL_MAX;
		cap = 0.5 * ( maxT - maxB ) / wb.header.height;
		size_t droppedY = 0;
		for( auto y : distanceMapY )
		{
			if( cap <= y )
			{
				if( y < minY )
				{
					minY = y;
				}
			}
			else
				droppedY++;
		}

		optimalRes.cx = VWB_int( ceil( ( maxR - maxL ) / minX ) );
		optimalRes.cy = VWB_int( ceil( ( maxT - maxB ) / minY ) );
		logStr( 1, "INFO: AutoView optimal resolution coverage is %i%%x%i%%.", 100 - droppedX * 100 / wb.header.height / wb.header.width, 100 - droppedY * 100 / wb.header.height / wb.header.width );
	}

	optimalRect.left = 0;
	optimalRect.top = 0;
	optimalRect.right = optimalRes.cx;
	optimalRect.bottom = optimalRes.cy;
	logStr( 1, "INFO: AutoView for channel [%s] yields:\n"
		"dir=[%.6f, %.6f, %.6f]\n"
		"fov=[%.6f, %.6f, %.6f, %.6f]\n"
		"screen=%.6f\n"
		"optimalRes=[%d, %d]\n"
		"optimalRect=[%d, %d, %d, %d]\n"
		"handedness=%s\n"
		"borderFitError=[%ipx, %ipx, %ipx, %ipx]\n",
		channel,
		dir[0],dir[1],dir[2],
		fov[0],fov[1],fov[2],fov[3],
		screenDist,
		optimalRes.cx, optimalRes.cy,
		optimalRect.left, optimalRect.top, optimalRect.right, optimalRect.bottom,
		m_bRH ? "right" : "left",
		VWB_int( borderFit.x * wb.header.width ), VWB_int( borderFit.y * wb.header.height ), VWB_int( borderFit.z * wb.header.width ), VWB_int( borderFit.w * wb.header.height ) );
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::FixWraparound( VWB_WarpBlend& wb )
{
	VWB_WarpRecord* pW = wb.pWarp;

	// check for wrap-arounds
	typedef struct { long x, y; float xF, yF; float vLTRB[4]; } SSPCoord;

	size_t qCand;
	vector<int32_t> rgn;
	long i, k, kS, kE, l, lS, lE;
	VWB_WarpRecord const* pWR, * pWL;
	float xF, yF, * pMain, * pTest;
	int32_t* pR, * pRS, * pRR, * pRL;
	std::vector<SSPCoord> lCand;
	const long _Width = wb.header.width;
	const long _Height = wb.header.height;
	const long qPx = _Width * _Height;
	const long lastX = _Width - 1;
	const long lastY = _Height - 1;
	VWB_WarpRecord* pWS = pW;
	VWB_WarpRecord const* pWE = pWS + qPx;
	long qRgn = 0;

	try
	{
		rgn.resize( qPx );
		lCand.resize( ( size_t )qPx );
	}
	catch( ... )
	{
		return VWB_ERROR_GENERIC;
	}

	pR = rgn.data();
	pRS = pR;

	do
	{
		for( ; ( pW < pWE ) && ( ( pW->z <= 0.5f ) || ( *pR > 0 ) ); pW++, pR++ );
		if( pW >= pWE )
			break;

		SSPCoord& c = lCand[0];

		i = ( long )( pW - pWS );
		c.y = i / _Width;
		c.x = i % _Width;
		c.xF = pW->x;
		c.yF = pW->y;
		qCand = 1;
		*pR = ++qRgn;

		do
		{
			SSPCoord& c = lCand[--qCand];

			kS = MAX( c.y - 1, 0 );
			kE = MIN( c.y + 1, lastY );
			lS = MAX( c.x - 1, 0 );
			lE = MIN( c.x + 1, lastX );
			xF = c.xF;
			yF = c.yF;
			i = kS * _Width + lS;

			for( pRL = pRS + i, pWL = pWS + i, k = kS; k <= kE; k++, pWL += _Width, pRL += _Width )
				for( pRR = pRL, pWR = pWL, l = lS; l <= lE; l++, pWR++, pRR++ )
					if( ( pWR->z > 0.5f ) && ( *pRR == 0 ) &&
						( ::fabsf( xF - pWR->x ) <= 0.5f ) && ( ::fabsf( yF - pWR->y ) <= 0.5f )
						)
					{
						SSPCoord& c = lCand[qCand++];

						c.y = k;
						c.x = l;
						c.xF = pWR->x;
						c.yF = pWR->y;
						*pRR = qRgn;
					}
		} while( qCand );

	} while( 1 );

	if( qRgn == 0 )
	{
		logStr( 1, "WARINIG: FixWraparound cannot find any region. Is this an empty map?\n" );
		return VWB_ERROR_GENERIC;
	}

	if( qRgn == 1 )
	{
		logStr( 2, "NOTE: FixWraparound found one region. No seam detected - nothing to do.\n" );
		return VWB_ERROR_NONE;
	}

	logStr( 2, "NOTE: FixWraparound found %d regions. Wrapping UVs.\n", qRgn );
	for( i = 1; i <= qRgn; i++ )
		lCand[i].x = 0;

	for( pR = pRS, pW = pWS; pW < pWE; pW++, pR++ )
		if( *pR > 0 )
		{
			SSPCoord& c = lCand[*pR];

			if( c.x == 0 )
			{
				c.vLTRB[0] = c.vLTRB[2] = pW->x;
				c.vLTRB[1] = c.vLTRB[3] = pW->y;
			}
			else
			{
				xF = pW->x;
				if( xF < c.vLTRB[0] )
					c.vLTRB[0] = xF;
				else if( xF > c.vLTRB[2] )
					c.vLTRB[2] = xF;
				yF = pW->y;
				if( yF < c.vLTRB[1] )
					c.vLTRB[1] = yF;
				else if( yF > c.vLTRB[3] )
					c.vLTRB[3] = yF;
			}
			c.x++;
		}

	k = 1;
	kS = lCand[1].x;
	for( i = 2; i <= qRgn; i++ )
		if( lCand[i].x > kS )
		{
			k = i;
			kS = lCand[i].x;
		}

	pMain = lCand[k].vLTRB;
	for( i = 1; i <= qRgn; i++ )
		if( i != k )
		{
			pTest = lCand[i].vLTRB;

			if( ( pMain[0] - ( pTest[2] - 1.0f ) ) < ( pTest[0] - pMain[2] ) )
				lCand[i].xF = -1.0f;
			else if( ( ( pTest[0] + 1.0f ) - pMain[2] ) < ( pMain[0] - pTest[2] ) )
				lCand[i].xF = 1.0f;
			else
				lCand[i].xF = 0.0f;

			if( ( pMain[1] - ( pTest[3] - 1.0f ) ) < ( pTest[1] - pMain[3] ) )
				lCand[i].yF = -1.0f;
			else if( ( ( pTest[1] + 1.0f ) - pMain[3] ) < ( pMain[1] - pTest[3] ) )
				lCand[i].yF = 1.0f;
			else
				lCand[i].yF = 0.0f;
		}
		else
		{
			lCand[i].xF = 0.0f;
			lCand[i].yF = 0.0f;
		}

	for( pR = pRS, pW = pWS; pW < pWE; pW++, pR++ )
		if( *pR > 0 )
		{
			SSPCoord& c = lCand[*pR];

			pW->x += c.xF;
			pW->y += c.yF;
		}

	{
		// we need to recalculate...
		float minU = FLT_MAX, minV = FLT_MAX;
		float maxU = -FLT_MAX, maxV = -FLT_MAX;
		for( auto const* p = wb.pWarp, *pE = p + wb.header.width * wb.header.height; p != pE; p++ )
		{
			if( 0.5f < p->z )
			{
				if( minU > p->x )
					minU = p->x;
				if( maxU < p->x )
					maxU = p->x;
				if( minV > p->y )
					minV = p->y;
				if( maxV < p->y )
					maxV = p->y;
			}
		}
		if( minU != FLT_MAX && minV != FLT_MAX &&
			maxU != -FLT_MAX && maxV != -FLT_MAX )
		{
			if( wb.header.vCntDispPx[4] || wb.header.vCntDispPx[4] ) { // if we got a content size, we keep it. Just recalculating the partial rect
				auto fX = 1.0f / ( wb.header.vCntDispPx[4] * wb.header.width * ( maxU - minU ) );
				auto fY = 1.0f / ( wb.header.vCntDispPx[5] * wb.header.height * ( maxV - minV ) );
				wb.header.vPartialCnt[0] = minU * fX;
				wb.header.vPartialCnt[1] = minV * fY;
				wb.header.vPartialCnt[2] = maxU * fX;
				wb.header.vPartialCnt[3] = maxV * fY;
			} else {
				wb.header.vCntDispPx[4] = ( maxU - minU ) / wb.header.width;
				wb.header.vCntDispPx[5] = ( maxV - minV ) / wb.header.height;
				wb.header.vPartialCnt[0] = minU;
				wb.header.vPartialCnt[1] = minV;
				wb.header.vPartialCnt[2] = maxU;
				wb.header.vPartialCnt[3] = maxV;
				logStr( 2, "FixWraparound calculated new content size in wrap mode:\n"
						"  optimalRes: [%d,%d]\n",
						int( 1.0f / wb.header.vCntDispPx[4] ), int( 1.0f / wb.header.vCntDispPx[4] ) );
			}
		}
		else
		{
			logStr( 1, "WARINIG: FixWraparound cannot find min-max bound of screen.\n" );
			return VWB_ERROR_GENERIC;
		}
	}

	logStr( 2, "FixWraparound calculated new bounds in wrap mode:\n"
			" optimalRect: [%d,%d,%d,%d]\n",
			int( wb.header.vPartialCnt[0] / wb.header.vCntDispPx[4] ),
			int( wb.header.vPartialCnt[1] / wb.header.vCntDispPx[5] ),
			int( wb.header.vPartialCnt[2] / wb.header.vCntDispPx[4] ),
			int( wb.header.vPartialCnt[3] / wb.header.vCntDispPx[5] )
	);

	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::Render( VWB_param inputTexture, VWB_uint stateMask )
{
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::getWarpBlend( VWB_WarpBlend const*& wb )
{
	return VWB_ERROR_NOT_IMPLEMENTED;
}

VWB_ERROR VWB_Warper_base::getShaderVPMatrix( VWB_float* pMVP )
{
	if( nullptr == pMVP )
		return VWB_ERROR_PARAMETER;
	memcpy( pMVP, &m_mVP._11, sizeof( m_mVP ) );
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::getWarpMesh( VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh )
{
	return VWB_ERROR_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////////

Dummywarper::Dummywarper(void)
	: VWB_Warper_base()
{
	memset( &m_wb, 0, sizeof( m_wb ) );
	m_type4cc = 'MMUD';
}

Dummywarper::~Dummywarper(void)
{
	DeleteVWF( m_wb );
}

VWB_ERROR Dummywarper::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = VWB_Warper_base::Init(wbs);
	if( VWB_ERROR_NONE == err )
	{
		// do a deep copy of the mappings, to keep
		m_wb = *wbs[calibIndex];
		int nRecords = m_wb.header.width * m_wb.header.height;
		if( m_wb.pWarp )
		{
			m_wb.pWarp = new VWB_WarpRecord[nRecords];
			memcpy( m_wb.pWarp, wbs[calibIndex]->pWarp, nRecords * sizeof( VWB_WarpRecord ) );
		}
		if( m_wb.pBlend2 )
		{
			m_wb.pBlend2 = new VWB_BlendRecord2[nRecords];
			memcpy( m_wb.pBlend2, wbs[calibIndex]->pBlend2, nRecords* sizeof( VWB_BlendRecord2 ) );
		}
		if( m_wb.pBlack )
		{
			m_wb.pBlack = new VWB_BlendRecord[nRecords];
			memcpy( m_wb.pBlack, wbs[calibIndex]->pBlack, nRecords * sizeof( VWB_BlendRecord ) );
		}
		if( m_wb.pWhite )
		{
			m_wb.pWhite = new VWB_BlendRecord[nRecords];
			memcpy( m_wb.pWhite, wbs[calibIndex]->pWhite, nRecords * sizeof( VWB_BlendRecord ) );
		}
	}
	return err;
}


inline VWB_MAT44f Dummywarper::UpdateView( VWB_MAT44f const& igView, VWB_VEC3f& e )
{
	VWB_MAT44f V; // return value

	// rotation matrix from angles
	VWB_MAT44f R;
	if( m_bRH )
		R = VWB_MAT44f::R( (VWB_float)m_ep.pitch, (VWB_float)m_ep.yaw, (VWB_float)m_ep.roll );
	else
		R = VWB_MAT44f::R_LH( (VWB_float)m_ep.pitch, (VWB_float)m_ep.yaw, (VWB_float)m_ep.roll );

	// reverse eye vector
	e = VWB_VEC3f( -(float)m_ep.x, -(float)m_ep.y, -(float)m_ep.z );

	// add platform-rotated eye offset
	if( 0 != this->eye[0] || 0 != this->eye[1] || 0 != this->eye[2] )
		e -= R * VWB_VEC3f::ptr( this->eye );

	// translate to local coordinates
	e = igView * e;

	VWB_MAT44f T = VWB_MAT44f::T( e );
	m_mVP = T * igView * m_mBaseI;

	if( bTurnWithView )
		V = igView * R;
	else
		V = T * igView;

	return V;
}

// set up the view matrix,
// use the same matrices as in your program, construct a view matrix relative to the actual screen
// use the same units (usually millimeters) for the screen and the scene
VWB_ERROR Dummywarper::GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj )
{
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret )
	{
		VWB_MAT44f P; // the projection matrix to return 

		VWB_VEC3f e;
		VWB_MAT44f V = UpdateView( m_mViewIG, e );

		VWB_float clip[6];
		getClip( e, clip );

		if( m_bRH )
			P = VWB_MAT44f::GLFrustumRH( clip );
		else
			P = VWB_MAT44f::GLFrustumLH( clip );

		m_mVP = P * m_mVP;

		if( pView )
		{
			V.Transposed().SetPtr( pView );
		}
		if( pProj )
		{
			P.Transposed().SetPtr( pProj );
		}
	}
	return ret;
}

// set up the view matrix,
// use the same matrices as in your program, construct a view matrix relative to the actual screen
// use the same units (usually millimeters) for the screen and the scene
VWB_ERROR Dummywarper::GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip )
{
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret )
	{
		VWB_MAT44f P;
		VWB_VEC3f e;
		VWB_MAT44f V = UpdateView( m_mViewIG, e );

		VWB_float clip[6];
		getClip( e, clip );

		if( m_bRH )
			P = VWB_MAT44f::GLFrustumRH( clip );
		else
			P = VWB_MAT44f::GLFrustumLH( clip );

		m_mVP = P * m_mVP;

		if( pView )
		{
			V.Transposed().SetPtr( pView );
		}

		if( pClip )
		{
			memcpy( pClip, clip, sizeof( clip ) );
		}
	}
	return ret;
}

VWB_ERROR Dummywarper::GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric, VWB_float aspect )
{
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret )
	{

		VWB_MAT44f P;
		VWB_VEC3f e;

		VWB_MAT44f V = UpdateView( m_mViewIG, e );

		VWB_float clip[6];
		getClip( e, clip );

		if( symmetric )
		{
			VWB_MAT44f ig = m_mViewIG;
			if( m_bRH )
				MakeSymmetricRH( ig, clip );
			else
				MakeSymmetricLH( ig, clip );
			V = UpdateView( ig, e );
		}

		if( pDir )
		{
			// extract rotation angles from upper View matrix
			VWB_VEC3f::ptr( pDir ) = V.Upper().GetR();
			if( !m_bRH )
				VWB_VEC3f::ptr( pDir ) *= -1;
		}

		if( pPos )
		{
			pPos[0] = V._14;
			pPos[1] = V._24;
			pPos[2] = V._34;
		}

		if( 0 != aspect )
		{
			VWB_float a = ( clip[0] + clip[2] ) / ( clip[1] + clip[3] );
			if( aspect > a ) // we need to make frustum wider
			{
				a = aspect / a;
				clip[0] *= a;
				clip[2] *= a;
			}
			else // we need to make frustum higher
			{
				a /= aspect;
				clip[1] *= a;
				clip[3] *= a;
			}
		}

		if( m_bRH )
			P = VWB_MAT44f::GLFrustumRH( clip );
		else
			P = VWB_MAT44f::GLFrustumLH( clip );

		m_mVP = P * m_mVP;

		if( pClip )
		{
			memcpy( pClip, clip, sizeof( clip ) );
		}
	}

	return ret;
}

VWB_ERROR Dummywarper::SetViewProjection( VWB_float const* pView, VWB_float const* pProj )
{
	if( pView && pProj )
		m_mVP = VWB_MAT44f::ptr( pProj ) * VWB_MAT44f::ptr( pView ) * m_mBaseI;
	else
		return VWB_ERROR_PARAMETER;
	return VWB_ERROR_NONE;
}

VWB_ERROR Dummywarper::Render( VWB_param inputTexture, VWB_uint stateMask )
{
	return VWB_ERROR_NOT_IMPLEMENTED;
}

_inline_ VWB_uint getTrianglePatch( 
	VWB_WarpBlend& out, VWB_float& x, VWB_float& y,
	VWB_WarpRecord* pW, VWB_BlendRecord2* pB, VWB_BlendRecord* pBl, VWB_BlendRecord* pWh,
	VWB_int dx1, VWB_int dy1, VWB_int dx2, VWB_int dy2 )
{
	// find uv coordinate range
	VWB_uint d1 = dx1 + out.header.width * dy1;
	VWB_uint d2 = dx2 + out.header.width * dy2;
	VWB_WarpRecord* pW1 = pW + d1;
	VWB_WarpRecord* pW2 = pW + d2;
	VWB_float uMax = pW->x, uMin = pW->x;
	VWB_float vMax = pW->y, vMin = pW->y;
	if( uMax < pW1->x )
		uMax = pW1->x;
	if( uMin > pW1->x )
		uMin = pW1->x;
	if( uMax < pW2->x )
		uMax = pW2->x;
	if( uMin > pW2->x )
		uMin = pW2->x;

	if( vMax < pW1->y )
		vMax = pW1->y;
	if( vMin > pW1->y )
		vMin = pW1->y;
	if( vMax < pW2->y )
		vMax = pW2->y;
	if( vMin > pW2->y )
		vMin = pW2->y;
	uMin = floor( uMin * out.header.width  ) + 0.5f;
	vMin = floor( vMin * out.header.height ) + 0.5f;
	uMax = floor( uMax * out.header.width  ) + 0.5f;
	vMax = floor( vMax * out.header.height ) + 0.5f;
	VWB_uint conv = 0;
	for( VWB_float v = vMin; v <= vMax; v++ )
	{
		for( VWB_float u = uMin; u <= uMax; u++ )
		{
			VWB_VEC3f l;
			if( VWB_VEC3f::Cart2Bary( VWB_VEC3f::ptr( &pW->x ), VWB_VEC3f::ptr( &pW1->x ), VWB_VEC3f::ptr( &pW2->x ), VWB_VEC3f(u/out.header.width,v/out.header.height,1), l ) )
			{
				VWB_uint o = VWB_int(u)+out.header.width*VWB_int(v);
				VWB_VEC3f::Bary2Cart( VWB_VEC3f( x,y,1 ), VWB_VEC3f( x+dx1,y+dy1,1 ), VWB_VEC3f( x+dx2,y+dy2,1 ), l, VWB_VEC3f::ptr(&out.pWarp[o].x) );
				if( pB && out.pBlend2 )
				{
					out.pBlend2[o].r = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->r + l.y * ( pB + d1 )->r + l.z * ( pB + d2 )->r ));
					out.pBlend2[o].g = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->g + l.y * ( pB + d1 )->g + l.z * ( pB + d2 )->g ));
					out.pBlend2[o].b = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->b + l.y * ( pB + d1 )->b + l.z * ( pB + d2 )->b ));
					out.pBlend2[o].a = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->a + l.y * ( pB + d1 )->a + l.z * ( pB + d2 )->a ));
				}
				if( pBl && out.pBlack )
				{
					out.pBlack[o].r = (VWB_byte)MIN( 255, MAX( 0, l.x * pBl->r + l.y * (pBl + d1)->r + l.z * (pBl + d2)->r ) );
					out.pBlack[o].g = (VWB_byte)MIN( 255, MAX( 0, l.x * pBl->g + l.y * (pBl + d1)->g + l.z * (pBl + d2)->g ) );
					out.pBlack[o].b = (VWB_byte)MIN( 255, MAX( 0, l.x * pBl->b + l.y * (pBl + d1)->b + l.z * (pBl + d2)->b ) );
					out.pBlack[o].a = (VWB_byte)MIN( 255, MAX( 0, l.x * pBl->a + l.y * (pBl + d1)->a + l.z * (pBl + d2)->a ) );
				}				
				if( pWh && out.pWhite )
				{				
					out.pWhite[o].r = (VWB_byte)MIN( 255, MAX( 0, l.x * pWh->r + l.y * (pWh + d1)->r + l.z * (pWh + d2)->r ) );
					out.pWhite[o].g = (VWB_byte)MIN( 255, MAX( 0, l.x * pWh->g + l.y * (pWh + d1)->g + l.z * (pWh + d2)->g ) );
					out.pWhite[o].b = (VWB_byte)MIN( 255, MAX( 0, l.x * pWh->b + l.y * (pWh + d1)->b + l.z * (pWh + d2)->b ) );
					out.pWhite[o].a = (VWB_byte)MIN( 255, MAX( 0, l.x * pWh->a + l.y * (pWh + d1)->a + l.z * (pWh + d2)->a ) );
				}
				conv++;
			}
		}
	}
	return conv;
}

_inline_ VWB_uint getTrianglePatchA( 
	VWB_WarpBlend& out, VWB_float& x, VWB_float& y,
	VWB_WarpRecord* pW, VWB_BlendRecord2* pB, VWB_BlendRecord* pBl, VWB_BlendRecord* pWh,
	VWB_int dx1, VWB_int dy1, VWB_int dx2, VWB_int dy2 )
{
	// find uv coordinate range
	VWB_uint d1 = dx1 + out.header.width * dy1;
	VWB_uint d2 = dx2 + out.header.width * dy2;
	VWB_WarpRecord* pW1 = pW + d1;
	VWB_WarpRecord* pW2 = pW + d2;
	VWB_float uMax = pW->x, uMin = pW->x;
	VWB_float vMax = pW->y, vMin = pW->y;
	if( uMax < pW1->x )
		uMax = pW1->x;
	if( uMin > pW1->x )
		uMin = pW1->x;
	if( uMax < pW2->x )
		uMax = pW2->x;
	if( uMin > pW2->x )
		uMin = pW2->x;

	if( vMax < pW1->y )
		vMax = pW1->y;
	if( vMin > pW1->y )
		vMin = pW1->y;
	if( vMax < pW2->y )
		vMax = pW2->y;
	if( vMin > pW2->y )
		vMin = pW2->y;
	uMin = floor( uMin * out.header.width  ) + 0.5f;
	vMin = floor( vMin * out.header.height ) + 0.5f;
	uMax = floor( uMax * out.header.width  ) + 0.5f;
	vMax = floor( vMax * out.header.height ) + 0.5f;
	if( 5 < uMax - uMin || 5 < vMax - vMin )
	{
		int i = 0;
	}
	VWB_uint conv = 0;
	for( VWB_float v = vMin; v <= vMax; v++ )
	{
		for( VWB_float u = uMin; u <= uMax; u++ )
		{
			VWB_VEC3f l;
			if( VWB_VEC3f::Cart2Bary( VWB_VEC3f::ptr( &pW->x ), VWB_VEC3f::ptr( &pW1->x ), VWB_VEC3f::ptr( &pW2->x ), VWB_VEC3f(u/out.header.width,v/out.header.height,1), l ) )
			{
				VWB_uint o = VWB_int(u)+out.header.width*VWB_int(v);
				VWB_VEC3f::Bary2Cart( VWB_VEC3f( x,y,1 ), VWB_VEC3f( x+1,y,1 ), VWB_VEC3f( x+1,y+1,1 ), l, VWB_VEC3f::ptr(&out.pWarp[o].x) );
				out.pBlend2[o].r = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->r + l.y * ( pB + d1 )->r + l.z * ( pB + d2 )->r ) );
				out.pBlend2[o].g = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->g + l.y * ( pB + d1 )->g + l.z * ( pB + d2 )->g ) );
				out.pBlend2[o].b = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->b + l.y * ( pB + d1 )->b + l.z * ( pB + d2 )->b ) );
				out.pBlend2[o].a = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->a + l.y * ( pB + d1 )->a + l.z * ( pB + d2 )->a ) );
				conv++;
			}
		}
	}
	return conv;
}

VWB_ERROR invertWB( VWB_WarpBlend const& in, VWB_WarpBlend& out )
{
	logStr( 2, "INFO: Calculate inverse maping of %s.\n", in.header.name );
	VWB_int const& w = in.header.width;
	VWB_int const& h = in.header.height;
	VWB_int const nRecords = w * h;

	out = in;
	if( in.pWarp )
	{
		out.pWarp = new VWB_WarpRecord[nRecords];
		memset( out.pWarp, 0, nRecords * sizeof( VWB_WarpRecord ) );
	}
	else
		return VWB_ERROR_PARAMETER;

	if( out.pBlend2 )
	{
		out.pBlend2 = new VWB_BlendRecord2[nRecords];
		memset( out.pBlend2, 0, nRecords* sizeof( VWB_BlendRecord2 ) );
	}
	if( out.pBlack )
	{
		out.pBlack = new VWB_BlendRecord[nRecords];
		memset( out.pBlack, 0, nRecords * sizeof( VWB_BlendRecord ) );
	}
	if( out.pWhite )
	{
		out.pWhite = new VWB_BlendRecord[nRecords];
		memset( out.pWhite, 0, nRecords * sizeof( VWB_BlendRecord ) );
	}

	// fill from triangles...
	VWB_uint conv = 0;
	VWB_WarpRecord   *pW  = in.pWarp  ;
	VWB_BlendRecord2 *pB  = in.pBlend2;
	VWB_BlendRecord* pBl = in.pBlack;
	VWB_BlendRecord* pWh = in.pWhite;
	VWB_WarpRecord const* pWE = pW + nRecords - w;


// if there was no triangle on the left, try point at lower left
// P5    P0
//     / | 
//    /  | 
//   / A | 
// P4----P3
//
// normal patch, P1 optional
// P0---(P1)
// | \ C |
// |  \  |
// | B \ |
// P3----P2
//
// if P2 is missing triangulate other way
// P0----P1
// |    /
// | D /
// |  /
// P3
//
	if( out.pBlack || out.pWhite || !out.pBlend2 )
	{
		for( VWB_float y = 0; pWE != pW; pW++, pB++, pBl++, pWh++, y++ )
		{
			VWB_WarpRecord const *pWLE = pW + w - 1;
			for( VWB_float x = 0; pWLE != pW; pW++, pB++, pBl++, pWh++, x++ )
			{
				if( 0.5f < pW->z )
				{
					VWB_WarpRecord* pW3 = pW+w;   // bottom

					// check for triangle A, this is only valid, if P5 is not valid
					if( 0 != x &&
						0.5f > (pW-1)->z &&
						0.5f < pW3->z )
					{
						VWB_WarpRecord* pW4 = pW+w-1;   // bottom-left
						if( 0.5f < pW4->z )
							conv+= getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 0, 1, -1, 1 );
					}

					VWB_WarpRecord* pW2 = pW+w+1; // bottomright
					VWB_WarpRecord* pW1 = pW+1; // right
					if( 0.5f < pW2->z )
					{
						if( 0.5f < pW1->z ) // Triangle A
							conv+= getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 1, 0, 1, 1 );

						if( 0.5f < pW3->z )  // Triangle B
							conv+= getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 1, 1, 0, 1 );
					}
					else
					{
						if( 0.5f < pW1->z && 0.5f < pW3->z ) // Triangle D
							conv+= getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 1, 0, 0, 1 );
					}
				}
			}
		}
	}
	else
	{
		for( VWB_float y = 0; pWE != pW; pW++, pB++, y++ )
		{
			VWB_WarpRecord const *pWLE = pW + w - 1;
			for( VWB_float x = 0; pWLE != pW; pW++, pB++, x++ )
			{
				if( 0.5f < pW->z )
				{
					VWB_WarpRecord* pW3 = pW+w;   // bottom

					// check for triangle A, this is only valid, if P5 is not valid
					if( 0 != x &&
						0.5f > (pW-1)->z &&
						0.5f < pW3->z )
					{
						VWB_WarpRecord* pW4 = pW+w-1;   // bottom-left
						if( 0.5f < pW4->z )
							conv+= getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 0, 1, -1, 1 );
					}

					VWB_WarpRecord* pW2 = pW+w+1; // bottomright
					VWB_WarpRecord* pW1 = pW+1; // right
					if( 0.5f < pW2->z )
					{
						if( 0.5f < pW1->z ) // Triangle A
							conv+= getTrianglePatchA( out, x, y, pW, pB, pBl, pWh, 1, 0, 1, 1 );

						if( 0.5f < pW3->z )  // Triangle B
							conv+= getTrianglePatchA( out, x, y, pW, pB, pBl, pWh, 1, 1, 0, 1 );
					}
					else
					{
						if( 0.5f < pW1->z && 0.5f < pW3->z ) // Triangle D
							conv+= getTrianglePatchA( out, x, y, pW, pB, pBl, pWh, 1, 0, 0, 1 );
					}

				}
			}
		}
	}

	// fill holes
	VWB_WarpRecord   *pOutW  = out.pWarp   + w + 1;
	VWB_BlendRecord2 *pOutB  = out.pBlend2 + w + 1;
	VWB_BlendRecord  *pOutBl = out.pBlack  + w + 1;
	VWB_BlendRecord  *pOutWh = out.pWhite  + w + 1;
	for( VWB_WarpRecord const* pOutWE = pOutW + ptrdiff_t( nRecords ) - 2 * w; pOutWE != pOutW; pOutW+= 2, pOutB+= 2, pOutBl+= 2, pOutWh+= 2 )
	{
		for( VWB_WarpRecord const* pOutWLE = pOutW + w - 2; pOutWLE != pOutW; pOutW++, pOutB++, pOutBl++, pOutWh++ )
		{
			if( 0.5f > pOutW->z )
			{
				VWB_WarpRecord* pW1 = pOutW-1; // left
				VWB_WarpRecord* pW2 = pOutW+1; // right
				VWB_WarpRecord* pW3 = pOutW-w; // top
				VWB_WarpRecord* pW4 = pOutW+w; // bottom
				if( 0.5f < pW1->z &&
					0.5f < pW2->z )
				{
					pOutW->x = ( pW1->x + pW2->x ) / 2;
					pOutW->y = ( pW1->y + pW2->y ) / 2;
					pOutW->z = ( pW1->z + pW2->z ) / 2;
					if( out.pBlend2 )
					{
						pOutB->r = (VWB_word)(( (VWB_int)(pOutB-1)->r + (VWB_int)(pOutB+1)->r ) / 2);
						pOutB->g = (VWB_word)(( (VWB_int)(pOutB-1)->g + (VWB_int)(pOutB+1)->g ) / 2);
						pOutB->b = (VWB_word)(( (VWB_int)(pOutB-1)->b + (VWB_int)(pOutB+1)->b ) / 2);
						pOutB->a = (VWB_word)(( (VWB_int)(pOutB-1)->a + (VWB_int)(pOutB+1)->a ) / 2);
					}
					conv++;
				}
				else if( 0.5f < pW3->z &&
					0.5f < pW4->z )
				{
					pOutW->x = ( pW3->x + pW4->x ) / 2;
					pOutW->y = ( pW3->y + pW4->y ) / 2;
					pOutW->z = ( pW3->z + pW4->z ) / 2;
					if( out.pBlend2 )
					{
						pOutB->r = (VWB_word)( ( (VWB_int)( pOutB - w )->r + (VWB_int)( pOutB + w )->r ) / 2 );
						pOutB->g = (VWB_word)( ( (VWB_int)( pOutB - w )->g + (VWB_int)( pOutB + w )->g ) / 2 );
						pOutB->b = (VWB_word)( ( (VWB_int)( pOutB - w )->b + (VWB_int)( pOutB + w )->b ) / 2 );
						pOutB->a = (VWB_word)( ( (VWB_int)( pOutB - w )->a + (VWB_int)( pOutB + w )->a ) / 2 );
					}
					conv++;
				}
			}
		}
	}
	if( 64 <= conv )
		return VWB_ERROR_NONE;
	else
		return VWB_ERROR_FALSE;
}

/*
VWB_uint subMesh( VWB_WarpBlendMesh::idx_t& idx, VWB_WarpBlendMesh::idx_t& oldRef, VWB_WarpBlendMesh::idx_t& oldTrail, VWB_WarpBlendMesh::idx_t& newIdx, VWB_WarpBlendMesh::idx_t& newRef, VWB_WarpBlendMesh::idx_t& newTrail )
{
	if( 3 > oldRef.size() )
		return 0;

	//newIdx.clear();
	//newRef.clear();
	//newTrail.clear();
	newIdx.push_back( idx[oldRef[0] + oldTrail[0] ] ); 
	newIdx.push_back( idx[oldRef[0] + oldTrail[1] ] ); 
	newIdx.push_back( idx[oldRef[1] + oldTrail[2] ] ); 
	newRef.push_back( 0 );
	newTrail.push_back( 0 );
	newTrail.push_back( 2 );

	VWB_uint sz = (VWB_uint)oldTrail.size();
	VWB_uint sz3 = sz-2;
	for( VWB_uint i = 2; i < sz3; i++ )
	{
		newRef.push_back( i * 3 );
		newIdx.push_back( idx[oldRef[i++] + oldTrail[i] ] ); 
		newIdx.push_back( idx[oldRef[i++] + oldTrail[i] ] ); 
		newIdx.push_back( idx[oldRef[i++] + oldTrail[i] ] ); 
		newTrail.push_back( 2 );
	}
	newRef.push_back( sz * 3 );
	newIdx.push_back( idx[oldRef[sz-3] + oldTrail[sz-2] ] ); 
	newIdx.push_back( idx[oldRef[sz-2] + oldTrail[sz-1] ] ); 
	newIdx.push_back( idx[oldRef[0] + oldTrail[0] ] ); 

	if( 3 < sz )
	{
		VWB_WarpBlendMesh::idx_t nnIdx;
		VWB_WarpBlendMesh::idx_t nnRef;
		VWB_WarpBlendMesh::idx_t nnTrail;
		subMesh( newIdx, newRef, newTrail, nnIdx, nnRef, nnTrail ); 
	}
	return 0;
}
*/

typedef enum ESPUMA_CtrlPointUseFlag
{
	FLAG_CTRLPT_USE_UNSPECIFIC				=0x00,							///<   unspecific using
	FLAG_CTRLPT_USE_SELECTED					=0x01,							///<   point is selected
	FLAG_CTRLPT_USE_SELECTED_TANGENT			=0x02,							///<   a tangent to point is selected
	FLAG_CTRLPT_USE_SELECTED_TANGENT_SECOND	=0x04,							///<   second tangent point is selected
	FLAG_CTRLPT_USE_SELECTED_LINE_RIGHT		=0x08,							///<   line to the right is selected
	FLAG_CTRLPT_USE_SELECTED_LINE_BOTTOM		=0x10,							///<   line down is selected
	FLAG_CTRLPT_USE_SELECTED_ALL				=0x1F
}ESPUMA_CtrlPointUseFlag;

typedef enum ESPUMA_CtrlPointPosFlag
{
	FLAG_CTRLPT_POS_NORMAL=0x0,												///<   normal control point
	FLAG_CTRLPT_POS_UNKNOWN=0x1,												///<   indicates an control point with unknown position
	FLAG_CTRLPT_POS_BORDER_TOP=0x2,											///<   indicates that the control point is located on the top border of the control point mesh
	FLAG_CTRLPT_POS_BORDER_LEFT=0x4,											///<   indicates that the control point is located on the left border of the control point mesh
	FLAG_CTRLPT_POS_BORDER_RIGHT=0x8,										///<   indicates that the control point is located on the right border of the control point mesh
	FLAG_CTRLPT_POS_BORDER_BOTTOM=0x10,										///<   indicates that the control point is located on the bottom border of the control point mesh
	FLAG_CTRLPT_POS_BORDER_DIAGONAL=0x20,									///<   indicates that the control point is located on the diagonal border of the control point mesh
	FLAG_CTRLPT_POS_CLONE=0x40,												///<   indicates that the control point is only a clone of another point
	FLAG_CTRLPT_POS_BORDER= FLAG_CTRLPT_POS_BORDER_TOP |					///<   all border flags
								FLAG_CTRLPT_POS_BORDER_LEFT | 
								FLAG_CTRLPT_POS_BORDER_RIGHT | 
								FLAG_CTRLPT_POS_BORDER_BOTTOM | 
								FLAG_CTRLPT_POS_BORDER_DIAGONAL,
	FLAG_CTRLPT_POS_BORDER_LR= FLAG_CTRLPT_POS_BORDER_LEFT | 				///<   right and left border
									FLAG_CTRLPT_POS_BORDER_RIGHT,
	FLAG_CTRLPT_POS_TOPLEFT=		FLAG_CTRLPT_POS_BORDER_TOP |				///<   edge top left, indicates the origin of the mesh
									FLAG_CTRLPT_POS_BORDER_LEFT,
	FLAG_CTRLPT_POS_TOPRIGHT=	FLAG_CTRLPT_POS_BORDER_TOP |				///<   edge top right
									FLAG_CTRLPT_POS_BORDER_RIGHT,
	FLAG_CTRLPT_POS_BOTTOMLEFT=	FLAG_CTRLPT_POS_BORDER_BOTTOM |			///<   edge bottom left
									FLAG_CTRLPT_POS_BORDER_LEFT,
	FLAG_CTRLPT_POS_BOTTOMRIGHT= FLAG_CTRLPT_POS_BORDER_BOTTOM |			///<   edge bottom right
									FLAG_CTRLPT_POS_BORDER_RIGHT
}ESPUMA_CtrlPointPosFlag;

typedef struct SPPair3f
{
	int                                 fUse;									///<   common flag to indicate special using of the point
																				///<  @see ESPUMA_CtrlPointUseFlag
	int                                 fPos;									///<   flag to describe the position of the point in a mesh, if any
																				///<  @see ESPUMA_CtrlPointPosFlag
	float                               lPt1[3];								///<   first point pair
	float                               lPt2[3];								///<   second point pair
	float                               lTangDescX[2];							///<   parameter list to descibe an optional tangent of the point for extrapolation in x direction ([0] distance, [1] degree)
	float                               lTangDescY[2];							///<   parameter list to descibe an optional tangent of the point for extrapolation in y direction ([0] distance, [1] degree)
	static const SPPair3f _empty;
} SPPair3f;
const SPPair3f SPPair3f::_empty = SPPair3f{ 0,0,{0,0,0},{0,0,0},{1,0},{1,0} };

typedef std::vector<SPPair3f> DynSPPointPairList3f;
typedef std::vector<long> DynLongList;
typedef std::vector<long*> DynLongPtrList;
typedef std::vector<unsigned int> DynDWORDList;

/// @ brief triangulate grid
/// good points are marked: fUse & 1
/// 
///	@note
///			x1  x2
///		y1	p1--p2
///			| \ A|
///			| B\ |
///		y2  p4--p3
///	
///			x1  x2
///		y1	p1--p2
///			| C/ |
///			| / D|
///		y2  p4--p3
/// we assume CW culling
/// @param [IN] vertices a list of vertices
/// @param [IN] wGrid columns
/// @param [IN] hGrid rows
/// @param [OUT] indices the index list
/// @param [template] W wrangler class to access vertex attributes
/// @param [template] TP type of vertex
/// @param [template] TP type of index
/// @return number of triangles generated
template< class W, class TP, class TI >
size_t triangulateGrid( std::vector<TP> const& vertices, long wGrid, long hGrid, std::vector<TI>& indices )
{
	size_t nOld = indices.size();
	indices.reserve( nOld + 6 * wGrid * hGrid );

	for( long y = 1; y != hGrid; y++ )
	{
		for( long x = 1; x != wGrid; x++ )
		{
			long pt3 = y * wGrid + x;
			long pt2 = pt3 - wGrid;
			long pt4 = pt3 - 1;
			long pt1 = pt2 - 1;

			if( W::valid( vertices[pt1] ) )
			{ // pt1 valid
				if( W::valid( vertices[pt3] ) )
				{
					if( W::valid( vertices[pt4] ) )
					{
						// triangle B valid, add it
						indices.push_back( pt1 );
						indices.push_back( pt3 );
						indices.push_back( pt4 );
					}
					if( W::valid( vertices[pt2] ) )
					{
						// triangle A valid, add it
						indices.push_back( pt1 );
						indices.push_back( pt2 );
						indices.push_back( pt3 );
					}
				}
				else if(
					W::valid( vertices[pt2] ) &&
					W::valid( vertices[pt4] ) )
				{
					// tiangle C valid
					indices.push_back( pt1 );
					indices.push_back( pt2 );
					indices.push_back( pt4 );
				}
			}
			else if(
				W::valid( vertices[pt2] ) &&
				W::valid( vertices[pt3] ) &&
				W::valid( vertices[pt4] ) )
			{
				// tiangle D valid
				indices.push_back( pt2 );
				indices.push_back( pt3 );
				indices.push_back( pt4 );
			}
		}
	}
	return ( indices.size() - nOld ) / 3;
}

/// @brief Repairs a grid by extrapolation. Grid positions are unchanged, so it works for warpers that rely on a fixed grid alignment.
/// @param grid		list of vertices
/// @param wGrid	columns
/// @param hGrid	rows
/// @param dist		extrapolation distance. Number of iterations the grid is expanded. This can be -1 to fill whole grid.
/// @param [template] W wrangler class to access vertex attributes
/// @param [template] TP type of vertex
/// @return true in case of success, false otherwise
template< class W, class TP >
bool RepairUniformGrid( std::vector<TP>& grid, int wGrid, int hGrid, int dist )
{
	enum POS_REL {
		POS_REL_T, // tops
		POS_REL_TT,
		POS_REL_TTT,
		POS_REL_TR,  // top-rights
		POS_REL_TRTR,
		POS_REL_TRTRTR,
		POS_REL_R, // rights
		POS_REL_RR,
		POS_REL_RRR,
		POS_REL_BR, // bottom-rights
		POS_REL_BRBR,
		POS_REL_BRBRBR,
		POS_REL_B,  // bottoms
		POS_REL_BB,
		POS_REL_BBB,
		POS_REL_BL, // bottom-lefts
		POS_REL_BLBL,
		POS_REL_BLBLBL,
		POS_REL_L, // lefts
		POS_REL_LL,
		POS_REL_LLL,
		POS_REL_TL, // top-lefts
		POS_REL_TLTL,
		POS_REL_TLTLTL,
		POS_REL_SIZE
	};

	SIZE const delta[POS_REL_SIZE] = {
		{  0, -1 }, {  0, -2 }, {  0, -3 }, // top
		{  1, -1 }, {  2, -2 }, {  3, -3 }, // top-right
		{  1,  0 }, {  2,  0 }, {  3,  0 }, // right
		{  1,  1 }, {  2,  2 }, {  3,  3 }, // bottom-right
		{  0,  1 }, {  0,  2 }, {  0,  3 }, // bottom
		{ -1,  1 }, { -2,  2 }, { -3,  3 }, // bottom-left
		{ -1,  0 }, { -2,  0 }, { -3,  0 }, // left
		{ -1, -1 }, { -2, -2 }, { -3, -3 }, // top-left
	};

	typedef enum EST_TYPE {
		EST_TYPE_INTER,
		EST_TYPE_EXTRA,
	} EST_TYPE;

	struct Match {
		POS_REL pos[3];
		EST_TYPE est;
		float score;
	};

	Match const matches[] = { // 1.0 horizintal or vertical interpolation, 0.9 diagonal interpolation, 0.75 extrapolation h/v, 0.675 diagonal extrap.
		{ { POS_REL_LL, POS_REL_L, POS_REL_R }, EST_TYPE_INTER, 1.0f },
		{ { POS_REL_RR, POS_REL_R, POS_REL_L }, EST_TYPE_INTER, 1.0f },
		{ { POS_REL_TT, POS_REL_T, POS_REL_B }, EST_TYPE_INTER, 1.0f },
		{ { POS_REL_BB, POS_REL_B, POS_REL_T }, EST_TYPE_INTER, 1.0f },

		{ { POS_REL_TRTR, POS_REL_TR, POS_REL_BL }, EST_TYPE_INTER, 0.95f },
		{ { POS_REL_BRBR, POS_REL_BR, POS_REL_TL }, EST_TYPE_INTER, 0.95f },
		{ { POS_REL_BLBL, POS_REL_BL, POS_REL_TR }, EST_TYPE_INTER, 0.95f },
		{ { POS_REL_TLTL, POS_REL_TL, POS_REL_BR }, EST_TYPE_INTER, 0.95f },

		{ { POS_REL_TTT, POS_REL_TT, POS_REL_T }, EST_TYPE_EXTRA, 0.875f },
		{ { POS_REL_BBB, POS_REL_BB, POS_REL_B }, EST_TYPE_EXTRA, 0.875f },
		{ { POS_REL_LLL, POS_REL_LL, POS_REL_L }, EST_TYPE_EXTRA, 0.875f },
		{ { POS_REL_RRR, POS_REL_RR, POS_REL_R }, EST_TYPE_EXTRA, 0.875f },

		{ { POS_REL_TRTRTR, POS_REL_TRTR, POS_REL_TR }, EST_TYPE_EXTRA, 0.823f },
		{ { POS_REL_BRBRBR, POS_REL_BRBR, POS_REL_BR }, EST_TYPE_EXTRA, 0.823f },
		{ { POS_REL_BLBLBL, POS_REL_BLBL, POS_REL_BL }, EST_TYPE_EXTRA, 0.823f },
		{ { POS_REL_TLTLTL, POS_REL_TLTL, POS_REL_TL }, EST_TYPE_EXTRA, 0.823f },
	};

	DynSPPointPairList3f newGrid;
	int numOp = 0; // global iteration counter
	int inValid = 0;
	int added = 0;
	int removed = 0;
	int found = 0;
	size_t nSz = size_t(wGrid) * hGrid;

	newGrid.resize( nSz );
	do {
		if( -1 != dist && dist <= numOp )
			return true;
		numOp++;
		added = 0;
		found = 0;
		inValid = 0;
		#ifdef _DEBUG
		int	good = 0;
		#endif

		// do all 
		DynSPPointPairList3f::iterator pO = grid.begin();
		DynSPPointPairList3f::iterator pN = newGrid.begin();
		for( int y = 0; y != hGrid; y++ )
		{
			for( int x = 0; x != wGrid; x++, pO++, pN++ )
			{
				*pN = SPPair3f{ 0 };

				if( 0.5f <= pO->lPt2[2] )
					continue;

				int nMatches = 0;
				for( int i = 0; i != ARRAYSIZE( matches ); i++ )
				{
					DynSPPointPairList3f::const_iterator ppOO[3] = { grid.end() };
					for( int j = 0; j != 3; j++ )
					{
						if( x + delta[matches[i].pos[j]].cx < (LONG)wGrid && // not over right border
							x + delta[matches[i].pos[j]].cx >= 0 && // not over left border
							y + delta[matches[i].pos[j]].cy < (LONG)hGrid && // not over bottom border
							y + delta[matches[i].pos[j]].cy >= 0 ) // not over top border
						{
							ppOO[j] = grid.begin() + ( ( ptrdiff_t( y ) + delta[matches[i].pos[j]].cy ) * wGrid + x + delta[matches[i].pos[j]].cx );
							if( FLT_EPSILON > ppOO[j]->lPt2[2] )
							{
								ppOO[0] = grid.end();
								break;
							}
						}
						else
						{
							ppOO[0] = grid.end();
							break;
						}
					}
					if( grid.end() == ppOO[0] )
						continue;

					// we have a match
					float vE[6] = { 0 };
					if( EST_TYPE_INTER == matches[i].est )
					{
						// get 1/3-point vector between first and last point
						float vM[2] = { 
							ppOO[0]->lPt2[0] * 2.0f / 3.0f + ppOO[2]->lPt2[0] / 3.0f,
							ppOO[0]->lPt2[1] * 2.0f / 3.0f + ppOO[2]->lPt2[1] / 3.0f 
						};
						// get distance vector of second point to linear estimation if second point
						float vD[2] = { 
							ppOO[1]->lPt2[0] - vM[0],
							ppOO[1]->lPt2[1] - vM[1] 
						}; // 
						// estimate missing point on line from first and last, by adding the distance to mid-point (mirroring) and add correction
						vE[0] = ppOO[0]->lPt2[0] / 3.0f + ppOO[2]->lPt2[0] * 2.0f / 3.0f + vD[0];
						vE[1] = ppOO[0]->lPt2[1] / 3.0f + ppOO[2]->lPt2[1] * 2.0f / 3.0f + vD[1];
						// blend value is just average of next points
						vE[2] = 0.5f * ( ppOO[1]->lTangDescX[0] + ppOO[2]->lTangDescX[0] );
						vE[3] = 0.5f * ( ppOO[1]->lTangDescX[1] + ppOO[2]->lTangDescX[1] );
						vE[4] = 0.5f * ( ppOO[1]->lTangDescY[0] + ppOO[2]->lTangDescY[0] );
						vE[5] = 0.5f * ( ppOO[1]->lTangDescY[1] + ppOO[2]->lTangDescY[1] );
					}
					else
					{
						// get mid-point vector between first and last point
						float vM[2] = { ( ppOO[2]->lPt2[0] + ppOO[0]->lPt2[0] ) / 2, ( ppOO[2]->lPt2[1] + ppOO[0]->lPt2[1] ) / 2 };
						// get distance vector of second point to linear estimation if second point
						float vD[2] = { ppOO[1]->lPt2[0] - vM[0], ppOO[1]->lPt2[1] - vM[1] }; // 
						// estimate missing point on line from first and last, by adding the distance to mid-point (mirroring) and subtract correction
						vE[0] = ppOO[2]->lPt2[0] * 1.5f - ppOO[0]->lPt2[0] / 2 - vD[0];
						vE[1] = ppOO[2]->lPt2[1] * 1.5f - ppOO[0]->lPt2[1] / 2 - vD[1];

						// blend value is linear extrapolation
						vE[2] = MAX( 0, MIN( 1, 2.0f * ppOO[2]->lTangDescX[0] - ppOO[1]->lTangDescX[0] ) );
						vE[3] = MAX( 0, MIN( 1, 2.0f * ppOO[2]->lTangDescX[1] - ppOO[1]->lTangDescX[1] ) );
						vE[4] = MAX( 0, MIN( 1, 2.0f * ppOO[2]->lTangDescY[0] - ppOO[1]->lTangDescY[0] ) );
						vE[5] = MAX( 0, MIN( 1, 2.0f * ppOO[2]->lTangDescY[1] - ppOO[1]->lTangDescY[1] ) );
					}
					float score = matches[i].score * pow( ppOO[0]->lPt2[2] * ppOO[1]->lPt2[2] * ppOO[2]->lPt2[2], 1.0f / 3.0f ); // 0 < score <= 1, use geometric mean
					nMatches++;
					for( int i = 0; i != 6; i++ )
						vE[i] *= score;

					pN->lPt2[0] += vE[0];
					pN->lPt2[1] += vE[1];
					pN->lPt2[2] += score;
					pN->lTangDescX[0] += vE[2];
					pN->lTangDescX[1] += vE[3];
					pN->lTangDescY[0] += vE[4];
					pN->lTangDescY[1] += vE[5];
				}

				if( FLT_EPSILON <= pN->lPt2[2] )
				{
					pN->lPt2[0] /= pN->lPt2[2];
					pN->lPt2[1] /= pN->lPt2[2];
					pN->lTangDescX[0] /= pN->lPt2[2];
					pN->lTangDescX[1] /= pN->lPt2[2];
					pN->lTangDescY[0] /= pN->lPt2[2];
					pN->lTangDescY[1] /= pN->lPt2[2];

					pN->lPt2[2] /= nMatches;

					if( FLT_EPSILON > pN->lPt2[2] )
						pN->lPt2[2] = FLT_EPSILON;
				}

			}
		}

		// repair grid

		float scoreMax = 0;
		for( pN = newGrid.begin(); pN != newGrid.end(); pN++ )
			if( scoreMax < pN->lPt2[2] )
				scoreMax = pN->lPt2[2];

		scoreMax *= 0.95f; // need to be in 95%

		// copy back to original
		for( pO = grid.begin(), pN = newGrid.begin(); pO != grid.end(); pO++, pN++ )
		{
			if( 0.5f <= pO->lPt2[2] )
			{
				#ifdef _DEBUG
				good++;
				#endif
				continue;
			}
			if( scoreMax <= pN->lPt2[2] ) // has maximum score, so guessing is as good as it gets
			{
				pO->lPt2[0] = pN->lPt2[0];
				pO->lPt2[1] = pN->lPt2[1];
				pO->lPt2[2] = +.5f + MAX( FLT_EPSILON, pN->lPt2[2] * 0.5f ); // fit back to "good value" window
				pO->lTangDescX[0] = pN->lTangDescX[0];
				pO->lTangDescX[1] = pN->lTangDescX[1];
				pO->lTangDescY[0] = pN->lTangDescY[0];
				pO->lTangDescY[1] = pN->lTangDescY[1];
				added++;
			}
			else
				inValid++;
		}
		logStr( 2, "Info: RepairUniformGrid: run %i added %i, removed %i points. Leaving %i invalid points.", numOp, added, removed, inValid );
	} while( 0 != added &&
				0 < inValid );

	if( 0 != inValid )
		return false;

	return true;
}

/// Wrangler for SPPair3f as vertex
struct SPPair3fWrangler : public SPPair3f
{
	// translate to map where lPt1 ist the vertex coordinate and lPt2 is the texture coordinate; fill the index list
	static inline float& x( SPPair3f& p ) {
		return p.lPt1[0];
	}
	static inline float& y( SPPair3f& p ) {
		return p.lPt1[1];
	}
	static inline float& z( SPPair3f& p ) {
		return p.lPt1[2];
	}
	static inline bool valid( SPPair3f const& p ) {
		return p.fUse & 1;
	}
	static inline bool validate( SPPair3f& p ) {
		return p.fUse |= 1;
	}
	static inline bool invalidate( SPPair3f& p ) {
		return p.fUse &= ~1;
	}
	static inline float& u( SPPair3f& p ) {
		return p.lPt2[0];
	}
	static inline float& v( SPPair3f& p ) {
		return p.lPt2[1];
	}
	static inline float& w( SPPair3f& p ) {
		return p.lPt2[2];
	}
	static inline float& r( SPPair3f& p ) {
		return p.lTangDescX[0];
	}
	static inline float& g( SPPair3f& p ) {
		return p.lTangDescX[1];
	}
	static inline float& b( SPPair3f& p ) {
		return p.lTangDescY[0];
	}
	static inline float& a( SPPair3f& p ) {
		return p.lTangDescY[1];
	}
	static inline SPPair3f make( bool valid, float x, float y, float z, float u, float v, float w, float r, float g, float b, float a  ) {
		SPPair3f p;
		p.fPos = 0;
		p.fUse = valid ? 1 : 0;
		p.lPt1[0] = x;
		p.lPt1[1] = y;
		p.lPt1[2] = z;
		p.lPt2[0] = u;
		p.lPt2[1] = v;
		p.lPt2[2] = w;
		p.lTangDescX[0] = r;
		p.lTangDescX[1] = g;
		p.lTangDescY[0] = b;
		p.lTangDescY[1] = a;
		return p;
	}
};

// test functions
bool testW( VWB_WarpRecord const& w ) { return 0.5 <= w.w && ( w.x != 0.0f || w.y != 0.0f || w.z != 0.0 ); } // for 3D vwf
bool testZ( VWB_WarpRecord const& w ) { return 0.5 <= w.z; } // for 2D vwf


template< class W, auto f, class TP, class TI>
void splitEdge( TP& p, TP& p1, float l, float r, long ip, long ip1, std::vector<TP>& newVtx, std::map<std::pair<TI, TI>, std::pair<TI, TI>>& cache, TI& i1, TI& i2, TI& c )
{
	// find weight
	float w = f( p1 ) / ( 1.0f - f( p ) + f( p1 ) );
	SPPair3f v; v.fUse = 1;
	// v1 point close to p
	W::x( v ) = w * W::x( p ) + ( 1.0f - w ) * W::x( p1 );
	W::y( v ) = w * W::y( p ) + ( 1.0f - w ) * W::y( p1 );
	W::z( v ) = w * W::z( p ) + ( 1.0f - w ) * W::z( p1 );
	W::u( v ) = w * W::u( p ) + ( 1.0f - w ) * W::u( p1 );
	W::v( v ) = w * W::v( p ) + ( 1.0f - w ) * W::v( p1 );
	W::w( v ) = w * W::w( p ) + ( 1.0f - w ) * W::w( p1 );
	W::r( v ) = w * W::r( p ) + ( 1.0f - w ) * W::r( p1 );
	W::g( v ) = w * W::g( p ) + ( 1.0f - w ) * W::g( p1 );
	W::b( v ) = w * W::b( p ) + ( 1.0f - w ) * W::b( p1 );
	W::a( v ) = w * W::a( p ) + ( 1.0f - w ) * W::a( p1 );
	f( v ) = r;
	newVtx.push_back( v );
	i1 = c++;
	// v2 point close to p1
	f( v ) = l;
	newVtx.push_back( v );
	i2 = c++;
	cache.emplace( std::pair( ip, ip1 ), std::pair( i1, i2 ) ); // instert hint to newly created vertices, note: p is always the one with max texture coordinate!
}

/// @brief fixes a seam by subdividing a triangle spanning a texture coordinate wrap into 3, while the original triangle is kept and altered and 2 others are appended into the index/vertex list
/// @param [IN|OUT] vertices the pixel dimension of the wrap, to make a half-pixel correction
/// @param [IN|OUT] indices the pixel dimension of the wrap, to make a half-pixel correction
/// @param [IN] dim the pixel dimension of the wrap, to make a half-pixel correction
/// @param [template] W the vertex wrangler
/// @param [template] f a function TP -> tex, to read/write a texture coordinate
/// @param [template] TP the vertex type
/// @param [template] TI the index type
template< class W, auto f, class TP, class TI >
bool fixSeam( std::vector<TP>& vertices, std::vector<TI>& indices, long dim )
{
	std::vector<TP> newVtx;
	std::vector<TI> newIdx;
	std::map<std::pair<TI, TI>, std::pair<TI, TI>> cache; // this map points from a pair of indices defining an edge to the indices of the newly created vertices to avoid creating same vertices twice
	TI c = (TI)vertices.size(); // the index base
	for( TI* idx = indices.data(), *idxE = idx + indices.size(); idx != idxE; idx += 3 )
	{
		// check edge if it wraps in U, this is when abs(u1-u2) > 0.5
		// there must always be 2 edges wrapping, as by design there is a seam in the P2C pixels, where one side u is 1 and other is 0, therefore each point belongs to some side
		TI* idWrap[6]{}; // we still allow for 3 edges in memory to avoid crash
		TI  n = 0;
		if( 0.5f < abs( f( vertices[idx[0]] ) - f( vertices[idx[1]] ) ) ) // check first edge
		{
			idWrap[n++] = idx;
			idWrap[n++] = idx + 1;
		}

		if( 0.5f < abs( f( vertices[idx[1]] ) - f( vertices[idx[2]] ) ) ) // check second edge
		{
			idWrap[n++] = idx + 1;
			idWrap[n++] = idx + 2;
		}

		if( 0.5f < abs( f( vertices[idx[2]] ) - f( vertices[idx[0]] ) ) ) // check third edge
		{
			idWrap[n++] = idx + 2;
			idWrap[n++] = idx;
		}
		if( 4 == n ) // this is a triangle with a seam
		{
			// in 2 cases id[1] == id[2] and thus solo
			// if id[0] and id[3] is same, we swap around
			if( idWrap[0] == idWrap[3] )
			{
				idWrap[0] = idx + 2;
				idWrap[1] = idx;
				idWrap[2] = idx;
				idWrap[3] = idx + 1;
			}

			TP& p = vertices[*idWrap[1]]; // the solo point
			TP& p1 = vertices[*idWrap[0]];
			TP& p2 = vertices[*idWrap[3]];
			bool bReverse = f( p1 ) > f( p ); // solo point got smaller texture coordinate, meaning the solo point is on other side

			// calculate half pixel; this is actually not right, as we would need the input texture size. But it looks better not causing a black line by design
			const float l = 0.5f / dim;
			const float r = 1.0f - 0.5f / dim;
			// we need 4 new vertices, 2 on each edge on the same XY but with 1 and 0 as U
			//           p               p     
			//          / \        	    /1\    
			//         /   \       	  v1---v3  
			//        /     \		 v2-----v4
			//       /       \		 / 2  \3 \
			//      p1--------p2    p1--------p2

			TI iv[4]; // the assigned indices

			if( bReverse )
			{
				// first edge p - p1
				if( auto found = cache.find( std::pair( *idWrap[0], *idWrap[1] ) ); found != cache.end() )
				{
					iv[1] = found->second.first;
					iv[0] = found->second.second;
					cache.erase( found );
				}
				else
					splitEdge<W,f>( p1, p, l, r, *idWrap[0], *idWrap[1], newVtx, cache, iv[1], iv[0], c );

				// second edge p - p2
				if( auto found = cache.find( std::pair( *idWrap[3], *idWrap[1] ) ); found != cache.end() )
				{ // these have already been created previously, we just use them
					iv[3] = found->second.first;
					iv[2] = found->second.second;
					cache.erase( found );
				}
				else // we need two new vertices
					splitEdge<W,f>( p2, p, l, r, *idWrap[3], *idWrap[1], newVtx, cache, iv[3], iv[2], c );
			}
			else
			{
				// first edge p - p1
				if( auto found = cache.find( std::pair( *idWrap[1], *idWrap[0] ) ); found != cache.end() )
				{
					iv[0] = found->second.first;
					iv[1] = found->second.second;
					cache.erase( found );
				}
				else
					splitEdge<W,f>( p, p1, l, r, *idWrap[1], *idWrap[0], newVtx, cache, iv[0], iv[1], c );

				// second edge p - p2
				if( auto found = cache.find( std::pair( *idWrap[1], *idWrap[3] ) ); found != cache.end() )
				{ // these have already been created previously, we just use them
					iv[2] = found->second.first;
					iv[3] = found->second.second;
					cache.erase( found );
				}
				else // we need two new vertices
					splitEdge<W,f>( p, p2, l, r, *idWrap[1], *idWrap[3], newVtx, cache, iv[2], iv[3], c );
			}

			// add two new triangles
			// triangle 2
			newIdx.push_back( iv[1] );
			newIdx.push_back( *idWrap[0] );
			newIdx.push_back( *idWrap[3] );

			// triangle 3
			newIdx.push_back( iv[1] );
			newIdx.push_back( *idWrap[3] );
			newIdx.push_back( iv[3] );

			// we need to alter the index list, to change into triangle 1
			*idWrap[0] = iv[0];
			*idWrap[3] = iv[2];
		}
		else if( n )
		{// this is somthing odd...
			logStr( 1, "WARNING: Malformed triangle seam detected." );
		}

	}
	// merge
	vertices.insert( vertices.end(), make_move_iterator( newVtx.begin() ), make_move_iterator( newVtx.end() ) );
	indices.insert( indices.end(), make_move_iterator( newIdx.begin() ), make_move_iterator( newIdx.end() ) );
	return true;
}

/// translate mapping to grid where (x,y,z) is the vertex position and (u,v,w) is the texture coordinate. It uses only coodrinates from mapping.
/// The grid-points are not perfectly uniform distributed, but they stay inside their grid-cell. We need a secondary coordinate to record the projector position, this goes to uv.
/// @param [IN] pSrcD the mapping
/// @param [IN] pSrcDB the blend map
/// @param [IN] width the mapping width
/// @param [IN] height the mapping height
/// @param [IN] wGrid the grid width (columns)
/// @param [IN] hGrid the grid height (rows)
/// @param [OUT] vertices the result grid; it generates all points (wGrid * hGrid) and marks valid points
/// @param [template] test a function VWB_WarpRecord->bool that indicates a valid lookup
/// @param [template] W a wrangler class that provides access to vertex attributes of template parameter TP
/// @param [template] TP a type compiling a vertex
/// @eturn 1 if successful, 0 otherwise
template< auto test, class W, class TP >
int pseudoGridFromMapping( VWB_WarpRecord* pSrcD, VWB_BlendRecord2* pSrcDB, long width, long height, long wGrid, long hGrid, std::vector<TP>& vertices )
{
	if( wGrid > width / 2 ) // half size is OK
		wGrid = width / 2;
	if( hGrid > height / 2 ) // half size is OK
		hGrid = height / 2;

	if( nullptr == pSrcD || nullptr == pSrcDB
		) return 0;

	vertices.clear();
	vertices.reserve( ptrdiff_t( wGrid ) * hGrid );

	// first we create a P2C pixel aligned grid and fill with the values from P2C
	for( long yG = 0; yG != hGrid; yG++ )
	{
		long y = yG * ( height - 1 ) / ( hGrid - 1 );
		for( long xG = 0; xG != wGrid; xG++ )
		{
			long x = xG * ( width - 1 ) / ( wGrid - 1 );

			ptrdiff_t i = ptrdiff_t( y ) * width + x;

			VWB_WarpRecord const* pW = pSrcD + i;
			VWB_BlendRecord2 const* pB = pSrcDB + i;

			vertices.emplace_back( W::make(
				test( *pW ),
				pW->x,pW->y, pW->z,
				float( x ), float( y ), 0,
				float( pB->r ) / 65535.0f, float( pB->g ) / 65535.0f,
				float( pB->b ) / 65535.0f, float( pB->a ) / 65535.0f
			));
		}
	}
	return 1;
}

/// find pseudo gridpoints close to valid gridpoints to get better border aproximation
/// note: this process cannot be repeated, as the algo relies on points that are still aligned as a grid
/// @param [IN] pSrcD the mapping
/// @param [IN] pSrcDB the blend map
/// @param [IN] width the mapping width
/// @param [IN] height the mapping height
/// @param [IN] wGrid the grid width (columns)
/// @param [IN] hGrid the grid height (rows)
/// @param [IN|OUT] vertices the result grid; fixed points are no longer on the same place
/// @param [template] test a function VWB_WarpRecord->bool that indicates a valid lookup
/// @param [template] W a wrangler class that provides access to vertex attributes of template parameter TP
/// @param [template] TP a type compiling a vertex
/// @return 1 if successful, 0 otherwise
template< auto test, class W, class TP >
int expandPseudoGrid( VWB_WarpRecord* pSrcD, VWB_BlendRecord2* pSrcDB, long width, long height, long wGrid, long hGrid, std::vector<TP>& vertices )
{
	enum POS_REL {
		POS_REL_T, // top
		POS_REL_TR,  // top-right
		POS_REL_R, // right
		POS_REL_BR, // bottom-right
		POS_REL_B,  // bottom
		POS_REL_BL, // bottom-left
		POS_REL_L, // left
		POS_REL_TL, // top-left
		POS_REL_SIZE
	};

	SIZE const delta[POS_REL_SIZE] = {
		{  0, -1 }, // top
		{  1, -1 }, // top-right
		{  1,  0 }, // right
		{  1,  1 }, // bottom-right
		{  0,  1 }, // bottom
		{ -1,  1 }, // bottom-left
		{ -1,  0 }, // left
		{ -1, -1 } // top-left
	};

	struct RepairItem
	{
		typename std::vector<TP>::iterator where; // the grid position 
		typename std::vector<TP>::value_type v;
	};
	std::list<RepairItem> repairs;

	long xs = width / wGrid;
	long ys = height / hGrid;


	if( xs && ys )
	{
		auto pt = vertices.begin();
		for( long yG = 0; yG != hGrid; yG++ )
		{
			for( long xG = 0; xG != wGrid; xG++, pt++ )
			{
				if( !W::valid( *pt ) ) // me is invalid, so try to find a pseudo grid point next to me
				{
					for( int i = POS_REL_T; i != POS_REL_SIZE; i++ ) // iterate over neighbors
					{
						long oxG = xG + delta[i].cx;
						long oyG = yG + delta[i].cy;

						if(
							oxG < wGrid && // not over right border
							oxG >= 0 && // not over left border
							oyG < hGrid && // not over bottom border
							oyG >= 0
							)
						{
							auto const& opt = pt + ( ptrdiff_t( delta[i].cy ) * wGrid + ptrdiff_t( delta[i].cx ) ); // get the iterator of the other point
							if( W::valid( *opt ) )
							{
								// got a valid point in some direction
								// so we go from me (pt) towards other (opt) until we find a valid point in the map
								long bx = long( W::u( *pt ) );
								long by = long( W::v( *pt ) );
								long ex = long( W::u( *opt ) );
								long ey = long( W::v( *opt ) );
								long x = bx + delta[i].cx;
								long y = by + delta[i].cy;

								while( x != ex || y != ey )
								{

									ptrdiff_t off = ptrdiff_t( x ) + y * width;
									VWB_WarpRecord const* pW = pSrcD + off;
									VWB_BlendRecord2 const* pB = pSrcDB + off;

									if( test(*pW) ) // hit!
									{
										repairs.emplace_back( RepairItem{ pt,
											W::make( 
												true, pW->x, pW->y, pW->z,
												float( x ), float( y ), 0,
												float( pB->r ) / 65535.0f, float( pB->g ) / 65535.0f ,	float( pB->b ) / 65535.0f, float( pB->a ) / 65535.0f 
											)
										} );

										break;
									}

									if( delta[i].cx && delta[i].cy ) // diagonal
									{
										long d = ( x - bx ) * delta[i].cy * ys / delta[i].cx / ( y - by );
										if( xs > d )
											x += delta[i].cx;
										else
											y += delta[i].cy;
									}
									else
									{
										x += delta[i].cx;
										y += delta[i].cy;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// find best match for each point, which is the closest to the original grid point
	auto minC = repairs.end();
	auto minS = float( 0 );
	for( auto r = repairs.begin(); r != repairs.end(); )
	{
		if( minC == repairs.end() || minC->where != r->where )
		{
			minC = r;
			float dx = W::u( *r->where ) - W::u( r->v );
			float dy = W::v( *r->where ) - W::v( r->v );
			minS = dx * dx + dy * dy; // we count in float to avoid int overflow
			r++;
		}
		else
		{
			float dx = W::u( *r->where ) - W::u( r->v );
			float dy = W::v( *r->where ) - W::v( r->v );
			float s = dx * dx + dy * dy; // we count in float to avoid int overflow
			if( s < minS ) // I am closer
			{
				minS = s;
				// delete other
				repairs.erase( minC );
				minC = r;
				r++;
			}
			else // other was closer
			{
				// delete me
				r = repairs.erase( r );
			}

		}
	}

	// consolidate; transfer points to grid
	for( auto& r : repairs )
	{
		*r.where = r.v;
	}
	return 1;
}

VWB_ERROR Dummywarper::getWarpBlend( VWB_WarpBlend const*& wb )
{
	wb = &m_wb;
	return VWB_ERROR_NONE;
}

VWB_ERROR Dummywarper::getWarpMesh( VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh )
{
	if(3 > cols || 3 > rows)
	{
		logStr(0, "ERROR: getWarpMesh needs >3 rows and columns.\n");
		return VWB_ERROR_PARAMETER;
	}

	VWB_int& w = m_wb.header.width;
	VWB_int& h = m_wb.header.height;
	int nRecords = w * h;

	if(1024 > nRecords || NULL == m_wb.pWarp || NULL == m_wb.pBlend)
	{
		logStr(0, "ERROR: getWarpMesh: warp map too small.\n");
		return VWB_ERROR_GENERIC;
	}
	logStr(2, "INFO: getWarpMesh( %i, %i, * ): params OK.\n", cols, rows);

	DynSPPointPairList3f vertices;
	DynLongList	indices;
	if( m_bDynamicEye )
	{
		if( pseudoGridFromMapping < testW, SPPair3fWrangler >( m_wb.pWarp, m_wb.pBlend2, w, h, cols, rows, vertices ) &&
			expandPseudoGrid< testW, SPPair3fWrangler >( m_wb.pWarp, m_wb.pBlend2, w, h, cols, rows, vertices ) &&
			triangulateGrid< SPPair3fWrangler >( vertices, cols, rows, indices ) )
		{
			logStr( 2, "INFO: getWarpMesh: triangulation.\n" );
			mesh.nVtx = (VWB_uint)vertices.size();
			mesh.vtx = new VWB_WarpBlendVertex[vertices.size()];
			for( VWB_uint i = 0; i != vertices.size(); i++ )
			{
				VWB_VEC3f pos = m_mBaseI * VWB_VEC3f::ptr( vertices[i].lPt1 );
				mesh.vtx[i] = VWB_WarpBlendVertex{
					{ pos.x, pos.y, pos.z },
					{ vertices[i].lPt2[0] / w, vertices[i].lPt2[1] / h },
					{ vertices[i].lTangDescX[0] * vertices[i].lTangDescY[1],
					  vertices[i].lTangDescX[1] * vertices[i].lTangDescY[1],
					  vertices[i].lTangDescY[0] * vertices[i].lTangDescY[1]}
				};
			}

			mesh.nIdx = (VWB_uint)indices.size();
			mesh.idx = new VWB_uint[mesh.nIdx];
			for( VWB_uint i = 0; i != indices.size(); i++ )
			{
				mesh.idx[i] = indices[i];
			}

			mesh.dim.cx = w;
			mesh.dim.cy = h;

			logStr( 2, "INFO: getWarpMesh: Triangulation succeeded. %u vertices with %u triangles.\n", mesh.nVtx, mesh.nIdx / 3 );
			return VWB_ERROR_NONE;
		}
		else
		{
			logStr( 0, "ERROR: getWarpMesh: Triangulation (3D) failed.\n" );
			return VWB_ERROR_GENERIC;
		}
	}
	else
	{
		if( pseudoGridFromMapping< testZ, SPPair3fWrangler >( m_wb.pWarp, m_wb.pBlend2, w, h, cols, rows, vertices ) &&
			expandPseudoGrid< testZ, SPPair3fWrangler >( m_wb.pWarp, m_wb.pBlend2, w, h, cols, rows, vertices ) &&
			triangulateGrid< SPPair3fWrangler >( vertices, cols, rows, indices ) &&
			fixSeam< SPPair3fWrangler, SPPair3fWrangler::x >( vertices, indices, w ) &&
			fixSeam< SPPair3fWrangler, SPPair3fWrangler::y >( vertices, indices, h )
			)
		{

			logStr(2, "INFO: getWarpMesh: triangulation of grid (%ix%i) with %lu indices (%5.1f%%)\n", cols, rows, indices.size(), float( indices.size() ) / ( rows - 1 ) / ( cols - 1 ) / 6 * 100 );

			mesh.nVtx = (VWB_uint)vertices.size();
			mesh.vtx = new VWB_WarpBlendVertex[vertices.size()];
			VWB_MAT44f mVI = m_mViewIG.Inverted();

			for( VWB_uint i = 0; i != vertices.size(); i++ )
			{
				// we create a screen plane by using the mapping lookups and transform them by the view and base matrix
				// this way 2D and 3D meshes can be used exact same way in the host program
				// lPt1 contains a uv lookup
				// unproject
				VWB_VEC4f sc(
					-m_viewSizes[0] + ( m_viewSizes[0] + m_viewSizes[2] ) * vertices[i].lPt1[0],
					m_viewSizes[1] - ( m_viewSizes[1] + m_viewSizes[3] ) * vertices[i].lPt1[1],
					m_bRH ? -screenDist : screenDist,
					1
					);
				// put into view
				VWB_VEC3f pos = VWB_VEC3f( mVI * sc );
				// lPt2 contains the pixel position on the projector, this needs to be normalized
				mesh.vtx[i] = VWB_WarpBlendVertex{
					{ pos.x, pos.y, pos.z },
					{ vertices[i].lPt2[0] / w, vertices[i].lPt2[1] / h },
					{ vertices[i].lTangDescX[0] * vertices[i].lTangDescY[1],
					  vertices[i].lTangDescX[1] * vertices[i].lTangDescY[1],
					  vertices[i].lTangDescY[0] * vertices[i].lTangDescY[1]
				} };
			}

			mesh.nIdx = (VWB_uint)indices.size();
			mesh.idx = new VWB_uint[mesh.nIdx];
			for( VWB_uint i = 0; i != indices.size(); i++ )
			{
				mesh.idx[i] = indices[i];
			}

			mesh.dim.cx = w;
			mesh.dim.cy = h;

			logStr(2, "INFO: getWarpMesh: Triangulation succeeded. %u vertices with %u triangles.\n", mesh.nVtx, mesh.nIdx / 3);
			return VWB_ERROR_NONE;
		}
		else
		{
			logStr(0, "ERROR: getWarpMesh: Triangulation (CT) failed.\n");
			return VWB_ERROR_GENERIC;
		}
	}
	return VWB_ERROR_FALSE;
}

////////////////////////////////////////////////////////////////////////////////////


inline void spliceVec( VWB_double& outX, VWB_double& outY, VWB_double& outZ, VWB_double const& inX, VWB_double const& inY, VWB_double const& inZ, VWB_uint sw )
{
	if( sw & 0x002 )
		outX = inY;
	else if( sw & 0x004 )
		outX = inZ;
	else
		outX = inX;
	if( sw & 0x001 )
		outX*= -1;

	if( sw & 0x020 )
		outY = inX;
	else if( sw & 0x040 )
		outY = inZ;
	else
		outY = inY;
	if( sw & 0x010 )
		outY*= -1;

	if( sw & 0x200 )
		outZ = inX;
	else if( sw & 0x400 )
		outZ = inY;
	else
		outZ = inZ;
	if( sw & 0x100 )
		outZ*= -1;

}

#ifdef WIN32
size_t copyCursorBitmapToMappedTexture( HBITMAP hbmMask, HBITMAP hbmColor, BITMAP& bmMask, BITMAP& bmColor,  void* bits, int pitch )
{
	size_t r = 0;

	size_t szMask = ptrdiff_t( bmMask.bmWidthBytes ) * abs( bmMask.bmHeight );

	bmMask.bmBits = malloc( szMask );
	BYTE* pData = reinterpret_cast<BYTE*>(malloc( ptrdiff_t( abs( bmMask.bmHeight ) ) * pitch ));
	BYTE* pMask = reinterpret_cast<BYTE*>(malloc( szMask ));
	BYTE* pM = pMask;
	LONG cb = LONG( szMask );
	if( cb != szMask )
		throw exception( "arithmetic overflow in copyCursorBitmapToMappedTexture" );
	r = ::GetBitmapBits( hbmMask, cb, pMask );
	if( hbmColor )
	{
		LONG szColor = bmColor.bmWidthBytes * abs( bmColor.bmHeight );
		r = ::GetBitmapBits( hbmColor, szColor, pData );
	}
	else
	{
		bmColor.bmWidth = bmMask.bmWidth;
		bmColor.bmWidthBytes = bmMask.bmWidth * 4;
		bmColor.bmHeight = bmMask.bmHeight / 2;
		bmColor.bmBitsPixel = 32;
		bmColor.bmPlanes = bmMask.bmPlanes;

		BYTE* data = pData;
		for( int y = 0; y != abs( bmMask.bmHeight / 2 ); y++, pM += bmMask.bmWidthBytes, data += pitch )
		{
			BYTE* byteAND = pM;
			BYTE maskAND = *byteAND;
			BYTE* byteXOR = pM + (bmMask.bmHeight * bmMask.bmWidthBytes / 2);
			BYTE maskXOR = *byteXOR;
			BYTE* pixel = data;

			for( int x = 0; x != bmMask.bmWidth; x++, pixel += 4 )
			{
				if( maskXOR & maskAND & 128 ) // -> reverse screen ??, we take green
				{
					pixel[0] = 0;
					pixel[1] = 0;
					pixel[2] = 0;
					pixel[3] = 255;
				}
				else if( maskXOR & 128 ) // -> white
				{
					pixel[0] = 255;
					pixel[1] = 255;
					pixel[2] = 255;
					pixel[3] = 255;
				}
				else if( maskAND & 128 ) // -> screen
				{
					pixel[0] = 0;
					pixel[1] = 0;
					pixel[2] = 0;
					pixel[3] = 0;
				}
				else // -> black
				{
					pixel[0] = 0;
					pixel[1] = 0;
					pixel[2] = 0;
					pixel[3] = 255;
				}

				if( 7 == x % 8 )
				{
					byteAND++;
					maskAND = *byteAND;
					byteXOR++;
					maskXOR = *byteXOR;
				}
				else
				{
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
	for (BYTE *p = pData, *pE = pData + r, *pS = (BYTE*)bits; p != pE; p += paddBm, pS+= paddTx )
	{
		for (BYTE const* pLE = p + 4 * bmColor.bmWidth; p != pLE; p += 4, pS+= 4 )
		{
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
BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
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
			if (nullptr != ShowSystemCursor &&
				!g_bCurEnabled)
			{
				ShowSystemCursor(TRUE);
			}
			break;
	}
    return TRUE;
}

#elif defined __GNU__
  void __attribute__ ((constructor)) my_init(void)
  {
  }

  void __attribute__ ((destructor)) my_fini(void)
  {
  }
#endif //def WIN32
