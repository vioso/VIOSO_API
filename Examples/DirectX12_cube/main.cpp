
// cpp includes
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <map>

#define NOMINMAX // disable min/max macros
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellscalingapi.h>

#define D3D12_BASE_IMPLEMENTATION
#include "D3D12Base.h"

#include "VIOSOWarpBlend.hpp"
#include "StringConversions.h"
extern "C" const GUID DECLSPEC_SELECTANY DXGI_DEBUG_D3D12{ 0xcf59a98c, 0xa950, 0x4326, { 0x91, 0xef, 0x9b, 0xba, 0xa1, 0x7b, 0xfd, 0x95 } };

// shortcuts
using namespace std;
using namespace DirectX;
namespace fs = std::filesystem;

// program settings and constants
int         g_windowPosX = 0;
int         g_windowPosY = 0;
int         g_windowWidth = 1000;
int         g_windowHeight = 1000;

// dpLib settings
std::string g_configfile = "data\\dynamic\\config.xml";
string         g_channel = "0";

bool        g_bDebug = false;

float       g_fWarping = 1.f;
float       g_fBlending = 1.f;
float       g_fBlackLevel = 1.f;
float       g_fSecondaryBlending = 1.f;

bool        g_bWarping = true;
bool        g_bBlending = true;
bool        g_bBlackLevel = true;
bool        g_bSecondaryBlending = true;

float       g_fPositionX = 0.f;
float       g_fPositionY = 0.f;
float       g_fPositionZ = 0.f;
float       g_fHeading = 0.f;
float       g_fPitch = 0.f;
float       g_fRoll = 0.f;

// DX12 related settings
static const int c_frames = 2; // the number of buffers to acquire; must be at least 2 in D3D12, to work with swapchain's present


/// dpi awareness for this application, make sure to call before creating any windows
/// Microsoft recommends using the application manifest rather than this call.
HRESULT beDPIAware( PROCESS_DPI_AWARENESS pda ) {
    HRESULT hr = E_FAIL;
    typedef HRESULT( STDAPICALLTYPE* PFN_Set_Process_Dpi_Awareness )( _In_ PROCESS_DPI_AWARENESS value );
    auto hDll = ::LoadLibrary( _T( "Shcore.dll" ) );
    if( hDll ) {
        auto SetProcessDpiAwareness = ( PFN_Set_Process_Dpi_Awareness )::GetProcAddress( hDll, "SetProcessDpiAwareness" );
        if( SetProcessDpiAwareness ) {
            hr = SetProcessDpiAwareness( pda );
        }
        ::FreeLibrary( hDll );
    }
    return hr;
}

/// dpi awareness for this thread, make sure to call before creating any windows
/// Use only, if you need different behaviour, like a full-screen window, and keep the UI scaled.
DPI_AWARENESS_CONTEXT threadBeDPIAware( DPI_AWARENESS_CONTEXT dac ) {
    DPI_AWARENESS_CONTEXT res = NULL;
    typedef DPI_AWARENESS_CONTEXT ( STDAPICALLTYPE* PFN_SetThreadDpiAwarenessContext )( _In_ DPI_AWARENESS_CONTEXT value );
    auto hDll = ::LoadLibrary( _T( "User32.dll" ) );
    if( hDll ) {
        auto SetThreadDpiAwarenessContext = ( PFN_SetThreadDpiAwarenessContext )::GetProcAddress( hDll, "SetThreadDpiAwarenessContext" );
        if( SetThreadDpiAwarenessContext ) {
            res = SetThreadDpiAwarenessContext( dac );
        }
        ::FreeLibrary( hDll );
    }
    return res;
}

//////////////////////////////////////////////////////////////////////////////////////

