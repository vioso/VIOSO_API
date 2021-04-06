#include "DXWarpBlend.h"
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

DXWarpBlend::DXWarpBlend()
: VWB_Warper_base()
{
	m_modelPath[0] = 0;
}

DXWarpBlend::~DXWarpBlend()
{
}

inline VWB_MAT44f DXWarpBlend::UpdateView( VWB_VEC3f& e )
{
	VWB_MAT44f V;
	// rotation matrix from angles
	VWB_MAT44f R = VWB_MAT44f::R_T( (VWB_float)m_ep.pitch, (VWB_float)m_ep.yaw, (VWB_float)m_ep.roll );

	// copy eye coordinate to e
	e = VWB_VEC3f( -(float)m_ep.x, -(float)m_ep.y, -(float)m_ep.z );
	// add eye offset rotated to platform
	if( 0 != this->eye[0] || 0 != this->eye[1] || 0 != this->eye[2] )
		e += R * VWB_VEC3f::ptr( this->eye );

	// translate to local coordinates
	e = e * m_mViewIG;

	VWB_MAT44f T = VWB_MAT44f::T_T( e );
	m_mVP = m_mBaseI * m_mViewIG * T; //TODO precalc

	if( bTurnWithView )
		V = m_mViewIG * R; //??
	else
		V = m_mViewIG * T;
	return V;
}

// set up the view matrix,
// use the same matrices as in your program, construct a view matrix relative to the actual screen
// use the same units (usually millimeters) for the screen and the scene
VWB_ERROR DXWarpBlend::GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj )
{
	#ifdef _DEBUG
	static HANDLE wtEvt = CreateEventA( nullptr, TRUE, FALSE, "VWB_debug_trigger" );
	static DWORD wtErr = GetLastError();
	if( wtEvt )
	{
		if( ERROR_ALREADY_EXISTS == wtErr )
			WaitForSingleObject( wtEvt, INFINITE );
		else
			PulseEvent( wtEvt );
	}
	#endif
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret )
	{
		VWB_MAT44f P; // the projection matrix to return 

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

		if( pProj )
		{
			P.SetPtr( pProj );
		}
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
		VWB_MAT44f P; // the projection matrix to return 

		// copy eye coordinate to e
		VWB_VEC3f e( (float)m_ep.x, (float)m_ep.y, (float)m_ep.z );

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

VWB_ERROR DXWarpBlend::GetPosDirFov( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pSymClip )
{
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret )
	{

		VWB_MAT44f P; // the projection matrix to return 

		// copy eye coordinate to e
		VWB_VEC3f e( (float)m_ep.x, (float)m_ep.y, (float)m_ep.z );

		VWB_MAT44f V = UpdateView( e );

		VWB_float clip[6];
		getClip(e, clip);

		pSymClip[2] = clip[4];
		pSymClip[3] = clip[5];
		pPos[0] = V[3];
		pPos[1] = V[7];
		pPos[2] = V[11];

		pSymClip[0] = atan2( clip[0], nearDist ) + atan2( clip[2], nearDist );
		pSymClip[1] = atan2( clip[1], nearDist ) + atan2( clip[3], nearDist );

		VWB_MAT33f RT = Upper<VWB_float>( VWB_MAT44f::ptr( V ) );
		VWB_VEC3f::ptr( pDir ) = RT.GetR(); // todo: check...

		pDir[0] += ( atan2( clip[2], nearDist ) - atan2( clip[0], nearDist ) );
		pDir[1] += ( atan2( clip[3], nearDist ) - atan2( clip[1], nearDist ) );

		//m_mVP = m_mBaseI * VWB_MAT44f::R( VWB_VEC3f::ptr( pDir ) ).Transposed() * T; // TODO FIX!

		clip[0] = clip[2] = tan( pSymClip[0] / 2.0f ) * nearDist;
		clip[1] = clip[3] = tan( pSymClip[1] / 2.0f ) * nearDist;
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
