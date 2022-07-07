#pragma once
#ifndef VIOSOWARPBLEND_HPP
#define VIOSOWARPBLEND_HPP
#include "VWBTypes.h"
#include <TCHAR.h>
#include <memory>
#include <map>
#include <exception>
#include <atomic>

class VWB
{
private:
	VWB_Warper* m_warper;
	static HMODULE hMVIOSOWARPBLEND_DYNAMIC;
	static std::atomic_int instanceCounter;
	#define VIOSOWARPBLEND_API( ret, name, args ) typedef ret (*pfn_##name)args;\
	static pfn_##name name;
	#include "VIOSOWarpBlend.h"

	static void loadDll( const TCHAR* dllPath )
	{
		try {
			if( NULL == dllPath || 0 == dllPath[0] )
			#if defined( _M_X64 )
				dllPath = _T( "ViosoWarpBlend64" );
			#else
				dllPath = _T( "ViosoWarpBlend" );
			#endif

			if( !hMVIOSOWARPBLEND_DYNAMIC )
			{
				hMVIOSOWARPBLEND_DYNAMIC = ::LoadLibrary( dllPath );

				#define VIOSOWARPBLEND_API( ret, name, args ) name = (pfn_##name)::GetProcAddress( hMVIOSOWARPBLEND_DYNAMIC, #name )
				#include "VIOSOWarpBlend.h"

				#define VIOSOWARPBLEND_API( ret, name, args ) if( NULL == name ) throw std::exception( #name )
				#include "VIOSOWarpBlend.h"
			}
		}
		catch( std::exception& e )
		{
			UNREFERENCED_PARAMETER( e );
			instanceCounter = 0;
			if( hMVIOSOWARPBLEND_DYNAMIC )
				::FreeLibrary( hMVIOSOWARPBLEND_DYNAMIC );
			throw VWB_ERROR_GENERIC;
		}
	}

	static void unloadDll()
	{
		#define VIOSOWARPBLEND_API( ret, name, args ) name = NULL;
		#include "VIOSOWarpBlend.h"
		if( hMVIOSOWARPBLEND_DYNAMIC )
			::FreeLibrary( hMVIOSOWARPBLEND_DYNAMIC );
		hMVIOSOWARPBLEND_DYNAMIC = 0;
	}

public:
	VWB( const TCHAR* dllPath, void* pDxDevice, TCHAR const* szConfigFile, TCHAR const* szChannelName, VWB_int logLevel = 2, TCHAR const* szLogFile = NULL ) 
		: m_warper( NULL )
	{
		if( 1 == ++instanceCounter )
			loadDll( dllPath );
		VWB_ERROR err = VWB_Create( pDxDevice, szConfigFile, szChannelName, &m_warper, logLevel, szLogFile );
		if( VWB_ERROR_NONE != err )
			throw err;
	}

	~VWB()
	{
		if( VWB_Destroy && m_warper )
			VWB_Destroy( m_warper );
		if( 0 == --instanceCounter )
			unloadDll();
	}

	VWB_Warper& get() { return *m_warper; }
	VWB_Warper const& get()  const { return *m_warper; }

	VWB_ERROR Init() { return VWB_Init( m_warper ); };
	VWB_ERROR InitExt( VWB_WarpBlendSet* extSet ) { return VWB_InitExt( m_warper, extSet ); }
	VWB_ERROR GetViewProj( VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pProj ) { return VWB_getViewProj( m_warper, pEye, pRot, pView, pProj ); }
	VWB_ERROR GetViewClip( VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pClip ) { return VWB_getViewClip( m_warper, pEye, pRot, pView, pClip ); }
	VWB_ERROR GetPosDirClip( VWB_float* pEye, VWB_float* pRot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric = false, VWB_float aspect = 0 ) { return VWB_getPosDirClip( m_warper, pEye, pRot, pPos, pDir, pClip, symmetric, aspect );	}
	VWB_ERROR GetScreenplane( VWB_float* pTL, VWB_float* pTR, VWB_float* pBL, VWB_float* pBR ) { return VWB_getScreenplane( m_warper, pTL, pTR, pBL, pBR );	}
	VWB_ERROR Render( VWB_param src = VWB_UNDEFINED_GL_TEXTURE, VWB_uint stateMask = 0 ) { return VWB_render( m_warper, src, stateMask ); }
	VWB_ERROR SetViewProj( VWB_float* pView, VWB_float* pProj ) { return VWB_setViewProj( m_warper, pView, pProj ); }
	static VWB_ERROR VwfInfo( char const* path, VWB_WarpBlendHeaderSet* set ) { return VWB_vwfInfo( path, set ); }
	static VWB_ERROR VwfInfo( const TCHAR* dllPath, char const* path, VWB_WarpBlendHeaderSet* set ) { 
		if( 1 == ++instanceCounter )
			loadDll( dllPath );
		VWB_ERROR ret = VWB_vwfInfo( path, set ); 
		if( 0 == --instanceCounter )
			unloadDll();
		return ret;
	}
	VWB_ERROR GetWarpBlend( VWB_WarpBlend const*& wb ) { return VWB_getWarpBlend( m_warper, wb ); }
	VWB_ERROR GetShaderVPMatrix( VWB_float* pMPV ) { return VWB_getShaderVPMatrix( m_warper, pMPV ); }
	VWB_ERROR GetWarpBlendMesh( VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh ) { return VWB_getWarpBlendMesh( m_warper, cols, rows, mesh ); }
	VWB_ERROR DestroyWarpBlendMesh( VWB_WarpBlendMesh& mesh ) { return VWB_destroyWarpBlendMesh( m_warper, mesh ); }
	static VWB_ERROR SetCryptoKey( uint8_t const* key ) { return VWB_setCryptoKey( key ); }
};

struct VWB_EmptyData {};
template< class _Base = VWB_EmptyData > struct VWBX: public _Base {	
	VWB	w; 
	VWBX( _Base const& own, const TCHAR* dllPath, void* pDxDevice, TCHAR const* szConfigFile, TCHAR const* szChannelName, VWB_int logLevel = 2, TCHAR const* szLogFile = NULL )	: _Base( own ), w( dllPath, pDxDevice, szConfigFile, szChannelName, logLevel, szLogFile ) {}
};
template< class _Key, class _Data = VWB_EmptyData > class VWBmap : public std::map< _Key, std::shared_ptr< VWBX< _Data> > > { public: typedef VWBX< _Data> PtrT; typedef _Data BaseT; };

HMODULE VWB::hMVIOSOWARPBLEND_DYNAMIC = 0;
std::atomic_int VWB::instanceCounter = 0;
#define VIOSOWARPBLEND_API( ret, name, args ) VWB::pfn_##name VWB::name = NULL;
#include "VIOSOWarpBlend.h"

#endif