class MyD3D12Wnd : public D3D12Wnd {
    virtual LRESULT WndProc( UINT uMsg, WPARAM wParam, LPARAM lParam ) override {
        if( uMsg == WM_KEYDOWN ) {
            switch( wParam )
            {
            case '1':
                g_fWarping = std::min( g_fWarping + 0.1f, 1.f );
                break;
            case '2':
                g_fWarping = std::max( g_fWarping - 0.1f, 0.f );
                break;
            case '3':
                g_fBlending = std::min( g_fBlending + 0.1f, 1.f );
                break;
            case '4':
                g_fBlending = std::max( g_fBlending - 0.1f, 0.f );
                break;
            case '5':
                g_fBlackLevel = g_fBlackLevel + 0.1f;
                break;
            case '6':
                g_fBlackLevel = std::max( g_fBlackLevel - 0.1f, 0.f );
                break;
            case '7':
                g_fSecondaryBlending = std::min( g_fSecondaryBlending + 0.1f, 1.f );
                break;
            case '8':
                g_fSecondaryBlending = std::max( g_fSecondaryBlending - 0.1f, 0.f );
                break;
            case 'Q':
                g_bWarping = !g_bWarping;
                break;
            case 'W':
                g_bBlending = !g_bBlending;
                break;
            case 'E':
                g_bBlackLevel = !g_bBlackLevel;
                break;
            case 'R':
                g_bSecondaryBlending = !g_bSecondaryBlending;
                break;
            case VK_LEFT:
                g_fPositionX -= 0.10f;
                break;
            case VK_RIGHT:
                g_fPositionX += 0.10f;
                break;
            case VK_UP:
                g_fPositionY += 0.10f;
                break;
            case VK_DOWN:
                g_fPositionY -= 0.10f;
                break;
            case VK_PRIOR:
                g_fPositionZ += 0.10f;
                break;
            case VK_NEXT:
                g_fPositionZ -= 0.10f;
                break;

            case VK_OEM_PERIOD:
                g_fRoll += 2.0f;
                break;
            case VK_OEM_COMMA:
                g_fRoll -= 2.0f;
                break;

            case VK_DELETE:
                g_fHeading += 2.0f;
                break;
            case VK_END:
                g_fHeading -= 2.0f;
                break;
            default:
                return D3D12Wnd::WndProc( uMsg, wParam, lParam );
            };
            // if we are here, we evaluated some movement, so we update the world matrix
            DirectX::XMMATRIX mat, mat2, matY, matP, matR, matT;
            matY = DirectX::XMMatrixRotationY( DirectX::XMConvertToRadians( g_fHeading ) );
            matP = DirectX::XMMatrixRotationX( DirectX::XMConvertToRadians( g_fPitch ) );
            matR = DirectX::XMMatrixRotationZ( DirectX::XMConvertToRadians( g_fRoll ) );
            mat2 = DirectX::XMMatrixMultiply( matP, matR );
            mat = DirectX::XMMatrixMultiply( matY, mat2 );
            matT = XMMatrixTranslation( -g_fPositionX, -g_fPositionY, -g_fPositionZ );
            m_world = XMMatrixMultiply( mat, matT );
            cout << "X: " << g_fPositionX << " Y: " << g_fPositionY << " Z: " << g_fPositionZ << "Y: " << g_fHeading << " P: " << g_fPitch << " R: " << g_fRoll << endl;
        }
        return D3D12Wnd::WndProc( uMsg, wParam, lParam );
    }

public:
    MyD3D12Wnd( int cmdShow = SW_SHOW ) 
    : D3D12Wnd( g_windowPosX, g_windowPosY, g_windowWidth, g_windowHeight, _T( "Simple D3D12 example" ), c_frames, cmdShow )
    {
    }
};

class SkyBox : public D3D12Renderer {
protected:
    static constexpr float w = 10.0f;
    static constexpr float h = 10.0f;
    static constexpr float d = 10.0f;

    // we got left-handed coordinate system, x->right, y->up, z->forward, we have clockwise back-face culling
    // we want the cube to surround us, thus all primitives must face towards (0,0,0)
    // texture coordinates are 0,0 to sample top-left
    // the texture index is refering to the index of the entry in texturePaths
    static constexpr Vertex_POS3_TX2_TXID vertices[] = {
        // front
        { DirectX::XMFLOAT3( -w / 2.0f,  h / 2.0f, d / 2.0f ), DirectX::XMFLOAT2( 0.0f, 0.0f ), 0 }, // left-top
        { DirectX::XMFLOAT3(  w / 2.0f,  h / 2.0f, d / 2.0f ), DirectX::XMFLOAT2( 1.0f, 0.0f ), 0 }, // right-top
        { DirectX::XMFLOAT3( -w / 2.0f, -h / 2.0f, d / 2.0f ), DirectX::XMFLOAT2( 0.0f, 1.0f ), 0 }, // left-bottom
        { DirectX::XMFLOAT3(  w / 2.0f, -h / 2.0f, d / 2.0f ), DirectX::XMFLOAT2( 1.0f, 1.0f ), 0 }, // right-bottom

        // back
        { DirectX::XMFLOAT3(  w / 2.0f,  h / 2.0f, -d / 2.0f ), DirectX::XMFLOAT2( 0.0f, 0.0f ), 1 },
        { DirectX::XMFLOAT3( -w / 2.0f,  h / 2.0f, -d / 2.0f ), DirectX::XMFLOAT2( 1.0f, 0.0f ), 1 },
        { DirectX::XMFLOAT3(  w / 2.0f, -h / 2.0f, -d / 2.0f ), DirectX::XMFLOAT2( 0.0f, 1.0f ), 1 },
        { DirectX::XMFLOAT3( -w / 2.0f, -h / 2.0f, -d / 2.0f ), DirectX::XMFLOAT2( 1.0f, 1.0f ), 1 },

        // left
        { DirectX::XMFLOAT3( -w / 2.0f,  h / 2.0f, -d / 2.0f ), DirectX::XMFLOAT2( 0.0f, 0.0f ), 2 },
        { DirectX::XMFLOAT3( -w / 2.0f,  h / 2.0f,  d / 2.0f ), DirectX::XMFLOAT2( 1.0f, 0.0f ), 2 },
        { DirectX::XMFLOAT3( -w / 2.0f, -h / 2.0f, -d / 2.0f ), DirectX::XMFLOAT2( 0.0f, 1.0f ), 2 },
        { DirectX::XMFLOAT3( -w / 2.0f, -h / 2.0f,  d / 2.0f ), DirectX::XMFLOAT2( 1.0f, 1.0f ), 2 },

        // right
        { DirectX::XMFLOAT3(  w / 2.0f,  h / 2.0f,  d / 2.0f ), DirectX::XMFLOAT2( 0.0f, 0.0f ), 3 },
        { DirectX::XMFLOAT3(  w / 2.0f,  h / 2.0f, -d / 2.0f ), DirectX::XMFLOAT2( 1.0f, 0.0f ), 3 },
        { DirectX::XMFLOAT3(  w / 2.0f, -h / 2.0f,  d / 2.0f ), DirectX::XMFLOAT2( 0.0f, 1.0f ), 3 },
        { DirectX::XMFLOAT3(  w / 2.0f, -h / 2.0f, -d / 2.0f ), DirectX::XMFLOAT2( 1.0f, 1.0f ), 3 },

        // bottom
        { DirectX::XMFLOAT3( -w / 2.0f, -h / 2.0f,  d / 2.0f ), DirectX::XMFLOAT2( 0.0f, 0.0f ), 4 },
        { DirectX::XMFLOAT3(  w / 2.0f, -h / 2.0f,  d / 2.0f ), DirectX::XMFLOAT2( 1.0f, 0.0f ), 4 },
        { DirectX::XMFLOAT3( -w / 2.0f, -h / 2.0f, -d / 2.0f ), DirectX::XMFLOAT2( 0.0f, 1.0f ), 4 },
        { DirectX::XMFLOAT3(  w / 2.0f, -h / 2.0f, -d / 2.0f ), DirectX::XMFLOAT2( 1.0f, 1.0f ), 4 },

        // top
        { DirectX::XMFLOAT3( -w / 2.0f, h / 2.0f, -d / 2.0f ), DirectX::XMFLOAT2( 0.0f, 0.0f ), 5 },
        { DirectX::XMFLOAT3(  w / 2.0f, h / 2.0f, -d / 2.0f ), DirectX::XMFLOAT2( 1.0f, 0.0f ), 5 },
        { DirectX::XMFLOAT3( -w / 2.0f, h / 2.0f,  d / 2.0f ), DirectX::XMFLOAT2( 0.0f, 1.0f ), 5 },
        { DirectX::XMFLOAT3(  w / 2.0f, h / 2.0f,  d / 2.0f ), DirectX::XMFLOAT2( 1.0f, 1.0f ), 5 },
    };
    static constexpr DWORD indices[] = {
        // front
         0,  1,  2,
         1,  3,  2,

        // back
         4,  5,  6,
         5,  7,  6,

        // left
         8,  9, 10,
         9, 11, 10,

        // right
        12, 13, 14,
        13, 15, 14,

        // bottom
        16, 17, 18,
        17, 19, 18,

        // top
        20, 21, 22,
        21, 23, 22
    };

