#pragma once

#include <algorithm>
#include <string>
#include <exception>
#include <atomic>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <list>
// TCHAR
#include <TCHAR.h>
#ifdef _UNICODE
#define tstring wstring
#define tcout wcout
#define tcerr wcerr
#define tcin wcin
typedef TCHAR MYTC;
#else // defined _UNICODE
#define tstring string
#define tcout cout
#define tcerr cerr
#define tcin cin
#endif // defined _UNICODE

// dx12 includes
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>
#include <DirectXMath.h>
#include <atlcomcli.h>

#include "d3dx12.h"

// 3rd party includes
#include "stb_image.h" // simple image library

///////////////////////////////////////////////////////
// helpers
class hr_exception : public std::runtime_error {
public:
	static std::string to_string( HRESULT hr, char const* desc = nullptr ) {
		std::ostringstream ss;
		ss << "Error HRESULT = 0x" << std::hex << std::setw( 8 ) << std::setfill( '0' ) << hr;
		if( desc )
			ss << desc;
		return ss.str();
	}
	HRESULT hr;
	explicit hr_exception( HRESULT h, char const* desc = nullptr ) : std::runtime_error( to_string( hr, desc ) ), hr( h ) {}
};
#define THROW_IF_FAILED( EXPR ) do { \
    if (auto hr = (EXPR); FAILED(hr)) { \
        throw hr_exception(hr); \
    } \
} while(0)
#define THROW_IF_FAILED1( EXPR, desc ) do { \
    if (auto hr = (EXPR); FAILED(hr)) { \
        throw hr_exception(hr,desc); \
    } \
} while(0)
#define THROW_IF_FAILED2( EXPR, blob ) do { \
    if (auto hr = (EXPR); FAILED(hr)) { \
        throw hr_exception(hr, blob ? (char const*)blob->GetBufferPointer() : nullptr ); \
    } \
} while(0)

/// loads an image from a path using stb image
struct ImgRGBA {
	UINT w = 0;
	UINT h = 0;
	unsigned char* data = nullptr;
	ImgRGBA( std::filesystem::path const& path ) {
		FILE* f = nullptr;
		auto err = _wfopen_s( &f, path.c_str(), L"rb" );
		if( NO_ERROR != err )
			return;
		int x, y;
		data = stbi_load_from_file( f, &x, &y, nullptr, 4 );
		if( data && x > 0 && y > 0 )
		{
			w = x;
			h = y; // check for negative value?
		}
	};
	~ImgRGBA() {
		if( data )
			stbi_image_free( data );
	}
	operator void const* ( ) { return data; }
	operator bool() { return data && w && h; }
};

///////////////////////////////////////////////////////
// Base

/// The base class for all d3d12 related stuff.
/// It provides function pointers to D3DCompiler and DXGIFactory.
/// It keeps the IDXGIFactoryX and some settings statically.
/// Also it keeps the device and command queue.
class D3D12Base {
public:
	// function definitions
	typedef HRESULT( WINAPI* PFN_CREATE_DXGI_FACTORY_2 )( UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory );

	typedef HRESULT( WINAPI* PFN_D3D_COMPILE )(
		_In_ LPCVOID pSrcData,
		_In_ SIZE_T SrcDataSize,
		_In_opt_ LPCSTR pFileName,
		_In_opt_ CONST D3D_SHADER_MACRO* pDefines,
		_In_opt_ ID3DInclude* pInclude,
		_In_ LPCSTR pEntrypoint,
		_In_ LPCSTR pTarget,
		_In_ UINT Flags1,
		_In_ UINT Flags2,
		_Out_ ID3DBlob** ppCode,
		_Always_( _Outptr_opt_result_maybenull_ ) ID3DBlob** ppErrorMsgs );

	typedef HRESULT( WINAPI* PFN_D3D_COMPILE_FROM_FILE )(
		_In_ LPCWSTR pFileName,
		_In_reads_opt_( _Inexpressible_( pDefines->Name != NULL ) ) CONST D3D_SHADER_MACRO* pDefines,
		_In_opt_ ID3DInclude* pInclude,
		_In_ LPCSTR pEntrypoint,
		_In_ LPCSTR pTarget,
		_In_ UINT Flags1,
		_In_ UINT Flags2,
		_Out_ ID3DBlob** ppCode,
		_Always_( _Outptr_opt_result_maybenull_ ) ID3DBlob** ppErrorMsgs );

	struct Alloc {
		CComPtr<ID3D12CommandQueue> cq;
		CComPtr<ID3D12PipelineState> ps;
		CComPtr<ID3D12CommandAllocator> ca;
		CComPtr<ID3D12GraphicsCommandList> cl;
		CComPtr<ID3D12Fence> f;
		UINT64 fv;
		HANDLE fh;
		bool open;
		bool waiting;

		Alloc() = delete; // no standard constructor
	
		Alloc( ID3D12Device* pDevice, ID3D12CommandQueue* pQueue, wchar_t const* name = nullptr, ID3D12PipelineState* pipelineState = nullptr, bool canWait = true ) {
			// check
			if( !pDevice || !pQueue )
				throw std::invalid_argument( "pDevice and pQueue must be valid." );
			cq = pQueue;

			ps = pipelineState;

			THROW_IF_FAILED1( pDevice->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &ca ) ), "Failed to create a command allocator." );

