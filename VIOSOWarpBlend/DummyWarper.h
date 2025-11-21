// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#pragma once

#include "WarperBase.h"

class Dummywarper : public VWB_Warper_base
{
protected:
	VWB_WarpBlend m_wb;
public:
	Dummywarper();

	virtual ~Dummywarper(void);

	virtual VWB_ERROR Init( VWB_WarpBlendSet& wbs );

	VWB_MAT44f UpdateView( VWB_MAT44f const& igView, VWB_VEC3f& e );
	virtual VWB_ERROR GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj );
	virtual VWB_ERROR GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip );
	virtual VWB_ERROR GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pSymClip, bool symmetric, VWB_float aspect ) override;

	virtual VWB_ERROR SetViewProjection( VWB_float const* pView, VWB_float const* pProj );

	virtual VWB_ERROR Render( VWB_param inputTexture, VWB_uint stateMask );  

	virtual VWB_ERROR getWarpBlend( VWB_WarpBlend const*& wb );
	virtual VWB_ERROR getWarpMesh( VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh );
};
