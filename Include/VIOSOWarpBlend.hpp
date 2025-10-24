// VIOSO API
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

// if used in more than one module, #define VIOSOWARPBLEND_HPP_NO_IMPLEMENT in all but one

#pragma once
#ifndef VIOSOWARPBLEND_HPP
#define VIOSOWARPBLEND_HPP

#include "VWBTypes.h"
#include "StringConversions.h"
#include <memory>
#include <map>
#include <exception>
#include <atomic>
#include <sstream>
#include <filesystem>

#ifndef WIN32
#include <dlfcn.h>
#include <unistd.h>
#endif // ndef WIN32

/// @brief a class to conveniantly wrap VWB_ functions
class VWB {
private:
	VWB_Warper* m_warper; /// the warper struct pointer
	static std::atomic_int instanceCounter; /// a static instance counter, to automatically load/unload the dll
#define VIOSOWARPBLEND_API( ret, name, args ) typedef ret (*pfn_##name)args;\
	static pfn_##name name;
#include "VIOSOWarpBlend.h"

#ifdef WIN32
	static HMODULE hMVIOSOWARPBLEND_DYNAMIC; /// the static library handle
	static void _loadLib( std::filesystem::path libPath ) {
		try {
			if( libPath.empty() ) {
				libPath = std::filesystem::current_path();
			#ifdef _M_X64
				libPath /= "VIOSOWarpBlend64.dll";
			#else
				libPath /= "VIOSOWarpBlend.dll";
			#endif
			}

			if( hMVIOSOWARPBLEND_DYNAMIC )
				::FreeLibrary( hMVIOSOWARPBLEND_DYNAMIC );
			hMVIOSOWARPBLEND_DYNAMIC = ::LoadLibraryW( libPath.c_str() );

			if( NULL == hMVIOSOWARPBLEND_DYNAMIC )
				throw std::runtime_error( std::string( "Could not library " ) + libPath.string() );

		#define VIOSOWARPBLEND_API( err, name, args ) name = (pfn_##name)::GetProcAddress( hMVIOSOWARPBLEND_DYNAMIC, #name )
		#include "VIOSOWarpBlend.h"

		#define VIOSOWARPBLEND_API( err, name, args ) if( NULL == name ) throw std::runtime_error( "Could not find function "#name )
		#include "VIOSOWarpBlend.h"

			int ver[4]{};
			if( VWB_ERROR_NONE != VWB_getVersion( &ver[0], &ver[1], &ver[2], &ver[3] ) || ver[0] < VWB_Version_MAJ )
				throw std::runtime_error( ( std::ostringstream() << "Library version mismatch. Header version (" << VWB_Version_MAJ << "." << VWB_Version_MIN << "." << VWB_Version_MAI << "." << VWB_Version_REV << ") is significantly higher as dll version " << ver[0] << "." << ver[1] << "." << ver[2] << "." << ver[3] << "." ).str() );
		} catch( std::exception& e ) {
			instanceCounter = 0;
			if( hMVIOSOWARPBLEND_DYNAMIC )
				::FreeLibrary( hMVIOSOWARPBLEND_DYNAMIC );
			throw e;
		}
	}