			THROW_IF_FAILED1( pDevice->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, ca, nullptr, IID_PPV_ARGS( &cl ) ), "Failed to create a command list." );

			if( name && name[0] )
			{
				ca->SetName( name );
				cl->SetName( name );
			}

			THROW_IF_FAILED1( pDevice->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &f ) ), "Failed to create a fence." );
			fv = 0;

			if( canWait ) {
				fh = ::CreateEvent( nullptr, FALSE, FALSE, nullptr );
				if( !fh )
					throw std::runtime_error( "Failed to create event." );
			}
			open = true;
			waiting = false;
		}

		Alloc( D3D12Base const& d3d, wchar_t const* name = nullptr, ID3D12PipelineState* pipelineState = nullptr, bool canWait = true ) : Alloc( d3d.m_device, d3d.m_queue, name, pipelineState, canWait ) {}

		Alloc( Alloc const& ) = delete; // no copy constructor
		Alloc( Alloc&& other ) noexcept // the move constructor
			: cq( std::move( other.cq ) )
			, ps( std::move( other.ps ) )
			, ca( std::move( other.ca ) )
			, cl( std::move( other.cl ) )
			,  f( std::move( other.f  ) )
			, fv( other.fv )
			, fh( other.fh )
			, open( other.open )
			, waiting( other.waiting )
		{
			other.fh = 0;
			other.fv = 0;
			other.open = false;
			other.waiting = false;
		}
		~Alloc() { 
			flush();
			if( fh ) ::CloseHandle( fh ); 
		}

		Alloc& operator=( Alloc&& other ) = default; // synthesize move assignment

		/// reset command allocator and command list to reuse it
		void reset( ID3D12PipelineState* newPSO = nullptr ) {
			if( newPSO && newPSO != ps )
				ps = newPSO;
			ca->Reset();
			cl->Reset( ca, ps );
			open = true;
		}

		/// wait on CPU for some signal, which is a value on my fence
		void wait( UINT64 waitFor ) {
			if( f && fh ) {
				auto curr = f->GetCompletedValue();
				if( curr < waitFor ) {
					f->SetEventOnCompletion( waitFor, fh );
					waiting = true;
					::WaitForSingleObject( fh, INFINITE );
					waiting = false;
				}
			}
		}
		void wait() {
			if( f && fh ) {
				auto curr = f->GetCompletedValue();
				if( curr < fv ) {
					f->SetEventOnCompletion(fv, fh);
					waiting = true;
					::WaitForSingleObject(fh, INFINITE);
					waiting = false;
				}
			}
		}
		void flush() {
			if( f && fh ) {
				cq->Signal(f, ++fv);
				auto curr = f->GetCompletedValue();
				if( curr < fv ) {
					f->SetEventOnCompletion(fv, fh);
					waiting = true;
					::WaitForSingleObject(fh, INFINITE);
					waiting = false;
				}
			}
		}

		void signal( ID3D12Fence* fence, UINT64 fenceValue ) {
			cq->Signal( fence, fenceValue );
		}

		void signalOther( Alloc& other ) {
			other.signal( f, ++fv );
		}

		/// Let my command queue wait for another alloc's queue to signal a fence
		/// We use it's latest fence value, so my command queue stops execution
		/// until the fence reaches the desired value on some other queue.
		/// Waiting on for the same queue might lead to a deadlock, if the fence value
		/// has been set on another queue
		void waitFor(Alloc const& other) const {
			cq->Wait(other.f, other.fv);
		}

		/// Make another alloc command queue wait for mine
		/// We increase our fence value and make the other queue signal us.
		/// Waiting for the same queue has no effect
		void makeWait( Alloc const& other ) {
			fv++;
			cq->Signal( f, fv );
			other.cq->Wait( f, fv );
		}

		/// check for a fence value reached
		bool hasGPUCompleted( UINT64 v ) const {
			return f->GetCompletedValue() >= v;
		}
		/// check for current fence value reached
		bool hasGPUCompleted() const {
			return f->GetCompletedValue() >= fv;
		}

		/// get current fence value signaled in GPU
		UINT64 getGPUCompleted() const { return f->GetCompletedValue(); }
		UINT64 getCurrent() const { return fv; }


		/// execute the commandlist
		/// @return the fence value signaled
		UINT64 exec( bool doWait = false, bool doReset = false ) {
			cl->Close();
			open = false;
			cq->ExecuteCommandLists( 1, ( ID3D12CommandList* const* )&cl.p );
			// we always signal
            auto v = ++fv;
			cq->Signal( f, v );
			if( doWait ) wait(v);
			if( doReset ) reset();
			return v;
		}

		operator bool() { return ca && cl && f; }
		ID3D12GraphicsCommandList* operator->() { return cl; }
		ID3D12GraphicsCommandList* get() { return cl; }
	};

	struct DescriptorHeap {
		UINT cpuIncrement = 0;
		//    UINT gpuIncrement = 0; // not needed atm.
		D3D12_CPU_DESCRIPTOR_HANDLE cpuStart{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpuStart{};
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		CComPtr<ID3D12DescriptorHeap> p = nullptr;
		DescriptorHeap() = default;
		DescriptorHeap( ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_DESC const& desc, wchar_t const* name = nullptr ) {
			DescriptorHeap::desc = desc;
			THROW_IF_FAILED1( pDevice->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &p ) ), "Failed to create descritpror heap for render target." );
			cpuStart = p->GetCPUDescriptorHandleForHeapStart();
			cpuIncrement = pDevice->GetDescriptorHandleIncrementSize( desc.Type );
			gpuStart.ptr = 0;
			if( desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE )
			{
				gpuStart = p->GetGPUDescriptorHandleForHeapStart();
				// gpuIncrement = pDevice->GetDescriptorHandleIncrementSize( desc.Type );
			}
			if( name && name[0] )
				p->SetName( name );
		}

		DescriptorHeap( DescriptorHeap const& ) = delete;
		DescriptorHeap( DescriptorHeap&& other ) noexcept
			: cpuIncrement( other.cpuIncrement )
			, cpuStart( other.cpuStart )
			, gpuStart( other.gpuStart )
			, desc( other.desc )
		{
			if( other.p )
			{
				this->p = other.p;
				other.p = nullptr;
			}
		};

		~DescriptorHeap() {}
		DescriptorHeap& operator=( DescriptorHeap const& other ) = delete;
		DescriptorHeap& operator=( DescriptorHeap&& other ) = default;

		D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle( UINT offset = 0 ) const
		{
			D3D12_CPU_DESCRIPTOR_HANDLE ret = cpuStart;
			ret.ptr += INT64( cpuIncrement ) * UINT64( offset );
			return ret;
		}
		// useful to set a contignous array
		D3D12_CPU_DESCRIPTOR_HANDLE const* getCPUHandleStartCPtr()
		{
			return &cpuStart;
		}
		D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle() const { return gpuStart; }
		D3D12_GPU_DESCRIPTOR_HANDLE const* getGPUHandleStartCPtr() const { return &gpuStart; }
		operator ID3D12DescriptorHeap* ( ) { return p; }
		ID3D12DescriptorHeap* const* operator&() const { return &p.p; }
		ID3D12DescriptorHeap** operator&() { return &p; }
		operator bool() const { return nullptr != p; }
	};

	struct Vertex_POS3_TX2
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 tex;
		UINT texID;
		static D3D12_INPUT_ELEMENT_DESC inputElementDescs[];
	};

	struct Vertex_POS3_TX2_TXID
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 tex;
		UINT texID;
		static D3D12_INPUT_ELEMENT_DESC inputElementDescs[];
	};

	struct GPUDesc : DXGI_ADAPTER_DESC1 {
		std::vector<DXGI_OUTPUT_DESC1> outputs;
		UINT32 nodeCount;
	};