    static constexpr char const* texturePaths[] = {
        "data/cubemap/front.png",
        "data/cubemap/back.png",
        "data/cubemap/left.png",
        "data/cubemap/right.png",
        "data/cubemap/bottom.png",
        "data/cubemap/top.png",
    };

    static constexpr const char* shaderSource = R"XX(
cbuffer TransformBuffer : register(b0)
{
    matrix viewProj;
};

struct VSInput
{
    float3 position : POSITION;
    float2 texCoords : TEXCOORD0;
    uint texID : TEXCOORD1;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoords : TEXCOORD0;
    uint texID : TEXCOORD1;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    
    // Apply view-projection transformation
    output.position = mul(viewProj, float4(input.position, 1.0f));
    output.texCoords = input.texCoords;
    output.texID = input.texID;
    return output;
}

Texture2D textures[6] : register(t0);
SamplerState samplerState : register(s0);

float4 PSMain(VSOutput input) : SV_Target
{
    return textures[input.texID].Sample(samplerState, input.texCoords);
//    return float4( input.texCoords , float(input.texID)/5, 1 );
}
)XX";

    PSO m_pso;

    CComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};

    CComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};

    vector<CComPtr<ID3D12Resource>> m_textures;
    DescriptorHeap m_srvHeap;

    CComPtr<ID3D12Resource> m_constantBuffer;
    struct ConstantBuffer
    {
        XMMATRIX vp; // the view-projection matrix
    };
    ConstantBuffer* m_constants = nullptr;

    static PSO createPSO( ID3D12Device* pDevice, D3D12_RESOURCE_DESC const& rtDesc ) {
        CComPtr<ID3D12RootSignature> sig;

        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if( FAILED( pDevice->CheckFeatureSupport( D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof( featureData ) ) ) )
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        // we create a range for the shader resource views
        D3D12_DESCRIPTOR_RANGE1 ranges[1]{
            D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            _countof( texturePaths ), // number of descriptors, we need 6 textures
            0,  // the base register, maps to register( t0 )
            0, // the register space #, maps to register( , space# ), we don't need register space defined
            D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC, // these textures are all static; in case we need volatile textures, we need another range and set another flag
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND };

        // the sig contains the above created range and the constant buffer view
        CD3DX12_ROOT_PARAMETER1 rootParameters[2]{};
        rootParameters[0].InitAsConstantBufferView( 0 /* register( b0 ) */ );
        rootParameters[1].InitAsDescriptorTable( 1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL ); // we put the range second as descriptor table

        // a sampler description list containig one simple linear sampler
        D3D12_STATIC_SAMPLER_DESC samplers[] =
        {
            { // register( s0 )
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
        };

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1( _countof( rootParameters ), rootParameters, _countof( samplers ), samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT );

        // create the serialized root signature; this is the shader and it's input/output
        CComPtr<ID3DBlob> signature;
        CComPtr<ID3DBlob> error;
        THROW_IF_FAILED( D3DX12SerializeVersionedRootSignature( &rootSignatureDesc, featureData.HighestVersion, &signature, &error ) );
        THROW_IF_FAILED( pDevice->CreateRootSignature( 0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS( &sig ) ) );

        CComPtr<ID3D12PipelineState> pso;

        // create shaders
        CComPtr<ID3DBlob> vertexShader;
        CComPtr<ID3DBlob> pixelShader;

        #if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        #else
        UINT compileFlags = 0;
        #endif

        CComPtr<ID3DBlob> err;
        THROW_IF_FAILED2( D3DCompile( shaderSource, strlen( shaderSource ), "VertexShader@SkyBox", nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &err ), err );
        if( err ) err.Release();

        THROW_IF_FAILED2( D3DCompile( shaderSource, strlen( shaderSource ), "PixelShader@SkyBox", nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &err ), err );

        // Define the vertex input layout.
        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { Vertex_POS3_TX2_TXID::inputElementDescs, _countof( Vertex_POS3_TX2_TXID::inputElementDescs ) };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC( D3D12_DEFAULT );
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.BlendState = CD3DX12_BLEND_DESC( D3D12_DEFAULT );
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        return PSO( pDevice, sig, vertexShader, pixelShader, Vertex_POS3_TX2_TXID::inputElementDescs, _countof( Vertex_POS3_TX2_TXID::inputElementDescs ), psoDesc, { rtDesc } );
    }

public:

    SkyBox( D3D12RT const& parent )
        : D3D12Renderer( parent )
        , m_pso( createPSO( parent.getDevice(), parent.getDescRT() ) )
        , m_srvHeap( parent.getDevice(), { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, _countof( texturePaths ), D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE }, L"SkyBox" )
    {
        static const D3D12_HEAP_PROPERTIES propsUpload{
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            1,1
        };

        D3D12_RESOURCE_DESC desc{
            D3D12_RESOURCE_DIMENSION_BUFFER,
            0, 0, 1, 1, 1,
            DXGI_FORMAT_UNKNOWN, {1,0},
            D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            D3D12_RESOURCE_FLAG_NONE
        };

        // Create the constant buffer and view.
        {
            const UINT cbBuffSize = ( sizeof(ConstantBuffer) + 255 ) & ~255U; // round up
            desc.Width = cbBuffSize;

            // Create the GPU upload buffer.
            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            THROW_IF_FAILED( m_device->CreateCommittedResource(
                &propsUpload,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS( &m_constantBuffer ) ) );
            // just map the buffer and keep it mapped
            CD3DX12_RANGE readRange( 0, 0 );        // We do not intend to read from this resource on the CPU.
            THROW_IF_FAILED( m_constantBuffer->Map( 0, &readRange, reinterpret_cast< void** >( &m_constants ) ) );
            m_constants->vp = XMMatrixIdentity();
        }

        // Create the vertex buffer.
        {
            const UINT vertexBufferSize = UINT( sizeof( vertices ) );
            desc.Width = vertexBufferSize;

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            THROW_IF_FAILED( m_device->CreateCommittedResource(
                &propsUpload,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS( &m_vertexBuffer ) ) );

            // Copy the triangle data to the vertex buffer.
            UINT8* pVertexDataBegin;
            CD3DX12_RANGE readRange( 0, 0 );        // We do not intend to read from this resource on the CPU.
            THROW_IF_FAILED( m_vertexBuffer->Map( 0, &readRange, reinterpret_cast< void** >( &pVertexDataBegin ) ) );
            memcpy( pVertexDataBegin, vertices, vertexBufferSize );
            m_vertexBuffer->Unmap( 0, nullptr );

            // Initialize the vertex buffer view.
            m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
            m_vertexBufferView.StrideInBytes = sizeof( vertices[0] );
            m_vertexBufferView.SizeInBytes = vertexBufferSize;
        }

        // create the index buffer
        {
            const UINT indexBufferSize = UINT( sizeof( indices ) );
            desc.Width = indexBufferSize;

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            THROW_IF_FAILED( m_device->CreateCommittedResource(
                &propsUpload,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS( &m_indexBuffer ) ) );

            // Copy the triangle data to the vertex buffer.
            UINT8* pIndexDataBegin;
            CD3DX12_RANGE readRange( 0, 0 );        // We do not intend to read from this resource on the CPU.
            THROW_IF_FAILED( m_indexBuffer->Map( 0, &readRange, reinterpret_cast< void** >( &pIndexDataBegin ) ) );
            memcpy( pIndexDataBegin, indices, indexBufferSize );
            m_indexBuffer->Unmap( 0, nullptr );

            // Initialize the index buffer view.
            m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
            m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
            m_indexBufferView.SizeInBytes = indexBufferSize;
        }

        // load textures
        {
            fs::path curr = fs::current_path();
            m_textures.resize( _countof(texturePaths) );
            for( auto i = 0; i != _countof( texturePaths ); i++ ) {
                THROW_IF_FAILED1( loadTexture( curr / texturePaths[i], &(m_textures[i]), m_srvHeap.getCPUHandle(i) ), (string("Failed to load texture \"") + texturePaths[i] + "\"").c_str() );
            }
        }
    }

    virtual ~SkyBox() {
        // unmap constant buffer
        m_constantBuffer->Unmap( 0, nullptr );
        m_constants = nullptr;
    }

    virtual bool preRender( Alloc& alloc, XMMATRIX& world, XMMATRIX& projection ) override
    {
        m_constants->vp = XMMatrixMultiply( world, projection );
        return true;
    }

    virtual bool render( Alloc& alloc, std::vector<D3D12_RESOURCE_DESC> const& descRTs ) override
    {
        m_pso.use( m_device, alloc.get(), descRTs );

        // set input descriptors
        alloc->SetDescriptorHeaps(1, &m_srvHeap.p.p );

        // set constant buffer
        alloc->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
        alloc->SetGraphicsRootDescriptorTable(1, m_srvHeap.getGPUHandle() );

        // set input
        alloc->IASetVertexBuffers( 0, 1, &m_vertexBufferView );
        alloc->IASetIndexBuffer( &m_indexBufferView );
        alloc->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        alloc->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0);

        // there is no call we can evaluate a result from, it's all just enqueed in the command list
        return true;
    }

    virtual bool postRender( Alloc& alloc ) override
    {
        return true;
    }
};