	static void _unloadLib() {
	#define VIOSOWARPBLEND_API( err, name, args ) name = NULL;
	#include "VIOSOWarpBlend.h"
		if( hMVIOSOWARPBLEND_DYNAMIC )
			::FreeLibrary( hMVIOSOWARPBLEND_DYNAMIC );
		hMVIOSOWARPBLEND_DYNAMIC = 0;
		instanceCounter = 0;
	}
#else
#define HMODULE void*
	static HMODULE hMVIOSOWARPBLEND_DYNAMIC; /// the static library handle
	static void _loadLib( std::filesystem::path libPath ) {
		try {
			if( libPath.empty() ) {
				libPath = std::filesystem::current_path();
				libPath /= "libVIOSOWarpBlend.so";
			}

			if( hMVIOSOWARPBLEND_DYNAMIC )
				::dlclose( hMVIOSOWARPBLEND_DYNAMIC );
			{
				hMVIOSOWARPBLEND_DYNAMIC = ::dlopen( libPath.c_str(), RTLD_LAZY );
				if( 0 == hMVIOSOWARPBLEND_DYNAMIC ) {
					throw std::runtime_error( dlerror() );
				}

			#define VIOSOWARPBLEND_API( ret, name, args ) name = (pfn_##name)::dlsym( hMVIOSOWARPBLEND_DYNAMIC, #name )
			#include "VIOSOWarpBlend.h"

			#define VIOSOWARPBLEND_API( ret, name, args ) if( NULL == name ) throw std::runtime_error( "Could not find function "#name )
			#include "VIOSOWarpBlend.h"

				int ver[4]{};
				if( VWB_ERROR_NONE != VWB_getVersion( &ver[0], &ver[1], &ver[2], &ver[3] ) || ver[0] < VWB_Version_MAJ )
					throw std::runtime_error( ( std::ostringstream() << "Library version mismatch. Header version (" << VWB_Version_MAJ << "." << VWB_Version_MIN << "." << VWB_Version_MAI << "." << VWB_Version_REV << ") is significantly higher as dll version " << ver[0] << "." << ver[1] << "." << ver[2] << "." << ver[3] << "." ).str() );
			}
		} catch( std::exception& e ) {
			instanceCounter = 0;
			if( hMVIOSOWARPBLEND_DYNAMIC )
				::dlclose( hMVIOSOWARPBLEND_DYNAMIC );
			throw e;
		}
	}

	static void _unloadLib() {
	#define VIOSOWARPBLEND_API( ret, name, args ) name = NULL;
	#include "VIOSOWarpBlend.h"
		if( hMVIOSOWARPBLEND_DYNAMIC )
			::dlclose( hMVIOSOWARPBLEND_DYNAMIC );
		hMVIOSOWARPBLEND_DYNAMIC = 0;
		instanceCounter = 0;
	}
#endif //def WIN32

public:
	/// @brief loads a warper library
	/// use to avoid multiple load/unload of the library. Call UnloadLib to release gracefully
	/// @param libPath 
	/// @throws
	static void LoadLib( std::filesystem::path const& libPath ) {
		if( 1 == ++instanceCounter )
			_loadLib( libPath );
	}
	/// @brief unloads a warper library
	/// use to avoid multiple load/unload of the library.
	static void UnloadLib() {
		if( 0 <= --instanceCounter )
			_unloadLib();
	}