private:
	// function init helper
	class FnInit {
		std::atomic_int ref;
		// mudule handles
		HMODULE hD3D12Dll = 0;
		HMODULE hDXGIDll = 0;
		HMODULE hD3DCompilerDll = 0;
		#ifdef _DEBUG
		HMODULE hD3D12SDKLayersDll = 0;
		#endif
		static std::unique_ptr<FnInit> _inst;

	public:
		FnInit() {
			// load libraries
			auto hDXGIDll = ::LoadLibrary( _T( "dxgi.dll" ) );
			if( !hDXGIDll )  throw std::runtime_error( "Could not load dxgi.dll." );

			TCHAR name[] = _T( "D3DCompiler_49.dll" );
			for( TCHAR* c = &name[13]; *c != _T( '3' ); ( *c )-- ) {
				hD3DCompilerDll = ::LoadLibrary( name );
				if( hD3DCompilerDll )
					break;
			}
			if( !hD3DCompilerDll ) throw std::runtime_error( "Could not load D3DCompiler." );

			#ifdef _DEBUG
			hD3D12SDKLayersDll = ::LoadLibrary( _T( "D3D12SDKLayers.dll" ) );;
			if( !hD3D12SDKLayersDll ) throw std::runtime_error( "Could not load D3D12SDKLayers.dll." );
			#endif

			// load functions
			CreateDXGIFactory2 = ( PFN_CREATE_DXGI_FACTORY_2 )::GetProcAddress( hDXGIDll, "CreateDXGIFactory2" );
			if( !CreateDXGIFactory2 ) throw std::runtime_error( "Could not find CreateDXGIFactory2." );

			D3DCompile = ( PFN_D3D_COMPILE )::GetProcAddress( hD3DCompilerDll, "D3DCompile" );
			if( !D3DCompile ) throw std::runtime_error( "Could not find D3DCompile." );
			D3DCompileFromFile = ( PFN_D3D_COMPILE_FROM_FILE )::GetProcAddress( hD3DCompilerDll, "D3DCompileFromFile" );
			if( !D3DCompileFromFile ) throw std::runtime_error( "Could not find D3DCompileFromFile." );

			#if defined(_DEBUG)
			// Enable the debug layer (requires the Graphics Tools "optional feature").
			// NOTE: Enabling the debug layer after device creation will invalidate the active device.
			{
				CComPtr<ID3D12Debug1> debugController;
				if( SUCCEEDED( D3D12GetDebugInterface( IID_PPV_ARGS( &debugController ) ) ) )
				{
					debugController->EnableDebugLayer();
					debugController->SetEnableGPUBasedValidation( TRUE );

					// Enable additional debug layers.
					s_dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
				}
				else
				{
					throw std::runtime_error( "Could not initialize debug interface." );
				}
			}
			#endif //_DEBUG
			THROW_IF_FAILED( CreateDXGIFactory2( s_dxgiFactoryFlags, IID_PPV_ARGS( &s_factory ) ) );
		}
		FnInit( FnInit const& ) = delete;

		~FnInit() {
			// release the factory
			s_factory.Release();

			// null the function ptrs
			D3DCompileFromFile = nullptr;
			D3DCompile = nullptr;

			#ifdef _DEBUG
			//D3D12GetDebugInterface = nullptr;
			#endif //def _DEBUG

			CreateDXGIFactory2 = nullptr;

			// free libraries
			#ifdef _DEBUG
			if( hD3D12SDKLayersDll )
				::FreeLibrary( hD3D12SDKLayersDll );
			#endif
			if( hD3DCompilerDll )
				::FreeLibrary( hD3DCompilerDll );
			if( hDXGIDll )
				::FreeLibrary( hDXGIDll );
			if( hD3D12Dll )
				::FreeLibrary( hD3D12Dll );
		}

		static int init() {
			if( !_inst )
			{
				_inst = std::make_unique<FnInit>();
			}
			return ++_inst->ref;
		}
		static int uninit() {
			if( _inst )
			{
				auto count = --_inst->ref;
				if( 0 == count )
					_inst.reset();
				return count;
			}
			return 0;
		}
	};

protected:
	// functions, this makes it feel like having the dlls linked statically
	static PFN_CREATE_DXGI_FACTORY_2 CreateDXGIFactory2;
	static PFN_D3D_COMPILE D3DCompile;
	static PFN_D3D_COMPILE_FROM_FILE D3DCompileFromFile;
	// some globals
	static UINT s_rtvDescriptorSize;
	static UINT s_srvDescriptorSize;
	static UINT s_dxgiFactoryFlags;
private:
	static CComPtr<IDXGIFactory4> s_factory;
protected:
	// shared
	CComPtr<ID3D12Device> m_device; // this is a shared resource, if not a window
	CComPtr<ID3D12CommandQueue> m_queue; // this is a shared resource, if not a window

	// when initializing a new pipeline
	D3D12Base( IDXGIAdapter1* adapter, wchar_t const* name )
		: m_device( createDevice( adapter, name ) )
		, m_queue( createQueue( m_device, name ) )
	{}

	// when used shared
	D3D12Base( D3D12Base const& parent )
		: m_device( parent.m_device )
		, m_queue( parent.m_queue )
	{}

	virtual ~D3D12Base()
	{
		#ifdef _DEBUG
		std::wostringstream os;
		ULONG refc;
		if (m_queue) {
			m_queue.p->AddRef();
			refc = m_queue.p->Release();
			os << "Releasing m_queue 0x" << m_queue.p << " refc " << refc << std::endl;
		}
		m_queue.Release();
		if (m_device) {
			m_device.p->AddRef();
			refc = m_device.p->Release();
			os << "Releasing m_device 0x" << m_device.p << " refc " << refc << std::endl;
		}
		m_device.Release();
		OutputDebugStringW( os.str().c_str() );
		#endif //def _DEBUG
	}

	public:
	static std::vector < GPUDesc > getGPUDescs( IDXGIFactory4* factory, D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_12_0 ) {
		std::vector < GPUDesc > descs;
		CComPtr<IDXGIAdapter1> adapter;
		for( UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1( adapterIndex, &adapter ); ++adapterIndex, adapter.Release() ) {
			GPUDesc desc;
			adapter->GetDesc1( &desc );

			// Don't show the Basic Render Driver adapter.
			if( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ) continue;

			// Don't show adapters that don't support our desired feature level, so we create a device
			CComPtr<ID3D12Device> device;
			if( FAILED( D3D12CreateDevice( adapter, level, IID_PPV_ARGS( &device ) ) ) ) continue;

			// collect the outputs
			CComPtr<IDXGIOutput> out;
			for( UINT i = 0; SUCCEEDED( adapter->EnumOutputs( i, &out ) ); i++, out.Release() ) {
				CComPtr<IDXGIOutput6> out1;
				out1 = out;
				DXGI_OUTPUT_DESC1 descOut{};

				if( !out1 || FAILED( out1->GetDesc1( &descOut ) ) ) continue;

				desc.outputs.push_back( descOut );
			}

			// collect nodes
			desc.nodeCount = device->GetNodeCount();
			descs.emplace_back( std::move( desc ) );
		}
		return descs;
	}

	protected:
	static CComPtr<IDXGIAdapter1> findAdapter( IDXGIFactory4* factory, D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_12_0, int posX = 0, int posY = 0 ) {
		CComPtr<IDXGIAdapter1> adapter;
		auto hm = ::MonitorFromPoint( POINT{ posX, posY }, MONITOR_DEFAULTTONEAREST );
		for( UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1( adapterIndex, &adapter ); ++adapterIndex )
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1( &desc );
			if( desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE )
			{
				// Don't select the Basic Render Driver adapter.
				continue;
			}

			CComPtr<IDXGIOutput> out;
			for( UINT i = 0; SUCCEEDED( adapter->EnumOutputs( i, &out ) ); i++ )
			{
				DXGI_OUTPUT_DESC descOut{};
				out->GetDesc( &descOut );
				if( descOut.Monitor == hm )
					break;
				out.Release();
			}
			if( !out )
			{
				adapter.Release();
				continue;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if( SUCCEEDED( D3D12CreateDevice( adapter, level, _uuidof( ID3D12Device ), nullptr ) ) )
			{
				return adapter;
			}
			adapter.Release();
		}
		throw std::runtime_error( "No suitable adapter found." );

		#if 0
		   // Create DXGI factory
                ComPtr<IDXGIFactory4> factory;
                CreateDXGIFactory1(IID_PPV_ARGS(&factory));

                UINT adapterIndex = 0;
                ComPtr<IDXGIAdapter1> adapter;
                std::vector<ComPtr<IDXGIAdapter1>> adapters;

                // Enumerate all adapters (GPUs)
                while (factory->EnumAdapters1(adapterIndex, &adapter) !=
                       DXGI_ERROR_NOT_FOUND) {
                  adapters.push_back(adapter);
                  adapterIndex++;
                }

                std::cout << "Number of GPUs: " << adapters.size() << std::endl;

                // Loop through each adapter and determine node count
                for (size_t i = 0; i < adapters.size(); ++i) {
                  ComPtr<ID3D12Device> device;
                  HRESULT hr = D3D12CreateDevice(adapters[i].Get(),
                                                 D3D_FEATURE_LEVEL_11_0,
                                                 IID_PPV_ARGS(&device));
                  if (SUCCEEDED(hr)) {
                    UINT nodeCount = device->GetNodeCount();
                    std::cout << "Adapter " << i << " has " << nodeCount
                              << " node(s)." << std::endl;

                    // Display node masks
                    for (UINT nodeIndex = 0; nodeIndex < nodeCount;
                         ++nodeIndex) {
                      UINT nodeMask =
                        (1 << nodeIndex); // Node mask for this GPU
                      std::cout << "  Node " << nodeIndex << " mask: 0x"
                                << std::hex << nodeMask << std::dec
                                << std::endl;
                    }
                  }
                }
				#endif

	}

	static CComPtr<ID3D12Device> createDevice( IDXGIAdapter1* adapter, wchar_t const* name = nullptr )
	{
		CComPtr<ID3D12Device> device;
		THROW_IF_FAILED( D3D12CreateDevice( adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS( &device ) ) );

		s_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV );
		s_srvDescriptorSize = device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
		return device;
	}

	static CComPtr<ID3D12CommandQueue> createQueue( ID3D12Device* device, wchar_t const* name ) {
		CComPtr<ID3D12CommandQueue> queue;
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		THROW_IF_FAILED( device->CreateCommandQueue( &queueDesc, IID_PPV_ARGS( &queue ) ) );
		if( name && name[0] )
			queue->SetName( name );

		return queue;
	}

