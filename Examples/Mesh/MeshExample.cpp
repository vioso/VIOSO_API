#include "../DirectX11/DX11Program.h"

#include "resource.h"
#include <string>
#include <vector>
#include <array>
#include <sstream>
#include <memory>
using namespace std;

////< start VIOSO API code
#include "../../Include/VIOSOWarpBlend.hpp"
LPCTSTR s_configFile = _T( "VIOSOWarpBlend.ini" );
////< end VIOSO API code

class ViosoMeshRenderer : public Renderer
{
    static const char s_szShaderWarp[];
    static const char s_szShaderDisplay[];
    CComPtr< ID3D11VertexShader > m_vs;
    CComPtr< ID3D11PixelShader > m_ps;
    CComPtr< ID3D11InputLayout > m_layout;
    CComPtr< ID3D11Buffer > m_vb;
    CComPtr< ID3D11Buffer > m_ib;
    CComPtr< ID3D11Buffer > m_cb;
    VSConstantBufferA m_vsConstants;
    UINT m_nIndices;
    UINT m_iIndex;
    UINT m_i;

public:
    ViosoMeshRenderer( ID3D11Device* dev, VWB_WarpBlendMesh const& mesh )
    : m_nIndices(0)
    , m_iIndex(0)
    , m_i(0)
    {
        char const* shaderCode = s_szShaderDisplay;
        // Compile the vertex shader
		CComPtr< ID3DBlob > codeBlob;
		CComPtr< ID3DBlob > errBlob;
		HRESULT hr = D3DCompile( shaderCode, strlen( shaderCode ), "mesh vertex shader", NULL, NULL, "VS", "vs_4_0", 0, 0, &codeBlob, &errBlob );
        if( FAILED( hr ) )
            throw exception( ( string( "failed to compile shader" ) + (char*)errBlob->GetBufferPointer() ).c_str() );

        // Create the vertex shader
        hr = dev->CreateVertexShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &m_vs );
        if( FAILED( hr ) )
            throw exception( "failed to create shader" );

        // Create the input layout
        hr = dev->CreateInputLayout( SimpleVertexTex::layout, ARRAYSIZE( SimpleVertexTex::layout ), codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), &m_layout );
        if( FAILED( hr ) )
            throw exception( "failed to create input layout" );

        // Compile the pixel shader
        codeBlob.Release();
        errBlob.Release();
        hr = D3DCompile( shaderCode, strlen( shaderCode ), "mesh pixel shader", NULL, NULL, "PS", "ps_4_0", 0, 0, &codeBlob, &errBlob );
        if( FAILED( hr ) )
            throw exception( ( string( "failed to compile shader" ) + (char*)errBlob->GetBufferPointer() ).c_str() );

        // Create the pixel shader
        hr = dev->CreatePixelShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &m_ps );
        if( FAILED( hr ) )
            throw exception( "failed to create pixel shader" );
        codeBlob.Release();
        errBlob.Release();

        // Create vertex buffer
        std::vector<SimpleVertexTex> vertices;

        vertices.reserve( mesh.nVtx );
        for( VWB_WarpBlendVertex const* w = mesh.vtx, *wE = w + mesh.nVtx; w != wE; w++ )
        {
            vertices.emplace_back( SimpleVertexTex{
                { w->pos[0], w->pos[1], w->pos[2] },
                { w->rgb[0], w->rgb[1], w->rgb[2], 1 },
                { w->uv[0], w->uv[1] }
            } );
        }

        std::vector<WORD> indices;
        indices.reserve( mesh.nIdx );
        for( UINT i = 0; i != mesh.nIdx; i++ )
            indices.emplace_back( (WORD)mesh.idx[i] );

        m_nIndices = (UINT)indices.size();

        D3D11_BUFFER_DESC bd;
        ZeroMemory( &bd, sizeof( bd ) );
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = SimpleVertexTex::stride * (UINT)vertices.size();
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA InitData;
        ZeroMemory( &InitData, sizeof( InitData ) );
        InitData.pSysMem = vertices.data();
        hr = dev->CreateBuffer( &bd, &InitData, &m_vb );
        if( FAILED( hr ) )
            throw exception( "failed to create vertex buffer" );

        bd.ByteWidth = sizeof( WORD ) * m_nIndices;
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        InitData.pSysMem = indices.data();
        hr = dev->CreateBuffer( &bd, &InitData, &m_ib );
        if( FAILED( hr ) )
            throw exception( "failed to create index buffer" );

        // Create the constant buffer
        bd.ByteWidth = sizeof( VSConstantBufferA );
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        hr = dev->CreateBuffer( &bd, NULL, &m_cb );
        if( FAILED( hr ) )
            throw exception( "failed to create constant buffer" );

        ZeroMemory( &m_vsConstants, sizeof( m_vsConstants ) );
    }

    virtual void render( ID3D11DeviceContext* ctx, XMMATRIX const& world, XMMATRIX const& view, XMMATRIX const& projection )
    {
        // Set the input layout
        ctx->IASetInputLayout( m_layout );

        // Set vertex buffer
        UINT offset = 0;
        ctx->IASetVertexBuffers( 0, 1, &m_vb.p, &SimpleVertexTex::stride, &offset );

        // Set index buffer
        ctx->IASetIndexBuffer( m_ib, DXGI_FORMAT_R16_UINT, 0 );

        // Set primitive topology
        ctx->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

        // update and set constant buffer
        m_vsConstants.mWorld = XMMatrixTranspose( world );
        m_vsConstants.mView = XMMatrixTranspose( view );
        m_vsConstants.mProjection = XMMatrixTranspose( projection );
        ctx->UpdateSubresource( m_cb, 0, NULL, &m_vsConstants, 0, 0 );
        ctx->VSSetConstantBuffers( 0, 1, &m_cb.p );

        // set shader
        ctx->VSSetShader( m_vs, NULL, 0 );
        ctx->PSSetShader( m_ps, NULL, 0 );

        // draw
        ctx->DrawIndexed( m_nIndices, 0, 0 );
    }
};