	/// @brief the constructor
	/// @param libPath a path to the warper dll to load
	/// @param pDxDevice set to NULL for OpenGL or pointer to a DirectX device for Direct3D 9 to 11
	///		for Direct3D 12 you need to specify a pointer to a ID3D12CommandQueue, set to VWB_DUMMYDEVICE, to just hold the data to create a textured mesh. 
	///		Supported: IDirect3DDevice9, IDirect3DDevice9Ex, ID3D10Device, ID3D10Device1, ID3D11Device, ID3D12CommandQueue (for ID3D12Device initialization)
	/// @param szConfigFile path to a .ini file containing settings, if empty the default values are used
	/// @param szChannelName the name of the channel, also the section name to look for in .ini-file.
	/// @param logLevel the log level. 0 quiet, 1 fatals only, 2 standard, 3 verbous, 4 debug, 5 debug verbous. Log levels less than 4 do no logging in the render loop.
	/// @param szLogFile a path to a log file
	VWB( std::filesystem::path const& libPath, void* pDxDevice, std::filesystem::path const& szConfigFile, char8_t const* szChannelName, VWB_int logLevel = 2, std::filesystem::path const& szLogFile = "" )
		: m_warper( NULL ) {
		if( 1 == ++instanceCounter )
			_loadLib( libPath );
		VWB_ERROR err = VWB_CreateU( pDxDevice, szConfigFile.u8string().c_str(), szChannelName, &m_warper, logLevel, szLogFile.u8string().c_str() );
		if( VWB_ERROR_NONE != err ) {
			_unloadLib();
			throw std::runtime_error( std::string( "VWB_Create returned error " ) + std::to_string( (int)err ) + ": " + GetErrorCStr( err ) );
		}
	}
	/// @overload
	VWB( std::filesystem::path const& libPath, void* pDxDevice, std::filesystem::path const& szConfigFile, char const* szChannelName, VWB_int logLevel = 2, std::filesystem::path const& szLogFile = "" )
		: m_warper( NULL ) {
		if( 1 == ++instanceCounter )
			_loadLib( libPath );
		VWB_ERROR err = VWB_CreateA( pDxDevice, szConfigFile.string().c_str(), szChannelName, &m_warper, logLevel, szLogFile.string().c_str() );
		if( VWB_ERROR_NONE != err ) {
			_unloadLib();
			throw std::runtime_error( std::string( "VWB_Create returned error " ) + std::to_string( (int)err ) + ": " + GetErrorCStr( err ) );
		}
	}
	/// @overload
	VWB( std::filesystem::path const& libPath, void* pDxDevice, std::filesystem::path const& szConfigFile, wchar_t const* szChannelName, VWB_int logLevel = 2, std::filesystem::path const& szLogFile = "" )
		: m_warper( NULL ) {
		if( 1 == ++instanceCounter )
			_loadLib( libPath );
		VWB_ERROR err = VWB_CreateU( pDxDevice, szConfigFile.u8string().c_str(), VWBUtil::to_u8string( szChannelName ).c_str(), &m_warper, logLevel, szLogFile.u8string().c_str() );
		if( VWB_ERROR_NONE != err ) {
			_unloadLib();
			throw std::runtime_error( std::string( "VWB_Create returned error " ) + std::to_string( (int)err ) + ": " + GetErrorCStr( err ) );
		}
	}

	/// @brief the destructor
	~VWB() {
		if( VWB_Destroy && m_warper )
			VWB_Destroy( m_warper );
		if( 0 <= --instanceCounter )
			_unloadLib();
	}

	/// @brief access to the warper struct
	/// @return the warper struct
	VWB_Warper& get() { return *m_warper; }
	/// @overload
	VWB_Warper const& get()  const { return *m_warper; }

	/// @brief check for warper valid
	operator const bool() const { return nullptr != m_warper; }
	/// @brief initialize the warper with the current warper struct 
	/// @return VWB_ERROR_NONE if successful, otherwise @see VWB_ERROR
	VWB_ERROR Init() { return VWB_Init( m_warper ); };

	/// @brief initialize the warper with the current warper struct using an existing set of mappings
	/// @param extSet the mappings
	/// @return VWB_ERROR_NONE if successful, otherwise @see VWB_ERROR
	VWB_ERROR InitExt( VWB_WarpBlendSet* extSet ) { return VWB_InitExt( m_warper, extSet ); }

	/// @brief Create a frustum on current target.
	/// @param pEye [IN] VWB_float[3] the current eye position
	/// @param pRot [IN] VWB_float[3] the current euler rotation in radians
	/// @param pView [OUT] the view matrix
	/// @param pProj [OUT] the projection matrix
	/// @return VWB_ERROR_NONE if successful, otherwise @see VWB_ERROR
	VWB_ERROR GetViewProj( VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pProj ) { return VWB_getViewProj( m_warper, pEye, pRot, pView, pProj ); }

	/// @brief Create a frustum on current target.
	/// @param pEye [IN] VWB_float[3] the current eye position
	/// @param pRot [IN] VWB_float[3] the current euler rotation
	/// @param pView [OUT] the view matrix
	/// @param pClip [OUT] a VWB_float[6] to receive a clip volume. left, top, right, bottom, near, far, where all components are usually positive
	/// @return VWB_ERROR_NONE if successful, otherwise @see VWB_ERROR
	VWB_ERROR GetViewClip( VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pClip ) { return VWB_getViewClip( m_warper, pEye, pRot, pView, pClip ); }