class MyFBO : public D3D12RT {
public:
    MyFBO( D3D12RT& parent, UINT nFrames, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM ) : D3D12RT( parent, g_windowWidth, g_windowHeight, nFrames, 1, format, L"D3D12 example fbo") {}
 };

class MyFBOBlit : public D3D12Renderer {
protected:

    static constexpr const char* shaderSource = R"XX(
// Define vertices as constants
static const float4 vertices[4] = {
    float4(-1.0f,  1.0f, 0.0f, 1.0f),   // Top-left
    float4( 1.0f,  1.0f, 0.0f, 1.0f),    // Top-right
    float4(-1.0f, -1.0f, 0.0f, 1.0f),  // Bottom-left
    float4( 1.0f, -1.0f, 0.0f, 1.0f)    // Bottom-right
};
static const float2 uvs[4] = {
    float2( 0.0f, 0.0f ),   // Top-left
    float2( 1.0f, 0.0f ),    // Top-right
    float2( 0.0f, 1.0f ),  // Bottom-left
    float2( 1.0f, 1.0f )    // Bottom-right
};

// Vertex shader input structure
struct VSInput
{
    uint vertexID : SV_VertexID;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoords : TEXCOORD0;
};

// Vertex shader entry point
void VSMain(in VSInput input,
          out VSOutput output )
{
    // Fetch vertex position from constants based on VertexID
    output.position = vertices[input.vertexID];
    output.texCoords = uvs[input.vertexID];
}

Texture2D texIn : register(t0);
SamplerState samplerState : register(s0);

// Pixel shader entry point
float4 PSMain(VSOutput input) : SV_Target
{
    return texIn.Sample(samplerState, input.texCoords);
}
)XX";

    PSO m_pso;
    D3D12RT& m_source;
    D3D12RT& m_target;

    /// we need a map to assign the proper SRV to our pipeline depending on the incoming source texture
    map<ID3D12Resource*,DescriptorHeap> m_srvHeapMap;

    /// create my PSO
    static PSO createPSO( ID3D12Device* pDevice, D3D12_RESOURCE_DESC const& rtDesc ) {
        CComPtr<ID3D12RootSignature> sig;

        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if( FAILED( pDevice->CheckFeatureSupport( D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof( featureData ) ) ) )
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        D3D12_DESCRIPTOR_RANGE1 ranges[]{ {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1} };
        D3D12_ROOT_PARAMETER1 rootParameters[1]{ D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { .DescriptorTable{ _countof(ranges), ranges } }, D3D12_SHADER_VISIBILITY_PIXEL };

        // a sampler description list containig one simple linear sampler
        D3D12_STATIC_SAMPLER_DESC samplers[] =
        {
            { // register( s0 )
                D3D12_FILTER_MIN_MAG_MIP_POINT,
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
        };

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1( _countof( rootParameters ), rootParameters, _countof( samplers ), samplers );

        // create the serialized root signature; this is the shader and it's input/output
        CComPtr<ID3DBlob> signature;
        CComPtr<ID3DBlob> error;
        THROW_IF_FAILED( D3DX12SerializeVersionedRootSignature( &rootSignatureDesc, featureData.HighestVersion, &signature, &error ) );
        THROW_IF_FAILED( pDevice->CreateRootSignature( 0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS( &sig ) ) );

        CComPtr<ID3D12PipelineState> pso;

        // create shaders
        CComPtr<ID3DBlob> vertexShader;
        CComPtr<ID3DBlob> pixelShader;

        #if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        #else
        UINT compileFlags = 0;
        #endif

        CComPtr<ID3DBlob> err;
        THROW_IF_FAILED2( D3DCompile( shaderSource, strlen( shaderSource ), "VertexShader@fboBlit", nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &err ), err );
        if( err ) err.Release();

        THROW_IF_FAILED2( D3DCompile( shaderSource, strlen( shaderSource ), "PixelShader@fboBlit", nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &err ), err );

        // Define the vertex input layout.
        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC( D3D12_DEFAULT );
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.BlendState = CD3DX12_BLEND_DESC( D3D12_DEFAULT );
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        return PSO( pDevice, sig, vertexShader, pixelShader, nullptr, 0, psoDesc, { rtDesc } );
    }

    XMMATRIX m_world;
    XMMATRIX m_projection;

public:
    /// @param parent the target we will blit into
    /// @param source the texture source we will use to fill the target
    MyFBOBlit( D3D12RT& parent, D3D12RT& source ) 
        : D3D12Renderer( parent )
        , m_target( parent )
        , m_source( source )
        , m_pso( createPSO( parent.getDevice(), parent.getDescRT() ) )
        , m_world{}
        , m_projection{}
    {
    }

    virtual bool preRender( Alloc& alloc, DirectX::XMMATRIX& world, DirectX::XMMATRIX& projection ) override {
        //m_world = world;
        //m_projection = projection;
        struct Camera {
            struct Dir { float x, y, z; } dir;
            struct Up { float x, y, z; } up;
            float tanLeft, tanRight, tanBottom, tanTop;
        } cam {
            {-0.809017062f, 0.00000000f, -0.587785244f },
            { 0.00000000f, 1.00000000f, 0.00000000f },
            -0.726542473f,
            0.753554106f,
            -0.600860596f,
            0.324919701f
        };
        XMVECTOR dir = XMVectorSet( -cam.dir.x, -cam.dir.y, -cam.dir.z, 1.0f );
        XMVECTOR up = XMVectorSet( cam.up.x, cam.up.y, cam.up.z, 1.0f );
        auto rot = XMMatrixLookAtLH( {}, dir, up );
        m_world = XMMatrixMultiply( world, rot );
        // then we build the projection matrix according to the given clipping planes, we use our own far and near values though
        const float fnear = 0.125f; // we use a nice power-of-2 value
        const float ffar = 16384.0f; // we use a nice power-of-2 value
        m_projection = XMMatrixPerspectiveOffCenterLH( cam.tanLeft * fnear, cam.tanRight * fnear, cam.tanBottom * fnear, cam.tanTop * fnear, fnear, ffar );
        return true;
    }

    virtual bool render( Alloc& alloc, std::vector<D3D12_RESOURCE_DESC> const& descRTs ) override {
        m_source.setView( m_world, m_projection );

        m_source.preRender();

        m_source.render();

        m_source.postRender();

        // we need to wait for the source to finish the frame, we assume the texture to be already in D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        alloc.waitFor( m_source.getCurrentAlloc() );
        auto& rt = m_source.getCurrentRTs()[0];

        // set up our pipeline
        m_pso.use( m_device, alloc.get(), descRTs );

        // we handle new textures from source pool
        auto entry = m_srvHeapMap.find( rt );
        if( entry == m_srvHeapMap.end() )
        {
            // create the SRV heap
            entry = m_srvHeapMap.emplace( rt, DescriptorHeap( m_device, { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE }, L"Blit" ) ).first;
            const D3D12_SHADER_RESOURCE_VIEW_DESC descSRV{
                descRTs[0].Format,
                D3D12_SRV_DIMENSION_TEXTURE2D,
                D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                {.Texture2D{0,1}}
            };
            m_device->CreateShaderResourceView( rt, &descSRV, entry->second.getCPUHandle() );
        }
        alloc->SetDescriptorHeaps( 1, &entry->second.p.p );
        alloc->SetGraphicsRootDescriptorTable( 0, entry->second.getGPUHandle() );

        // draw; we have our quad statically compiled into the shader, so we don't need to bind buffers
        alloc->IASetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );
        alloc->DrawInstanced( 4, 1, 0, 0 );
        return true;
    }
    virtual bool postRender( Alloc& alloc ) override {
        return true;
    }
};