const char ViosoMeshRenderer::s_szShaderWarp[] = R"END(
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
}

//-------------------------------------------------------------
struct VS_INPUT
{
    float3 Pos : POSITION;
    float4 Col : COLOR;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Col : COLOR;
    float2 Tex : TEXCOORD0;
};

//-------------------------------------------------------------
// Vertex Shader
//-------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    // position calculates from the projector pixel
    //output.Pos.xy = input.Tex.xy;
    output.Pos.x = 2 * input.Tex.x - 1;
    output.Pos.y = -2 * input.Tex.y + 1;
    output.Pos.z = 1;
    output.Pos.w = 1;
    float4 p = mul( float4( input.Pos, 1 ), World );
    p = mul( p, View );
    p = mul( p, Projection );
    output.Tex.x = -0.5 + p.x / p.w /  2;
    output.Tex.y =  0.5 + p.y / p.w / -2;
    output.Col = input.Col;
    return output;
}

//-------------------------------------------------------------
// Pixel Shader
//-------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
    // use input.Tex to sample from content, generated with the same matrices
    return float4( input.Col.xy * input.Tex.xy, input.Col.z, 1 );
}
)END";

const char ViosoMeshRenderer::s_szShaderDisplay[] = R"END(
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
}

//-------------------------------------------------------------
struct VS_INPUT
{
    float3 Pos : POSITION;
    float4 Col : COLOR;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Col : COLOR;
    float2 Tex : TEXCOORD0;
};

//-------------------------------------------------------------
// Vertex Shader
//-------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul( float4( input.Pos, 1 ), World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Tex = input.Tex;
    output.Col = input.Col;
    return output;
}

//-------------------------------------------------------------
// Pixel Shader
//-------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
    // use input.Tex to sample from some projector test image
    return float4( input.Col.xy * input.Tex.xy, input.Col.z, 1 );
}
)END";

class VIOSOMeshWindow : public OutputWindow {
    __declspec( align( 4 ) ) struct Clip {
        float l, t, r, b, n, f;
        operator float* ( ) { return &l; }
        operator float const* ( ) const { return &l; }
    } m_clip;
public:
    VIOSOMeshWindow( LPCTSTR channelName, HINSTANCE hInstance, int x, int y, int width, int height, int nCmdShow, DXGI_SWAP_EFFECT effect, int bufferCount, int createDeviceFlags, bool withDepth )
        : OutputWindow( hInstance, channelName, x, y, width, height, nCmdShow, effect, bufferCount, createDeviceFlags, withDepth )
    {

        if( 1 )
        {
            // use wireframe instead of solid
            m_rs.Release();
            m_dev->CreateRasterizerState( &s_rasterDescWire, &m_rs );
        }
        if( 0 )
        {
            // use sky color to clear
            m_rt->setClearColor( RenderTarget::s_sky );
        }

        ////< start VIOSO API code
        unique_ptr<VWB> w;
        try {
            // create warper, it's contructor throws
            w = make_unique<VWB>( nullptr, VWB_DUMMYDEVICE, s_configFile, channelName );
        }
        catch( VWB_ERROR const& err )
        {
            throw exception( "failed to create warper", int(err) );
        }
        // initialize
        if( VWB_ERROR_NONE != w->Init() )
            throw exception( "failed to initialize warper" );

        VWB_WarpBlendMesh m{};
        w->GetWarpBlendMesh( 33, 17, m );
        addRenderer( make_shared< ViosoMeshRenderer >( m_dev, m ) );
        w->DestroyWarpBlendMesh( m );
        float eye[3]{};
        w->GetViewClip( eye, eye, (float*)&m_mView.r, m_clip );
        ////< end VIOSO API code
    }

    virtual void preRender()
    {
        m_mProjection = XMMatrixPerspectiveOffCenterLH( -m_clip[0], m_clip[2], -m_clip[3], m_clip[1], m_clip[4], m_clip[5] );
        __super::preRender();

        //// do not process keys
        //GFXPipeline::preRender();
    }
};

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = NULL;
vector<shared_ptr<OutputWindow>>    g_windows;

std::string utf16ToUtf8( const std::wstring& in )
{
    std::string out( "\0", WideCharToMultiByte(CP_UTF8, 0, in.data(), (int)in.size(), nullptr, 0, nullptr, nullptr) );
    WideCharToMultiByte( CP_UTF8, 0, in.data(), (int)in.size(), out.data(), (int)out.size(), nullptr, nullptr );
    return out;
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    try
    {
        UNREFERENCED_PARAMETER( hPrevInstance );

        // read command line, if any
        std::wistringstream cmd( lpCmdLine );
        std::wstring channel = L"IGX";
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        cmd >> channel >> x >> y >> w >> h;

        UINT createDeviceFlags = 0;
    #ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

        g_windows.push_back( make_shared<VIOSOMeshWindow>( utf16ToUtf8(channel).c_str(), hInstance, x, y, w, h, nCmdShow, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );

        // Main message loop
        MSG msg = { 0 };
        while( WM_QUIT != msg.message )
        {
            if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
            else
            {
                for( auto& wnd : g_windows )
                {
                    wnd->preRender();
                    wnd->render();
                    wnd->postRender();
                }
            }
        }

        return (int)msg.wParam;
    }
    catch( exception& e )
    {
        OutputDebugStringA( e.what() );
        OutputDebugStringA( "\n" );
        return -1;
    }
}