public:
	static int init() {
		return FnInit::init();
	}
	static int uninit() {
		return FnInit::uninit();
	}

	ID3D12Device* getDevice() const { return m_device; }
	ID3D12CommandQueue* getQueue() const { return m_queue; }
	static IDXGIFactory4* getFactory() { return s_factory; }

	HRESULT loadTexture( std::filesystem::path const& path, ID3D12Resource** texture, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle ) {
		// Note: CComPtr's are CPU objects but this resource needs to stay in scope until
		 // the command list that references it has finished executing on the GPU.
		 // We will flush the GPU at the end of this method to ensure the resource is not
		 // prematurely destroyed.

		// load image to memory
		ImgRGBA img( path );
		if( !img )
			return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

		Alloc alloc( m_device, m_queue );

		CComPtr<ID3D12Resource> textureUploadHeap;

		// Describe and create a Texture2D.
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = img.w;
		textureDesc.Height = img.h;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		static const D3D12_HEAP_PROPERTIES propsDef{
			D3D12_HEAP_TYPE_DEFAULT,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			1,1
		};
		static const D3D12_HEAP_PROPERTIES propsUpload{
			D3D12_HEAP_TYPE_UPLOAD,
			D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			D3D12_MEMORY_POOL_UNKNOWN,
			1,1
		};
		HRESULT hr = m_device->CreateCommittedResource(
			&propsDef,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS( texture ) );
		if( FAILED( hr ) )
			return  hr;
		( *texture )->SetName( path.c_str() );
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize( *texture, 0, 1 );

		// Create the GPU upload buffer.
		D3D12_RESOURCE_DESC desc{
			D3D12_RESOURCE_DIMENSION_BUFFER,
			0, uploadBufferSize, 1, 1, 1,
			DXGI_FORMAT_UNKNOWN, {1,0},
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			D3D12_RESOURCE_FLAG_NONE
		};
		hr = m_device->CreateCommittedResource(
			&propsUpload,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &textureUploadHeap ) );

		if( FAILED( hr ) )
			return hr;

		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = img;
		textureData.RowPitch = img.w * 4;
		textureData.SlicePitch = textureData.RowPitch * img.h;

		UpdateSubresources( alloc.get(), *texture, textureUploadHeap, 0, 0, 1, &textureData );
		D3D12_RESOURCE_BARRIER barr{ 
			D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			D3D12_RESOURCE_BARRIER_FLAG_NONE,
			{.Transition{ 
				*texture,
				D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE }
			}
		};
		alloc->ResourceBarrier( 1, &barr );

		// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		m_device->CreateShaderResourceView( *texture, &srvDesc, srvHandle );

		alloc.exec(true);

		return S_OK;
	}

	void saveTexture( std::filesystem::path fileName, ID3D12Resource* texture )
	{
		D3D12_RESOURCE_DESC textureDesc = texture->GetDesc();
		UINT64 requiredSize = 0;

		m_device->GetCopyableFootprints( &textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &requiredSize );

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
		THROW_IF_FAILED( m_device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS( &stagingResource )
		) );

		D3D12_TEXTURE_COPY_LOCATION dst = {};
		dst.pResource = stagingResource;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		m_device->GetCopyableFootprints( &textureDesc, 0, 1, 0, &dst.PlacedFootprint, nullptr, nullptr, nullptr );

		D3D12_TEXTURE_COPY_LOCATION src = {};
		src.pResource = texture;
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.SubresourceIndex = 0;

		{
			Alloc alloc( *this );
			alloc->CopyTextureRegion( &dst, 0, 0, 0, &src, nullptr );
			alloc.exec( true, false );
		}

		// Map the staging resource
		void* data;
		D3D12_RANGE readRange = { 0, requiredSize }; // We intend to read the entire buffer
		THROW_IF_FAILED( stagingResource->Map( 0, &readRange, &data ) );

		// Save the data to a BMP file
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
		m_device->GetCopyableFootprints( &textureDesc, 0, 1, 0, &footprint, nullptr, nullptr, nullptr );
		UINT rowPitch = footprint.Footprint.RowPitch;
		UINT height = textureDesc.Height;
		UINT width = (UINT)textureDesc.Width;
		UINT bytesPerPixel = 4; // Assuming DXGI_FORMAT_R8G8B8A8_UNORM

		BITMAPFILEHEADER fileHeader = {};
		BITMAPINFOHEADER infoHeader = {};

		fileHeader.bfType = 0x4D42; // 'BM'
		fileHeader.bfOffBits = sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPINFOHEADER );
		fileHeader.bfSize = fileHeader.bfOffBits + rowPitch * height;

		infoHeader.biSize = sizeof( BITMAPINFOHEADER );
		infoHeader.biWidth = width;
		infoHeader.biHeight = -static_cast< int >( height ); // Negative height to indicate top-down bitmap
		infoHeader.biPlanes = 1;
		infoHeader.biBitCount = bytesPerPixel * 8;
		infoHeader.biCompression = BI_RGB;
		infoHeader.biSizeImage = rowPitch * height;

		std::ofstream file( fileName, std::ios::binary );
		file.write( reinterpret_cast< const char* >( &fileHeader ), sizeof( fileHeader ) );
		file.write( reinterpret_cast< const char* >( &infoHeader ), sizeof( infoHeader ) );
		file.write( reinterpret_cast< const char* >( data ), rowPitch * height );
		file.close();

		// Unmap the resource when done
		stagingResource->Unmap( 0, nullptr );
	}
};