class MyDPCorrection : public D3D12Renderer {
protected:
    D3D12RT& m_source;
    D3D12RT& m_target;
	std::unique_ptr<VWB> m_ctx;
    string m_channel;
    XMMATRIX m_world;
    XMMATRIX m_projection;
    bool m_turnWithView;

public:
    // initlalize
    MyDPCorrection( D3D12RT& parent, D3D12RT& source, fs::path inifile, string channel, bool turnWithView = true ) 
        : D3D12Renderer( parent )
        , m_target( parent )
        , m_source( source )
        , m_ctx( nullptr )
        , m_channel( channel )
        , m_world{}
        , m_projection{}
		, m_turnWithView( turnWithView )
    {
#ifdef _DEBUG
        {
            if (HMODULE h = GetModuleHandleA("Dxgidebug.dll"))
            {
                if (auto fn = (decltype(DXGIGetDebugInterface)*)GetProcAddress(h, "DXGIGetDebugInterface"))
                {
                    if (IDXGIDebug* dbg; SUCCEEDED(fn(__uuidof(IDXGIDebug), (void**)(IDXGIDebug**)&dbg)))
                    {
                        dbg->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL);
                        dbg->Release();
                    }
                }
            }
        }
#endif //dev _DEBUG
        // initialize VIOSO_API
        auto descRT = parent.getDescRT();
        m_ctx = std::make_unique<VWB>( "", m_queue, inifile, channel.c_str(), 3, "stdout" );
        cout << "VIOSO Warper context created." << endl;
		m_ctx->get().bTurnWithView = m_turnWithView;
        auto res = m_ctx->Init();
        if( VWB_ERROR_NONE != res ) {
            cerr << "Failed to initialize channel " << channel << " with " << descRT.Width << "x" << descRT.Height << "." << endl;
            throw runtime_error( "VIOSO API Init failed." );
        }
        else
            cout << "Channel " << channel << " with " << descRT.Width << "x" << descRT.Height << " initialized." << endl;
    }

    virtual ~MyDPCorrection() {
        if( m_ctx )
            m_ctx.reset();
#ifdef _DEBUG
        {
            if (HMODULE h = GetModuleHandleA("Dxgidebug.dll"))
            {
                if (auto fn = (decltype(DXGIGetDebugInterface)*)GetProcAddress(h, "DXGIGetDebugInterface"))
                {
                    if (IDXGIDebug* dbg; SUCCEEDED(fn(__uuidof(IDXGIDebug), (void**)(IDXGIDebug**)&dbg)))
                    {
                        dbg->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL);
                        dbg->Release();
                    }
                }
            }
        }
