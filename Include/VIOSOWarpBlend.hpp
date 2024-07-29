#pragma once
#ifndef VIOSOWARPBLEND_HPP
#define VIOSOWARPBLEND_HPP
#include "VWBTypes.h"
#include "StringConversions.h"
#include <memory>
#include <map>
#include <exception>
#include <atomic>
#include <filesystem>
#ifndef WIN32
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#endif // ndef WIN32

class VWB
{
private:
	VWB_Warper* m_warper;
	static std::atomic_int instanceCounter;
#define VIOSOWARPBLEND_API( ret, name, args ) typedef ret (*pfn_##name)args;\
	static pfn_##name name;
#include "VIOSOWarpBlend.h"

#ifdef WIN32
	static HMODULE hMVIOSOWARPBLEND_DYNAMIC;
	static void loadDll(const char* dllPath)
	{
		try {
			if ( NULL == dllPath || 0 == dllPath[0] )
#if defined( _M_X64 )
				dllPath = "ViosoWarpBlend64";
#else
				dllPath = "ViosoWarpBlend";
#endif

			if ( !hMVIOSOWARPBLEND_DYNAMIC )
			{
				hMVIOSOWARPBLEND_DYNAMIC = ::LoadLibraryA(dllPath);

#define VIOSOWARPBLEND_API( ret, name, args ) name = (pfn_##name)::GetProcAddress( hMVIOSOWARPBLEND_DYNAMIC, #name )
#include "VIOSOWarpBlend.h"

#define VIOSOWARPBLEND_API( ret, name, args ) if( NULL == name ) throw std::exception( #name )
#include "VIOSOWarpBlend.h"
			}
		}
		catch ( std::exception& e )
		{
			UNREFERENCED_PARAMETER(e);
			instanceCounter = 0;
			if ( hMVIOSOWARPBLEND_DYNAMIC )
				::FreeLibrary(hMVIOSOWARPBLEND_DYNAMIC);
			throw VWB_ERROR_GENERIC;
		}
	}

	static void loadDll(const wchar_t* dllPath)
	{
		try {
			if ( NULL == dllPath || 0 == dllPath[0] )
#if defined( _M_X64 )
				dllPath = L"ViosoWarpBlend64";
#else
				dllPath = L"ViosoWarpBlend";
#endif

			if ( !hMVIOSOWARPBLEND_DYNAMIC )
			{
				hMVIOSOWARPBLEND_DYNAMIC = ::LoadLibraryW(dllPath);

#define VIOSOWARPBLEND_API( ret, name, args ) name = (pfn_##name)::GetProcAddress( hMVIOSOWARPBLEND_DYNAMIC, #name )
#include "VIOSOWarpBlend.h"

#define VIOSOWARPBLEND_API( ret, name, args ) if( NULL == name ) throw std::exception( "Could not load "#name )
#include "VIOSOWarpBlend.h"
			}
		}
		catch ( std::exception& e )
		{
			UNREFERENCED_PARAMETER(e);
			instanceCounter = 0;
			if ( hMVIOSOWARPBLEND_DYNAMIC )
				::FreeLibrary(hMVIOSOWARPBLEND_DYNAMIC);
			throw VWB_ERROR_GENERIC;
		}
	}

	static void unloadDll()
	{
#define VIOSOWARPBLEND_API( ret, name, args ) name = NULL;
#include "VIOSOWarpBlend.h"
		if ( hMVIOSOWARPBLEND_DYNAMIC )
			::FreeLibrary(hMVIOSOWARPBLEND_DYNAMIC);
		hMVIOSOWARPBLEND_DYNAMIC = 0;
	}
#else
#define HMODULE void*
	static HMODULE hMVIOSOWARPBLEND_DYNAMIC;
	static void loadDll(const char* dllPath)
	{
		char str[MAX_PATH]{};
		try {
			if ( NULL == dllPath || 0 == dllPath[0] )
			{
				getcwd(str, MAX_PATH);
				strcat(str, "/libViosoWarpBlend.so");
				dllPath = str;
			}

			if ( !hMVIOSOWARPBLEND_DYNAMIC )
			{
				hMVIOSOWARPBLEND_DYNAMIC = ::dlopen(dllPath, RTLD_LAZY);
				if ( 0 == hMVIOSOWARPBLEND_DYNAMIC )
				{
					throw std::runtime_error(dlerror());
				}

#define VIOSOWARPBLEND_API( ret, name, args ) name = (pfn_##name)::dlsym( hMVIOSOWARPBLEND_DYNAMIC, #name )
#include "VIOSOWarpBlend.h"
			}
		}
		catch ( std::exception& e )
		{
			(e);
			instanceCounter = 0;
			if ( hMVIOSOWARPBLEND_DYNAMIC )
				::dlclose(hMVIOSOWARPBLEND_DYNAMIC);
			throw VWB_ERROR_GENERIC;
		}
	}