#ifdef D3D12_BASE_IMPLEMENTATION
std::unique_ptr<D3D12Base::FnInit> D3D12Base::FnInit::_inst;
D3D12Base::PFN_CREATE_DXGI_FACTORY_2 D3D12Base::CreateDXGIFactory2 = nullptr;
D3D12Base::PFN_D3D_COMPILE D3D12Base::D3DCompile = nullptr;
D3D12Base::PFN_D3D_COMPILE_FROM_FILE D3D12Base::D3DCompileFromFile = nullptr;
UINT D3D12Base::s_rtvDescriptorSize = 0;
UINT D3D12Base::s_srvDescriptorSize = 0;
UINT D3D12Base::s_dxgiFactoryFlags = 0;
CComPtr<IDXGIFactory4>  D3D12Base::s_factory = nullptr;

D3D12_INPUT_ELEMENT_DESC D3D12Base::Vertex_POS3_TX2::inputElementDescs[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};

D3D12_INPUT_ELEMENT_DESC D3D12Base::Vertex_POS3_TX2_TXID::inputElementDescs[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 1, DXGI_FORMAT_R32_UINT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif // def D3D12_BASE_IMPLEMENTATION


/// prototype of renderer
class D3D12Renderer : public D3D12Base {
public:
	/// The pipline state object
	/// It is decribing the input, output, resources and the shader programs. Just add geometry...
	/// It maintains all data to rebuild it on the fly, this is shader code, layout and it's desc.

	struct PSO {
		CComPtr<ID3D12PipelineState> state = nullptr;
		CComPtr<ID3D12RootSignature> sig;
		CComPtr<ID3DBlob> VS = nullptr;
		CComPtr<ID3DBlob> PS = nullptr;
		std::vector< D3D12_INPUT_ELEMENT_DESC > layout;
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};

		PSO( ID3D12Device* pDevice, ID3D12RootSignature* rootSignature, ID3DBlob* vs, ID3DBlob* ps, D3D12_INPUT_ELEMENT_DESC const* inputElementDesc, size_t NumElements, D3D12_GRAPHICS_PIPELINE_STATE_DESC const& psoDesc, std::vector<D3D12_RESOURCE_DESC> const& descRTs )
			: sig( rootSignature )
			, VS( vs )
			, PS( ps )
			, layout( inputElementDesc, inputElementDesc + NumElements )
			, desc( psoDesc )
		{
			desc.pRootSignature = sig;
			desc.VS.pShaderBytecode = VS->GetBufferPointer();
			desc.VS.BytecodeLength = VS->GetBufferSize();
			desc.PS.pShaderBytecode = PS->GetBufferPointer();
			desc.PS.BytecodeLength = PS->GetBufferSize();
			desc.InputLayout.NumElements = ( UINT )layout.size();
			desc.InputLayout.pInputElementDescs = layout.data();
			update( pDevice, descRTs );
			create( pDevice );
		}

		void setParams( ID3D12RootSignature* rootSignature, ID3DBlob* vs, ID3DBlob* ps, D3D12_INPUT_ELEMENT_DESC const* inputElementDesc, size_t NumElements, D3D12_GRAPHICS_PIPELINE_STATE_DESC const& psoDesc ) {
			sig = rootSignature;
			VS = vs;
			PS = ps;
			layout = std::vector< D3D12_INPUT_ELEMENT_DESC >( inputElementDesc, inputElementDesc + NumElements );
			desc = psoDesc;
			desc.pRootSignature = sig;
			desc.VS.pShaderBytecode = VS->GetBufferPointer();
			desc.VS.BytecodeLength = VS->GetBufferSize();
			desc.PS.pShaderBytecode = PS->GetBufferPointer();
			desc.PS.BytecodeLength = PS->GetBufferSize();
			desc.InputLayout.NumElements = ( UINT )layout.size();
			desc.InputLayout.pInputElementDescs = layout.data();
		}
		template< size_t _SZ >
		void setParams( ID3D12RootSignature* rootSignature, ID3DBlob* vs, ID3DBlob* ps, const D3D12_INPUT_ELEMENT_DESC( &inputElementDesc )[_SZ], D3D12_GRAPHICS_PIPELINE_STATE_DESC const& psoDesc ) { setParams( rootSignature, vs, ps, inputElementDesc, _SZ, psoDesc ); }

		void update( ID3D12Device* pDevice, std::vector<D3D12_RESOURCE_DESC> const& descRTs ) {
			bool descChanged = descRTs.size() != desc.NumRenderTargets;
			for( size_t i = 0; i != descRTs.size(); i++ )
			{
				auto& descRT = descRTs[i];
				DXGI_FORMAT newFormat;
				switch( descRT.Format )
				{
				case DXGI_FORMAT_R8G8B8A8_UNORM:
				case DXGI_FORMAT_R32G32_FLOAT:
				case DXGI_FORMAT_R32_FLOAT:
				case DXGI_FORMAT_R16_FLOAT:
				case DXGI_FORMAT_R8_UINT:
				case DXGI_FORMAT_R8_UNORM:
				case DXGI_FORMAT_BC1_UNORM_SRGB:
				case DXGI_FORMAT_BC1_UNORM:
				case DXGI_FORMAT_BC2_UNORM_SRGB:
				case DXGI_FORMAT_BC2_UNORM:
				case DXGI_FORMAT_BC3_UNORM_SRGB:
				case DXGI_FORMAT_BC3_UNORM:
				case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
				case DXGI_FORMAT_R16G16B16A16_FLOAT:
				case DXGI_FORMAT_R11G11B10_FLOAT:
				case DXGI_FORMAT_R10G10B10A2_UNORM:
				case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				case DXGI_FORMAT_B8G8R8A8_UNORM:
				case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				case DXGI_FORMAT_A8_UNORM:
					newFormat = descRT.Format;
					break;
				case DXGI_FORMAT_B5G5R5A1_UNORM:
					newFormat = DXGI_FORMAT_B5G5R5A1_UNORM;
					break;
				case DXGI_FORMAT_R8G8B8A8_TYPELESS:
					newFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
					break;
				case DXGI_FORMAT_D32_FLOAT:
				case DXGI_FORMAT_R32_TYPELESS:
					newFormat = DXGI_FORMAT_R32_FLOAT;
					break;
				case DXGI_FORMAT_R16_TYPELESS:
					newFormat = DXGI_FORMAT_R16_FLOAT;
					break;
				case DXGI_FORMAT_R8_TYPELESS:
					newFormat = DXGI_FORMAT_R8_UNORM;
					break;
				case DXGI_FORMAT_BC1_TYPELESS:
					newFormat = DXGI_FORMAT_BC1_UNORM;
					break;
				case DXGI_FORMAT_BC2_TYPELESS:
					newFormat = DXGI_FORMAT_BC2_UNORM;
					break;
				case DXGI_FORMAT_BC3_TYPELESS:
					newFormat = DXGI_FORMAT_BC3_UNORM;
					break;
				case DXGI_FORMAT_D24_UNORM_S8_UINT:
				case DXGI_FORMAT_R24G8_TYPELESS:
					newFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
					break;
				case DXGI_FORMAT_R16G16B16A16_TYPELESS:
					newFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
					break;
				case DXGI_FORMAT_R32G32_TYPELESS:
					newFormat = DXGI_FORMAT_R32G32_FLOAT;
					break;
				case DXGI_FORMAT_R16G16_FLOAT:
				case DXGI_FORMAT_R16G16_TYPELESS:
					newFormat = DXGI_FORMAT_R16G16_FLOAT;
					break;
				case DXGI_FORMAT_R32G32B32A32_TYPELESS:
				case DXGI_FORMAT_R32G32B32A32_FLOAT:
					newFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
					break;
				case DXGI_FORMAT_R16_UINT:
					newFormat = DXGI_FORMAT_R16_UINT;
					break;
				case DXGI_FORMAT_R16_SINT:
					newFormat = DXGI_FORMAT_R16_SINT;
					break;
				case DXGI_FORMAT_R32_UINT:
					newFormat = DXGI_FORMAT_R32_UINT;
					break;
				case DXGI_FORMAT_UNKNOWN:
					newFormat = DXGI_FORMAT_UNKNOWN;
					break;
				default:
					newFormat = DXGI_FORMAT_UNKNOWN;
				};
				if( desc.RTVFormats[i] != newFormat )
				{
					descChanged = true;
					desc.RTVFormats[i] = newFormat;
				}
			}

			if( desc.SampleDesc.Count != descRTs[0].SampleDesc.Count || desc.SampleDesc.Quality != descRTs[0].SampleDesc.Quality )
			{
				desc.SampleDesc = descRTs[0].SampleDesc;
				descChanged = true;
			}

			if( descChanged )
			{
				if( state )
				{
					state.Release();
					THROW_IF_FAILED(create( pDevice ));
				}
			}
		}

		HRESULT create( ID3D12Device* pDevice ) {
			if( nullptr == pDevice )
				return E_INVALIDARG;
			return pDevice->CreateGraphicsPipelineState( &desc, IID_PPV_ARGS( &state ) );
		}

		void use( ID3D12Device* pDevice, ID3D12GraphicsCommandList* cl, std::vector<D3D12_RESOURCE_DESC> const& descRTs ) {
			update( pDevice, descRTs );
			if( !state )
				THROW_IF_FAILED(create( pDevice ));
			cl->SetPipelineState( state );
			cl->SetGraphicsRootSignature( sig );
		}
	};

	D3D12Renderer( D3D12Base const& parent ) : D3D12Base( parent ) {}

	virtual bool preRender( Alloc& alloc, DirectX::XMMATRIX& world, DirectX::XMMATRIX& projection ) = 0;
	virtual bool render( Alloc& alloc, std::vector<D3D12_RESOURCE_DESC> const& descRTs ) = 0;
	virtual bool postRender( Alloc& alloc ) = 0;
};

