// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "DummyWarper.h"
#include "common.h"
#include "VWF.h"

#include <list>
#include <map>
#include <fstream>
#include <limits>

Dummywarper::Dummywarper( void )
	: VWB_Warper_base() {
	memset( &m_wb, 0, sizeof( m_wb ) );
	m_type4cc = 'MMUD';
}

Dummywarper::~Dummywarper( void ) {
	DeleteVWF( m_wb );
}

VWB_ERROR Dummywarper::Init( VWB_WarpBlendSet& wbs ) {
	VWB_ERROR err = VWB_Warper_base::Init( wbs );
	if( VWB_ERROR_NONE == err ) {
		// do a deep copy of the mappings, to keep
		m_wb = *wbs[calibIndex];
		int nRecords = m_wb.header.width * m_wb.header.height;
		if( m_wb.pWarp ) {
			m_wb.pWarp = new VWB_WarpRecord[nRecords];
			memcpy( m_wb.pWarp, wbs[calibIndex]->pWarp, nRecords * sizeof( VWB_WarpRecord ) );
		}

		if( m_wb.pBlend ) {
			if( m_wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV2 ) {
				m_wb.pBlend2 = new VWB_BlendRecord2[nRecords];
				memcpy( m_wb.pBlend2, wbs[calibIndex]->pBlend2, nRecords * sizeof( VWB_BlendRecord2 ) );
			} else if( m_wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV3 ) {
				m_wb.pBlend3 = new VWB_BlendRecord3[nRecords];
				memcpy( m_wb.pBlend3, wbs[calibIndex]->pBlend3, nRecords * sizeof( VWB_BlendRecord3 ) );
			} else {
				m_wb.pBlend = new VWB_BlendRecord[nRecords];
				memcpy( m_wb.pBlend, wbs[calibIndex]->pBlend, nRecords * sizeof( VWB_BlendRecord ) );
			}
		}
		if( m_wb.pBlack ) {
			m_wb.pBlack = new VWB_BlendRecord[nRecords];
			memcpy( m_wb.pBlack, wbs[calibIndex]->pBlack, nRecords * sizeof( VWB_BlendRecord ) );
		}
		if( m_wb.pWhite ) {
			m_wb.pWhite = new VWB_BlendRecord[nRecords];
			memcpy( m_wb.pWhite, wbs[calibIndex]->pWhite, nRecords * sizeof( VWB_BlendRecord ) );
		}

		if( m_wb.pMesh ) {
			m_wb.pMesh = new VWB_WarpBlendMeshEx;

			memcpy( m_wb.pMesh, wbs[calibIndex]->pMesh, sizeof( VWB_WarpBlendMeshEx ) );
			m_wb.pMesh->vtx = new VWB_WarpBlendVertexEx[m_wb.pMesh->nVtx];
			memcpy( m_wb.pMesh->vtx, wbs[calibIndex]->pMesh->vtx, m_wb.pMesh->nVtx * sizeof( VWB_WarpBlendVertexEx ) );
			m_wb.pMesh->idx = new VWB_uint[m_wb.pMesh->nIdx];
			memcpy( m_wb.pMesh->idx, wbs[calibIndex]->pMesh->idx, m_wb.pMesh->nIdx * sizeof( VWB_uint ) );
		}	

		if( m_wb.p2ndBlend ) {
			m_wb.p2ndBlend = new VWB_BlendRecord[nRecords];
			memcpy( m_wb.p2ndBlend, wbs[calibIndex]->p2ndBlend, nRecords * sizeof( VWB_BlendRecord ) );
		}

		if( m_wb.pDirectional ) {
			m_wb.pDirectional = new VWB_BlendRecord[m_wb.directionalSz.cx * m_wb.directionalSz.cy];
			memcpy( m_wb.pDirectional, wbs[calibIndex]->pDirectional, m_wb.directionalSz.cx * m_wb.directionalSz.cy * sizeof( VWB_BlendRecord ) );
		}
	}
	return err;
}

inline VWB_MAT44f Dummywarper::UpdateView( VWB_MAT44f const& igView, VWB_VEC3f& e ) {
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
VWB_ERROR Dummywarper::GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj ) {
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret ) {
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

		if( pView ) {
			V.Transposed().SetPtr( pView );
		}
		if( pProj ) {
			P.Transposed().SetPtr( pProj );
		}
	}
	return ret;
}

// set up the view matrix,
// use the same matrices as in your program, construct a view matrix relative to the actual screen
// use the same units (usually millimeters) for the screen and the scene
VWB_ERROR Dummywarper::GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip ) {
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret ) {
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

		if( pView ) {
			V.Transposed().SetPtr( pView );
		}

		if( pClip ) {
			memcpy( pClip, clip, sizeof( clip ) );
		}
	}
	return ret;
}

VWB_ERROR Dummywarper::GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric, VWB_float aspect ) {
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret ) {

		VWB_MAT44f P;
		VWB_VEC3f e;

		VWB_MAT44f V = UpdateView( m_mViewIG, e );

		VWB_float clip[6];
		getClip( e, clip );

		if( symmetric ) {
			VWB_MAT44f ig = m_mViewIG;
			if( m_bRH )
				MakeSymmetricRH( ig, clip );
			else
				MakeSymmetricLH( ig, clip );
			V = UpdateView( ig, e );
		}

		if( pDir ) {
			// extract rotation angles from upper View matrix
			VWB_VEC3f::ptr( pDir ) = V.Upper().GetR();
			if( !m_bRH )
				VWB_VEC3f::ptr( pDir ) *= -1;
		}

		if( pPos ) {
			pPos[0] = V._14;
			pPos[1] = V._24;
			pPos[2] = V._34;
		}

		if( 0 != aspect ) {
			VWB_float a = ( clip[0] + clip[2] ) / ( clip[1] + clip[3] );
			if( aspect > a ) // we need to make frustum wider
			{
				a = aspect / a;
				clip[0] *= a;
				clip[2] *= a;
			} else // we need to make frustum higher
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

		if( pClip ) {
			memcpy( pClip, clip, sizeof( clip ) );
		}
	}

	return ret;
}

