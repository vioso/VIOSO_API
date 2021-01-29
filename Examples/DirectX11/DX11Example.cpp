#include "DX11Program.h"

#include "resource.h"
#include <string>
#include <vector>
using namespace std;

////< start VIOSO API code
#define VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT
#include "../../Include/VIOSOWarpBlend.h"
LPCTSTR s_configFile = _T( "VIOSOWarpBlend.ini" );
////< end VIOSO API code

class CubesRenderer : public Renderer
{
    static const int s_nCubes = 9;
    static const int s_nnCubes = 2 * s_nCubes * s_nCubes + 2 * s_nCubes * ( s_nCubes - 2 ) + 2 * ( s_nCubes - 2 ) * ( s_nCubes - 2 );
    static const char s_szShader[];
    CComPtr< ID3D11VertexShader > m_vs;
    CComPtr< ID3D11PixelShader > m_ps;
    CComPtr< ID3D11InputLayout > m_layout;
    CComPtr< ID3D11Buffer > m_vb;
    CComPtr< ID3D11Buffer > m_ib;
    CComPtr< ID3D11Buffer > m_cb;
    CComPtr< ID3D11RasterizerState > m_rs;
    VSConstantBuffer m_vsConstants;

public:
    CubesRenderer( ID3D11Device* dev )
    {
        // Compile the vertex shader
		CComPtr< ID3DBlob > codeBlob;
		CComPtr< ID3DBlob > errBlob;
		HRESULT hr = D3DCompile( s_szShader, strlen( s_szShader ), "cubes vertex shader", NULL, NULL, "VS", "vs_4_0", 0, 0, &codeBlob, &errBlob );
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
        hr = D3DCompile( s_szShader, strlen( s_szShader ), "cubes pixel shader", NULL, NULL, "PS", "ps_4_0", 0, 0, &codeBlob, &errBlob );
        if( FAILED( hr ) )
            throw exception( ( string( "failed to compile shader" ) + (char*)errBlob->GetBufferPointer() ).c_str() );

        // Create the pixel shader
        hr = dev->CreatePixelShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &m_ps );
        if( FAILED( hr ) )
            throw exception( "failed to create pixel shader" );
        codeBlob.Release();
        errBlob.Release();

        // Create vertex buffer
        int n = s_nnCubes;
        SimpleVertex* vertices = new SimpleVertex[n * 8];
        WORD* indices = new WORD[n * 36];
        float sz = 10.0f / ( 3 * s_nCubes - 1 );
        int gg = s_nCubes / 2;
        SimpleVertex* pv = vertices;
        WORD* pi = indices;
        WORD ioffs = 0;
        for( int z = 0; z != s_nCubes; z++ )
        {
            float fz = ( z - gg ) * 3.0f * sz;
            for( int y = 0; y != s_nCubes; y++ )
            {
                float fy = ( y - gg ) * 3.0f * sz;
                for( int x = 0; x != s_nCubes; x++ )
                {
                    float fx = ( x - gg ) * 3.0f * sz;
                    if( ( 0 == x ) || ( 2 * gg == x ) || ( 0 == y ) || ( 2 * gg == y ) || ( 0 == z ) || ( 2 * gg == z ) )
                    {
                        SimpleVertex v[8] = {
                            { XMFLOAT3( fx + sz, fy - sz, fz + sz ), XMFLOAT4( float( x ) / ( s_nCubes - 1 ), float( y ) / ( s_nCubes - 1 ), float( z ) / ( s_nCubes - 1 ), 1.0f ) },
                            { XMFLOAT3( fx + sz, fy + sz, fz + sz ), XMFLOAT4( float( x ) / ( s_nCubes - 1 ), float( y ) / ( s_nCubes - 1 ), float( z ) / ( s_nCubes - 1 ), 1.0f ) },
                            { XMFLOAT3( fx - sz, fy + sz, fz + sz ), XMFLOAT4( float( x ) / ( s_nCubes - 1 ), float( y ) / ( s_nCubes - 1 ), float( z ) / ( s_nCubes - 1 ), 1.0f ) },
                            { XMFLOAT3( fx - sz, fy - sz, fz + sz ), XMFLOAT4( float( x ) / ( s_nCubes - 1 ), float( y ) / ( s_nCubes - 1 ), float( z ) / ( s_nCubes - 1 ), 1.0f ) },
                            { XMFLOAT3( fx + sz, fy - sz, fz - sz ), XMFLOAT4( float( x ) / ( s_nCubes - 1 ), float( y ) / ( s_nCubes - 1 ), float( z ) / ( s_nCubes - 1 ), 1.0f ) },
                            { XMFLOAT3( fx + sz, fy + sz, fz - sz ), XMFLOAT4( float( x ) / ( s_nCubes - 1 ), float( y ) / ( s_nCubes - 1 ), float( z ) / ( s_nCubes - 1 ), 1.0f ) },
                            { XMFLOAT3( fx - sz, fy + sz, fz - sz ), XMFLOAT4( float( x ) / ( s_nCubes - 1 ), float( y ) / ( s_nCubes - 1 ), float( z ) / ( s_nCubes - 1 ), 1.0f ) },
                            { XMFLOAT3( fx - sz, fy - sz, fz - sz ), XMFLOAT4( float( x ) / ( s_nCubes - 1 ), float( y ) / ( s_nCubes - 1 ), float( z ) / ( s_nCubes - 1 ), 1.0f ) },
                        };
                        for( int i = 0; i != 8; i++ )
                            *( pv++ ) = v[i];

                        WORD in[36] =
                        {
                            3,1,0,
                            2,1,3,

                            0,5,4,
                            1,5,0,

                            3,4,7,
                            0,4,3,

                            1,6,5,
                            2,6,1,

                            2,7,6,
                            3,7,2,

                            6,4,5,
                            7,4,6
                        };

                        for( int i = 0; i != 36; i++ )
                            *( pi++ ) = in[i] + ioffs;
                        ioffs += 8;
                    }
                }
            }
        }

