// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#ifndef VWB_WIN7_COMPAT
#include "DX12WarpBlend.h"
#include "../3rdparty/d3dX/Include/d3dx12.h"
#include "StringConversions.h"

#include "pixelshader.h"
#include "atlbase.h"
#include <span>
#include <fstream>
#ifdef _DEBUG
#include <iomanip>
#endif
//#pragma comment( lib, "d3d12.lib" )

extern "C" HRESULT WINAPI D3D12SerializeVersionedRootSignature(
	_In_ const D3D12_VERSIONED_ROOT_SIGNATURE_DESC * pRootSignature,
	_Out_ ID3DBlob * *ppBlob,
	_Always_( _Outptr_opt_result_maybenull_ ) ID3DBlob * *ppErrorBlob )
{
	typedef HRESULT (WINAPI *FnD3D12SerializeVersionedRootSignature)(
	_In_ const D3D12_VERSIONED_ROOT_SIGNATURE_DESC * pRootSignature,
		_Out_ ID3DBlob * *ppBlob,
		_Always_( _Outptr_opt_result_maybenull_ ) ID3DBlob * *ppErrorBlob );

	HRESULT res = E_FAIL;
	if( ppBlob )
		*ppBlob = nullptr;
	if( ppErrorBlob )
		*ppErrorBlob = nullptr;

	HMODULE hDll = ::LoadLibraryA( "D3D12.dll" );
	if( hDll )
	{
		FnD3D12SerializeVersionedRootSignature pFn = ( FnD3D12SerializeVersionedRootSignature )::GetProcAddress( hDll, "D3D12SerializeVersionedRootSignature" );
		if( pFn )
		{
			res = pFn( pRootSignature, ppBlob, ppErrorBlob );
		}
		::FreeLibrary( hDll );
	}
	return res;
}

#define RPT_HR_FATAL_BL( EXP, MSG, BLOB, RET ) do { \
	auto hr_ = ( EXP );\
	if( FAILED( hr_ ) ) {\
		if( BLOB ) {\
			logStr( 0, "FATAL: %s [%08X]\n%s\n", MSG, hr, (char*)BLOB->GetBufferPointer() );\
		} else {\
			logStr( 0, "FATAL: %s [%08X]\n", MSG, hr );\
		}\
		return RET;\
	}\
} while( 0 )

#define RPT_HR_FATAL( EXP, MSG, RET ) do { \
	auto hr_ = ( EXP );\
	if( FAILED( hr_ ) ) {\
		logStr( 0, "FATAL: %s [%08X]\n", MSG, hr );\
		return RET;\
	}\
} while( 0 )

#define RPT_FATAL( EXP, MSG, RET ) do { \
	if( !( EXP ) ){\
		logStr( 0, "FATAL: %s\n", MSG );\
		return RET;\
	}\
} while( 0 )

struct D3D12Allocator {
	ID3D12CommandQueue* cq;
	ID3D12PipelineState* ps;
	ID3D12CommandAllocator* ca;
	ID3D12GraphicsCommandList* cl;
	ID3D12Fence* f;
	UINT64 fv;
	HANDLE fh;

	D3D12Allocator() : cq( nullptr ), ps( nullptr ), ca( nullptr ), cl( nullptr ), f( nullptr ), fv( 0 ), fh( 0 ) {}
	D3D12Allocator( ID3D12Device* pDevice, ID3D12CommandQueue* pQueue, ID3D12PipelineState* pipelineState = nullptr, bool canWait = true ) {
		init( pDevice, pQueue, pipelineState, canWait );
	}

	bool init( ID3D12Device* pDevice, ID3D12CommandQueue* pQueue, ID3D12PipelineState* pipelineState = nullptr, bool canWait = true ) {
		// check
		if( !pDevice || !pQueue )
			return false;
		cq = pQueue;
		cq->AddRef();

		ps = pipelineState;
		if( ps ) ps->AddRef(); // this is optional, might be NULL

		HRESULT hr = pDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &ca ) );
		if( FAILED( hr ) ) return false;
		hr = pDevice->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, ca, nullptr, IID_PPV_ARGS( &cl ) );
		if( FAILED( hr ) ) return false;
		if( canWait ) {
			hr = pDevice->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &f ) );
			if( FAILED( hr ) ) return false;
			fh = ::CreateEvent( nullptr, FALSE, FALSE, nullptr );
			if( !fh )
				return false;
			fv = 1;
		}
		return true;
	}

	void swap( D3D12Allocator& other ) noexcept {
		ID3D12CommandQueue* tcq = cq;
		cq = other.cq;
		other.cq = tcq;

		ID3D12PipelineState* tps = ps;
		ps = other.ps;
		other.ps = tps;

		ID3D12CommandAllocator* tca = ca;
		ca = other.ca;
		other.ca = tca;

		ID3D12GraphicsCommandList* tcl = cl;
		cl = other.cl;
		other.cl = tcl;

		ID3D12Fence* tf = f;
		f = other.f;
		other.f = tf;

		UINT64 tfv = fv;
		fv = other.fv;
		other.fv = tfv;

		HANDLE tfh = fh;
		fh = other.fh;
		other.fh = tfh;
	}

	D3D12Allocator( D3D12Allocator const& ) = delete;
	D3D12Allocator( D3D12Allocator&& other ) noexcept {
		other.swap( *this );
	}

	D3D12Allocator& operator=( D3D12Allocator const& ) = delete;
	D3D12Allocator& operator=( D3D12Allocator&& other ) noexcept {
		other.swap( *this );
		return *this;
	}

	~D3D12Allocator() {
		if( fh )
			::CloseHandle( fh );
		SAFE_RELEASE( f );
		SAFE_RELEASE( cl );
		SAFE_RELEASE( ca );
		SAFE_RELEASE( ps );
		SAFE_RELEASE( cq );
	}

	void reset( ID3D12PipelineState* newPSO = nullptr ) {
		if( newPSO && newPSO != ps )
			ps = newPSO;
		ca->Reset();
		cl->Reset( ca, ps );
	}

	void wait() {
		if( f && fh ) {
			auto old = fv++;
			cq->Signal( f, old );
			if( f->GetCompletedValue() < old ) {
				f->SetEventOnCompletion( old, fh );
				::WaitForSingleObject( fh, INFINITE );
			}
		}
	}

	void waitFor( ID3D12Fence* fence, UINT64 fenceValue ) {
		if( fence->GetCompletedValue() < fenceValue ) {
			fence->SetEventOnCompletion( fenceValue, fh );
			::WaitForSingleObject( fh, INFINITE );
		}
	}

	void signal( ID3D12Fence* fence, UINT64 fenceValue ) const {
		cq->Signal( fence, fenceValue );
	}

	void exec( bool doWait = false, bool doReset = false ) {
		cl->Close();
		cq->ExecuteCommandLists( 1, ( ID3D12CommandList* const* )&cl );
		if( doWait ) wait();
		if( doReset ) reset();
	}

	void trans( ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, UINT subres = 0 ) {
		D3D12_RESOURCE_BARRIER barr{ D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, { .Transition{ res, subres, before, after } } };
		cl->ResourceBarrier( 1, { &barr } );
	}

	operator bool() { return ca && cl && ( ( f && fh ) || !( f || fh ) ); }
	ID3D12GraphicsCommandList* operator->() { return cl; }
	ID3D12GraphicsCommandList* getCl() { return cl; }
};

float half2float( uint16_t half ) {
	auto Mantissa = static_cast<uint32_t>(half & 0x03FF);

	uint32_t Exponent = (half & 0x7C00);
	if (Exponent == 0x7C00) // INF/NAN
	{
		Exponent = 0x8f;
	}
	else if (Exponent != 0)  // The value is normalized
	{
		Exponent = static_cast<uint32_t>((static_cast<int>(half) >> 10) & 0x1F);
	}
	else if (Mantissa != 0)     // The value is denormalized
	{
		// Normalize the value in the resulting float
		Exponent = 1;

		do
		{
			Exponent--;
			Mantissa <<= 1;
		} while ((Mantissa & 0x0400) == 0);

		Mantissa &= 0x03FF;
	}
	else                        // The value is zero
	{
		Exponent = static_cast<uint32_t>(-112);
	}

	uint32_t Result =
		((static_cast<uint32_t>(half) & 0x8000) << 16) // Sign
		| ((Exponent + 112) << 23)                      // Exponent
		| (Mantissa << 13);                             // Mantissa

	return reinterpret_cast<float*>(&Result)[0];
}

