// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#pragma once

#include "../WarperBase.h"
#include "../common.h"
#include <D3Dcompiler.h>
//#include <DxErr.h>
#include "../3rdparty/d3dX/Include/d3dx9math.h"
struct DXSCREENVERTEX
{
	D3DXVECTOR3 pos;
	D3DXVECTOR2 tex1;

	static const DWORD FVF;
};


class DXWarpBlend : public VWB_Warper_base
{
public:

	typedef __declspec( align(4) ) struct PosT {FLOAT x,y,z;} PosT;
	typedef __declspec( align(4) ) struct TexT { FLOAT u,v; } TexT;
	typedef __declspec( align(4) ) struct ColT { FLOAT r,g,b,a; } ColT;
	typedef __declspec( align(4) ) struct SimpleVertex
	{
		PosT Pos;
		TexT Tex;
	} SimpleVertex;

	typedef __declspec( align(4) ) struct DirVertex
	{
		PosT Pos;
		TexT Tex;
		PosT Nor;
		PosT Tan;
	} DirVertex;

	typedef  __declspec( align( 4 ) ) struct ConstantBuffer
	{
		FLOAT matView[16];
		FLOAT border[4];
		FLOAT params[4];
		FLOAT offsScale[4];
		FLOAT offsScaleCur[4];
		FLOAT blackBias[4];
	} ConstantBuffer;

	typedef  __declspec( align( 4 ) ) struct DPConstantBuffer
	{
		FLOAT gamma;
		FLOAT doWarp;
		FLOAT doBlend;
		FLOAT doBlack;
		FLOAT do2ndBlend;
		FLOAT inputGamma;
		FLOAT outputGamma;
		FLOAT colorCorr;
		FLOAT flip_v;
		FLOAT reserved[7]; // in total 16 until here
		FLOAT mvp[16];
		FLOAT pos[4];
	} DPConstantBuffer;

	const FLOAT _black[4] = { 0,0,0,1 };

protected:
	char					m_modelPath[MAX_PATH];  // the path to a successful loaded model
public:
	///< the constructor
	DXWarpBlend();

    ///< the destructor
	virtual ~DXWarpBlend();

	virtual VWB_ERROR Init( VWB_WarpBlendSet& wbs );

	VWB_MAT44f UpdateView( VWB_MAT44f const& igView, VWB_VEC3f& e );
	virtual VWB_ERROR GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj );
	virtual VWB_ERROR GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip );
	virtual VWB_ERROR GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric, VWB_float aspect ) override;

	virtual VWB_ERROR SetViewProjection( VWB_float const* pView, VWB_float const* pProj );

};