        D3D11_BUFFER_DESC bd;
        ZeroMemory( &bd, sizeof( bd ) );
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof( SimpleVertex ) * n * 8;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA InitData;
        ZeroMemory( &InitData, sizeof( InitData ) );
        InitData.pSysMem = vertices;
        hr = dev->CreateBuffer( &bd, &InitData, &m_vb );
        if( FAILED( hr ) )
            throw exception( "failed to create vertex buffer" );


        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof( WORD ) * n * 36;        // 36 vertices needed for 12 triangles in a triangle list
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bd.CPUAccessFlags = 0;
        InitData.pSysMem = indices;
        hr = dev->CreateBuffer( &bd, &InitData, &m_ib );
        if( FAILED( hr ) )
            throw exception( "failed to create index buffer" );

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
        ctx->DrawIndexed( s_nnCubes * 36, 0, 0 );
    }
};

const char CubesRenderer::s_szShader[] = R"END(
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
    return input.Color;
}
)END";

class MyRenderToTexture : public RenderToTexture
{
public:
    MyRenderToTexture( ID3D11Device* dev, ID3D11DeviceContext* ic, int width, int height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, bool withDepth = false )
        : RenderToTexture( dev, ic, width, height, format, withDepth ) {}

    virtual void preRender()
    {
        typedef FLOAT Color[4];
        m_rt->setAsRTto( m_ic );
        m_rt->clear( m_ic, RenderTarget::s_sky );
        m_ic->RSSetViewports( 1, &m_vp );
    }
};

class VIOSOWarperWindow : public OutputWindow {
    VWB_Warper* m_warper;
    shared_ptr<MyRenderToTexture> m_rtt;
public:
    VIOSOWarperWindow( LPCSTR channelName, HINSTANCE hInstance, int x, int y, int width, int height, int nCmdShow, DXGI_SWAP_EFFECT effect, int bufferCount, int createDeviceFlags, bool withDepth )
    : OutputWindow( hInstance, x, y, width, height, nCmdShow, effect, bufferCount, createDeviceFlags, withDepth )
    , m_warper( nullptr )
    {

        ////< start VIOSO API code
        // this will initialize function pointers from dll
        #define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
        #include "../../Include/VIOSOWarpBlend.h"
        
        // check all needed functions
        if( !( VWB_Create && VWB_Init && VWB_render && VWB_getViewProj && VWB_Destroy ) )
            throw exception( "failed to load VIOSO API dll" );

        // create and initialize
        if( 
        	VWB_ERROR_NONE != VWB_Create( m_dev, s_configFile, channelName, &m_warper, 0, NULL ) ||
        	VWB_ERROR_NONE != VWB_Init( m_warper )
        )
        	throw exception( "failed to initialize warper" );
        ////< end VIOSO API code

        m_rtt = make_shared< MyRenderToTexture >( m_dev, m_ic, (int)m_vp.Width, (int)m_vp.Height, DXGI_FORMAT_R8G8B8A8_UNORM, true );
    }

    virtual void addRenderer( std::shared_ptr< Renderer > renderer )
    {
        m_rtt->addRenderer( renderer );
    }

    virtual void preRender()
    {
        ////< start VIOSO API code
        XMFLOAT3 vEyePt( 0.0, 0.0f, 0.0f );
        XMFLOAT3 vRot( 0.0f, 0.0f, 0.0f );
   	    VWB_getViewProj( m_warper, &vEyePt.x, &vRot.x, (float*)m_mView.r, (float*)m_mProjection.r );
        ////< end VIOSO API code

        m_rtt->setMProjection( m_mProjection );
        m_rtt->setMProjection( m_mView );

        m_rtt->preRender();
    }

    virtual void render( XMMATRIX const& world )
    {
        m_rtt->render( world );
    }

    virtual void postRender()
    {
        m_rt->setAsRTto( m_ic );
        m_rt->clear( m_ic, RenderTarget::s_black );
        m_ic->RSSetViewports( 1, &m_vp );
        ////< start VIOSO API code
        // this applies warp
		VWB_render( m_warper, dynamic_cast<RenderTexture*>(m_rtt->getRenderTarget().get())->getResource(), VWB_STATEMASK_STANDARD );
        ////< end VIOSO API code
        __super::postRender();
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

        g_windows.push_back( make_shared<VIOSOWarperWindow>( "IGX", hInstance, 0, 0, 0, 0, nCmdShow, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
        g_windows.back()->addRenderer( make_shared< CubesRenderer >( g_windows.front()->getDevice() ) );

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