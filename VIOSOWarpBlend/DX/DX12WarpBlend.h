// VIOSO API
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#pragma once

#include "DXWarpBlend.h"
#include <D3D12.h>

// Frames in flight
// ----------------
// Render() uses a ring of FRAMES_IN_FLIGHT slots. Each slot has its own descriptor for the content texture (heap index 4 + slot,
// root parameter 2), its own range in the constant buffer and, if the caller passes no command list, its own command allocator
// and command list. A slot is reused only after the GPU has finished the work that used it; the warper waits on its own fence
// on the command queue if that has not happened yet.
//
// Assumption: with VWB_D3D12_RENDERINPUT::commandList set, the caller submits that list to the command queue passed to
// VWB_Create before it calls Render() for the same warper again (the usual way: record, close, ExecuteCommandLists, next frame).
// The warper signals its fence at the begin of every Render(); that signal covers the list of the previous call.
// Without commandList the warper executes its own list and signals right after it. Not supported: Render() of one warper from
// several threads at the same time.
class DX12WarpBlend : public DXWarpBlend
{
public:
	static const UINT FRAMES_IN_FLIGHT = 3;	///< number of slots, see above

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
	ID3D12Resource*				m_texWarp;
	ID3D12Resource*				m_texBlend;
	ID3D12Resource*				m_texBlack;
	ID3D12Resource*				m_texCur;
	ID3D12Resource*				m_cb; // the constant buffer, FRAMES_IN_FLIGHT ranges of m_cbStride bytes
	void*						m_cbMap; // the constant buffer map address
	UINT						m_cbStride; // size of one constant buffer range, a multiple of 256

	ID3D12Fence*				m_fence;			// signals the end of the GPU work that used a slot
	HANDLE						m_fenceEvent;		// to wait for m_fence
	UINT64						m_fenceValue;		// last value signalled
	UINT						m_slotNext;			// the slot of the next Render() call
	int							m_slotPending;		// slot of the last Render() with a caller's command list, not yet covered by a signal, or -1
	struct Slot
	{
		ID3D12Resource*				res;		// the texture the SRV of this slot points to (referenced)
		bool						resOwn;		// res is a copy of the render target made by the warper
		VWB_size					size;		// size of res
		UINT64						fence;		// value of m_fence that is reached when the GPU is done with this slot
		ID3D12CommandAllocator*		ca;			// used only without the caller's command list
		ID3D12GraphicsCommandList*	cl;
	} m_slot[FRAMES_IN_FLIGHT];
	D3D12_VIEWPORT				m_vp;				// the viewport, if width and height is set, use it on begin of rendering

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

    virtual VWB_ERROR Render( VWB_param inputTexture, VWB_uint stateMask );  

protected:
};