HRESULT saveTextureToFile( ID3D12Device* device, ID3D12CommandQueue* queue, std::filesystem::path fileName, ID3D12Resource* texture, D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE )
{
	D3D12_RESOURCE_DESC textureDesc = texture->GetDesc();
	UINT64 requiredSize = 0;

	device->GetCopyableFootprints( &textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &requiredSize );

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Alignment = 0;
	bufferDesc.Width = requiredSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CComPtr<ID3D12Resource> stagingResource;
	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS( &stagingResource ) );
	if( FAILED( hr ) ) return hr;

	D3D12_TEXTURE_COPY_LOCATION dst = {};
	dst.pResource = stagingResource;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	device->GetCopyableFootprints( &textureDesc, 0, 1, 0, &dst.PlacedFootprint, nullptr, nullptr, nullptr );

	D3D12_TEXTURE_COPY_LOCATION src = {};
	src.pResource = texture;
	src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	src.SubresourceIndex = 0;

	{
		D3D12Allocator alloc( device, queue );
		alloc.trans( texture, state, D3D12_RESOURCE_STATE_COPY_SOURCE );
		alloc->CopyTextureRegion( &dst, 0, 0, 0, &src, nullptr );
		alloc.trans( texture, D3D12_RESOURCE_STATE_COPY_SOURCE, state );
		alloc.exec( true, false );
	}

	// Map the staging resource
	void* data;
	D3D12_RANGE readRange = { 0, requiredSize }; // We intend to read the entire buffer
	hr = stagingResource->Map( 0, &readRange, &data );
	if( FAILED( hr ) ) return hr;

	// Save the data to a BMP file
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	device->GetCopyableFootprints( &textureDesc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr );
	UINT rowPitch = footprint.Footprint.RowPitch;
	UINT height = textureDesc.Height;
	UINT width = (UINT)textureDesc.Width;
	UINT bytesPerPixel = 4; // Assuming DXGI_FORMAT_R8G8B8A8_UNORM
	switch( textureDesc.Format ) {
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		bytesPerPixel = 8;
		break;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		bytesPerPixel = 16;
		break;
	};
	UINT bmRowPitch = ( UINT )(( textureDesc.Width * 4 + 3) & (~3));
	UINT padd = bmRowPitch - UINT( textureDesc.Width * 4 );

	BITMAPFILEHEADER fileHeader = {};
	BITMAPINFOHEADER infoHeader = {};

	fileHeader.bfType = 0x4D42; // 'BM'
	fileHeader.bfOffBits = sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER );
	fileHeader.bfSize = fileHeader.bfOffBits + bmRowPitch * height;

	infoHeader.biSize = sizeof( BITMAPINFOHEADER );
	infoHeader.biWidth = width;
	infoHeader.biHeight = -static_cast< int >( height ); // Negative height to indicate top-down bitmap
	infoHeader.biPlanes = 1;
	infoHeader.biBitCount = bytesPerPixel * 8;
	infoHeader.biCompression = BI_RGB;
	infoHeader.biSizeImage = bmRowPitch * height;

	std::ofstream file( fileName, std::ios::binary );
	if( !file.is_open() ) return E_INVALIDARG;
	file.write( reinterpret_cast< const char* >( &fileHeader ), sizeof( fileHeader ) );
	file.write( reinterpret_cast< const char* >( &infoHeader ), sizeof( infoHeader ) );
	// note: bitmap has brga ordering
	switch( textureDesc.Format ) {
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		for( auto p = reinterpret_cast< uint8_t* >( data ), pE = p + rowPitch * height; p != pE; p+= rowPitch )
			for( auto pp = p, ppE = p + 8 * width; pp != ppE; pp += 8 ) {
				uint8_t bgra[4]{
					uint8_t( 255 * half2float( *(uint16_t*)( pp + 4 ) ) ),
					uint8_t( 255 * half2float( *(uint16_t*)( pp + 2 ) ) ),
					uint8_t( 255 * half2float( *(uint16_t*)( pp ) ) ),
					uint8_t( 255 * half2float( *(uint16_t*)( pp + 6 ) ) ),
				};
				file.write( (char const*)bgra, 4 );
			}
		break;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		for( auto p = reinterpret_cast< uint8_t* >( data ), pE = p + rowPitch * height; p != pE; p+= rowPitch )
			for( auto pp = p, ppE = p + 16 * width; pp != ppE; pp += 16 ) {
				uint8_t bgra[4]{
					uint8_t( 255 * *(float*)( pp + 8 ) ),
					uint8_t( 255 * *(float*)( pp + 4 ) ),
					uint8_t( 255 * *(float*)( pp ) ),
					uint8_t( 255 * *(float*)( pp + 12 ) ),
				};
				file.write( (char const*)bgra, 4 );
			}
		break;
	default:
		for( auto p = reinterpret_cast< const char* >( data ), pE = p + rowPitch * height; p != pE; p+= rowPitch )
			for( auto pp = p, ppE = p + bmRowPitch; pp != ppE; pp += bytesPerPixel ) {
				file.put( pp[2] );
				file.put( pp[1] );
				file.put( pp[0] );
				file.put( pp[3] );
			}
	};
	file.close();

	// Unmap the resource when done
	stagingResource->Unmap( 0, nullptr );
	return S_OK;
}

DX12WarpBlend::DX12WarpBlend( ID3D12CommandQueue* pCQ )
: DXWarpBlend()
, m_cq( NULL )
, m_device( NULL )
, m_ca( NULL )
, m_cl( NULL )
, m_srvHeap( NULL )
, m_rootSignature( NULL )
, m_pipelineState( NULL )
, m_vertexBuffer( NULL )
, m_vertexBufferView( { 0, 0, 0 } )
, m_indexBuffer( NULL )
, m_indexBufferView( { 0, 0, DXGI_FORMAT_R32_UINT } )
, m_texWarp( NULL )
, m_texBlend( NULL )
, m_texBlend2( NULL )
, m_texDirectionalShading( NULL )
, m_texBlack( NULL )
, m_texCur( NULL )
, m_texDUMMY( NULL )
, m_texBB( NULL )
, m_cb( NULL )
, m_cbMap( NULL )
, m_vp{0,0,1,1,0,1}
, m_nIndices( 0 )
{
	if( NULL == pCQ )
		throw( VWB_ERROR_PARAMETER );
	if( FAILED( pCQ->QueryInterface( &m_cq ) ) )
		throw( VWB_ERROR_PARAMETER );
	if( FAILED( m_cq->GetDevice( IID_PPV_ARGS( &m_device  ) ) ) )
		throw( VWB_ERROR_PARAMETER );
		m_type4cc = '21XD';
}

DX12WarpBlend::~DX12WarpBlend(void)
{
	SAFE_RELEASE( m_texWarp );
	SAFE_RELEASE( m_texBlend );
	SAFE_RELEASE( m_texBlack );
	SAFE_RELEASE( m_texCur );
	SAFE_RELEASE( m_texBlend2 );
	SAFE_RELEASE( m_texDirectionalShading );
	SAFE_RELEASE( m_texDUMMY );
	SAFE_RELEASE( m_texBB );
	if( m_cb && m_cbMap )
		m_cb->Unmap( 0, nullptr );
	SAFE_RELEASE( m_cb );
	SAFE_RELEASE( m_vertexBuffer );
	SAFE_RELEASE( m_indexBuffer );
	SAFE_RELEASE( m_pipelineState );
	SAFE_RELEASE( m_rootSignature );
	SAFE_RELEASE( m_srvHeap );
	SAFE_RELEASE( m_device );
	SAFE_RELEASE( m_cl );
	SAFE_RELEASE( m_ca );
	logStr( 1, "INFO: DX12-Warper destroyed.\n" );
}