VWB_ERROR Dummywarper::SetViewProjection( VWB_float const* pView, VWB_float const* pProj ) {
	if( pView && pProj )
		m_mVP = VWB_MAT44f::ptr( pProj ) * VWB_MAT44f::ptr( pView ) * m_mBaseI;
	else
		return VWB_ERROR_PARAMETER;
	return VWB_ERROR_NONE;
}

VWB_ERROR Dummywarper::Render( VWB_param inputTexture, VWB_uint stateMask ) {
	return VWB_ERROR_NOT_IMPLEMENTED;
}

inline VWB_uint getTrianglePatch(
	VWB_WarpBlend& out, VWB_float& x, VWB_float& y,
	VWB_WarpRecord* pW, VWB_BlendRecord2* pB, VWB_BlendRecord* pBl, VWB_BlendRecord* pWh,
	VWB_int dx1, VWB_int dy1, VWB_int dx2, VWB_int dy2 ) {
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
	uMin = floor( uMin * out.header.width ) + 0.5f;
	vMin = floor( vMin * out.header.height ) + 0.5f;
	uMax = floor( uMax * out.header.width ) + 0.5f;
	vMax = floor( vMax * out.header.height ) + 0.5f;
	VWB_uint conv = 0;
	for( VWB_float v = vMin; v <= vMax; v++ ) {
		for( VWB_float u = uMin; u <= uMax; u++ ) {
			VWB_VEC3f l;
			if( VWB_VEC3f::Cart2Bary( VWB_VEC3f::ptr( &pW->x ), VWB_VEC3f::ptr( &pW1->x ), VWB_VEC3f::ptr( &pW2->x ), VWB_VEC3f( u / out.header.width, v / out.header.height, 1 ), l ) ) {
				VWB_uint o = VWB_int( u ) + out.header.width * VWB_int( v );
				VWB_VEC3f::Bary2Cart( VWB_VEC3f( x, y, 1 ), VWB_VEC3f( x + dx1, y + dy1, 1 ), VWB_VEC3f( x + dx2, y + dy2, 1 ), l, VWB_VEC3f::ptr( &out.pWarp[o].x ) );
				if( pB && out.pBlend2 ) {
					out.pBlend2[o].r = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->r + l.y * ( pB + d1 )->r + l.z * ( pB + d2 )->r ) );
					out.pBlend2[o].g = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->g + l.y * ( pB + d1 )->g + l.z * ( pB + d2 )->g ) );
					out.pBlend2[o].b = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->b + l.y * ( pB + d1 )->b + l.z * ( pB + d2 )->b ) );
					out.pBlend2[o].a = (VWB_word)MAX( 65535, (VWB_int)MIN( 0, l.x * pB->a + l.y * ( pB + d1 )->a + l.z * ( pB + d2 )->a ) );
				}
				if( pBl && out.pBlack ) {
					out.pBlack[o].r = (VWB_byte)MIN( 255, MAX( 0, l.x * pBl->r + l.y * ( pBl + d1 )->r + l.z * ( pBl + d2 )->r ) );
					out.pBlack[o].g = (VWB_byte)MIN( 255, MAX( 0, l.x * pBl->g + l.y * ( pBl + d1 )->g + l.z * ( pBl + d2 )->g ) );
					out.pBlack[o].b = (VWB_byte)MIN( 255, MAX( 0, l.x * pBl->b + l.y * ( pBl + d1 )->b + l.z * ( pBl + d2 )->b ) );
					out.pBlack[o].a = (VWB_byte)MIN( 255, MAX( 0, l.x * pBl->a + l.y * ( pBl + d1 )->a + l.z * ( pBl + d2 )->a ) );
				}
				if( pWh && out.pWhite ) {
					out.pWhite[o].r = (VWB_byte)MIN( 255, MAX( 0, l.x * pWh->r + l.y * ( pWh + d1 )->r + l.z * ( pWh + d2 )->r ) );
					out.pWhite[o].g = (VWB_byte)MIN( 255, MAX( 0, l.x * pWh->g + l.y * ( pWh + d1 )->g + l.z * ( pWh + d2 )->g ) );
					out.pWhite[o].b = (VWB_byte)MIN( 255, MAX( 0, l.x * pWh->b + l.y * ( pWh + d1 )->b + l.z * ( pWh + d2 )->b ) );
					out.pWhite[o].a = (VWB_byte)MIN( 255, MAX( 0, l.x * pWh->a + l.y * ( pWh + d1 )->a + l.z * ( pWh + d2 )->a ) );
				}
				conv++;
			}
		}
	}
	return conv;
}