	static void unloadDll()
	{
#define VIOSOWARPBLEND_API( ret, name, args ) name = NULL;
#include "VIOSOWarpBlend.h"
		if ( hMVIOSOWARPBLEND_DYNAMIC )
			::dlclose(hMVIOSOWARPBLEND_DYNAMIC);
		hMVIOSOWARPBLEND_DYNAMIC = 0;
	}
#endif //def WIN32

public:
	VWB(std::filesystem::path const& dllPath, void* pDxDevice, std::filesystem::path const& szConfigFile, char8_t const* szChannelName, VWB_int logLevel = 2, std::filesystem::path const& szLogFile = "")
		: m_warper(NULL)
	{
		if ( 1 == ++instanceCounter )
			loadDll(dllPath.c_str());
		VWB_ERROR err = VWB_CreateU(pDxDevice, szConfigFile.u8string().c_str(), szChannelName, &m_warper, logLevel, szLogFile.u8string().c_str());
		if ( VWB_ERROR_NONE != err )
			throw err;
	}
	VWB(std::filesystem::path const& dllPath, void* pDxDevice, std::filesystem::path const& szConfigFile, char const* szChannelName, VWB_int logLevel = 2, std::filesystem::path const& szLogFile = "")
		: m_warper(NULL)
	{
		if ( 1 == ++instanceCounter )
			loadDll(dllPath.c_str());
		VWB_ERROR err = VWB_CreateU(pDxDevice, szConfigFile.u8string().c_str(), VWBUtil::to_u8string( szChannelName ).c_str(), &m_warper, logLevel, szLogFile.u8string().c_str());
		if ( VWB_ERROR_NONE != err )
			throw err;
	}
	VWB(std::filesystem::path const& dllPath, void* pDxDevice, std::filesystem::path const& szConfigFile, wchar_t const* szChannelName, VWB_int logLevel = 2, std::filesystem::path const& szLogFile = "")
		: m_warper(NULL)
	{
		if ( 1 == ++instanceCounter )
			loadDll(dllPath.c_str());
		VWB_ERROR err = VWB_CreateU(pDxDevice, szConfigFile.u8string().c_str(), VWBUtil::to_u8string( szChannelName ).c_str(), &m_warper, logLevel, szLogFile.u8string().c_str());
		if ( VWB_ERROR_NONE != err )
			throw err;
	}

	~VWB()
	{
		if ( VWB_Destroy && m_warper )
			VWB_Destroy(m_warper);
		if ( 0 == --instanceCounter )
			unloadDll();
	}

	VWB_Warper& get() { return *m_warper; }
	VWB_Warper const& get()  const { return *m_warper; }
	operator const bool() const { return nullptr != m_warper; }
	VWB_ERROR Init() { return VWB_Init(m_warper); };
	VWB_ERROR InitExt(VWB_WarpBlendSet* extSet) { return VWB_InitExt(m_warper, extSet); }
	VWB_ERROR GetViewProj(VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pProj) { return VWB_getViewProj(m_warper, pEye, pRot, pView, pProj); }
	VWB_ERROR GetViewClip(VWB_float* pEye, VWB_float* pRot, VWB_float* pView, VWB_float* pClip) { return VWB_getViewClip(m_warper, pEye, pRot, pView, pClip); }
	VWB_ERROR GetPosDirClip(VWB_float* pEye, VWB_float* pRot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric = false, VWB_float aspect = 0) { return VWB_getPosDirClip(m_warper, pEye, pRot, pPos, pDir, pClip, symmetric, aspect); }
	VWB_ERROR GetScreenplane(VWB_float* pTL, VWB_float* pTR, VWB_float* pBL, VWB_float* pBR) { return VWB_getScreenplane(m_warper, pTL, pTR, pBL, pBR); }
	VWB_ERROR Render(VWB_param src = VWB_UNDEFINED_GL_TEXTURE, VWB_uint stateMask = 0) { return VWB_render(m_warper, src, stateMask); }
	VWB_ERROR SetViewProj(VWB_float* pView, VWB_float* pProj) { return VWB_setViewProj( m_warper, pView, pProj ); }