/// A simple base class for a frame buffer ring, either a swapchain or some textures
class D3D12RT : public D3D12Base {
protected:
	/// A simple wrapper for a render target resource
	struct RT {
		std::vector<CComPtr<ID3D12Resource>> rts; /// the managed resources
		//! this is where the depth tex and stencil goes if added
		DescriptorHeap rtHeap; /// the RTV heap
		Alloc alloc; /// the allocator, consisting of a command allocator, a command list and synchronization facilties
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

		/// add a resource transition to the current command queue
		void trans( D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after ) {
			std::vector< D3D12_RESOURCE_BARRIER > trans;
			for( auto& rt : rts )
				trans.emplace_back(D3D12_RESOURCE_BARRIER{
					D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					D3D12_RESOURCE_BARRIER_FLAG_NONE,
					{.Transition {
						rt,
						D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
						before,
						after
					} } });
			alloc->ResourceBarrier( (UINT)trans.size(), trans.data() );
		}
		/// set myself as a render target
		void set() {
			alloc->OMSetRenderTargets( ( UINT )rts.size(), rtHeap.getCPUHandleStartCPtr(), TRUE, nullptr );
		}
		/// clear the render target
		/// @param clearColor the clear color as FLOAT[4] in RGBA
		void clear( FLOAT const (& clearColor)[4] ) {
			alloc->ClearRenderTargetView( rtHeap.getCPUHandle(), clearColor, 0, nullptr );
		}
	};
	CComPtr<IDXGISwapChain3> m_swapchain; // maybe null, if render texture
	HWND m_hWnd = 0; // maybe null, if render texture

	std::vector<RT> m_renderTargets;
	std::list<std::shared_ptr<D3D12Renderer>> m_renderers;

	UINT m_frameIndex = 0;

	D3D12_RESOURCE_DESC m_descRT;
	CD3DX12_VIEWPORT m_viewport{};
	CD3DX12_RECT m_scissorRect{};


	DirectX::XMMATRIX m_world;
	DirectX::XMMATRIX m_projection;

	D3D12_RESOURCE_STATES m_otherState = D3D12_RESOURCE_STATE_PRESENT;

public:

	static CComPtr< IDXGISwapChain3 > createSwapChain( IDXGIFactory4* factory, ID3D12CommandQueue* queue, HWND hWnd, UINT nFrames, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM ) {
		RECT r;
		::GetClientRect( hWnd, &r );
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = nFrames;
		swapChainDesc.Width = r.right;
		swapChainDesc.Height = r.bottom;
		swapChainDesc.Format = format;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		CComPtr<IDXGISwapChain1> swapChain1;
		THROW_IF_FAILED( factory->CreateSwapChainForHwnd(
			queue,        // Swap chain needs the queue so that it can force a flush on it.
			hWnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain1
		) );

		// This sample does not support fullscreen transitions. Set the window size and position to cover a display
		THROW_IF_FAILED( factory->MakeWindowAssociation( hWnd, DXGI_MWA_NO_ALT_ENTER ) );

		CComPtr< IDXGISwapChain3 > sc;
		THROW_IF_FAILED( swapChain1.QueryInterface( &sc ) );
		return sc;
		// a swapchain can't be named for debugging
	}

	// init as render textures
	D3D12RT( D3D12Base const& parent, UINT w, UINT h, UINT nFrames, UINT nRT, DXGI_FORMAT format, wchar_t const* name = nullptr )
		: D3D12Base( parent )
		, m_otherState( D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE )
		, m_descRT{
			D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			0, w, h, 1, 1, format, { 1, 0 },
			D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET }
	{
		D3D12_HEAP_PROPERTIES props{ D3D12_HEAP_TYPE_DEFAULT };
		D3D12_CLEAR_VALUE clv{ format, {0.0f, 0.0f, 0.0f, 1.0f} };

		for( UINT i = 0; i != nFrames; i++ )
		{
			std::vector<CComPtr<ID3D12Resource>> rts( nRT );
			std::wstring sName = name ? name : L"";
			for( UINT j = 0; j != nRT; j++ )
			{
				THROW_IF_FAILED1( m_device->CreateCommittedResource( &props, D3D12_HEAP_FLAG_NONE, &m_descRT, m_otherState, &clv, IID_PPV_ARGS( &rts[j] ) ), "Failed to create render texture." );
				if( name && name[0] )
				{
					sName += L"_" + std::to_wstring( i );
					if( nRT > 1 )
					{
						sName += L"_" + std::to_wstring( j );
						rts[j]->SetName( ( sName + L"_" + std::to_wstring( j ) ).c_str() );
					} else {
						rts[j]->SetName( sName.c_str() );
					}
				}
			}
			D3D12_DESCRIPTOR_HEAP_DESC desc{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV, nRT };
			m_renderTargets.emplace_back( RT{ std::vector<CComPtr<ID3D12Resource>>( rts.begin(), rts.end() ), DescriptorHeap( m_device, desc, sName.c_str() ), Alloc( m_device, m_queue ) } );
			m_renderTargets.back().alloc.cl->Close();
			for( auto j = 0; j != nRT; j++ )
			{
				m_device->CreateRenderTargetView( m_renderTargets.back().rts[j], nullptr, m_renderTargets.back().rtHeap.getCPUHandle( j ) );
			}
		}
		// fill viewport and scissor rect
		m_viewport = CD3DX12_VIEWPORT( 0.0f, 0.0f, FLOAT( w ), FLOAT( h ) );
		m_scissorRect = CD3DX12_RECT( 0, 0, w, h );

		m_world = DirectX::XMMatrixIdentity();
		m_projection = DirectX::XMMatrixPerspectiveFovLH( DirectX::XMConvertToRadians( 90 ), float( w ) / h, 0.125f, 16384.0f );
	}