inline VWB_uint getTrianglePatchA(
	VWB_WarpBlend& out, VWB_float& x, VWB_float& y,
	VWB_WarpRecord* pW, VWB_BlendRecord2* pB, VWB_BlendRecord* pBl, VWB_BlendRecord* pWh,
	VWB_int dx1, VWB_int dy1, VWB_int dx2, VWB_int dy2 ) {
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
	uMin = floor( uMin * out.header.width ) + 0.5f;
	vMin = floor( vMin * out.header.height ) + 0.5f;
	uMax = floor( uMax * out.header.width ) + 0.5f;
	vMax = floor( vMax * out.header.height ) + 0.5f;
	if( 5 < uMax - uMin || 5 < vMax - vMin ) {
		int i = 0;
	}
	VWB_uint conv = 0;
	for( VWB_float v = vMin; v <= vMax; v++ ) {
		for( VWB_float u = uMin; u <= uMax; u++ ) {
			VWB_VEC3f l;
			if( VWB_VEC3f::Cart2Bary( VWB_VEC3f::ptr( &pW->x ), VWB_VEC3f::ptr( &pW1->x ), VWB_VEC3f::ptr( &pW2->x ), VWB_VEC3f( u / out.header.width, v / out.header.height, 1 ), l ) ) {
				VWB_uint o = VWB_int( u ) + out.header.width * VWB_int( v );
				VWB_VEC3f::Bary2Cart( VWB_VEC3f( x, y, 1 ), VWB_VEC3f( x + 1, y, 1 ), VWB_VEC3f( x + 1, y + 1, 1 ), l, VWB_VEC3f::ptr( &out.pWarp[o].x ) );
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

VWB_ERROR invertWB( VWB_WarpBlend const& in, VWB_WarpBlend& out ) {
	logStr( 2, "INFO: Calculate inverse maping of %s.\n", in.header.name );
	VWB_int const& w = in.header.width;
	VWB_int const& h = in.header.height;
	VWB_int const nRecords = w * h;

	out = in;
	if( in.pWarp ) {
		out.pWarp = new VWB_WarpRecord[nRecords];
		memset( out.pWarp, 0, nRecords * sizeof( VWB_WarpRecord ) );
	} else
		return VWB_ERROR_PARAMETER;

	if( out.pBlend2 ) {
		out.pBlend2 = new VWB_BlendRecord2[nRecords];
		memset( out.pBlend2, 0, nRecords * sizeof( VWB_BlendRecord2 ) );
	}
	if( out.pBlack ) {
		out.pBlack = new VWB_BlendRecord[nRecords];
		memset( out.pBlack, 0, nRecords * sizeof( VWB_BlendRecord ) );
	}
	if( out.pWhite ) {
		out.pWhite = new VWB_BlendRecord[nRecords];
		memset( out.pWhite, 0, nRecords * sizeof( VWB_BlendRecord ) );
	}

	// fill from triangles...
	VWB_uint conv = 0;
	VWB_WarpRecord* pW = in.pWarp;
	VWB_BlendRecord2* pB = in.pBlend2;
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
	if( out.pBlack || out.pWhite || !out.pBlend2 ) {
		for( VWB_float y = 0; pWE != pW; pW++, pB++, pBl++, pWh++, y++ ) {
			VWB_WarpRecord const* pWLE = pW + w - 1;
			for( VWB_float x = 0; pWLE != pW; pW++, pB++, pBl++, pWh++, x++ ) {
				if( 0.5f < pW->z ) {
					VWB_WarpRecord* pW3 = pW + w;   // bottom

					// check for triangle A, this is only valid, if P5 is not valid
					if( 0 != x &&
						0.5f > ( pW - 1 )->z &&
						0.5f < pW3->z ) {
						VWB_WarpRecord* pW4 = pW + w - 1;   // bottom-left
						if( 0.5f < pW4->z )
							conv += getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 0, 1, -1, 1 );
					}

					VWB_WarpRecord* pW2 = pW + w + 1; // bottomright
					VWB_WarpRecord* pW1 = pW + 1; // right
					if( 0.5f < pW2->z ) {
						if( 0.5f < pW1->z ) // Triangle A
							conv += getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 1, 0, 1, 1 );

						if( 0.5f < pW3->z )  // Triangle B
							conv += getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 1, 1, 0, 1 );
					} else {
						if( 0.5f < pW1->z && 0.5f < pW3->z ) // Triangle D
							conv += getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 1, 0, 0, 1 );
					}
				}
			}
		}
	} else {
		for( VWB_float y = 0; pWE != pW; pW++, pB++, y++ ) {
			VWB_WarpRecord const* pWLE = pW + w - 1;
			for( VWB_float x = 0; pWLE != pW; pW++, pB++, x++ ) {
				if( 0.5f < pW->z ) {
					VWB_WarpRecord* pW3 = pW + w;   // bottom

					// check for triangle A, this is only valid, if P5 is not valid
					if( 0 != x &&
						0.5f > ( pW - 1 )->z &&
						0.5f < pW3->z ) {
						VWB_WarpRecord* pW4 = pW + w - 1;   // bottom-left
						if( 0.5f < pW4->z )
							conv += getTrianglePatch( out, x, y, pW, pB, pBl, pWh, 0, 1, -1, 1 );
					}

					VWB_WarpRecord* pW2 = pW + w + 1; // bottomright
					VWB_WarpRecord* pW1 = pW + 1; // right
					if( 0.5f < pW2->z ) {
						if( 0.5f < pW1->z ) // Triangle A
							conv += getTrianglePatchA( out, x, y, pW, pB, pBl, pWh, 1, 0, 1, 1 );

						if( 0.5f < pW3->z )  // Triangle B
							conv += getTrianglePatchA( out, x, y, pW, pB, pBl, pWh, 1, 1, 0, 1 );
					} else {
						if( 0.5f < pW1->z && 0.5f < pW3->z ) // Triangle D
							conv += getTrianglePatchA( out, x, y, pW, pB, pBl, pWh, 1, 0, 0, 1 );
					}

				}
			}
		}
	}

	// fill holes
	VWB_WarpRecord* pOutW = out.pWarp + w + 1;
	VWB_BlendRecord2* pOutB = out.pBlend2 + w + 1;
	VWB_BlendRecord* pOutBl = out.pBlack + w + 1;
	VWB_BlendRecord* pOutWh = out.pWhite + w + 1;
	for( VWB_WarpRecord const* pOutWE = pOutW + ptrdiff_t( nRecords ) - 2 * w; pOutWE != pOutW; pOutW += 2, pOutB += 2, pOutBl += 2, pOutWh += 2 ) {
		for( VWB_WarpRecord const* pOutWLE = pOutW + w - 2; pOutWLE != pOutW; pOutW++, pOutB++, pOutBl++, pOutWh++ ) {
			if( 0.5f > pOutW->z ) {
				VWB_WarpRecord* pW1 = pOutW - 1; // left
				VWB_WarpRecord* pW2 = pOutW + 1; // right
				VWB_WarpRecord* pW3 = pOutW - w; // top
				VWB_WarpRecord* pW4 = pOutW + w; // bottom
				if( 0.5f < pW1->z &&
					0.5f < pW2->z ) {
					pOutW->x = ( pW1->x + pW2->x ) / 2;
					pOutW->y = ( pW1->y + pW2->y ) / 2;
					pOutW->z = ( pW1->z + pW2->z ) / 2;
					if( out.pBlend2 ) {
						pOutB->r = (VWB_word)( ( (VWB_int)( pOutB - 1 )->r + (VWB_int)( pOutB + 1 )->r ) / 2 );
						pOutB->g = (VWB_word)( ( (VWB_int)( pOutB - 1 )->g + (VWB_int)( pOutB + 1 )->g ) / 2 );
						pOutB->b = (VWB_word)( ( (VWB_int)( pOutB - 1 )->b + (VWB_int)( pOutB + 1 )->b ) / 2 );
						pOutB->a = (VWB_word)( ( (VWB_int)( pOutB - 1 )->a + (VWB_int)( pOutB + 1 )->a ) / 2 );
					}
					conv++;
				} else if( 0.5f < pW3->z &&
						   0.5f < pW4->z ) {
					pOutW->x = ( pW3->x + pW4->x ) / 2;
					pOutW->y = ( pW3->y + pW4->y ) / 2;
					pOutW->z = ( pW3->z + pW4->z ) / 2;
					if( out.pBlend2 ) {
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

typedef enum ESPUMA_CtrlPointUseFlag {
	FLAG_CTRLPT_USE_UNSPECIFIC = 0x00,							///<   unspecific using
	FLAG_CTRLPT_USE_SELECTED = 0x01,							///<   point is selected
	FLAG_CTRLPT_USE_SELECTED_TANGENT = 0x02,							///<   a tangent to point is selected
	FLAG_CTRLPT_USE_SELECTED_TANGENT_SECOND = 0x04,							///<   second tangent point is selected
	FLAG_CTRLPT_USE_SELECTED_LINE_RIGHT = 0x08,							///<   line to the right is selected
	FLAG_CTRLPT_USE_SELECTED_LINE_BOTTOM = 0x10,							///<   line down is selected
	FLAG_CTRLPT_USE_SELECTED_ALL = 0x1F
}ESPUMA_CtrlPointUseFlag;

typedef enum ESPUMA_CtrlPointPosFlag {
	FLAG_CTRLPT_POS_NORMAL = 0x0,												///<   normal control point
	FLAG_CTRLPT_POS_UNKNOWN = 0x1,												///<   indicates an control point with unknown position
	FLAG_CTRLPT_POS_BORDER_TOP = 0x2,											///<   indicates that the control point is located on the top border of the control point mesh
	FLAG_CTRLPT_POS_BORDER_LEFT = 0x4,											///<   indicates that the control point is located on the left border of the control point mesh
	FLAG_CTRLPT_POS_BORDER_RIGHT = 0x8,										///<   indicates that the control point is located on the right border of the control point mesh
	FLAG_CTRLPT_POS_BORDER_BOTTOM = 0x10,										///<   indicates that the control point is located on the bottom border of the control point mesh
	FLAG_CTRLPT_POS_BORDER_DIAGONAL = 0x20,									///<   indicates that the control point is located on the diagonal border of the control point mesh
	FLAG_CTRLPT_POS_CLONE = 0x40,												///<   indicates that the control point is only a clone of another point
	FLAG_CTRLPT_POS_BORDER = FLAG_CTRLPT_POS_BORDER_TOP |					///<   all border flags
	FLAG_CTRLPT_POS_BORDER_LEFT |
	FLAG_CTRLPT_POS_BORDER_RIGHT |
	FLAG_CTRLPT_POS_BORDER_BOTTOM |
	FLAG_CTRLPT_POS_BORDER_DIAGONAL,
	FLAG_CTRLPT_POS_BORDER_LR = FLAG_CTRLPT_POS_BORDER_LEFT | 				///<   right and left border
	FLAG_CTRLPT_POS_BORDER_RIGHT,
	FLAG_CTRLPT_POS_TOPLEFT = FLAG_CTRLPT_POS_BORDER_TOP |				///<   edge top left, indicates the origin of the mesh
	FLAG_CTRLPT_POS_BORDER_LEFT,
	FLAG_CTRLPT_POS_TOPRIGHT = FLAG_CTRLPT_POS_BORDER_TOP |				///<   edge top right
	FLAG_CTRLPT_POS_BORDER_RIGHT,
	FLAG_CTRLPT_POS_BOTTOMLEFT = FLAG_CTRLPT_POS_BORDER_BOTTOM |			///<   edge bottom left
	FLAG_CTRLPT_POS_BORDER_LEFT,
	FLAG_CTRLPT_POS_BOTTOMRIGHT = FLAG_CTRLPT_POS_BORDER_BOTTOM |			///<   edge bottom right
	FLAG_CTRLPT_POS_BORDER_RIGHT
}ESPUMA_CtrlPointPosFlag;

typedef struct SPPair3f {
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
size_t triangulateGrid( std::vector<TP> const& vertices, long wGrid, long hGrid, std::vector<TI>& indices ) {
	size_t nOld = indices.size();
	indices.reserve( nOld + 6 * wGrid * hGrid );

	for( long y = 1; y != hGrid; y++ ) {
		for( long x = 1; x != wGrid; x++ ) {
			long pt3 = y * wGrid + x;
			long pt2 = pt3 - wGrid;
			long pt4 = pt3 - 1;
			long pt1 = pt2 - 1;

			if( W::valid( vertices[pt1] ) ) { // pt1 valid
				if( W::valid( vertices[pt3] ) ) {
					if( W::valid( vertices[pt4] ) ) {
						// triangle B valid, add it
						indices.push_back( pt1 );
						indices.push_back( pt3 );
						indices.push_back( pt4 );
					}
					if( W::valid( vertices[pt2] ) ) {
						// triangle A valid, add it
						indices.push_back( pt1 );
						indices.push_back( pt2 );
						indices.push_back( pt3 );
					}
				} else if(
					W::valid( vertices[pt2] ) &&
					W::valid( vertices[pt4] ) ) {
					// tiangle C valid
					indices.push_back( pt1 );
					indices.push_back( pt2 );
					indices.push_back( pt4 );
				}
			} else if(
				W::valid( vertices[pt2] ) &&
				W::valid( vertices[pt3] ) &&
				W::valid( vertices[pt4] ) ) {
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
bool RepairUniformGrid( std::vector<TP>& grid, int wGrid, int hGrid, int dist ) {
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

	VWB_size const delta[POS_REL_SIZE] = {
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
	size_t nSz = size_t( wGrid ) * hGrid;

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
		for( int y = 0; y != hGrid; y++ ) {
			for( int x = 0; x != wGrid; x++, pO++, pN++ ) {
				*pN = SPPair3f{ 0 };

				if( 0.5f <= pO->lPt2[2] )
					continue;

				int nMatches = 0;
				for( int i = 0; i != ARRAYSIZE( matches ); i++ ) {
					DynSPPointPairList3f::const_iterator ppOO[3] = { grid.end() };
					for( int j = 0; j != 3; j++ ) {
						if( x + delta[matches[i].pos[j]].cx < wGrid && // not over right border
							x + delta[matches[i].pos[j]].cx >= 0 && // not over left border
							y + delta[matches[i].pos[j]].cy < hGrid && // not over bottom border
							y + delta[matches[i].pos[j]].cy >= 0 ) // not over top border
						{
							ppOO[j] = grid.begin() + ( ( ptrdiff_t( y ) + delta[matches[i].pos[j]].cy ) * wGrid + x + delta[matches[i].pos[j]].cx );
							if( std::numeric_limits<float>::epsilon() > ppOO[j]->lPt2[2] ) {
								ppOO[0] = grid.end();
								break;
							}
						} else {
							ppOO[0] = grid.end();
							break;
						}
					}
					if( grid.end() == ppOO[0] )
						continue;

					// we have a match
					float vE[6] = { 0 };
					if( EST_TYPE_INTER == matches[i].est ) {
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
					} else {
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

				if( std::numeric_limits<float>::epsilon() <= pN->lPt2[2] ) {
					pN->lPt2[0] /= pN->lPt2[2];
					pN->lPt2[1] /= pN->lPt2[2];
					pN->lTangDescX[0] /= pN->lPt2[2];
					pN->lTangDescX[1] /= pN->lPt2[2];
					pN->lTangDescY[0] /= pN->lPt2[2];
					pN->lTangDescY[1] /= pN->lPt2[2];

					pN->lPt2[2] /= nMatches;

					if( std::numeric_limits<float>::epsilon() > pN->lPt2[2] )
						pN->lPt2[2] = std::numeric_limits<float>::epsilon();
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
		for( pO = grid.begin(), pN = newGrid.begin(); pO != grid.end(); pO++, pN++ ) {
			if( 0.5f <= pO->lPt2[2] ) {
			#ifdef _DEBUG
				good++;
			#endif
				continue;
			}
			if( scoreMax <= pN->lPt2[2] ) // has maximum score, so guessing is as good as it gets
			{
				pO->lPt2[0] = pN->lPt2[0];
				pO->lPt2[1] = pN->lPt2[1];
				pO->lPt2[2] = +.5f + MAX( std::numeric_limits<float>::epsilon(), pN->lPt2[2] * 0.5f ); // fit back to "good value" window
				pO->lTangDescX[0] = pN->lTangDescX[0];
				pO->lTangDescX[1] = pN->lTangDescX[1];
				pO->lTangDescY[0] = pN->lTangDescY[0];
				pO->lTangDescY[1] = pN->lTangDescY[1];
				added++;
			} else
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
struct SPPair3fWrangler : public SPPair3f {
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
	static inline SPPair3f make( bool valid, float x, float y, float z, float u, float v, float w, float r, float g, float b, float a ) {
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
void splitEdge( TP& p, TP& p1, float l, float r, long ip, long ip1, std::vector<TP>& newVtx, std::map<std::pair<TI, TI>, std::pair<TI, TI>>& cache, TI& i1, TI& i2, TI& c ) {
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
bool fixSeam( std::vector<TP>& vertices, std::vector<TI>& indices, long dim ) {
	std::vector<TP> newVtx;
	std::vector<TI> newIdx;
	std::map<std::pair<TI, TI>, std::pair<TI, TI>> cache; // this map points from a pair of indices defining an edge to the indices of the newly created vertices to avoid creating same vertices twice
	TI c = (TI)vertices.size(); // the index base
	for( TI* idx = indices.data(), *idxE = idx + indices.size(); idx != idxE; idx += 3 ) {
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
			if( idWrap[0] == idWrap[3] ) {
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

			if( bReverse ) {
				// first edge p - p1
				if( auto found = cache.find( std::pair( *idWrap[0], *idWrap[1] ) ); found != cache.end() ) {
					iv[1] = found->second.first;
					iv[0] = found->second.second;
					cache.erase( found );
				} else
					splitEdge<W, f>( p1, p, l, r, *idWrap[0], *idWrap[1], newVtx, cache, iv[1], iv[0], c );

				// second edge p - p2
				if( auto found = cache.find( std::pair( *idWrap[3], *idWrap[1] ) ); found != cache.end() ) { // these have already been created previously, we just use them
					iv[3] = found->second.first;
					iv[2] = found->second.second;
					cache.erase( found );
				} else // we need two new vertices
					splitEdge<W, f>( p2, p, l, r, *idWrap[3], *idWrap[1], newVtx, cache, iv[3], iv[2], c );
			} else {
				// first edge p - p1
				if( auto found = cache.find( std::pair( *idWrap[1], *idWrap[0] ) ); found != cache.end() ) {
					iv[0] = found->second.first;
					iv[1] = found->second.second;
					cache.erase( found );
				} else
					splitEdge<W, f>( p, p1, l, r, *idWrap[1], *idWrap[0], newVtx, cache, iv[0], iv[1], c );

				// second edge p - p2
				if( auto found = cache.find( std::pair( *idWrap[1], *idWrap[3] ) ); found != cache.end() ) { // these have already been created previously, we just use them
					iv[2] = found->second.first;
					iv[3] = found->second.second;
					cache.erase( found );
				} else // we need two new vertices
					splitEdge<W, f>( p, p2, l, r, *idWrap[1], *idWrap[3], newVtx, cache, iv[2], iv[3], c );
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
		} else if( n ) {// this is somthing odd...
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
int pseudoGridFromMapping( VWB_WarpRecord* pSrcD, VWB_BlendRecord2* pSrcDB, long width, long height, long wGrid, long hGrid, std::vector<TP>& vertices ) {
	if( wGrid > width / 2 ) // half size is OK
		wGrid = width / 2;
	if( hGrid > height / 2 ) // half size is OK
		hGrid = height / 2;

	if( nullptr == pSrcD || nullptr == pSrcDB
		) return 0;

	vertices.clear();
	vertices.reserve( ptrdiff_t( wGrid ) * hGrid );

	// first we create a P2C pixel aligned grid and fill with the values from P2C
	for( long yG = 0; yG != hGrid; yG++ ) {
		long y = yG * ( height - 1 ) / ( hGrid - 1 );
		for( long xG = 0; xG != wGrid; xG++ ) {
			long x = xG * ( width - 1 ) / ( wGrid - 1 );

			ptrdiff_t i = ptrdiff_t( y ) * width + x;

			VWB_WarpRecord const* pW = pSrcD + i;
			VWB_BlendRecord2 const* pB = pSrcDB + i;

			vertices.emplace_back( W::make(
				test( *pW ),
				pW->x, pW->y, pW->z,
				float( x ), float( y ), 0,
				float( pB->r ) / 65535.0f, float( pB->g ) / 65535.0f,
				float( pB->b ) / 65535.0f, float( pB->a ) / 65535.0f
			) );
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
int expandPseudoGrid( VWB_WarpRecord* pSrcD, VWB_BlendRecord2* pSrcDB, long width, long height, long wGrid, long hGrid, std::vector<TP>& vertices ) {
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

	VWB_size const delta[POS_REL_SIZE] = {
		{  0, -1 }, // top
		{  1, -1 }, // top-right
		{  1,  0 }, // right
		{  1,  1 }, // bottom-right
		{  0,  1 }, // bottom
		{ -1,  1 }, // bottom-left
		{ -1,  0 }, // left
		{ -1, -1 } // top-left
	};

	struct RepairItem {
		typename std::vector<TP>::iterator where; // the grid position 
		typename std::vector<TP>::value_type v;
	};
	std::list<RepairItem> repairs;

	long xs = width / wGrid;
	long ys = height / hGrid;


	if( xs && ys ) {
		auto pt = vertices.begin();
		for( long yG = 0; yG != hGrid; yG++ ) {
			for( long xG = 0; xG != wGrid; xG++, pt++ ) {
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
							) {
							auto const& opt = pt + ( ptrdiff_t( delta[i].cy ) * wGrid + ptrdiff_t( delta[i].cx ) ); // get the iterator of the other point
							if( W::valid( *opt ) ) {
								// got a valid point in some direction
								// so we go from me (pt) towards other (opt) until we find a valid point in the map
								long bx = long( W::u( *pt ) );
								long by = long( W::v( *pt ) );
								long ex = long( W::u( *opt ) );
								long ey = long( W::v( *opt ) );
								long x = bx + delta[i].cx;
								long y = by + delta[i].cy;

								while( x != ex || y != ey ) {

									ptrdiff_t off = ptrdiff_t( x ) + y * width;
									VWB_WarpRecord const* pW = pSrcD + off;
									VWB_BlendRecord2 const* pB = pSrcDB + off;

									if( test( *pW ) ) // hit!
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
									} else {
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
	for( auto r = repairs.begin(); r != repairs.end(); ) {
		if( minC == repairs.end() || minC->where != r->where ) {
			minC = r;
			float dx = W::u( *r->where ) - W::u( r->v );
			float dy = W::v( *r->where ) - W::v( r->v );
			minS = dx * dx + dy * dy; // we count in float to avoid int overflow
			r++;
		} else {
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
			} else // other was closer
			{
				// delete me
				r = repairs.erase( r );
			}

		}
	}

	// consolidate; transfer points to grid
	for( auto& r : repairs ) {
		*r.where = r.v;
	}
	return 1;
}

VWB_ERROR Dummywarper::getWarpBlend( VWB_WarpBlend const*& wb ) {
	wb = &m_wb;
	return VWB_ERROR_NONE;
}

// dump geometry to a stream in ply format
std::ostream& operator<<( std::ostream& os, VWB_WarpBlendMesh const& mesh ) {
	// dump the mesh as string in ply format using positions, uv and blend as color, the indices are uint32
	// write ply header
	os << "ply\nformat ascii 1.0\n";
	os << "element vertex " << mesh.nVtx << "\n";
	os << "property float x\nproperty float y\nproperty float z\n";
	os << "property float u\nproperty float v\n";
	os << "property float red\nproperty float green\nproperty float blue\n";
	os << "element face " << mesh.nIdx << "\n";
	os << "property list uchar int vertex_indices\n";
	os << "end_header\n";
	// dump vertices
	for( VWB_int i = 0; i != mesh.nVtx; i++ ) {
		auto const& v = mesh.vtx[i];
		os <<
			v.pos[0] << " " << v.pos[1] << " " << v.pos[2] << " " <<
			v.uv[0] << " " << v.uv[1] << " " <<
			v.rgb[0] << " " << v.rgb[1] << " " << v.rgb[2] << "\n";
	}
	// dump indices
	for( VWB_int i = 0; i != mesh.nIdx; i += 3 ) {
		os << "3 " << mesh.idx[i] << " " << mesh.idx[i + 1] << " " << mesh.idx[i + 2] << "\n";
	}

	os << std::endl;

	return os;
}

VWB_ERROR Dummywarper::getWarpMesh( VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh ) {
	if( 3 > cols || 3 > rows ) {
		logStr( 0, "ERROR: getWarpMesh needs >3 rows and columns.\n" );
		return VWB_ERROR_PARAMETER;
	}

	if( m_bDP ) {
		if( nullptr == m_wb.pMesh || nullptr == m_wb.pMesh->vtx || nullptr == m_wb.pMesh->idx ) {
			logStr( 0, "ERROR: getWarpMesh: no warp mesh available.\n" );
			return VWB_ERROR_GENERIC;
		}

		// tranalate from VWB_WarpBlendMeshEx to VWB_WarpBlendMesh

		logStr( 2, "INFO: getWarpMesh: transferring dp warp mesh to mesh...\n" );
		// create vertex buffer, we do not try to delete previous buffer, as this may be shared outside
		// remarks say it is up to the caller to deal with the buffer memory. This is due to Unreal Engine, which seem to hand dirty pointers over initially.
		mesh.nVtx = m_wb.pMesh->nVtx;
		mesh.vtx = new VWB_WarpBlendVertex[mesh.nVtx];

		// copy positions and UVs
		auto dst = mesh.vtx;
		for( auto src = m_wb.pMesh->vtx, srcE = src + m_wb.pMesh->nVtx; src != srcE; src++, dst++ ) {
			( m_mBaseI * VWB_VEC3f::ptr( src->pos ) ).SetPtr( dst->pos );
			for( int i = 0; i != _countof( src->uv ); i++ ) 
				dst->uv[i] = src->uv[i];
			// blend
			float x = dst->uv[0] * ( m_wb.header.width - 1 );
			float y = dst->uv[1] * ( m_wb.header.height - 1 );
			float x0 = floor( x );
			float y0 = floor( y );
			float tx = x - x0;
			float ty = y - y0;

			float x1 = x0 + 1;
			if( x1 >= m_wb.header.width ) // clamp
				x1 = x0;
			float y1 = y0 + 1;
			if( y1 >= m_wb.header.height ) // clamp
				y1 = y0;

			VWB_VEC3f rgb00; // blend at x0,y0
			VWB_VEC3f rgb01; // blend at x0,y1
			VWB_VEC3f rgb10; // blend at x1,y0
			VWB_VEC3f rgb11; // blend at x1,y1
			if( m_wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV2 ) {
				VWB_BlendRecord2 const* b = m_wb.pBlend2 + VWB_int( y0 ) * m_wb.header.width + VWB_int( x0 );
				rgb00 = VWB_VEC3f( float( b->r ) / 65535.f, float( b->g ) / 65535.f, float( b->b ) / 65535.f );
				b = m_wb.pBlend2 + VWB_int( y1 ) * m_wb.header.width + VWB_int( x0 );
				rgb10 = VWB_VEC3f( float( b->r ) / 65535.f, float( b->g ) / 65535.f, float( b->b ) / 65535.f );
				b = m_wb.pBlend2 + VWB_int( y0 ) * m_wb.header.width + VWB_int( x1 );
				rgb01 = VWB_VEC3f( float( b->r ) / 65535.f, float( b->g ) / 65535.f, float( b->b ) / 65535.f );
				b = m_wb.pBlend2 + VWB_int( y1 ) * m_wb.header.width + VWB_int( x1 );
				rgb11 = VWB_VEC3f( float( b->r ) / 65535.f, float( b->g ) / 65535.f, float( b->b ) / 65535.f );
			} else if( m_wb.header.flags & FLAG_WARPFILE_HEADER_BLENDV3 ) {
				VWB_BlendRecord3 const* b = m_wb.pBlend3 + VWB_int( y0 ) * m_wb.header.width + VWB_int( x0 );
				rgb00 = VWB_VEC3f( b->r, b->g, b->b );
				b = m_wb.pBlend3 + VWB_int( y1 ) * m_wb.header.width + VWB_int( x0 );
				rgb10 = VWB_VEC3f( b->r, b->g, b->b );
				b = m_wb.pBlend3 + VWB_int( y0 ) * m_wb.header.width + VWB_int( x1 );
				rgb01 = VWB_VEC3f( b->r, b->g, b->b );
				b = m_wb.pBlend3 + VWB_int( y1 ) * m_wb.header.width + VWB_int( x1 );
				rgb11 = VWB_VEC3f( b->r, b->g, b->b );
			} else {
				VWB_BlendRecord const* b = m_wb.pBlend + VWB_int( y0 ) * m_wb.header.width + VWB_int( x0 );
				rgb00 = VWB_VEC3f( float( b->r ) / 255.f, float( b->g ) / 255.f, float( b->b ) / 255.f );
				b = m_wb.pBlend + VWB_int( y1 ) * m_wb.header.width + VWB_int( x0 );
				rgb10 = VWB_VEC3f( float( b->r ) / 255.f, float( b->g ) / 255.f, float( b->b ) / 255.f );
				b = m_wb.pBlend + VWB_int( y0 ) * m_wb.header.width + VWB_int( x1 );
				rgb01 = VWB_VEC3f( float( b->r ) / 255.f, float( b->g ) / 255.f, float( b->b ) / 255.f );
				b = m_wb.pBlend + VWB_int( y1 ) * m_wb.header.width + VWB_int( x1 );
				rgb11 = VWB_VEC3f( float( b->r ) / 255.f, float( b->g ) / 255.f, float( b->b ) / 255.f );
			}

			// bilinear interpolation
			for( int i = 0; i != 3; i++ ) {
				dst->rgb[i] = 
					( 1.f - tx ) * ( 1.f - ty ) * rgb00[i] +
					( 1.f - tx ) * (       ty ) * rgb10[i] +
					(       tx ) * ( 1.f - ty ) * rgb01[i] +
					(       tx ) * (       ty ) * rgb11[i];
			}
		}

		// create index buffer
		mesh.nIdx = m_wb.pMesh->nIdx;
		mesh.idx = new VWB_uint[mesh.nIdx];

		// copy indices
		for( auto dstI = 0; dstI != mesh.nIdx; dstI++ )
			mesh.idx[dstI] = m_wb.pMesh->idx[dstI];

		// set dimension
		mesh.dim.cx = m_wb.header.width;
		mesh.dim.cy = m_wb.header.height;
	} else {
		VWB_int& w = m_wb.header.width;
		VWB_int& h = m_wb.header.height;
		int nRecords = w * h;

		if( 1024 > nRecords || NULL == m_wb.pWarp || NULL == m_wb.pBlend ) {
			logStr( 0, "ERROR: getWarpMesh: warp map too small.\n" );
			return VWB_ERROR_GENERIC;
		}
		logStr( 2, "INFO: getWarpMesh( %i, %i, * ): params OK.\n", cols, rows );

		DynSPPointPairList3f vertices;
		DynLongList	indices;
		if( m_bDynamicEye ) {
			if( pseudoGridFromMapping < testW, SPPair3fWrangler >( m_wb.pWarp, m_wb.pBlend2, w, h, cols, rows, vertices ) &&
				expandPseudoGrid< testW, SPPair3fWrangler >( m_wb.pWarp, m_wb.pBlend2, w, h, cols, rows, vertices ) &&
				triangulateGrid< SPPair3fWrangler >( vertices, cols, rows, indices ) ) {
				logStr( 2, "INFO: getWarpMesh: triangulation.\n" );
				mesh.nVtx = (VWB_uint)vertices.size();
				mesh.vtx = new VWB_WarpBlendVertex[vertices.size()];
				for( VWB_uint i = 0; i != vertices.size(); i++ ) {
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
				for( VWB_uint i = 0; i != indices.size(); i++ ) {
					mesh.idx[i] = indices[i];
				}

				mesh.dim.cx = w;
				mesh.dim.cy = h;
			} else {
				logStr( 0, "ERROR: getWarpMesh: Triangulation (3D) failed.\n" );
				return VWB_ERROR_GENERIC;
			}
		} else {
			if( pseudoGridFromMapping< testZ, SPPair3fWrangler >( m_wb.pWarp, m_wb.pBlend2, w, h, cols, rows, vertices ) &&
				expandPseudoGrid< testZ, SPPair3fWrangler >( m_wb.pWarp, m_wb.pBlend2, w, h, cols, rows, vertices ) &&
				triangulateGrid< SPPair3fWrangler >( vertices, cols, rows, indices ) &&
				fixSeam< SPPair3fWrangler, SPPair3fWrangler::x >( vertices, indices, w ) &&
				fixSeam< SPPair3fWrangler, SPPair3fWrangler::y >( vertices, indices, h )
				) {

				logStr( 2, "INFO: getWarpMesh: triangulation of grid (%ix%i) with %lu indices (%5.1f%%)\n", cols, rows, indices.size(), float( indices.size() ) / ( rows - 1 ) / ( cols - 1 ) / 6 * 100 );

				mesh.nVtx = (VWB_uint)vertices.size();
				mesh.vtx = new VWB_WarpBlendVertex[vertices.size()];
				VWB_MAT44f mVI = m_mViewIG.Inverted();

				for( VWB_uint i = 0; i != vertices.size(); i++ ) {
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
				for( VWB_uint i = 0; i != indices.size(); i++ ) {
					mesh.idx[i] = indices[i];
				}

				mesh.dim.cx = w;
				mesh.dim.cy = h;

			} else {
				logStr( 0, "ERROR: getWarpMesh: Triangulation (CT) failed.\n" );
				return VWB_ERROR_GENERIC;
			}
		}
	}
	logStr( 2, "INFO: getWarpMesh: Triangulation succeeded. %u vertices with %u triangles.\n", mesh.nVtx, mesh.nIdx / 3 );
	if( g_logLevel >= 3 && g_logFilePath.has_extension() ) { // lazy checking if a real log file is set
		std::filesystem::path meshpath( g_logFilePath ); // dupe path
		// add channel and change extension
		if( m_bUTF8 )
			meshpath.replace_extension( std::u8string( (char8_t const*)channel ) + u8".ply" );
		else
			meshpath.replace_extension( std::string( channel ) + ".ply" );
		std::ofstream ofs( meshpath );
		if( !ofs.is_open() ) {
			logStr( 3, "WARNING: getWarpMesh: cannot open mesh dump file '%s'\n", meshpath.string().c_str() );
		}
		ofs << mesh;
	}
	return VWB_ERROR_NONE;
}
