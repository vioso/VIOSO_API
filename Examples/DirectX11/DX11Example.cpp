#include "DX11Program.h"

#include "resource.h"
#include <vector>
#include <atlstr.h>
#include <string>
#include <sstream>
using namespace std;

class CubesRenderer : public Renderer
{
    static const int s_nCubes = 9;
    static const int s_nnCubes = 2 * s_nCubes * s_nCubes + 2 * s_nCubes * ( s_nCubes - 2 ) + 2 * ( s_nCubes - 2 ) * ( s_nCubes - 2 );
    static const char s_szShader[];
    CComPtr< ID3D11VertexShader > m_vs;
    CComPtr< ID3D11PixelShader > m_ps;
    CComPtr< ID3D11InputLayout > m_layout;
    CComPtr< ID3D11Buffer > m_vb;
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

        // render some cubes around 0,0,0
        static const float sz = 10.0f / ( 3 * s_nCubes - 1 );
        static const int gg = s_nCubes / 2;
        std::vector<SimpleVertex> vertices;
        vertices.reserve( s_nnCubes * 36 );

        // cube corners calculated from cube center
        //     E/-----/|F   
        //     /  6  / |      ^y / z
        //   A|-----|B2|      | /
        //   5|  3  | / G     |---->x
        //    |-----|/          
        //   D   1   C
        static const float corners[8][3] =
        {
            { -sz,  sz, -sz }, //A 0
            {  sz,  sz, -sz	}, //B 1
            {  sz, -sz, -sz	}, //C 2
            { -sz, -sz, -sz	}, //D 3
            { -sz,  sz,  sz	}, //E 4
            {  sz,  sz,  sz	}, //F 5
            {  sz, -sz,  sz	}, //G 6
            { -sz, -sz,  sz	}  //H 7
        };

        // faces
        //  1: quad DCGH tri DCG DGH
        //  2: quad BFGC tri BFG BGC
        //  3: quad ABCD tri ABC ACD
        //  4: quad FEHG tri FEH FHG
        //  5: quad EADH tri EAD EDH
        //  6: quad AEFB tri AEF AFB
        static const int faces[6][4] = {
            { 3, 2, 6, 7 }, // face "1"
            { 1, 5, 6, 2 }, // face "2"
            { 0, 1, 2, 3 }, // face "3"
            { 5, 4, 7, 6 }, // face "4"
            { 4, 0, 3, 7 }, // face "5"
            { 0, 4, 5, 2 }  // face "6"
        };

        // faces for uv's, faces are packed into a rectangular image:
        //  1 2 3       0.0,0.0 - 0.3333,0.5 ; 0.3333,0.0 - 0.6666,0.5 ; 0.66666,0.0 - 1.0,0.5
        //  4 5 6  ==>  0.5,0.0 - 0.3333,1.0 ; 0.3333,0.5 - 0.6666,1.0 ; 0.66666,0.5 - 1.0,1.0
        static const float uv[6][4][2] = {
            { { 0.0f, 0.0f }, { 0.33333f, 0.0f }, { 0.3333f, 0.5f }, { 0.0f, 0.5f } },
            { { 0.33333f, 0.0f }, { 0.66666f, 0.0f }, { 0.66666f, 0.5f }, { 0.333333f, 0.5f } },
            { { 0.66666f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.5f }, { 0.66666f, 0.5f } },
            { { 0.0f, 0.5f }, { 0.33333f, 0.5f }, { 0.3333f, 1.0f }, { 0.0f, 1.0f } },
            { { 0.33333f, 0.5f }, { 0.66666f, 0.5f }, { 0.66666f, 1.0f }, { 0.333333f, 1.0f } },
            { { 0.66666f, 0.5f }, { 1.0f, 0.5f }, { 1.0f, 1.0f }, { 0.66666f, 1.0f } }
        };