#endif //dev _DEBUG
    }

    virtual bool preRender( Alloc& alloc, DirectX::XMMATRIX& world, DirectX::XMMATRIX& projection ) override {
        //XMFLOAT3 eye( 0,1.5,0 );
		XMFLOAT3 eye( g_fPositionX, g_fPositionY + 1.5f, g_fPositionZ );
        XMFLOAT3 rot( DirectX::XMConvertToRadians( g_fPitch ), DirectX::XMConvertToRadians( g_fHeading ), DirectX::XMConvertToRadians( g_fRoll ) );
        XMMATRIX p, v;
        // we ask dpLib for the current eye position, direction and projection matrix   

        // we get the world and projection matrix presented, so we multiply with given from dpLib
        auto res = m_ctx->GetViewProj( &eye.x, &rot.x, &v.r[0].m128_f32[0], &p.r[0].m128_f32[0] );
        //v.r[3].m128_f32[0] += g_fPositionX;
        //v.r[3].m128_f32[1] += g_fPositionY;
        //v.r[3].m128_f32[2] += g_fPositionZ;
		if( !m_ctx->get().bTurnWithView )
            v.r[3].m128_f32[1] += 1.5f;
    #if 0
        if( dpNoError == res )
        {

            XMMATRIX mat, mat2, matY, matP, matR;
            matY = XMMatrixRotationY( XMConvertToRadians( orientation.x ) );
            matP = XMMatrixRotationX( XMConvertToRadians( orientation.y ) );
            matR = XMMatrixRotationZ( XMConvertToRadians( orientation.z ) );
            mat2 = XMMatrixMultiply( matP, matR );
            mat  = XMMatrixMultiply( matY, mat2 );
            m_world = XMMatrixMultiply( world, mat );
            m_projection = XMMATRIX( P.matrix );
        }
    #endif
        if( VWB_ERROR_NONE == res )
        {
            m_world = v;
            m_projection = p;
            //m_projection = XMMatrixSet( 
            //    1.5f, 0.0f,        0, 0,
            //    0.0f, 1.87499988f, 0, 0,
            //    0.0f, 0.0f,        1.00000620f, 1,
            //    0.0f, 0.0f,       -0.125000775, 0 );
            //m_projection = XMMatrixSet( 
            //    1.5f, 0.0f,        0, 0,
            //    0.0f, 1.87499988f, 0, 0,
            //    0.0f, 0.0f,        1.00000620f, -0.125000775,
            //    0.0f, 0.0f,        1, 0 );
            return true;
        }

        else
        {
            cerr << "Error " << res << " in GetViewProj." << endl;
            return false;
        }
    }
    virtual bool render( Alloc& alloc, std::vector<D3D12_RESOURCE_DESC> const& descRTs ) override {
        // we propagate the combined view and projection matrix to the FBO and trigger it's render steps
        m_source.setView( m_world, m_projection );
        m_source.preRender();
        m_source.render();
        m_source.postRender();
        return true;
    }

    virtual bool postRender( Alloc& alloc ) override {
        // we need to wait for the source to finish the frame, we assume the texture to be already in D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        alloc.waitFor( m_source.getCurrentAlloc() );
        auto& texIn = m_source.getCurrentRTs()[0];
        auto& texOut = m_target.getCurrentRTs()[0];
		auto& rtHeap = m_target.getCurrentRTHeap();

        // This draws into it's own command list and executes it, no wait, but the postDraw will signal after execution
        VWB_D3D12_RENDERINPUT ri{
            texIn, 
            texOut,
            rtHeap.getCPUHandle().ptr,
            {},
            alloc.cl
        };
        auto res = m_ctx->Render( &ri );

        // we wait, until the queue is done on GPU, because we need to reset the list
       
        alloc.exec(true,true);

        return true;
    }

};

