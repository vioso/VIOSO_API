// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#pragma once

#include "DXWarpBlend.h"
#include <D3D12.h>

class DX12WarpBlend : public DXWarpBlend
{
public:

protected:
	ID3D12CommandQueue*			m_cq;				// the command queue
	ID3D12Device*				m_device;           // the d3d device
	ID3D12DescriptorHeap*		m_srvHeap;			// the heap of shader resource views
//	ID3D12DescriptorHeap*		m_samHeap;			// the heap of sampler views
	ID3D12CommandAllocator*		m_ca;               // the command allocator
	ID3D12GraphicsCommandList*	m_cl;				// the command list
	ID3D12RootSignature*		m_rootSignature;    // the root signature
	ID3D12PipelineState*		m_pipelineState;	// the pipeline
	ID3D12Resource*				m_vertexBuffer;		// the vertex buffer
	D3D12_VERTEX_BUFFER_VIEW    m_vertexBufferView;    // the vertex buffer view
	ID3D12Resource*				m_indexBuffer;		// the index buffer
	D3D12_INDEX_BUFFER_VIEW		m_indexBufferView;    // the index buffer view
	ID3D12Resource*				m_texWarp;
	ID3D12Resource*				m_texBlend;
	ID3D12Resource*				m_texBlendX;
	ID3D12Resource*				m_texBlack;
	ID3D12Resource*				m_texCur;
	ID3D12Resource*				m_texBlend2; // the second blend texture
	ID3D12Resource*				m_texDirectionalShading; // the directional shading texture
	ID3D12Resource*				m_texDUMMY; // the directional shading texture
	ID3D12Resource*				m_texBB;
	ID3D12Resource*				m_cb; // the constant buffer
	void*						m_cbMap; // the constant buffer map address
	D3D12_VIEWPORT				m_vp;				// the viewport, if width and height is set, use it on begin of rendering
	VWB_uint					m_nIndices;			// number of indices in the index buffer


public:
	///< the constructor
	DX12WarpBlend( ID3D12CommandQueue* pCQ );

    ///< the destructor
	virtual ~DX12WarpBlend();

	virtual VWB_ERROR Init( VWB_WarpBlendSet& wbs );

 
    /** setModelDX9
	* set a 3D Model to calculate a surface 
    * @param [IN]		pModelView	it gets the updated view matrix to translate and rotate into the viewer's perspective
    * @param [IN]		path path to the model file
	* @param [IN,OPT]	nBlendChannels number of other channels to blend with
	* @param [IN,OPT]	pBlendChannels a list of handles of other channels
    * @return TRUE on success, FALSE otherwise */
	//VWB_ERROR DX9WarpBlend::setModel( D3DXMATRIX* pModelView, LPCSTR path, int nBlendChannels, DX9WarpBlend* pBlendChannels );

	virtual VWB_ERROR Render( VWB_param inputTexture, VWB_uint stateMask ) override;  
	virtual VWB_ERROR Render2( VWB_param inputTexture, VWB_uint stateMask ) override;  

protected:
};