	/// @brief Create a frustum on current target.
	/// @param pEye [IN] the current eye position
	/// @param pRot [IN] the current euler rotation
	/// @param pPos [OUT] a VWB_float[3] with the position offset
	/// @param pDir [OUT] a VWB_float[3] with the euler roatation offset
	/// @param pClip [OUT] a VWB_float[6] to receive a clip volume. left, top, right, bottom, near, far, where all components are usually positive
	/// @param symmetric set true, to have symmetric FoV
	/// @param aspect set to 0, to get variable FoV, set to some other value to get a FoV with this tangens aspect ratio
	/// @return VWB_ERROR_NONE if successful, otherwise @see VWB_ERROR
	VWB_ERROR GetPosDirClip( VWB_float* pEye, VWB_float* pRot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric = false, VWB_float aspect = 0 ) { return VWB_getPosDirClip( m_warper, pEye, pRot, pPos, pDir, pClip, symmetric, aspect ); }

	/// @brief Get the current screen plane (target)
	/// @param pTL top-left corner of the target rectangle
	/// @param pTR top-right corner of the target rectangle
	/// @param pBL bottom-left corner of the target rectangle
	/// @param pBR bottom-right corner of the target rectangle
	/// @return VWB_ERROR_NONE if successful, otherwise @see VWB_ERROR
	VWB_ERROR GetScreenplane( VWB_float* pTL, VWB_float* pTR, VWB_float* pBL, VWB_float* pBR ) { return VWB_getScreenplane( m_warper, pTL, pTR, pBL, pBR ); }

	/// @brief render, this will render an input to the current render target
	/// @param src the source texture, a IDirect3DTexture9*, ID3D10Texture2D*, ID3D11Texture2D*, VWB_D3D12_RENDERINPUT* or a GLint texture index; 
	///		if current backbuffer must be read, set to NULL in any DX mode except 12 or to -1 in OpenGL mode
	///		in case of directX 12 you need to provide a @see VWB_D3D12_RENDERINPUT as parameter.
	/// @param stateMask @see VWB_STATEMASK enumeration, default is 0 to restore usual stuff
	///		In D3D12 all flags except VWB_STATEMASK_CLEARBACKBUFFER are ignored.
	//		The application is required to set inputs and shader in each term anyway.
	/// @return VWB_ERROR_NONE if successful, otherwise VWB_ERROR_GENERIC
	VWB_ERROR Render( VWB_param src = VWB_UNDEFINED_GL_TEXTURE, VWB_uint stateMask = 0 ) { return VWB_render( m_warper, src, stateMask ); }

	/// @brief set a frustum
	/// @param pView the view matrix
	/// @param pProj the projection matrix
	/// @return VWB_ERROR_NONE if successful, otherwise VWB_ERROR_GENERIC
	VWB_ERROR SetViewProj( VWB_float* pView, VWB_float* pProj ) { return VWB_setViewProj( m_warper, pView, pProj ); }

	/// @brief 
	/// @param libPath 
	/// @param path 
	/// @param set 
	/// @return (1) VWB_ERROR, VWB_ERROR_NONE on success, otherwise @see VWB_ERROR
	/// (2)(3) a vector<VWB_WarpBlendHeader>, throws on error
	static VWB_ERROR VwfInfo( std::filesystem::path const& libPath, std::filesystem::path const& path, std::vector<VWB_WarpBlendHeader>& set ) {
		VWB_ERROR err = VWB_ERROR_NONE;
		if( 1 == ++instanceCounter )
			_loadLib( libPath );
		VWB_uint c = 0;
		err = VWB_vwfInfoCU( path.u8string().c_str(), nullptr, &c );
		if( VWB_ERROR_NONE == err ) {
			set.resize( c );
			err = VWB_vwfInfoCU( path.u8string().c_str(), set.data(), &c );
		}
		if( 0 <= --instanceCounter )
			_unloadLib();
		return err;
	}
	/// @overload
	static std::vector<VWB_WarpBlendHeader> VwfInfo( std::filesystem::path const& libPath, std::filesystem::path const& path ) {
		std::vector<VWB_WarpBlendHeader> set;
		auto err = VwfInfo( libPath, path, set );
		if( VWB_ERROR_NONE != err )
			throw std::runtime_error( std::string( "VWB_vwfInfoCU returned error " ) + std::to_string( (int)err ) + ": " + GetErrorCStr( err ) );
		return set;
	}
	/// @overload
	static std::vector<VWB_WarpBlendHeader> VwfInfo( std::filesystem::path const& path ) {
		std::vector<VWB_WarpBlendHeader> set;
		auto err = VwfInfo( "", path, set );
		if( VWB_ERROR_NONE != err )
			throw std::runtime_error( std::string( "VWB_vwfInfoCU returned error " ) + std::to_string( (int)err ) + ": " + GetErrorCStr( err ) );
		return set;
	}