HRESULT CreateAndFillResource( ID3D12Device* dev, ID3D12GraphicsCommandList* cl, D3D12_RESOURCE_DESC const& desc, D3D12_SUBRESOURCE_DATA& data, ID3D12Resource*& res, std::vector< CComPtr< ID3D12Resource > >& sts, LPCWSTR name = L"", D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATES finalState = D3D12_RESOURCE_STATE_GENERIC_READ )
{
	CD3DX12_HEAP_PROPERTIES hp( D3D12_HEAP_TYPE_DEFAULT );
	HRESULT hr = dev->CreateCommittedResource(
		&hp,
		//&D3D12_HEAP_PROPERTIES( { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 } ),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		initialState,
		nullptr,
		IID_PPV_ARGS( &res ) );
	if( FAILED( hr ) )
		return hr;

	UINT64 uploadBufferSize = 0;
	dev->GetCopyableFootprints( &desc, 0, 1, 0, nullptr, nullptr, nullptr, &uploadBufferSize );

	CComPtr<ID3D12Resource> uploadHeapTex;
	// Create the GPU upload buffer.
	hp.Type = D3D12_HEAP_TYPE_UPLOAD;
	CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Buffer( uploadBufferSize );
	hr = dev->CreateCommittedResource(
		&hp,
		//&D3D12_HEAP_PROPERTIES( { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 } ),
		D3D12_HEAP_FLAG_NONE,
		&rd,
		//&D3D12_RESOURCE_DESC( { D3D12_RESOURCE_DIMENSION_BUFFER, 0, uploadBufferSize, 1, 1, 1, DXGI_FORMAT_UNKNOWN, {1,0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE } ),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS( &uploadHeapTex ) );
	if( FAILED( hr ) )
		return hr;

	hr = uploadBufferSize == UpdateSubresources( cl, res, uploadHeapTex, 0, 0, 1, &data ) ? S_OK : E_FAIL;
	if( SUCCEEDED( hr ) )
	{
		sts.push_back( uploadHeapTex );
		if( nullptr != name && 0 != name[0] )
			res->SetName( name );
	}

	if( SUCCEEDED( hr ) && finalState != initialState ) {
		CD3DX12_RESOURCE_BARRIER barr = CD3DX12_RESOURCE_BARRIER::Transition( res, initialState, finalState );
		cl->ResourceBarrier( 1, &barr );
	}

	return hr;
}

VWB_ERROR DX12WarpBlend::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = __super::Init( wbs );
	HRESULT hr = E_FAIL;
	if( VWB_ERROR_NONE == err )
	{
		// the staging textures are needed to be copied over to 
		// the static GPU-only-access textures. We can release after executing the initialisation command list
		std::vector<CComPtr<ID3D12Resource>> stagingTexs;
		if( m_bDP ) {
			VWB_WarpBlend& wb = *wbs[calibIndex];
			if( !wb.pMesh ) {
				logStr( 0, "ERROR: no screen shape in mapping." );
				return VWB_ERROR_GENERIC;
			}
			auto& warpmap = *wb.pMesh;

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = 5;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			heapDesc.NodeMask = 0;

			RPT_HR_FATAL( m_device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &m_srvHeap ) ), "Failed to create SRV/CBV descriptor heap.", VWB_ERROR_GENERIC );
			m_srvHeap->SetName( L"VWB_srvheap" );

			CComPtr< ID3DBlob > errBlob;

			// root signature
			{
				// these are the shader resource descriptions, we need 2 slots
				CD3DX12_ROOT_PARAMETER1 rootParameters[2] = {};
				// we put the constant buffer signature to slot 0 of the root parameters
				rootParameters[0].InitAsConstantBufferView( 0 );

				// we create a descriptor table for the SRVs, as there are 2 different kind of textures
				CD3DX12_DESCRIPTOR_RANGE1 ranges1[2] = {};
				// first we put the input texture signature, this is a READ_WRITE texture
				ranges1[0].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE );
				// second go our 4 STATIC textures used for blend,secondary blend, blacklevel and color-corretion
				ranges1[1].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC );

				// we put the SRVs in slot 1 of the root parameters
				rootParameters[1].InitAsDescriptorTable( _countof( ranges1 ), ranges1, D3D12_SHADER_VISIBILITY_PIXEL );

				// now we define the sampler descs, which is just one
				D3D12_STATIC_SAMPLER_DESC samplers[] =
				{
					{ // linear border, s0
						D3D12_FILTER_MIN_MAG_MIP_LINEAR,
						D3D12_TEXTURE_ADDRESS_MODE_BORDER,
						D3D12_TEXTURE_ADDRESS_MODE_BORDER,
						D3D12_TEXTURE_ADDRESS_MODE_BORDER,
						0,
						0,
						D3D12_COMPARISON_FUNC_NEVER,
						D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
						0.0f,
						D3D12_FLOAT32_MAX,
						0,
						0,
						D3D12_SHADER_VISIBILITY_ALL
					},
					// point clamp, s1
					{
						D3D12_FILTER_MIN_MAG_MIP_POINT,
						D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
						D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
						D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
						0,
						0,
						D3D12_COMPARISON_FUNC_NEVER,
						D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
						0.0f,
						D3D12_FLOAT32_MAX,
						1,
						0,
						D3D12_SHADER_VISIBILITY_ALL
					},
					// linear wrap, s2
					{
						D3D12_FILTER_MIN_MAG_MIP_LINEAR,
						D3D12_TEXTURE_ADDRESS_MODE_WRAP,
						D3D12_TEXTURE_ADDRESS_MODE_WRAP,
						D3D12_TEXTURE_ADDRESS_MODE_WRAP,
						0,
						0,
						D3D12_COMPARISON_FUNC_NEVER,
						D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
						0.0f,
						D3D12_FLOAT32_MAX,
						2,
						0,
						D3D12_SHADER_VISIBILITY_ALL
					}
				};

				// now we bake this all into the root signature desc
				CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
				rootSignatureDesc.Init_1_1(
					_countof( rootParameters ), rootParameters,
					_countof( samplers ), samplers, 
					D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT );
				// to create the root signature, we need to serialize the desc into byte-code
				CComPtr<ID3DBlob> signature;
				RPT_HR_FATAL_BL( D3D12SerializeVersionedRootSignature( &rootSignatureDesc, &signature, &errBlob ), "Failed to serialize root signature", errBlob, VWB_ERROR_GENERIC );
				errBlob.Release();
				// now we create the root signature
				RPT_HR_FATAL( m_device->CreateRootSignature( 0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS( &m_rootSignature ) ), "Failed to create root signature", VWB_ERROR_SHADER );
			}

			// pipeline
			{
				CComPtr<ID3DBlob> VSBlob;
				CComPtr<ID3DBlob> PSBlob;
				UINT compileFlags = 0;
			#ifdef _DEBUG
				compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
			#endif
				// Compile shaders
				auto compile = [&errBlob,compileFlags]( LPCSTR name, LPCSTR entryPoint, LPCSTR target, ID3DBlob** code ) {
					HRESULT hr = D3DCompile( s_dpShaderDX4, strlen( s_dpShaderDX4 ), name, nullptr, nullptr, entryPoint, target, compileFlags, 0, code, &errBlob );
					auto sz = errBlob ? (char const*)errBlob->GetBufferPointer() : "";
					return hr;
					};

				RPT_HR_FATAL_BL( compile( "dpShaderD3D11_VSStatic", "VSMESH", "vs_5_0", &VSBlob ), "Failed to compile static vertex shader", errBlob, VWB_ERROR_SHADER );
				errBlob.Release();
				RPT_HR_FATAL_BL( compile( "dpShaderD3D11_VSDynamic", "PSDP", "ps_5_0", &PSBlob ), "Failed to compile correction pixel shader", errBlob, VWB_ERROR_SHADER );
				errBlob.Release();

				// Define the input layout
				D3D12_INPUT_ELEMENT_DESC const layout[] =
				{
					{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
					{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
					{ "NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
					{ "TANGENT" , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				};

				// Describe and create the graphics pipeline state object (PSO).
				D3D12_GRAPHICS_PIPELINE_STATE_DESC psDesc = {};
				psDesc.InputLayout = { layout, _countof( layout ) };
				psDesc.pRootSignature = m_rootSignature;
				psDesc.VS = CD3DX12_SHADER_BYTECODE( VSBlob );
				psDesc.PS = CD3DX12_SHADER_BYTECODE( PSBlob );
				psDesc.RasterizerState = CD3DX12_RASTERIZER_DESC( D3D12_DEFAULT );
				psDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
				psDesc.BlendState = CD3DX12_BLEND_DESC( D3D12_DEFAULT );
				psDesc.DepthStencilState.DepthEnable = FALSE;
				psDesc.DepthStencilState.StencilEnable = FALSE;
				psDesc.SampleMask = UINT_MAX;
				psDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				psDesc.NumRenderTargets = 1;
				psDesc.RTVFormats[0] = (DXGI_FORMAT)D3D12RTVF;
				psDesc.SampleDesc.Count = 1;

				RPT_HR_FATAL( m_device->CreateGraphicsPipelineState( &psDesc, IID_PPV_ARGS( &m_pipelineState ) ), "Could not create graphics pipeline", VWB_ERROR_SHADER );
				m_pipelineState->SetName( L"Vdp_pipeline" );
			}

			// create command allocator and command list
			{
				RPT_HR_FATAL( m_device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &m_ca ) ), "Could not create command allocator", VWB_ERROR_SHADER );
				m_ca->SetName( L"VWB_commandallocator" );

				RPT_HR_FATAL( m_device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_ca, m_pipelineState, IID_PPV_ARGS( &m_cl ) ), "Could not create command list", VWB_ERROR_SHADER );
				m_cl->SetName( L"VWB_commandlist" );
			}

			// Create the constant buffer and view.
			{
				const UINT cbSize = sizeof( DPConstantBuffer );
				const UINT cbBuffSize = ( cbSize + 255 ) & ~255U; // round up

				CD3DX12_HEAP_PROPERTIES props( D3D12_HEAP_TYPE_UPLOAD );
				CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer( cbBuffSize );

				// we create a dynamic buffer inside the upload heap
				// this will be marshalled every time, the vertex shader is executed
				// we can just keep the buffer mapped
				HRESULT hr = m_device->CreateCommittedResource(
					&props,
					D3D12_HEAP_FLAG_NONE,
					&desc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS( &m_cb ) );

				RPT_HR_FATAL( hr, "Could not create constant buffer", VWB_ERROR_GENERIC );

				// just map the buffer and keep it mapped
				CD3DX12_RANGE readRange( 0, 0 );        // We do not intend to read from this resource on the CPU.
				RPT_HR_FATAL( m_cb->Map( 0, &readRange, &m_cbMap ), "Could not map constant buffer", VWB_ERROR_GENERIC );
			}

			// create and fill textures 
			{
				VWB_WarpBlend& wb = *wbs[calibIndex];
				D3D12_RESOURCE_DESC textureDesc = {
					D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					0, //UINT64 Alignment;
					(UINT64)m_sizeMap.cx,//UINT Width;
					(UINT)m_sizeMap.cy,//UINT Height;
					1,//UINT16 DepthOrArraySize;
					1,//UINT16 MipLevels;
					DXGI_FORMAT_R8G8B8A8_UNORM,//DXGI_FORMAT Format;
					{1,0},//DXGI_SAMPLE_DESC SampleDesc;
					D3D12_TEXTURE_LAYOUT_UNKNOWN,// D3D12_TEXTURE_LAYOUT Layout;
					D3D12_RESOURCE_FLAG_NONE,// D3D12_RESOURCE_FLAGS Flags;
				};
				static const uint8_t _white[4] = { 255,255,255,255 };
				struct TexDesc {
					UINT64 width;
					UINT height;
					DXGI_FORMAT format;
					ID3D12Resource** ppTex;
					D3D12_SUBRESOURCE_DATA pData;
					LPCWSTR					name;
				} texDescs[] {
					{ (UINT64)wb.directionalSz.cx, (UINT)wb.directionalSz.cy, DXGI_FORMAT_R8G8B8A8_UNORM, &m_texDirectionalShading, { wb.pDirectional, wb.directionalSz.cx * sizeof( VWB_BlendRecord ) }, L"dp_texDirectionalShading" },
					{ (UINT64)m_sizeMap.cx, (UINT)m_sizeMap.cy, DXGI_FORMAT_R16G16B16A16_UNORM, &m_texBlend, { wb.pBlend2, m_sizeMap.cx * sizeof( VWB_BlendRecord2 ) }, L"dp_texBlend" },
					{ (UINT64)m_sizeMap.cx, (UINT)m_sizeMap.cy, DXGI_FORMAT_R8G8B8A8_UNORM, &m_texBlack, { wb.pBlack, m_sizeMap.cx * sizeof( VWB_BlendRecord ) }, L"dp_texBlack" },
					{ (UINT64)m_sizeMap.cx, (UINT)m_sizeMap.cy, DXGI_FORMAT_R8G8B8A8_UNORM, &m_texBlend2, { wb.p2ndBlend, m_sizeMap.cx * sizeof( VWB_BlendRecord ) }, L"dp_texBlend2" },
				};

				for( auto& td : std::span( texDescs ) )
					if( td.pData.pData ) {
						textureDesc.Width = td.width;
						textureDesc.Height = td.height;
						textureDesc.Format = td.format;
						RPT_HR_FATAL( CreateAndFillResource( m_device, m_cl, textureDesc, td.pData, *td.ppTex, stagingTexs, td.name, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ), (std::string( "Could not create texture \"" ) + VWBUtil::to_string( td.name ) + "\"" ).c_str(), VWB_ERROR_GENERIC );
					}

				// create dummy texture
				textureDesc.Width = 1;
				textureDesc.Height = 1;
				textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				D3D12_SUBRESOURCE_DATA dummyData = { _white, sizeof( _white ), sizeof( _white ) };
				RPT_HR_FATAL( CreateAndFillResource( m_device, m_cl, textureDesc, dummyData, m_texDUMMY, stagingTexs, L"dp_texDUMMY", D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE ), "Could not create DUMMY texture", VWB_ERROR_GENERIC );

				// create list for SRVs, we take dummy texture if one of the other textures is not available
				std::vector<ID3D12Resource*> textureRes;
				for( auto& td : std::span( texDescs, 4 ) )
					if( *td.ppTex ) {
						textureRes.push_back( *td.ppTex );
					} else {
						textureRes.push_back( m_texDUMMY ); // put dummy texture if not available
					}

				// Describe and create a SRVs for the textures on srvHeap.
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
					.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
					.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
					.Texture2D{ .MipLevels = 1 }
				};
				SIZE_T descSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
				for( SIZE_T i = 0; i != textureRes.size(); i++ )
					m_device->CreateShaderResourceView( textureRes[i], NULL, { m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr + i * descSize } );
			}

			// Create the vertex buffer.
			{
				std::vector<DirVertex> vertices( warpmap.nVtx, DirVertex{} );
				auto const* wv = warpmap.vtx;
				for( auto& v : vertices ) {
					v.Pos.x = wv->pos[0];
					v.Pos.y = wv->pos[1];
					v.Pos.z = wv->pos[2];

					v.Tex.u = wv->uv[0];
					v.Tex.v = wv->uv[1];

					v.Nor.x = wv->n[0];
					v.Nor.y = wv->n[1];
					v.Nor.z = wv->n[2];

					v.Tan.x = wv->t[0];
					v.Tan.y = wv->t[1];
					v.Tan.z = wv->t[2];

					wv++;
				}

				const UINT vertexBufferSize = UINT(vertices.size()) * sizeof( DirVertex );

				auto data = D3D12_SUBRESOURCE_DATA{ vertices.data(), vertexBufferSize } ;
				auto desc = CD3DX12_RESOURCE_DESC::Buffer( vertexBufferSize );
				RPT_HR_FATAL( CreateAndFillResource( m_device, m_cl, desc, data, m_vertexBuffer, stagingTexs, L"dp_VertexBuffer", D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER ), "Could not create vertex buffer", VWB_ERROR_GENERIC );

				// Initialize the vertex buffer view.
				m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
				m_vertexBufferView.StrideInBytes = sizeof( DirVertex );
				m_vertexBufferView.SizeInBytes = vertexBufferSize;
			}

			// Create the index buffer.
			{
				VWB_WarpBlend& wb = *wbs[calibIndex];

				const UINT indexBufferSize = UINT( warpmap.nIdx ) * sizeof( uint32_t );

				auto data = D3D12_SUBRESOURCE_DATA{ warpmap.idx, indexBufferSize } ;
				auto desc = CD3DX12_RESOURCE_DESC::Buffer( indexBufferSize );
				RPT_HR_FATAL( CreateAndFillResource( m_device, m_cl, desc, data, m_indexBuffer, stagingTexs, L"dp_IndexBuffer", D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER ), "Could not create index buffer", VWB_ERROR_GENERIC );

				// Initialize the vertex buffer view.
				m_indexBufferView.BufferLocation = ( m_indexBuffer )->GetGPUVirtualAddress();
				m_indexBufferView.SizeInBytes = indexBufferSize;
				m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;

				m_nIndices = (UINT)warpmap.nIdx;
			}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		} else {
			// Describe and create a shader resource view (SRV) heap for the texture.
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = 5;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			heapDesc.NodeMask = 0;

			if( FAILED( m_device->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &m_srvHeap ) ) ) ) {
				logStr( 0, "FATAL ERROR: Failed to create SRV/CBV descriptor heap.\n" );
				return VWB_ERROR_GENERIC;
			}
			m_srvHeap->SetName( L"VWB_srvheap" );

			{
				D3D12_FEATURE_DATA_ROOT_SIGNATURE sig = { D3D_ROOT_SIGNATURE_VERSION_1 };

				if( FAILED( m_device->CheckFeatureSupport( D3D12_FEATURE_ROOT_SIGNATURE, &sig, sizeof( sig ) ) ) ) {
					logStr( 0, "FATAL ERROR: Failed to create root signature.\n" );
					return VWB_ERROR_GENERIC;
				}

				CD3DX12_DESCRIPTOR_RANGE1 ranges1[2] = {};
				ranges1[0].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC, 0 );
				ranges1[1].Init( D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 4 );
				CD3DX12_ROOT_PARAMETER1 rootParameters[2] = {};
				rootParameters[0].InitAsConstantBufferView( 0 );
				rootParameters[1].InitAsDescriptorTable( _countof( ranges1 ), ranges1, D3D12_SHADER_VISIBILITY_PIXEL );

				D3D12_STATIC_SAMPLER_DESC samplers[] =
				{
					{
						D3D12_FILTER_MIN_MAG_MIP_LINEAR,
						D3D12_TEXTURE_ADDRESS_MODE_BORDER,
						D3D12_TEXTURE_ADDRESS_MODE_BORDER,
						D3D12_TEXTURE_ADDRESS_MODE_BORDER,
						0,
						0,
						D3D12_COMPARISON_FUNC_NEVER,
						D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
						0.0f,
						D3D12_FLOAT32_MAX,
						0,
						0,
						D3D12_SHADER_VISIBILITY_ALL
					},
					{
						D3D12_FILTER_MIN_MAG_MIP_POINT,
						D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
						D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
						D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
						0,
						0,
						D3D12_COMPARISON_FUNC_NEVER,
						D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
						0.0f,
						D3D12_FLOAT32_MAX,
						1,
						0,
						D3D12_SHADER_VISIBILITY_ALL
					},
					{
						D3D12_FILTER_MIN_MAG_MIP_LINEAR,
						bFixWraparound ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_BORDER,
						bFixWraparound ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_BORDER,
						bFixWraparound ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_BORDER,
						0,
						0,
						D3D12_COMPARISON_FUNC_NEVER,
						D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
						0.0f,
						D3D12_FLOAT32_MAX,
						2,
						0,
						D3D12_SHADER_VISIBILITY_ALL
					},
				};

				CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
				rootSignatureDesc.Init_1_1( _countof( rootParameters ), rootParameters, _countof( samplers ), samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT );
				CComPtr< ID3DBlob > signature;
				CComPtr< ID3DBlob > error;

				if( FAILED( D3D12SerializeVersionedRootSignature( &rootSignatureDesc, &signature, &error ) ) ) {
					logStr( 0, "Error: Serializing root signature: %s", nullptr != error ? (char*)error->GetBufferPointer() : "unkown" );
					return VWB_ERROR_SHADER;
				}
				hr = m_device->CreateRootSignature( 0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS( &m_rootSignature ) );
				if( FAILED( hr ) ) {
					logStr( 0, "Error: CreateRootSignature failed with %x", hr );
					return VWB_ERROR_SHADER;
				}
				m_rootSignature->SetName( L"VWB_rootsignature" );
			}

			// build pipeline
			{
				// Define the input layout
				D3D12_INPUT_ELEMENT_DESC layout[] =
				{
					{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof( SimpleVertex, Pos ), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
					{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof( SimpleVertex, Tex ), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				};

				// compile shader
			#if defined(_DEBUG)
				// Enable better shader debugging with the graphics debugging tools.
				UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
			#else
				UINT compileFlags = 0;
			#endif
				CComPtr< ID3DBlob > errBlob;
				CComPtr< ID3DBlob > vsBlob;
				CComPtr< ID3DBlob > psBlob;
				if( bFlipDXVs ) {
					hr = D3DCompile( s_pixelShaderDX4_vFlip, sizeof( s_pixelShaderDX4_vFlip ), NULL, NULL, NULL, "VS", "vs_5_0", compileFlags, 0, &vsBlob, &errBlob );
				} else {
					hr = D3DCompile( s_pixelShaderDX4, sizeof( s_pixelShaderDX4 ), NULL, NULL, NULL, "VS", "vs_5_0", compileFlags, 0, &vsBlob, &errBlob );
				}
				if( FAILED( hr ) ) {
					logStr( 0, "ERROR: The vertex shader code cannot be compiled: %s\n", errBlob->GetBufferPointer() );
					return VWB_ERROR_SHADER;
				}

				std::string pixelShader = "PS"; // or "TST"
				if( m_bDynamicEye ) {
					pixelShader = "PSWB3D";
				} else {
					pixelShader = "PSWB";
				}
				if( bBicubic )
					pixelShader.append( "BC" );
				if( bFlipDXVs ) {
					hr = D3DCompile( s_pixelShaderDX4_vFlip, sizeof( s_pixelShaderDX4_vFlip ), NULL, NULL, NULL, pixelShader.c_str(), "ps_5_0", compileFlags, 0, &psBlob, &errBlob );
				} else {
					hr = D3DCompile( s_pixelShaderDX4, sizeof( s_pixelShaderDX4 ), NULL, NULL, NULL, pixelShader.c_str(), "ps_5_0", compileFlags, 0, &psBlob, &errBlob );
				}
				if( FAILED( hr ) ) {
					logStr( 0, "ERROR: The pixel shader code cannot be compiled (%08X): %s\n", hr, errBlob->GetBufferPointer() );
					return VWB_ERROR_SHADER;
				}

				// Describe and create the graphics pipeline state object (PSO).
				D3D12_GRAPHICS_PIPELINE_STATE_DESC psDesc = {};
				psDesc.InputLayout = { layout, _countof( layout ) };
				psDesc.pRootSignature = m_rootSignature;
				psDesc.VS = CD3DX12_SHADER_BYTECODE( vsBlob );
				psDesc.PS = CD3DX12_SHADER_BYTECODE( psBlob );
				psDesc.RasterizerState = CD3DX12_RASTERIZER_DESC( D3D12_DEFAULT );
				psDesc.BlendState = CD3DX12_BLEND_DESC( D3D12_DEFAULT );
				psDesc.DepthStencilState.DepthEnable = FALSE;
				psDesc.DepthStencilState.StencilEnable = FALSE;
				psDesc.SampleMask = UINT_MAX;
				psDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
				psDesc.NumRenderTargets = 1;
				psDesc.RTVFormats[0] = (DXGI_FORMAT)D3D12RTVF;
				psDesc.SampleDesc.Count = 1;

				hr = m_device->CreateGraphicsPipelineState( &psDesc, IID_PPV_ARGS( &m_pipelineState ) );
				if( FAILED( hr ) ) {
					logStr( 0, "ERROR: Could not create graphics pipeline: %08X\n", hr );
					return VWB_ERROR_SHADER;
				}
				m_pipelineState->SetName( L"VWB_pipeline" );
			}

			// create command allocator and command list
			{
				hr = m_device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &m_ca ) );
				if( FAILED( hr ) ) {
					logStr( 0, "ERROR: Could not create command allocator: %08X\n", hr );
					return VWB_ERROR_SHADER;
				}
				m_ca->SetName( L"VWB_commandallocator" );
				hr = m_device->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_ca, m_pipelineState, IID_PPV_ARGS( &m_cl ) );
				if( FAILED( hr ) ) {
					logStr( 0, "ERROR: Could not create command list: %08X\n", hr );
					return VWB_ERROR_SHADER;
				}
				m_cl->SetName( L"VWB_commandlist" );
			}

			// create and fill vertex buffer
			{
				FLOAT dx = 0;//.5f / m_sizeMap.cx;
				FLOAT dy = 0;//.5f / m_sizeMap.cy;
				SimpleVertex quad[] = {

					{ {  1.0f + dx,  1.0f + dy, 0.5f }, { 1.0f, 0.0f } },
					{ {  1.0f + dx, -1.0f - dy, 0.5f }, { 1.0f, 1.0f } },
					{ { -1.0f - dx, -1.0f - dy, 0.5f }, { 0.0f, 1.0f } },

					{ {  1.0f + dx,  1.0f + dy, 0.5f }, { 1.0f, 0.0f } },
					{ { -1.0f - dx, -1.0f - dy, 0.5f }, { 0.0f, 1.0f } },
					{ { -1.0f - dx,  1.0f + dy, 0.5f }, { 0.0f, 0.0f } },
				};

				CD3DX12_HEAP_PROPERTIES hp( D3D12_HEAP_TYPE_UPLOAD );
				CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Buffer( sizeof( quad ) );
				hr = m_device->CreateCommittedResource(
					&hp,
					D3D12_HEAP_FLAG_NONE,
					&rd,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS( &m_vertexBuffer ) );

				// Copy the triangle data to the vertex buffer.
				UINT8* pVertexDataBegin;
				CD3DX12_RANGE readRange( 0, 0 );        // We do not intend to read from this resource on the CPU.
				hr = m_vertexBuffer->Map( 0, &readRange, reinterpret_cast<void**>( &pVertexDataBegin ) );
				memcpy( pVertexDataBegin, quad, sizeof( quad ) );
				// our buffer is static, so we can Unmap
				m_vertexBuffer->Unmap( 0, nullptr );

				// Initialize the vertex buffer view.
				m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
				m_vertexBufferView.StrideInBytes = sizeof( SimpleVertex );
				m_vertexBufferView.SizeInBytes = sizeof( quad );

				m_vertexBuffer->SetName( L"VWB_vertexbuffer" );
			}

			// Create the constant buffer
			{
				UINT szA = sizeof( ConstantBuffer );
				UINT sz = ( ( szA + 255 ) / 256 ) * 256;
				CD3DX12_HEAP_PROPERTIES hp( D3D12_HEAP_TYPE_UPLOAD );
				CD3DX12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Buffer( sz );
				hr = m_device->CreateCommittedResource(
					&hp,
					D3D12_HEAP_FLAG_NONE,
					&rd,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS( &m_cb )
				);

				if( SUCCEEDED( hr ) ) {
					void* pData;
					CD3DX12_RANGE readRange( 0, 0 );
					m_cb->Map( 0, &readRange, &pData );
					m_cbMap = pData;
					memset( m_cbMap, 0, sizeof( ConstantBuffer ) );
				} else {
					logStr( 0, "ERROR: Could not create constant buffer: %08X\n", hr );
					return VWB_ERROR_SHADER;
				}

				m_cb->SetName( L"VWB_constantbuffer" );
			}

			// create and fill textures
			{
				VWB_WarpBlend& wb = *wbs[calibIndex];

				// the warp texture
				// it is RGBA32, we need RG16U in case of 2D and RGB32F in case of 3D
				D3D12_RESOURCE_DESC textureDesc = {
					D3D12_RESOURCE_DIMENSION_TEXTURE2D,
					0, //UINT64 Alignment;
					(UINT64)m_sizeMap.cx,//UINT Width;
					(UINT64)m_sizeMap.cy,//UINT Height;
					1,//UINT16 DepthOrArraySize;
					1,//UINT16 MipLevels;
					0 != ( wb.header.flags & FLAG_WARPFILE_HEADER_3D ) ? DXGI_FORMAT_R32G32B32_FLOAT : DXGI_FORMAT_R16G16_UNORM,//DXGI_FORMAT Format;
					{1,0},//DXGI_SAMPLE_DESC SampleDesc;
					D3D12_TEXTURE_LAYOUT_UNKNOWN,// D3D12_TEXTURE_LAYOUT Layout;
					D3D12_RESOURCE_FLAG_NONE,// D3D12_RESOURCE_FLAGS Flags;
				};

				D3D12_SUBRESOURCE_DATA data;
				if( wb.header.flags & FLAG_WARPFILE_HEADER_3D ) {
					UINT sz = 3 * m_sizeMap.cx;
					data.RowPitch = sizeof( float ) * sz;
					sz *= m_sizeMap.cy;
					data.SlicePitch = sizeof( float ) * sz;
					data.pData = new float[sz];
					float* d = (float*)data.pData;
					for( const VWB_WarpRecord* s = wb.pWarp, *sE = wb.pWarp + (ptrdiff_t)m_sizeMap.cx * (ptrdiff_t)m_sizeMap.cy; s != sE; d += 3, s++ ) {
						d[0] = s->x;
						d[1] = s->y;
						d[2] = s->z;
					}
				} else {
					UINT sz = 2 * m_sizeMap.cx;
					data.RowPitch = sizeof( unsigned short ) * sz;
					sz *= m_sizeMap.cy;
					data.SlicePitch = sizeof( unsigned short ) * sz;
					data.pData = new unsigned short[sz];
					unsigned short* d = (unsigned short*)data.pData;
					for( const VWB_WarpRecord* s = wb.pWarp, *sE = wb.pWarp + (ptrdiff_t)m_sizeMap.cx * (ptrdiff_t)m_sizeMap.cy; s != sE; d += 2, s++ ) {
						d[0] = (unsigned short)( 65535.0f * MIN( 1.0f, MAX( 0.0f, s->x ) ) );
						d[1] = (unsigned short)( 65535.0f * MIN( 1.0f, MAX( 0.0f, s->y ) ) );
					}
				}

				hr = CreateAndFillResource( m_device, m_cl, textureDesc, data, m_texWarp, stagingTexs, L"VWB_warptexture", D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );
				delete[]( float* )data.pData;
				if( FAILED( hr ) ) {
					logStr( 0, "ERROR: Could not fill warp texture: %08X\n", hr );
					return VWB_ERROR_SHADER;
				}

				textureDesc.Format = DXGI_FORMAT_R16G16B16A16_UNORM;
				data.RowPitch = sizeof( unsigned short ) * 4 * m_sizeMap.cx;
				data.SlicePitch = data.RowPitch * m_sizeMap.cy;
				data.pData = wb.pBlend2;

				hr = CreateAndFillResource( m_device, m_cl, textureDesc, data, m_texBlend, stagingTexs, L"VWB_blendtexture" );
				if( FAILED( hr ) ) {
					logStr( 0, "ERROR: Could not fill warp texture: %08X\n", hr );
					return VWB_ERROR_SHADER;
				}

				// fill black texture
				char empty[16 * 16 * 4] = { 0 };
				if( wb.pBlack ) {
					textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					data.RowPitch = sizeof( unsigned char ) * 4 * m_sizeMap.cx;
					data.SlicePitch = data.RowPitch * m_sizeMap.cy;
					data.pData = wb.pBlack;

					hr = CreateAndFillResource( m_device, m_cl, textureDesc, data, m_texBlack, stagingTexs, L"VWB_blackleveltexture" );
					if( FAILED( hr ) ) {
						logStr( 0, "ERROR: Could not fill black level texture: %08X\n", hr );
						return VWB_ERROR_SHADER;
					}
				} else {
					textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					textureDesc.Width = 16;
					textureDesc.Height = 16;
					data.RowPitch = sizeof( unsigned char ) * 4 * 16;
					data.SlicePitch = data.RowPitch * 16;
					data.pData = empty;

					hr = CreateAndFillResource( m_device, m_cl, textureDesc, data, m_texBlack, stagingTexs, L"VWB_blackleveltexture" );
					if( FAILED( hr ) ) {
						logStr( 0, "ERROR: Could not fill black level texture: %08X\n", hr );
						return VWB_ERROR_SHADER;
					}
				}
				// prepare cursor texture
				textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				textureDesc.Width = 16;
				textureDesc.Height = 16;
				data.RowPitch = sizeof( unsigned char ) * 4 * 16;
				data.SlicePitch = data.RowPitch * 16;
				data.pData = empty;

				hr = CreateAndFillResource( m_device, m_cl, textureDesc, data, m_texCur, stagingTexs, L"VWB_cursortexture" );
				if( FAILED( hr ) ) {
					logStr( 0, "ERROR: Could not fill cursor texture: %08X\n", hr );
					return VWB_ERROR_SHADER;
				}

				// Describe and create a SRVs for the textures on srvHeap.
				ID3D12Resource* textureRes[]{ m_texWarp, m_texBlend, m_texCur, m_texBlack };
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				SIZE_T descSize = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

				for( SIZE_T i = 0; i != _countof( textureRes ); i++ ) {
					m_device->CreateShaderResourceView( textureRes[i], NULL, { m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr + i * descSize } );
				}
			}
		}
		// execute
		{
			hr = m_cl->Close();
			if( FAILED( hr ) ) {
				logStr( 0, "ERROR: Could not fill command list: %08X\n", hr );
				return VWB_ERROR_SHADER;
			}
			// Create synchronization objects and wait until assets have been uploaded to the GPU.
			CComPtr<ID3D12Fence> fence;
			hr = m_device->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &fence ) );
			if( FAILED( hr ) ) {
				logStr( 0, "ERROR: Could not create fence: %08X\n", hr );
				return VWB_ERROR_SHADER;
			}
			hr = m_cq->Signal( fence, 0 );
			if( FAILED( hr ) ) {
				logStr( 0, "ERROR: Could not set signal: %08X\n", hr );
				return VWB_ERROR_SHADER;
			}
			UINT64 last = fence->GetCompletedValue();

			// Create an event handle to use for frame synchronization.
			HANDLE fenceEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr );
			if( fenceEvent == nullptr ) {
				logStr( 0, "ERROR: Could not create event: %08X\n", HRESULT_FROM_WIN32( GetLastError() ) );
				return VWB_ERROR_GENERIC;
			}

			// run list
			m_cq->ExecuteCommandLists( 1, (ID3D12CommandList* const*)&m_cl );

			hr = m_cq->Signal( fence, ++last );
			UINT64 current = fence->GetCompletedValue();
			if( current < last ) {
				hr = fence->SetEventOnCompletion( last, fenceEvent );
				WaitForSingleObject( fenceEvent, INFINITE );
			}
		}
		logStr( 1, "SUCCESS: DX12-Warper initialized.\n" );
	}
	return err;
}