	// a swapchain
	D3D12RT( IDXGIAdapter1* adapter, HWND hWnd, UINT nFrames, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, wchar_t const* name = nullptr )
		: D3D12Base( adapter, name )
		, m_swapchain( createSwapChain( getFactory(), m_queue, hWnd, nFrames, format ))
		, m_hWnd( hWnd )
		, m_otherState( D3D12_RESOURCE_STATE_PRESENT )
	{
		for( UINT i = 0; i != nFrames; i++ )
		{
			CComPtr<ID3D12Resource> buff;
			THROW_IF_FAILED1( m_swapchain->GetBuffer( i, IID_PPV_ARGS( &buff ) ), "Failed to get buffer from swapchain.");
			D3D12_DESCRIPTOR_HEAP_DESC desc{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1 };
			std::wstring sName = name ? name : L"";
			if( name && name[0] )
			{
				sName += L"_" + std::to_wstring( i );
				buff->SetName( sName.c_str() );
			}
			m_renderTargets.emplace_back( RT{ std::vector<CComPtr<ID3D12Resource>>{ buff }, DescriptorHeap( m_device, desc, sName.c_str() ), Alloc( m_device, m_queue ) } );
			m_device->CreateRenderTargetView( m_renderTargets.back().rts[0], nullptr, m_renderTargets.back().rtHeap.getCPUHandle() );
			m_renderTargets.back().alloc.cl->Close();
		}
		m_descRT = m_renderTargets.front().rts.front()->GetDesc();
		m_frameIndex = m_swapchain->GetCurrentBackBufferIndex();

		// fill viewport and scissor rect
		m_viewport = CD3DX12_VIEWPORT( 0.0f, 0.0f, FLOAT( m_descRT.Width ), FLOAT( m_descRT.Height ) );
		m_scissorRect = CD3DX12_RECT( 0, 0, UINT( m_descRT.Width ) , m_descRT.Height );

		m_world = DirectX::XMMatrixIdentity();
		m_projection = DirectX::XMMatrixPerspectiveFovLH( DirectX::XMConvertToRadians( 90 ), FLOAT( m_descRT.Height ) / m_descRT.Height, 0.125f, 16383.f );
	}

	virtual ~D3D12RT() {
		// release the renderes
		m_renderers.clear();
		// wait for all queues to finish
		for( auto& rt : m_renderTargets )
		{
			rt.alloc.wait();
		}

		std::wostringstream os;

		m_renderTargets.clear();
#ifdef _DEBUG
		if (m_queue) {
			m_queue.p->AddRef();
			ULONG refc = m_queue.p->Release();
			os << "Releasing m_queue 0x" << m_queue.p << " refc " << refc << std::endl;
			OutputDebugStringW(os.str().c_str());
		}
#endif //def _DEBUG
		m_queue.Release();
		m_swapchain.Release();
	}

	HRESULT resizeSwapChain( UINT w, UINT h, wchar_t const* name = nullptr ) {
		// Wait for the GPU to finish all previous work
		m_device->GetDeviceRemovedReason();

		// Resize the swap chain
		HRESULT hr = m_swapchain->ResizeBuffers( 0, w, h, DXGI_FORMAT_UNKNOWN, 0 );
		if( FAILED( hr ) ) {
			std::cerr << "Failed to resize swap chain buffers." << std::endl;
			return hr;
		}

		for( UINT i = 0; i != (UINT)m_renderTargets.size(); i++ )
		{
			CComPtr<ID3D12Resource> buff;
			THROW_IF_FAILED1( m_swapchain->GetBuffer( i, IID_PPV_ARGS( &buff ) ), "Failed to get buffer from swapchain." );
			D3D12_DESCRIPTOR_HEAP_DESC desc{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1 };
			std::wstring sName = name ? name : L"";
			if( name && name[0] )
			{
				sName += L"_" + std::to_wstring( i );
				buff->SetName( sName.c_str() );
			}
			m_renderTargets[i].rts[0] = buff;
			m_device->CreateRenderTargetView( m_renderTargets[i].rts[0], nullptr, m_renderTargets[i].rtHeap.getCPUHandle() );
		}
		return hr;
	}

	// base implementation
	virtual bool preRender()
	{
		// get new frame index
		if( m_swapchain )
			m_frameIndex = m_swapchain->GetCurrentBackBufferIndex();
		else
			m_frameIndex = ( m_frameIndex + 1 ) % m_renderTargets.size();

		auto& curr = m_renderTargets[m_frameIndex];
		curr.alloc.reset();

		// transition of current rts to to render target state
		curr.trans( m_otherState, D3D12_RESOURCE_STATE_RENDER_TARGET );

		// clear
		static const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		for( UINT i = 0; i != ( UINT )curr.rts.size(); i++ )
			curr.alloc->ClearRenderTargetView( curr.rtHeap.getCPUHandle( i ), clearColor, 0, nullptr );

		curr.alloc->OMSetRenderTargets( ( UINT )curr.rts.size(), curr.rtHeap.getCPUHandleStartCPtr(), TRUE, nullptr );

		// set viewport && scissorRects
		curr.alloc->RSSetViewports( 1, &m_viewport );
		curr.alloc->RSSetScissorRects( 1, &m_scissorRect );

		for( auto& renderer : m_renderers )
			if( !renderer->preRender( curr.alloc, m_world, m_projection ) )
				return false;

		return true;
	}

	// base implementation
	virtual bool render()
	{
		auto& alloc = m_renderTargets[m_frameIndex].alloc;
		for( auto& renderer : m_renderers )
			if( !renderer->render( alloc, { m_descRT } ) )
				return false;
		return true;
	}

	// base implamentation, call last in your derived postRender
	virtual HRESULT postRender()
	{
		auto& curr = m_renderTargets[m_frameIndex];

		for( auto& renderer : m_renderers )
			if( !renderer->postRender( curr.alloc ) )
				return E_FAIL;

		curr.trans( D3D12_RESOURCE_STATE_RENDER_TARGET, m_otherState );


		if( m_swapchain )
		{
			curr.alloc.exec( true ); // we wait for the cl to finish execution
			HRESULT hr = m_swapchain->Present( 1, 0 );
			return hr;
		}
		else
		{
			curr.alloc.exec(); // we don't wait for the cl to finish execution, so we must add a wait in the receiver
			return S_OK;
		}
	}

	bool addRenderer( std::unique_ptr<D3D12Renderer>&& renderer ) {
		m_renderers.emplace_back( std::move( renderer ) );
		return true;
	}