	/// @brief get the current mappings. Only works with a DUMMY warper, GPU warper do not keep mappings around
	/// @param wb [OUT] the pointer to a warp mapping
	/// @return VWB_ERROR_NONE on success, otherwise @see VWB_ERROR
	VWB_ERROR GetWarpBlend( VWB_WarpBlend const*& wb ) { return VWB_getWarpBlend( m_warper, wb ); }

	/// @brief get the current view and projection matrix of the warper
	/// use in your self-implemented warper shader
	/// @param wb [OUT] VWB_float[16] to fill with the matrix data
	/// @return VWB_ERROR_NONE on success, otherwise @see VWB_ERROR
	VWB_ERROR GetShaderVPMatrix( VWB_float* pMPV ) { return VWB_getShaderVPMatrix( m_warper, pMPV ); }

	/// @brief get a mesh with warp and blend info, mainly for preview. Only works with DUMMY warper, as it works on the mappings on CPU
	/// this is a pseudo grid, but it is not on uniform positions
	/// @param cols number of columns of the mesh
	/// @param rows number of rows of the mesh
	/// @param mesh [OUT] the mesh
	/// @return VWB_ERROR_NONE on success, otherwise @see VWB_ERROR
	VWB_ERROR GetWarpBlendMesh( VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh ) { return VWB_getWarpBlendMesh( m_warper, cols, rows, mesh ); }

	/// @brief destroys a mesh. Call this to release allocated memory gracefully via the library
	/// @param mesh the mesh generated by GetWarpBlendMesh
	/// @return VWB_ERROR_NONE on success, otherwise @see VWB_ERROR
	VWB_ERROR DestroyWarpBlendMesh( VWB_WarpBlendMesh& mesh ) { return VWB_destroyWarpBlendMesh( m_warper, mesh ); }

	/// log some string to the API's log file, exposed version; use VWB_logString instead.
	/// @param [IN]			level	a level indicator. The string is only written to log file, if this is lower or equal to currently set global log level
	/// @param [IN]			str		a null terminated multibyte character string
	/// @return VWB_ERROR_NONE on success, VWB_ERROR_PARAMETER otherwise
	VWB_ERROR logStr( VWB_int level, char const* str ) { return VWB__logString( level, str ); }

	// clear roll over the current log file. 
	// @return VWB_ERROR_NONE on success, VWB_ERROR_PARAMETER otherwise
	VWB_ERROR logClear() { return VWB_logClear(); }

	/// get the version of the API
	/// @param [IN] path a path to a warper dll. it will be used, if there is no dll loaded yet
	/// @param[OUT]			major	major version
	/// @param[OUT]			minor	minor version
	/// @param[OUT]			maintenance	maintenance revision
	/// @param[OUT]			build	build number
	/// @return VWB_ERROR_NONE on success, VWB_ERROR_PARAMETER, if parameters are out of range 
	static VWB_ERROR getVersion( std::filesystem::path const& libPath, VWB_int* major, VWB_int* minor, VWB_int* maintenance, VWB_int* build ) {
		VWB_ERROR err = VWB_ERROR_NONE;
		if( 1 == ++instanceCounter )
			_loadLib( libPath );
		VWB_getVersion( major, minor, maintenance, build );
		if( 0 <= --instanceCounter )
			_unloadLib();
		return err;
	}