	static std::vector<VWB_WarpBlendHeader> VwfInfo(std::filesystem::path const& path, std::filesystem::path const& dllPath = "" ) {
		std::vector<VWB_WarpBlendHeader> set;
		if ( 1 == ++instanceCounter )
			loadDll(dllPath.c_str());
		VWB_uint c = 0;
		auto ret = VWB_vwfInfoCU(path.u8string().c_str(), nullptr, &c);
		if ( VWB_ERROR_NONE == ret )
		{
			set.resize(c);
			ret = VWB_vwfInfoCU(path.u8string().c_str(), set.data(), &c);
		}
		if ( 0 == --instanceCounter )
			unloadDll();
		if ( VWB_ERROR_NONE != ret )
			throw ret;
		return set;
	}
	VWB_ERROR GetWarpBlend(VWB_WarpBlend const*& wb) { return VWB_getWarpBlend(m_warper, wb); }
	VWB_ERROR GetShaderVPMatrix(VWB_float* pMPV) { return VWB_getShaderVPMatrix(m_warper, pMPV); }
	VWB_ERROR GetWarpBlendMesh(VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh) { return VWB_getWarpBlendMesh(m_warper, cols, rows, mesh); }
	VWB_ERROR DestroyWarpBlendMesh(VWB_WarpBlendMesh& mesh) { return VWB_destroyWarpBlendMesh(m_warper, mesh); }
	static VWB_ERROR SetCryptoKey(uint8_t const* key) { return VWB_setCryptoKey(key); }
};

/// wrapper to add a warper to some class, typically a window or display resource
/// _Base must be move-constructible 
template< class _Base > struct VWBX: public _Base {	
	VWB	w; 
	VWBX(_Base&& own, std::filesystem::path const& dllPath, void* pDxDevice, std::filesystem::path const& szConfigFile, char8_t const* szChannelName, VWB_int logLevel = 2, std::filesystem::path const& szLogFile = "") : _Base(std::move(own)), w(dllPath, pDxDevice, szConfigFile, szChannelName, logLevel, szLogFile) {}
	VWBX(_Base&& own, std::filesystem::path const& dllPath, void* pDxDevice, std::filesystem::path const& szConfigFile, char const* szChannelName, VWB_int logLevel = 2, std::filesystem::path const& szLogFile = "") : _Base(std::move(own)), w(dllPath, pDxDevice, szConfigFile, szChannelName, logLevel, szLogFile) {}
	VWBX(_Base&& own, std::filesystem::path const& dllPath, void* pDxDevice, std::filesystem::path const& szConfigFile, wchar_t const* szChannelName, VWB_int logLevel = 2, std::filesystem::path const& szLogFile = "") : _Base(std::move(own)), w(dllPath, pDxDevice, szConfigFile, szChannelName, logLevel, szLogFile) {}
};
/// organize all channels in a map, it uses the VWBX struct
template< class _Key, class _Data > class VWBmap : public std::map< _Key, std::shared_ptr< VWBX< _Data> > > { public: typedef VWBX< _Data> PtrT; typedef _Data BaseT; };

HMODULE VWB::hMVIOSOWARPBLEND_DYNAMIC{ 0 };
std::atomic_int VWB::instanceCounter{ 0 };
#define VIOSOWARPBLEND_API( ret, name, args ) VWB::pfn_##name VWB::name = NULL;
#include "VIOSOWarpBlend.h"

#endif