	D3D12_RESOURCE_DESC const& getDescRT() const { return m_descRT; }

	std::vector<CComPtr<ID3D12Resource>> const& getCurrentRTs() const { return m_renderTargets[m_frameIndex].rts; }
	Alloc const& getCurrentAlloc() const { return m_renderTargets[m_frameIndex].alloc; }
	DescriptorHeap const& getCurrentRTHeap() const { return m_renderTargets[m_frameIndex].rtHeap; }

	void setView( DirectX::XMMATRIX const& world, DirectX::XMMATRIX const& projection ) {
		m_world = world;
		m_projection = projection;
	}
	void getView( DirectX::XMMATRIX& world, DirectX::XMMATRIX& projection ) const {
		world = m_world;
		projection = m_projection;
	}
};

class D3D12Wnd : public D3D12RT {
protected:
	std::wstring m_name;
	std::atomic_bool m_running = false;

	// special message handlers
	void OnResize( int w, int h ) // reallocate backbuffers
	{
		resizeSwapChain( w, h, m_name.c_str() );
	}

	// the window proc member function
	virtual LRESULT WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
	{
		switch( uMsg )
		{
		case WM_SYSCOMMAND:							// Intercept System Commands
		{
			switch( wParam )							// Check System Calls
			{
			case SC_SCREENSAVE:					// Screensaver Trying To Start?
			case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;							// Prevent From Happening
			}
		}
		break;

		case WM_CLOSE:
		{
			PostQuitMessage( 0 );	// Send A Quit Message
			return 0;
		}

		case WM_KEYDOWN:
		{
			if( VK_ESCAPE == wParam )
			{
				PostQuitMessage( 0 );	// Send A Quit Message
				return 0;
			}
		}
		break;

		case WM_SIZE:
		{
			OnResize( LOWORD( lParam ), HIWORD( lParam ) );  // LoWord=Width, HiWord=Height
			return 0;
		}

		case WM_DPICHANGED:
		{
			RECT* const prcNewWindow = ( RECT* )lParam;
			// this will trigger WM_SIZE message in case
			SetWindowPos( m_hWnd,
						  NULL,
						  prcNewWindow->left,
						  prcNewWindow->top,
						  prcNewWindow->right - prcNewWindow->left,
						  prcNewWindow->bottom - prcNewWindow->top,
						  SWP_NOZORDER | SWP_NOACTIVATE );
			return 0;
		}
		};
		return -1;
	}

	// the actual window proc callback
	static LRESULT CALLBACK _WndProc( HWND	hWnd,			// Handle For This Window
									  UINT	uMsg,			// Message For This Window
									  WPARAM	wParam,			// Additional Message Information
									  LPARAM	lParam )			// Additional Message Information
	{
		D3D12Wnd* _this = nullptr;
		if( uMsg == WM_NCCREATE ) {
			CREATESTRUCT* pCreate = reinterpret_cast< CREATESTRUCT* >( lParam );
			_this = reinterpret_cast< D3D12Wnd* >( pCreate->lpCreateParams );
			SetWindowLongPtr( hWnd, GWLP_USERDATA, reinterpret_cast< LONG_PTR >( _this ) );
		}
		else {
			_this = reinterpret_cast< D3D12Wnd* >( GetWindowLongPtr( hWnd, GWLP_USERDATA ) );
		}
		if( _this && _this->m_hWnd == hWnd )
		{
			auto res = _this->WndProc( uMsg, wParam, lParam );
			if( 0 == res ) // check, if message was handled
				return res;
		}
		// Pass All Unhandled Messages To DefWindowProc
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}

	static HWND createWnd( int x, int y, int w, int h, std::tstring const& windowName, int cmdShow, D3D12Wnd* me ) {
		HWND hWnd;
		HINSTANCE hInstance = GetModuleHandle( NULL );				// Grab An Instance For Our Window
		WNDCLASS wc{};
		LPCTSTR clsName = _T( "VIOSOdomeprojectionWindowCLS" );
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
		wc.lpfnWndProc = ( WNDPROC )_WndProc;					// WndProc Handles Messages
		wc.cbClsExtra = 0;									// No Extra Window Data
		wc.cbWndExtra = 0;									// No Extra Window Data
		wc.hInstance = hInstance;							// Set The Instance
		wc.hIcon = LoadIcon( NULL, IDI_WINLOGO );			// Load The Default Icon
		wc.hCursor = LoadCursor( NULL, IDC_ARROW );			// Load The Arrow Pointer
		wc.hbrBackground = NULL;									// No Background Required For GL
		wc.lpszMenuName = NULL;									// We Don't Want A Menu
		wc.lpszClassName = clsName;								// Set The Class Name

		if( !RegisterClass( &wc ) )									// Attempt To Register The Window Class
		{
			throw std::runtime_error( "Failed To Register The Window Class." );
		}

		DWORD dwExStyle = WS_EX_APPWINDOW;								// Window Extended Style
		DWORD dwStyle = WS_POPUP;										// Windows Style
		RECT rc{ x, y, x + w, y + h };
		AdjustWindowRectEx( &rc, dwStyle, FALSE, dwExStyle );		// Adjust Window To True Requested Size

		// Create The Window
		if( !( hWnd = CreateWindowEx( dwExStyle,							// Extended Style For The Window
									  clsName,							// Class Name
									  windowName.c_str(),								// Window Title
									  dwStyle |							// Defined Window Style
									  WS_CLIPSIBLINGS |					// Required Window Style
									  WS_CLIPCHILDREN,					// Required Window Style
									  rc.left, rc.top,								// Window Position
									  rc.right - rc.left,	// Calculate Window Width
									  rc.bottom - rc.top,	// Calculate Window Height
									  NULL,								// No Parent Window
									  NULL,								// No Menu
									  hInstance,							// Instance
									  me ) ) )								// Pass my class pointer
			throw std::runtime_error( "Failed to create window." );
		ShowWindow( hWnd, cmdShow );
		return hWnd;
	}

public:

	D3D12Wnd( int x, int y, int w, int h, std::tstring const& windowName, UINT nFrames, int cmdShow )
		: D3D12RT( findAdapter( getFactory(), D3D_FEATURE_LEVEL_12_0, x, y ), createWnd( x, y, w, h, windowName, cmdShow, this ), nFrames, DXGI_FORMAT_R8G8B8A8_UNORM, windowName.c_str() )
		, m_name( windowName )
	{}

	virtual HRESULT postRender() override
	{
		HRESULT hr = D3D12RT::postRender();

		// try to recover
		if( DXGI_ERROR_DEVICE_RESET == hr )
		{
			/// do smth.
			int i = 0;
			std::tcerr << "Error: Device Reset." << std::endl;
		}
		return hr;
	}

	int loop()
	{
		std::tcout << _T( "Begin render loop." ) << std::endl;

		MSG msg{};
		m_running = true;
		while( m_running )
		{
			if( !PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
			{
				if(
					!preRender() ||
					!render() ||
					!SUCCEEDED( postRender() ) )
				{
					std::tcerr << _T( "Error in render loop." ) << std::endl;
					m_running = false;
				}
			}
			else if( WM_QUIT != msg.message )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			else
			{
				m_running = false;
			}
		}

		std::tcout << _T( "End render loop." ) << std::endl;

		return ( int )msg.wParam;
	}
};