	/// get set a 128bit AES key for encryption and decryption of mappings
	/// @param[IN]			key	    a key of 16 bytes (128 bit) provided for encryption/decryption of AES128 algorythm loading and saving vwf files, set to nullptr to disable encryption/decryption
	/// @return VWB_ERROR_NONE on success, VWB_ERROR_PARAMETER, if parameters are out of range 
	/// @remark Internally we keep only the pointer to your buffer. Make sure to keep the key there as long as it is needed. It will be used during VWB_init VWB_initExt */
	VWB_ERROR SetCryptoKey( uint8_t const* key ) { return VWB_setCryptoKey( key ); }

	/// @brief get the error description of an error code
	/// @param err the error code
	/// @return a null-terminated C string with the description
	static constexpr char const* GetErrorCStr( VWB_ERROR const& err ) {
		switch( err ) {
		case VWB_ERROR_NONE: return "no";
		case VWB_ERROR_GENERIC: return "generic error";
		case VWB_ERROR_PARAMETER: return "parameter error";
		case VWB_ERROR_INI_LOAD: return "ini could not be loaded";
		case VWB_ERROR_BLEND: return "blend invalid or coud not be loaded to graphic hardware";
		case VWB_ERROR_WARP: return "warp invalid or could not be loaded to graphic hardware";
		case VWB_ERROR_SHADER: return "shader program failed to load";
		case VWB_ERROR_VWF_LOAD: return "mappings file broken or version mismatch";
		case VWB_ERROR_VWF_FILE_NOT_FOUND: return "can't find mapping file";
		case VWB_ERROR_NOT_IMPLEMENTED: return "not implemented, this function is yet to come";
		case VWB_ERROR_NETWORK: return "network could not be initialized";
		case VWB_ERROR_NDI: return "NDI could not be initialized";
		case VWB_ERROR_FALSE: return "false";
		default: return "unknown";
		}
	}
};

/// wrapper to add a warper to some class, typically a window or display resource
/// _Base must be move-constructible 
template< class _Base > struct VWBX : public _Base {
	VWB	w;
	VWBX( _Base&& own, std::filesystem::path const& libPath, void* pDxDevice, std::filesystem::path const& szConfigFile, char8_t const* szChannelName, VWB_int logLevel = 2, std::filesystem::path const& szLogFile = "" ) : _Base( std::move( own ) ), w( libPath, pDxDevice, szConfigFile, szChannelName, logLevel, szLogFile ) {}
	VWBX( _Base&& own, std::filesystem::path const& libPath, void* pDxDevice, std::filesystem::path const& szConfigFile, char const* szChannelName, VWB_int logLevel = 2, std::filesystem::path const& szLogFile = "" ) : _Base( std::move( own ) ), w( libPath, pDxDevice, szConfigFile, szChannelName, logLevel, szLogFile ) {}
	VWBX( _Base&& own, std::filesystem::path const& libPath, void* pDxDevice, std::filesystem::path const& szConfigFile, wchar_t const* szChannelName, VWB_int logLevel = 2, std::filesystem::path const& szLogFile = "" ) : _Base( std::move( own ) ), w( libPath, pDxDevice, szConfigFile, szChannelName, logLevel, szLogFile ) {}
};
/// organize all channels in a map, it uses the VWBX struct
template< class _Key, class _Data > class VWBmap : public std::map< _Key, std::shared_ptr< VWBX< _Data> > > { public: typedef VWBX< _Data> PtrT; typedef _Data BaseT; };

#ifndef VIOSOWARPBLEND_HPP_NO_IMPLEMENT
HMODULE VWB::hMVIOSOWARPBLEND_DYNAMIC{ 0 };
std::atomic_int VWB::instanceCounter{ 0 };
// declare the 
#define VIOSOWARPBLEND_API( ret, name, args ) VWB::pfn_##name VWB::name = NULL;
#include "VIOSOWarpBlend.h"
#endif
#endif // ndef VIOSOWARPBLEND_HPP
