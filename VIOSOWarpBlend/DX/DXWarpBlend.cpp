#include "DXWarpBlend.h"
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "d3dcompiler.lib" )
#include <DirectXMath.h>
using namespace DirectX;

DXWarpBlend::DXWarpBlend()
: VWB_Warper_base()
{
	m_modelPath[0] = 0;
}

DXWarpBlend::~DXWarpBlend()
{
}

VWB_ERROR DXWarpBlend::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = __super::Init( wbs );
	// transpose view matrices to use DX common vector pre-multiplication
	m_mBaseI.Transpose();
	m_mViewIG.Transpose();
	return err;
}
inline VWB_MAT44f DXWarpBlend::UpdateView( VWB_VEC3f& e )
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
	e = e * m_mViewIG;

	VWB_MAT44f T = VWB_MAT44f::T( e ).Transposed();
	m_mVP = m_mBaseI * m_mViewIG * T; //TODO precalc

	if( bTurnWithView )
		V = R * m_mViewIG;
	else
		V = m_mViewIG * T;

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
		VWB_MAT44f V = UpdateView( e ); // the view matrix to return

		VWB_float clip[6];
		getClip( e, clip );

		if( m_bRH )
			P = VWB_MAT44f::DXFrustumRH( clip );
		else
			P = VWB_MAT44f::DXFrustumLH( clip );

		m_mVP = m_mVP * P;

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
		VWB_MAT44f V = UpdateView( e );

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

VWB_ERROR DXWarpBlend::GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pSymClip )
{
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret )
	{

		VWB_MAT44f P;
		VWB_VEC3f e;
		VWB_MAT44f V = UpdateView( e );

		VWB_float clip[6];
		getClip(e, clip);

		pSymClip[2] = clip[4];
		pSymClip[3] = clip[5];
		pPos[0] = V[3];
		pPos[1] = V[7];
		pPos[2] = V[11];

		VWB_float angles[4] = {
			atan2( clip[0], nearDist ),
			atan2( clip[1], nearDist ),
			atan2( clip[2], nearDist ),
			atan2( clip[3], nearDist ),
		};

		// this is the sum of both angles, left+right resp. top+bottom 
		pSymClip[0] = tan( ( angles[0] + angles[2] + ( angles[2] - angles[0] ) ) / 2 ) * 2 * nearDist;
		pSymClip[1] = tan( ( angles[1] + angles[3] + ( angles[1] - angles[3] ) ) / 2 ) * 2 * nearDist;

		// extract rotation angles from upper View matrix
		VWB_VEC3f::ptr( pDir ) = V.Upper().Transposed().GetR();
		if( !m_bRH )
			VWB_VEC3f::ptr( pDir ) *= -1;

		// add difference of left/right, which add to rotation around y, and upper/lower clip, which add to rotation around x, as angles
		pDir[0] += angles[1] - angles[3]; // rotation around x, "pitch" aka up and down
		pDir[1] += angles[2] - angles[0]; // rotation around y, "yaw" aka left and right

		V = m_bRH ? VWB_MAT44f::R( VWB_VEC3f::ptr( pDir ) ).Transposed() : VWB_MAT44f::R_LH( VWB_VEC3f::ptr( pDir ) ).Transposed();
		m_mVP = m_mBaseI * V * VWB_MAT44f::T( pPos[0], pPos[1], pPos[2] ).Transposed();

		clip[0] = clip[2] = pSymClip[0] / 2;
		clip[1] = clip[3] = pSymClip[1] / 2;
		if( m_bRH )
			P = VWB_MAT44f::DXFrustumRH( clip );
		else
			P = VWB_MAT44f::DXFrustumLH( clip );

		m_mVP *= P;
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
