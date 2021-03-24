#include "../DirectX11/DX11Program.h"

#include "resource.h"
#include <string>
#include <vector>
#include <sstream>
using namespace std;

////< start VIOSO API code
#define VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT
#include "../../Include/VIOSOWarpBlend.h"
LPCTSTR s_configFile = _T( "VIOSOWarpBlend.ini" );
////< end VIOSO API code

class ViosoMeshRenderer : public Renderer
{
    static const char s_szShader[];
    CComPtr< ID3D11VertexShader > m_vs;
    CComPtr< ID3D11PixelShader > m_ps;
    CComPtr< ID3D11InputLayout > m_layout;
    CComPtr< ID3D11Buffer > m_vb;
    CComPtr< ID3D11Buffer > m_ib;
    CComPtr< ID3D11Buffer > m_cb;
    CComPtr< ID3D11RasterizerState > m_rs;
    VSConstantBuffer m_vsConstants;
    UINT m_nIndices;
    UINT m_iIndex;
    UINT m_i;

public:
    ViosoMeshRenderer( ID3D11Device* dev, VWB_WarpBlendMesh const& mesh )
    : m_nIndices(0)
    , m_iIndex(0)
    , m_i(0)
    {
        // Compile the vertex shader
		CComPtr< ID3DBlob > codeBlob;
		CComPtr< ID3DBlob > errBlob;
		HRESULT hr = D3DCompile( s_szShader, strlen( s_szShader ), "mesh vertex shader", NULL, NULL, "VS", "vs_4_0", 0, 0, &codeBlob, &errBlob );
        if( FAILED( hr ) )
            throw exception( ( string( "failed to compile shader" ) + (char*)errBlob->GetBufferPointer() ).c_str() );

        // Create the vertex shader
        hr = dev->CreateVertexShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &m_vs );
        if( FAILED( hr ) )
            throw exception( "failed to create shader" );

        // Define the input layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };
        UINT numElements = ARRAYSIZE( layout );

        // Create the input layout
        hr = dev->CreateInputLayout( layout, numElements, codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), &m_layout );
        if( FAILED( hr ) )
            throw exception( "failed to create input layout" );

        // Compile the pixel shader
        codeBlob.Release();
        errBlob.Release();
        hr = D3DCompile( s_szShader, strlen( s_szShader ), "mesh pixel shader", NULL, NULL, "PS", "ps_4_0", 0, 0, &codeBlob, &errBlob );
        if( FAILED( hr ) )
            throw exception( ( string( "failed to compile shader" ) + (char*)errBlob->GetBufferPointer() ).c_str() );

        // Create the pixel shader
        hr = dev->CreatePixelShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &m_ps );
        if( FAILED( hr ) )
            throw exception( "failed to create pixel shader" );
        codeBlob.Release();
        errBlob.Release();

        // Create vertex buffer
        SimpleVertex* vertices = new SimpleVertex[mesh.nVtx];
        VWB_WarpBlendVertex const* w = mesh.vtx;
        for( SimpleVertex* v = vertices, *vE = vertices + mesh.nVtx; v != vE; v++, w++ )
        {
            v->Pos.x = 10.0f * w->pos[0] - 5.0f;
            v->Pos.y = 10.0f * w->pos[1] - 5.0f;
            v->Pos.z = 10;
            v->Color.x = w->uv[0];
            v->Color.y = w->uv[1];
            v->Color.z = 0;
            v->Color.w = 0.3333f*(w->rgb[0] + w->rgb[1] + w->rgb[2]);
       }
        m_nIndices = mesh.nIdx;
        WORD* indices = new WORD[mesh.nIdx];
        for( UINT i = 0; i != mesh.nIdx; i++ )
            indices[i] = (WORD)mesh.idx[i];

        D3D11_BUFFER_DESC bd;
        ZeroMemory( &bd, sizeof( bd ) );
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof( SimpleVertex ) * mesh.nVtx;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA InitData;
        ZeroMemory( &InitData, sizeof( InitData ) );
        InitData.pSysMem = vertices;
        hr = dev->CreateBuffer( &bd, &InitData, &m_vb );
        if( FAILED( hr ) )
            throw exception( "failed to create vertex buffer" );
        delete[] vertices;

        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof( WORD ) * mesh.nIdx;        // 36 vertices needed for 12 triangles in a triangle list
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bd.CPUAccessFlags = 0;
        InitData.pSysMem = indices;
        hr = dev->CreateBuffer( &bd, &InitData, &m_ib );
        if( FAILED( hr ) )
            throw exception( "failed to create index buffer" );
        delete[] indices;

        // Create the constant buffer
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof( VSConstantBuffer );
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer( &bd, NULL, &m_cb );
        if( FAILED( hr ) )
            throw exception( "failed to create constant buffer" );

        // Turn off culling, so we see the front and back of the triangle
        D3D11_RASTERIZER_DESC rasterDesc;
        rasterDesc.AntialiasedLineEnable = false;
        rasterDesc.CullMode = D3D11_CULL_NONE;
        rasterDesc.DepthBias = 0;
        rasterDesc.DepthBiasClamp = 0.0f;
        rasterDesc.DepthClipEnable = true;
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.FrontCounterClockwise = true;
        rasterDesc.MultisampleEnable = false;
        rasterDesc.ScissorEnable = false;
        rasterDesc.SlopeScaledDepthBias = 0.0f;
        if( FAILED( dev->CreateRasterizerState( &rasterDesc, &m_rs ) ) )
            throw exception( "failed to create rasterizer state" );

        ZeroMemory( &m_vsConstants, sizeof( m_vsConstants ) );
    }

    virtual void render( ID3D11DeviceContext* ctx, XMMATRIX const& world, XMMATRIX const& view, XMMATRIX const& projection )
    {

        // Set the input layout
        ctx->IASetInputLayout( m_layout );

        // Set vertex buffer
        UINT stride = sizeof( SimpleVertex );
        UINT offset = 0;
        ctx->IASetVertexBuffers( 0, 1, &m_vb.p, &stride, &offset );

        // Set index buffer
        ctx->IASetIndexBuffer( m_ib, DXGI_FORMAT_R16_UINT, 0 );

        // Set primitive topology
        ctx->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

        // set raster state
        ctx->RSSetState( m_rs );

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

        m_i += 3;
        if( m_i == m_nIndices )
            m_i = 0;
    }
};