int main( int argc, char** argv ) {
    D3D12Base::init();
    auto descs = D3D12Base::getGPUDescs( D3D12Base::getFactory() );
    D3D12Base::uninit();

    try {
        for( int i = 1; i < argc; i++ )
        {
            std::string p( argv[i] );
            if( p[0] == '-' )
            {
                std::string param = p.substr( 1, std::string::npos );
                if( param == "x" && i < argc - 1 )
                {
                    g_windowPosX = atoi( argv[i + 1] );
                    i++;
                }
                if( param == "y" && i < argc - 1 )
                {
                    g_windowPosY = atoi( argv[i + 1] );
                    i++;
                }
                if( param == "w" && i < argc - 1 )
                {
                    g_windowWidth = atoi( argv[i + 1] );
                    i++;
                }
                if( param == "h" && i < argc - 1 )
                {
                    g_windowHeight = atoi( argv[i + 1] );
                    i++;
                }
                if( param == "c" && i < argc - 1 )
                {
                    g_channel = argv[i + 1];
                    i++;
                }
                if( param == "v" && i < argc - 1 )
                {
                    g_bDebug = ( bool )atoi( argv[i + 1] );
                    i++;
                }
            }
            else
            {
                g_configfile = argv[i];
            }
        }

        cout << "Begin D3D12 demo.\n\nParameters:\n"
                "  config: " << g_configfile << "\n"
                "   pos x: " << g_windowPosX << "\n"
                "   pos y: " << g_windowPosY << "\n"
                "   width: " << g_windowWidth << "\n"
                "  height: " << g_windowHeight << "\n"
                " channel: " << g_channel << "\n"
            << endl;

        threadBeDPIAware( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
        int ret = 0;
        D3D12Base::init();
        try{
            cout << "Creating output window...";
            MyD3D12Wnd wnd;
            cout << "OK" << endl;
            #if 0   
            wnd.addRenderer( make_unique<SkyBox>( wnd ) );
            #elif 0
            MyFBO fbo( wnd, 2 );
            fbo.addRenderer( make_unique<SkyBox>( fbo ) );
            wnd.addRenderer( make_unique<MyFBOBlit>( wnd, fbo ) );
            #else
            cout << "Creating FBO...";
            MyFBO fbo( wnd, 2 );
            cout << "OK" << endl;
            cout << "Creating SkyBox renderer...";
            fbo.addRenderer( make_unique<SkyBox>( fbo ) );
            cout << "OK" << endl;
            cout << "Creating Correction renderer...";
            wnd.addRenderer( make_unique <MyDPCorrection>( wnd, fbo, g_configfile, g_channel ));
            cout << "OK" << endl;
            #endif
            ret = wnd.loop();
        }
        catch( exception& e )
        {
            // uninitialize
            D3D12Base::uninit();
            // rethrow
            throw e;
        }
        D3D12Base::uninit();
        #ifdef _DEBUG
        {
            if( HMODULE h = GetModuleHandleA( "Dxgidebug.dll" ) )
            {
                if( auto fn = ( decltype( DXGIGetDebugInterface )* )GetProcAddress( h, "DXGIGetDebugInterface" ) )
                {
                    if( IDXGIDebug* dbg; SUCCEEDED( fn( __uuidof( IDXGIDebug ), ( void** )( IDXGIDebug** )&dbg ) ) )
                    {
                        dbg->ReportLiveObjects( DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_DETAIL );
                        dbg->Release();
                    }
                }
            }
        }
        #endif //dev _DEBUG

        return ret;
    }
    catch( exception& e ) {
        cerr << "Exception: " << e.what() << endl;
        return -2;
    }
    catch( ... ) {
        tcerr << _T( "Unhandled Exception." ) << endl;
        return -3;
    }
}

// hide WinMain of other flavour
#ifdef _UNICODE
#define WinMain _WinMain
#else
#define wWinMain _WinMain
#endif

int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int nCmdShow )
{
    // generate input for main()
    std::vector<std::string> sArgs;

    // put own executable first
    std::string sPath( MAX_PATH, 0 );
    DWORD l = GetModuleFileNameA( 0, sPath.data(), MAX_PATH );
    sPath.erase( l ); //trim
    sArgs.emplace_back( sPath );

    bool inQuotes = false;
    char* argBegin = lpCmdLine;
    char* argEnd = argBegin;

    while( *argEnd ) {
        if( *argEnd == '\"' ) {
            inQuotes = !inQuotes;
        }
        else if( *argEnd == ' ' && !inQuotes ) {
            if( argBegin != argEnd ) {
                sArgs.emplace_back( argBegin, argEnd );
                sArgs.back() += '\0';
            }
            argBegin = argEnd + 1;
        }
        ++argEnd;
    }

    if( argBegin != argEnd ) {
        sArgs.emplace_back( argBegin, argEnd );
        sArgs.back() += '\0';
    }

    std::vector<char*> argv;
    for( auto& s : sArgs )
        argv.push_back( s.data() );

    return main( ( int )argv.size(), argv.data() );
}


int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow
)
{
    // generate input for WinMain()
    // we are using filesystem to translate wstring into proper single-byte ansi of the user codepage
    return WinMain( hInstance, hPrevInstance, ( std::filesystem::path( lpCmdLine ).string() + '\0' ).data(), nCmdShow);
}