        for( int z = 0; z != s_nCubes; z++ )
            //int z = 0;
        {
            float fz = ( z - gg ) * 3.0f * sz;
            for( int y = 0; y != s_nCubes; y++ )
                //int y = 0;
            {
                float fy = ( y - gg ) * 3.0f * sz;
                for( int x = 0; x != s_nCubes; x++ )
                    //int x = 0;
                {
                    float fx = ( x - gg ) * 3.0f * sz;
                    if( ( 0 == x ) || ( 2 * gg == x ) || ( 0 == y ) || ( 2 * gg == y ) || ( 0 == z ) || ( 2 * gg == z ) )
                    {
                        const float col[3] = { float( x ) / ( s_nCubes - 1 ), float( y ) / ( s_nCubes - 1 ), float( z ) /  ( s_nCubes - 1 ) };
                        for( int i = 0; i != ARRAYSIZE( uv ); i++ )
                        {
                            for( int j = 0; j != 3; j++ )
                            {
                                vertices.push_back( SimpleVertex{ {fx + corners[faces[i][j]][0], fy + corners[faces[i][j]][1], fz + corners[faces[i][j]][2]}, {col[0], col[1], col[2], 1.0f}/*, {uv[i][j][0], uv[i][j][1] }*/ } );
                            }
                            vertices.push_back( SimpleVertex{ {fx + corners[faces[i][0]][0], fy + corners[faces[i][0]][1], fz + corners[faces[i][0]][2] }, {col[0], col[1], col[2], 1.0f}/*, {uv[i][0][0], uv[i][0][1] }*/ } );
                            for( int j = 2; j != 4; j++ )
                            {
                                vertices.push_back( SimpleVertex{ {fx + corners[faces[i][j]][0], fy + corners[faces[i][j]][1], fz + corners[faces[i][j]][2] }, {col[0], col[1], col[2], 1.0f} /*, {uv[i][j][0], uv[i][j][1] }*/ } );
                            }
                            //break;
                        }
                    }
                }
            }
        }