const char ViosoMeshRenderer::s_szShader[] = R"END(
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
}

//-------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

//-------------------------------------------------------------
// Vertex Shader
//-------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Color = input.Color;

    return output;
}

//-------------------------------------------------------------
// Pixel Shader
//-------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
    return float4( input.Color.xyz * input.Color.w, 1 );
}
)END";


class VIOSOMeshWindow : public OutputWindow {
public:
    VIOSOMeshWindow( LPCTSTR channelName, HINSTANCE hInstance, int x, int y, int width, int height, int nCmdShow, DXGI_SWAP_EFFECT effect, int bufferCount, int createDeviceFlags, bool withDepth )
        : OutputWindow( hInstance, channelName, x, y, width, height, nCmdShow, effect, bufferCount, createDeviceFlags, withDepth )
    {

        ////< start VIOSO API code
        // this will initialize function pointers from dll
        #define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
        #include "../../Include/VIOSOWarpBlend.h"

        // check all needed functions
        if( !( VWB_Create && VWB_Init && VWB_render && VWB_getViewProj && VWB_Destroy ) )
            throw exception( "failed to load VIOSO API dll" );

        VWB_Warper* w;
        // create and initialize
        if(
            VWB_ERROR_NONE != VWB_Create( VWB_DUMMYDEVICE, s_configFile, channelName, &w, 0, NULL ) ||
            VWB_ERROR_NONE != VWB_Init( w )
            )
            throw exception( "failed to initialize warper" );

        VWB_WarpBlendMesh m = { 0 };
        VWB_getWarpBlendMesh( w, 129, 129, m );
        addRenderer( make_shared< ViosoMeshRenderer >( m_dev, m ) );
        ////< end VIOSO API code


    }
};

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = NULL;
vector<shared_ptr<OutputWindow>>    g_windows;
XMMATRIX g_mWorld;

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    try
    {
        UNREFERENCED_PARAMETER( hPrevInstance );
        UNREFERENCED_PARAMETER( lpCmdLine );

        UINT createDeviceFlags = 0;
    #ifdef _DEBUG
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

        g_windows.push_back( make_shared<VIOSOMeshWindow>( "IGX", hInstance, 0, 0, 0, 0, nCmdShow, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );

        // initialize world matrix
        g_mWorld = XMMatrixIdentity();

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
                    wnd->render( g_mWorld );
                    wnd->postRender();
                }
            }
        }

        return (int)msg.wParam;
    }
    catch( exception& e )
    {
        UNREFERENCED_PARAMETER( e );
        return -1;
    }
}