VWB_ERROR DX12WarpBlend::Render( VWB_param inputTexture, VWB_uint stateMask ) {
	__super::Render( inputTexture, stateMask );
	logStr( 4, "DX12::Render..." );
	auto in = (VWB_D3D12_RENDERINPUT2*)inputTexture;
	VWB_D3D12_RENDERINPUT2 ex = {};
	memcpy( &ex, in, sizeof( VWB_D3D12_RENDERINPUT ) );
	return Render2( (VWB_param)&ex, stateMask );
}

VWB_ERROR DX12WarpBlend::Render2( VWB_param inputTexture, VWB_uint stateMask )
{
	__super::Render2( inputTexture, stateMask );
	logStr( 4, "DX12::Render2..." );

	if( !m_licenseInfo->isValid )
	{
		logStr( 1, "WARNING: Invalid license for DX12 warper. Output may be watermarked." );
	}

	if( VWB_STATEMASK_STANDARD == stateMask )
		stateMask = VWB_STATEMASK_DEFAULT_D3D12;

	auto in = (VWB_D3D12_RENDERINPUT2*)inputTexture;
	if( nullptr == in || nullptr == in->renderTarget || 0 == in->rtvHandlePtr )
		return VWB_ERROR_PARAMETER;
	HRESULT hr = S_OK;

	ID3D12GraphicsCommandList* cl = nullptr;
	if( in->commandList )
	{
		cl = (ID3D12GraphicsCommandList*)in->commandList;
		cl->SetPipelineState( m_pipelineState );
	}
	else
	{
		hr = m_ca->Reset();
		hr = m_cl->Reset( m_ca, m_pipelineState );
		cl = m_cl;
	}

	ID3D12Resource* rt = (ID3D12Resource*)in->renderTarget;
	D3D12_RESOURCE_DESC descRT = rt->GetDesc();

	if( nullptr == in->textureResource )
	{

		if( nullptr == m_texBB || descRT.Width != m_texBB->GetDesc().Width )
		{
			// create res#
			CD3DX12_HEAP_PROPERTIES hd( D3D12_HEAP_TYPE_DEFAULT );
			hr = m_device->CreateCommittedResource(
				&hd,
				D3D12_HEAP_FLAG_NONE,
				&descRT,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				nullptr,
				IID_PPV_ARGS( &m_texBB ) );
			SIZE_T base = m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr;
			SIZE_T ds = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
			SIZE_T des = base + 4 * ds; // put content to 5th slot
			m_device->CreateShaderResourceView(
				m_texBB,
				nullptr,
				{ des }
			);
			m_texBB->SetName( L"VWB_backbuffercopytexture" );
			m_sizeIn.cx = (VWB_int)descRT.Width;
			m_sizeIn.cy = (VWB_int)descRT.Height;
		}

		// prepare pipeline for copy
		D3D12_RESOURCE_BARRIER rB[]
		{
			{
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				D3D12_RESOURCE_BARRIER_FLAG_NONE,
				D3D12_RESOURCE_TRANSITION_BARRIER{
					(ID3D12Resource*)in->renderTarget,
					D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_COPY_SOURCE
				}
			},
			{
				D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				D3D12_RESOURCE_BARRIER_FLAG_NONE,
				D3D12_RESOURCE_TRANSITION_BARRIER{
					(ID3D12Resource*)m_texBB,
					D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					D3D12_RESOURCE_STATE_COPY_DEST
				}
			},
		};
		cl->ResourceBarrier( _countof( rB ), rB );

		// issue copy
		cl->CopyResource( m_texBB, rt );

		// make rt render target again
		rB[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
		rB[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rB[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		rB[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		cl->ResourceBarrier( _countof( rB ), rB );
	}
	else
	{
		if( m_texBB != (ID3D12Resource*)in->textureResource )
		{
			SAFE_RELEASE( m_texBB );
			SIZE_T base = m_srvHeap->GetCPUDescriptorHandleForHeapStart().ptr;
			SIZE_T ds = m_device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
			SIZE_T des = base + 4 * ds; // put content to 5th slot
			hr = in->textureResource->QueryInterface( &m_texBB );
			if( SUCCEEDED( hr ) )
			{
				m_device->CreateShaderResourceView(
					m_texBB,
					nullptr,
					{ des }
				);
				D3D12_RESOURCE_DESC descIn = m_texBB->GetDesc();
				m_sizeIn.cx = (VWB_int)descIn.Width;
				m_sizeIn.cy = (VWB_int)descIn.Height;
			}
		}
	}

	if( g_logLevel > 4 && m_texBB ) {
		saveTextureToFile( m_device, m_cq, "debug_backbuffercopy.bmp", m_texBB );
	}

	// update constant buffer
	if( m_cbMap ) {
		if( m_bDP ) {
			DPConstantBuffer& cb = *reinterpret_cast<DPConstantBuffer*>( m_cbMap );
			cb.gamma = gamma;
			cb.doWarp = 1;
			cb.doBlend = m_texBlend && !bDoNotBlend ? 1 : 0;
			cb.doBlack = m_texBlack && !bDoNoBlack  ? 1 : 0;
			cb.do2ndBlend = m_texBlend2 ? 1 : 0;
			cb.inputGamma = inputGamma;
			cb.outputGamma = outputGamma;
			cb.colorCorr = m_texDirectionalShading ? 1 : 0;
			cb.flip_v = bFlipWarpmeshTexcoords ? 1 : 0;
			//cb.reserved[7]; // in total 16 until here
			if( m_bDynamicEye ) {
				// copy current view-projection matrix and eye position
				memcpy( cb.mvp, m_mVP.Transposed(), sizeof( cb.mvp ) );
				cb.pos[0] = m_ep.x;
				cb.pos[1] = m_ep.y;
				cb.pos[2] = m_ep.z;
				cb.pos[3] = 1.0f;
			} else {
				// set a view-projection, that renders a square from 0,0 to 1,1, as this is where the warp-map's position coordinates are defined
				static VWB_float clip[]{ 0.f, 1.f, 1.f, 0.f, 1.f, 10.0f };
				static VWB_MAT44f P = VWB_MAT44f::I(); // we use identity as projection, to just keep x and y as is
				memcpy( cb.mvp, P, sizeof( cb.mvp ) );
				cb.pos[0] = 0.f;
				cb.pos[1] = 0.f;
				cb.pos[2] = 0.f;
				cb.pos[3] = 1.f;
			}

		#ifdef _DEBUG
			static bool once = true;
			if( once ) {
				std::ofstream fs( "debug_cb.txt" );
				fs << "cb: \n";
				float const* pf = &cb.gamma;
				for( unsigned int y = 0; y != 4; y++ ) {
					fs << pf[y*4];
					for( unsigned int x = 1; x != 4; x++ )
						fs << ", " << pf[y * 4 + x];
					fs << "\n";
				}
				fs << "camera_mvp: \n";
				for( unsigned int y = 0; y != 4; y++ ) {
					fs << cb.mvp[y * 4];
					for( unsigned int x = 1; x != 4; x++ )
						fs << ", " << cb.mvp[y * 4 + x];
					fs << "\n";
				}
				fs << "camera_position: \n";
				fs << cb.pos[0];
				for( unsigned int x = 1; x != 4; x++ )
					fs << ", " << cb.pos[x];
				fs << "\n";
				once = false;
			}
		#endif

		} else {
			// the constant buffer is still mapped
			// to avoid pipeline stall, we must not read (or perform operations on) that memory range
			ConstantBuffer& cb = *reinterpret_cast<ConstantBuffer*>( m_cbMap );
			memcpy( cb.matView, m_mVP.Transposed(), sizeof( cb.matView ) );
			cb.border[0] = m_bBorder;
			cb.border[1] = bDoNotBlend ? 0.0f : 1.0f;
			cb.border[2] = bDoNoBlack ? 0.0f : 1.0f;
			cb.border[3] = 0.0f;
			cb.params[0] = (FLOAT)m_sizeIn.cx;
			cb.params[1] = (FLOAT)m_sizeIn.cy;
			cb.params[2] = 1.0f / (FLOAT)m_sizeIn.cx;
			cb.params[3] = 1.0f / (FLOAT)m_sizeIn.cy;
			if( bPartialInput ) {
				cb.offsScale[0] = (FLOAT)optimalRect.left / (FLOAT)optimalRes.cx;
				cb.offsScale[1] = (FLOAT)optimalRect.top / (FLOAT)optimalRes.cy;
				cb.offsScale[2] = (FLOAT)optimalRes.cx / ( (FLOAT)optimalRect.right - (FLOAT)optimalRect.left );
				cb.offsScale[3] = (FLOAT)optimalRes.cy / ( (FLOAT)optimalRect.bottom - (FLOAT)optimalRect.top );
			} else {
				cb.offsScale[0] = 0.0f;
				cb.offsScale[1] = 0.0f;
				cb.offsScale[2] = 1.0f;
				cb.offsScale[3] = 1.0f;
			}
			cb.blackBias[0] = m_blackBias.x * blackScale;
			if( blackDarkAdjust <= 0.5f )
				cb.blackBias[1] = blackDarkAdjust * 2.0f * m_blackBias.y;
			else
				cb.blackBias[1] = m_blackBias.y + ( blackDarkAdjust - 0.5f ) * 2.0f * ( 1.0f - m_blackBias.y );
			if( blackBrightAdjust <= 0.5f )
				cb.blackBias[2] = blackBrightAdjust * 2.0f * m_blackBias.z;
			else
				cb.blackBias[2] = m_blackBias.z + ( blackBrightAdjust - 0.5f ) * 2.0f * ( 1.0f - m_blackBias.z );
			cb.blackBias[2] *= cb.blackBias[1];
			cb.blackBias[3] = inputGamma;
		}
	}

	cl->SetGraphicsRootSignature( m_rootSignature );

	ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap };
	cl->SetDescriptorHeaps( _countof(ppHeaps) , ppHeaps );

	cl->SetGraphicsRootConstantBufferView( 0, m_cb->GetGPUVirtualAddress() );
	cl->SetGraphicsRootDescriptorTable( 1, m_srvHeap->GetGPUDescriptorHandleForHeapStart() );

	D3D12_VIEWPORT vp;
	if( 0 != in->viewport[2] && 0 != in->viewport[3] )
	{
		vp = { in->viewport[0], in->viewport[1], in->viewport[2], in->viewport[3], in->viewport[4], in->viewport[5] };
	}
	else
	{
		vp = { 0.0f, 0.0f, (FLOAT)descRT.Width, (FLOAT)descRT.Height, 0.0f, 1.0f };
	}
	cl->RSSetViewports( 1, &vp );
	const D3D12_RECT sr{ 0, 0, (LONG)descRT.Width, (LONG)descRT.Height };
	cl->RSSetScissorRects( 1, &sr );
	const D3D12_CPU_DESCRIPTOR_HANDLE rtvs[]{ { SIZE_T(in->rtvHandlePtr) } };
	cl->OMSetRenderTargets( _countof( rtvs ), rtvs, FALSE, nullptr );
	if( VWB_STATEMASK_CLEARBACKBUFFER & stateMask )
	{
		if( in->rtvHandlePtr )
		{
			const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
			cl->ClearRenderTargetView( { SIZE_T(in->rtvHandlePtr) }, clearColor, 0, nullptr );
		}
		else
		{
			static bool showwarning = true;
			if( showwarning )
			{
				logStr( 2, "WARNING: cannot clear render target if rtvHandlePtr is not set." );
				showwarning = false;
			}
			else
				logStr( 4, "WARNING: cannot clear render target if rtvHandlePtr is not set." );

		}
	}

	if( m_bDP ) {
		cl->IASetVertexBuffers( 0, 1, &m_vertexBufferView );
		cl->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		cl->IASetIndexBuffer( &m_indexBufferView );
		cl->DrawIndexedInstanced( m_nIndices, 1, 0, 0, 0 );
	} else {
		cl->IASetVertexBuffers( 0, 1, &m_vertexBufferView );
		cl->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		cl->DrawInstanced( 6, 1, 0, 0 );
	}
	if( !in->commandList )
	{
		hr = m_cl->Close();
		m_cq->ExecuteCommandLists( 1, ( ID3D12CommandList* const* )&m_cl );
	}

	return SUCCEEDED( hr ) ? VWB_ERROR_NONE : VWB_ERROR_GENERIC;
}
#endif //ndef VWB_WIN7_COMPAT