        D3D11_BUFFER_DESC bd;
        ZeroMemory( &bd, sizeof( bd ) );
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof( SimpleVertex ) * s_nnCubes * 36;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA InitData;
        ZeroMemory( &InitData, sizeof( InitData ) );
        InitData.pSysMem = &vertices.front();
        hr = dev->CreateBuffer( &bd, &InitData, &m_vb );
        if( FAILED( hr ) )
            throw exception( "failed to create vertex buffer" );

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
        ctx->Draw( s_nnCubes * 36, 0 );
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

////< start VIOSO API code
#define VIOSOWARPBLEND_DYNAMIC_DEFINE_IMPLEMENT
#include "../../Include/VIOSOWarpBlend.h"
LPCTSTR s_configFile = _T( "VIOSOWarpBlend.ini" );

class VIOSOWarperWindow : public OutputWindow {
    VWB_Warper* m_warper;
    shared_ptr<MyRenderToTexture> m_rtt;
public:
    VIOSOWarperWindow( LPCTSTR channelName, HINSTANCE hInstance, int x, int y, int width, int height, int nCmdShow, DXGI_SWAP_EFFECT effect, int bufferCount, int createDeviceFlags, bool withDepth )
    : OutputWindow( hInstance, channelName, x, y, width, height, nCmdShow, effect, bufferCount, createDeviceFlags, withDepth )
    , m_warper( nullptr )
    {

        ////< start VIOSO API code
        if( !( VWB_Create && VWB_Init && VWB_render && VWB_getViewProj && VWB_Destroy ) )
        {
            // this will initialize function pointers from dll
            // only needed, if first time
            #define VIOSOWARPBLEND_DYNAMIC_INITIALIZE
            #include "../../Include/VIOSOWarpBlend.h"
        }
        
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

    ~VIOSOWarperWindow()
    {
        if( m_warper )
            VWB_Destroy( m_warper );
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

        // we're switching methods each call, to show they are equivalent
        // VWB_getPosDir can yield a symmetric frustum. Image quality suffers, if view angle is
        // far off from perpendicular to the screen. Try using asymetric frustum, especially in 
        // dynamic eye-point scenarios. If a symmetric frustum is requested.
        XMMATRIX mv1, mv2, mv3, mp1, mp2, mp3;
        static unsigned int pass = 2;
        if( 3 == ++pass )
        {
            pass = 0;
            // get view and projection matrix directly
            VWB_getViewProj( m_warper, &vEyePt.x, &vRot.x, (float*)mv1.r, (float*)mp1.r );
            m_mView = mv1;
            m_mProjection = mp1;
        }
        else if( 1 == pass )
        {
            float clip[6];
            VWB_getViewClip( m_warper, &vEyePt.x, &vRot.x, (float*)mv2.r, clip );
            mp2 = XMMatrixPerspectiveOffCenterLH( -clip[0], clip[2], -clip[3], clip[1], clip[4], clip[5] );
            m_mView = mv2;
            m_mProjection = mp2;
        }
        else
        {
            float pos[3];
            float dir[3];
            float clip[6];
            VWB_getPosDirClip( m_warper, &vEyePt.x, &vRot.x, pos, dir, clip, true, m_vp.Width / m_vp.Height );
            mv3 = XMMatrixRotationRollPitchYaw( -dir[0],  dir[1],  dir[2] );
            mp3 = XMMatrixPerspectiveLH( clip[0] + clip[2], clip[1] + clip[3], clip[4], clip[5] );
            m_mView = mv3;
            m_mProjection = mp3;
        }
        ////< end VIOSO API code

        m_rtt->setMProjection( m_mProjection );
        m_rtt->setMView( m_mView );

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
////< end VIOSO API code


//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = NULL;
vector<shared_ptr<OutputWindow>>    g_windows;
XMMATRIX g_mWorld;

BOOL AddWindowWithRenderer(
    HMONITOR,
    HDC,
    LPRECT pR,
    LPARAM lp
)
{
    #ifdef _DEBUG
        constexpr UINT createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
    #else
        constexpr UINT createDeviceFlags = 0;
    #endif
    
    HINSTANCE hInstance = (HINSTANCE)lp;
    CString s; s.Format( _T("Display%i"), (int)g_windows.size() + 1 );
    g_windows.push_back( make_shared<VIOSOWarperWindow>( s, hInstance, pR->left, pR->top, pR->right - pR->left, pR->bottom - pR->top, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
    //g_windows.push_back( make_shared<OutputWindow>( hInstance, s, pR->left, pR->top, pR->right - pR->left, pR->bottom - pR->top, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
    g_windows.back()->addRenderer( make_shared< CubesRenderer >( g_windows.back()->getDevice() ) );

    return TRUE;
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );
    UNREFERENCED_PARAMETER( nCmdShow );

    try
    {
        #if 0 // fill all desktop monitors
        ::EnumDisplayMonitors( 0, NULL, AddWindowWithRenderer, (LPARAM)hInstance );
   
        
        #elif 0 // manual config
        #ifdef _DEBUG
        constexpr UINT createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
        #else
        constexpr UINT createDeviceFlags = 0;
        #endif

        g_windows.push_back( make_shared<VIOSOWarperWindow>( "Display1", hInstance, 0, 0, 1280, 1600, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
        g_windows.back()->addRenderer( make_shared< CubesRenderer >( g_windows.back()->getDevice() ) );

        g_windows.push_back( make_shared<VIOSOWarperWindow>( "Display2", hInstance, 1280, 0, 1280, 1600, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
        g_windows.back()->addRenderer( make_shared< CubesRenderer >( g_windows.back()->getDevice() ) );
        g_windows.push_back( make_shared<VIOSOWarperWindow>( "Display3", hInstance, 7040, 0, 0, 0, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
        g_windows.back()->addRenderer( make_shared< CubesRenderer >( g_windows.back()->getDevice() ) );

        #else // from command line

        // preinitialize with defaults
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        std::wstring channel = L"Display1";
        // set from cmdline
        std::wistringstream cmd( lpCmdLine );
        cmd >> channel >> x >> y >> w >> h;

        #ifdef _DEBUG
        constexpr UINT createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
        #else
        constexpr UINT createDeviceFlags = 0;
        #endif

        g_windows.push_back( make_shared<VIOSOWarperWindow>( CString(channel.c_str()), hInstance, x, y, w, h, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
        g_windows.back()->addRenderer( make_shared< CubesRenderer >( g_windows.back()->getDevice() ) );

        #endif

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