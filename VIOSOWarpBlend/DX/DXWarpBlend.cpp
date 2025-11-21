// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "DXWarpBlend.h"
#pragma comment( lib, "dxguid.lib" )
//#pragma comment( lib, "d3dcompiler.lib" )
#include <DirectXMath.h>
using namespace DirectX;

typedef HRESULT ( WINAPI *D3DCompileFn )( _In_reads_bytes_( SrcDataSize ) LPCVOID pSrcData,
				   _In_ SIZE_T SrcDataSize,
				   _In_opt_ LPCSTR pSourceName,
				   _In_reads_opt_( _Inexpressible_( pDefines->Name != NULL ) ) CONST D3D_SHADER_MACRO* pDefines,
				   _In_opt_ ID3DInclude* pInclude,
				   _In_opt_ LPCSTR pEntrypoint,
				   _In_ LPCSTR pTarget,
				   _In_ UINT Flags1,
				   _In_ UINT Flags2,
				   _Out_ ID3DBlob** ppCode,
				   _Always_( _Outptr_opt_result_maybenull_ ) ID3DBlob** ppErrorMsgs );
D3DCompileFn pfnD3DCompile = nullptr;
HMODULE hmD3DCompilerDll = 0;

extern "C" HRESULT WINAPI
D3DCompile( _In_reads_bytes_( SrcDataSize ) LPCVOID pSrcData,
			_In_ SIZE_T SrcDataSize,
			_In_opt_ LPCSTR pSourceName,
			_In_reads_opt_( _Inexpressible_( pDefines->Name != NULL ) ) CONST D3D_SHADER_MACRO * pDefines,
			_In_opt_ ID3DInclude * pInclude,
			_In_opt_ LPCSTR pEntrypoint,
			_In_ LPCSTR pTarget,
			_In_ UINT Flags1,
			_In_ UINT Flags2,
			_Out_ ID3DBlob * *ppCode,
			_Always_( _Outptr_opt_result_maybenull_ ) ID3DBlob * *ppErrorMsgs )
{
	if( !pfnD3DCompile )
	{
		*ppCode = nullptr;
		if( ppErrorMsgs )
			*ppErrorMsgs = nullptr;
		return E_FAIL;
	}
	return pfnD3DCompile( pSrcData, SrcDataSize, pSourceName, pDefines, pInclude, pEntrypoint, pTarget, Flags1, Flags2, ppCode, ppErrorMsgs );
}


DXWarpBlend::DXWarpBlend()
: VWB_Warper_base()
{
	hmD3DCompilerDll = ::LoadLibraryA( "D3DCompiler_47.dll" );
	if( !hmD3DCompilerDll )
		hmD3DCompilerDll = ::LoadLibraryA( "D3DCompiler_43.dll" );
	if( !hmD3DCompilerDll )
		hmD3DCompilerDll = ::LoadLibraryA( "D3DCompiler_42.dll" );
	if( !hmD3DCompilerDll )
	{
		logStr( 0, "ERROR: Could not load D3DCompiler_4*.dll" );
		throw VWB_ERROR_SHADER;
	}
	pfnD3DCompile = (D3DCompileFn)::GetProcAddress( hmD3DCompilerDll, "D3DCompile" );
	m_modelPath[0] = 0;
}

DXWarpBlend::~DXWarpBlend()
{
	if( hmD3DCompilerDll )
		::FreeLibrary( hmD3DCompilerDll );
}

VWB_ERROR DXWarpBlend::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = __super::Init( wbs );

	// transpose view matrices to use DX common vector pre-multiplication
	m_mBaseI.Transpose();
	m_mViewIG.Transpose();
	return err;
}
inline VWB_MAT44f DXWarpBlend::UpdateView( VWB_MAT44f const& igView, VWB_VEC3f& e )
{
	VWB_MAT44f V; //return value

	// rotation matrix from angles
	VWB_MAT44f R;

	if( m_bRH )
		R = VWB_MAT44f::R( (VWB_float)m_ep.pitch, (VWB_float)m_ep.yaw, (VWB_float)m_ep.roll ).Transposed();
	else
		R = VWB_MAT44f::R_LH( (VWB_float)m_ep.pitch, (VWB_float)m_ep.yaw, (VWB_float)m_ep.roll ).Transposed();
		 
	// reverse eye vector
	e = VWB_VEC3f( -(float)m_ep.x, -(float)m_ep.y, -(float)m_ep.z );

	// add platform-rotated eye offset
	if( 0 != this->eye[0] || 0 != this->eye[1] || 0 != this->eye[2] )
		e -= VWB_VEC3f::ptr( this->eye ) * R;

	// translate to local coordinates
	e = e * igView;

	VWB_MAT44f T = VWB_MAT44f::T( e ).Transposed();
	m_mVP = m_mBaseI * igView * T;

	if( bTurnWithView )
		V = R * igView;
	else
		V = igView * T;

	return V;
}

// set up the view matrix,
// use the same matrices as in your program, construct a view matrix relative to the actual screen
// use the same units (usually millimeters) for the screen and the scene
VWB_ERROR DXWarpBlend::GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj )
{
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret )
	{
		VWB_MAT44f P; // the projection matrix to return 

		VWB_VEC3f e;
		VWB_MAT44f V = UpdateView( m_mViewIG, e ); // the view matrix to return

		VWB_float clip[6];
		getClip( e, clip );

		if( m_bRH )
			P = VWB_MAT44f::DXFrustumRH( clip );
		else
			P = VWB_MAT44f::DXFrustumLH( clip );

		m_mVP*= P;

		if( pView )
			V.SetPtr( pView );

		if( pProj )
			P.SetPtr( pProj );
	}
	return ret;
}

// set up the view matrix,
// use the same matrices as in your program, construct a view matrix relative to the actual screen
// use the same units (usually millimeters) for the screen and the scene
VWB_ERROR DXWarpBlend::GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip )
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
			P = VWB_MAT44f::DXFrustumRH( clip );
		else
			P = VWB_MAT44f::DXFrustumLH( clip );

		m_mVP *= P;

		if( pView )
		{
			V.SetPtr( pView );
		}

		if( pClip )
		{
			memcpy( pClip, clip, sizeof( clip ) );
		}
	}
	return ret;
}

VWB_ERROR DXWarpBlend::GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric, VWB_float aspect )
{

	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret )
	{

		VWB_MAT44f P;
		VWB_VEC3f e;

		VWB_MAT44f V = UpdateView( m_mViewIG, e );;

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
			VWB_VEC3f::ptr( pDir ) = V.Upper().Transposed().GetR();
			if( !m_bRH )
				VWB_VEC3f::ptr( pDir ) *= -1;

		}

		if( pPos )
		{
			pPos[0] = V._41;
			pPos[1] = V._42;
			pPos[2] = V._43;
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
				a/= aspect;
				clip[1] *= a;
				clip[3] *= a;
			}
		}

		if( m_bRH )
			P = VWB_MAT44f::DXFrustumRH( clip );
		else
			P = VWB_MAT44f::DXFrustumLH( clip );

		m_mVP *= P;

		if( pClip )
		{
			memcpy( pClip, clip, sizeof( clip ) );
		}
	}

	return ret;
}

VWB_ERROR DXWarpBlend::SetViewProjection( VWB_float const* pView, VWB_float const* pProj )
{
	if( pView && pProj )
		m_mVP = m_mBaseI * VWB_MAT44f::ptr( pView ) * VWB_MAT44f::ptr( pProj );
	else
		return VWB_ERROR_PARAMETER;
	return VWB_ERROR_NONE;
}
