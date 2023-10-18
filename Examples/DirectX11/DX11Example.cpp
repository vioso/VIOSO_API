#define CONFIG_ENUMERATE_MONITORS 1
#define CONFIG_MANUAL 2
#define CONFIG_FROM_COMMANDLINE 3
#define CONFIG_DISPLAYS CONFIG_FROM_COMMANDLINE
//#define CONFIG_NOWARP

// use at least one of the below configs
//#define CONFIG_PNG // this renders a quad with a texture, this always will win the z-test, so nothing else will show
#define CONFIG_ASSIMP
//#define CONFIG_CUBES

#include "DX11Program.h"
#include "AssimpRenderer.h"

#include "resource.h"
#include <vector>
#include <atlstr.h>
#include <string>
#include <sstream>

#include "../../Include/VIOSOWarpBlend.hpp"

using namespace std;

#include "ImageRenderer.h"

#include "CubesRenderer.h"

class MyRenderToTexture : public RenderToTexture
{
public:
    MyRenderToTexture( ID3D11Device* dev, int width, int height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, bool withDepth = false )
        : RenderToTexture( dev, width, height, format, withDepth ) 
    {
        m_rt->setClearColor( RenderTarget::s_sky );
    }
};

////< start VIOSO API code

class VIOSOWarperWindow : public OutputWindow {
    unique_ptr<VWB> m_pWarper;
    unique_ptr<MyRenderToTexture> m_rtt;
public:
    VIOSOWarperWindow( LPCTSTR channelName, HINSTANCE hInstance, int x, int y, int width, int height, int nCmdShow, DXGI_SWAP_EFFECT effect, int bufferCount, int createDeviceFlags, bool withDepth )
    : OutputWindow( hInstance, channelName, x, y, width, height, nCmdShow, effect, bufferCount, createDeviceFlags, withDepth )
    {

        try {
            m_pWarper = make_unique<VWB>(nullptr, m_dev, _T("VIOSOWarpBlend.ini"), channelName);
        }
        catch (VWB_ERROR& err)
        {
            throw exception( (string( "Creating Warper failed with error ") + to_string( int(err)) ).c_str() );
        }

        // create and initialize
        if( 
            VWB_ERROR_NONE != m_pWarper->Init()
        )
        	throw exception( "failed to initialize warper" );

        m_rtt = make_unique< MyRenderToTexture >( m_dev, (int)m_vp.Width, (int)m_vp.Height, DXGI_FORMAT_R8G8B8A8_UNORM, true );
    }

    virtual void addRenderer( std::shared_ptr< Renderer > renderer )
    {
        m_rtt->addRenderer( renderer );
    }

    virtual void preRender() override
    {
        XMFLOAT3 vEyePt( 0.0, 0.0f, 0.0f );
        XMFLOAT3 vRot( 0.0f, 0.0f, 0.0f );

        // we're switching methods each call, to show they are equivalent
        // VWB_getPosDir can yield a symmetric frustum. Image quality suffers, if view angle is
        // far off from perpendicular to the screen. Try using asymetric frustum, especially in 
        // dynamic eye-point scenarios. If a symmetric frustum is requested.
        XMMATRIX mv1, mv2, mv3;
        XMMATRIX mp1, mp2, mp3;
        static unsigned int pass = 1;
        if( 0 == pass )
        {
            pass = 0;
            // get view and projection matrix directly
            m_pWarper->GetViewProj( &vEyePt.x, &vRot.x, (float*)mv1.r, (float*)mp1.r );
            m_mView = mv1;
            m_mProjection = mp1;
        }
        else if( 1 == pass )
        {
            float clip[6];
            m_pWarper->GetViewClip( &vEyePt.x, &vRot.x, (float*)mv2.r, clip );
            mp2 = XMMatrixPerspectiveOffCenterLH( -clip[0], clip[2], -clip[3], clip[1], clip[4], clip[5] );
            m_mView = mv2;
            m_mProjection = mp2;
        }
        else
        {  // make symmetric frustum
            float pos[3];
            float dir[3];
            float clip[6];
            m_pWarper->GetPosDirClip( &vEyePt.x, &vRot.x, pos, dir, clip, true, m_vp.Width / m_vp.Height );
            mv3 = XMMatrixRotationRollPitchYaw( -dir[0],  dir[1],  dir[2] );
            mp3 = XMMatrixPerspectiveLH( clip[0] + clip[2], clip[1] + clip[3], clip[4], clip[5] );
            m_mView = mv3;
            m_mProjection = mp3;
        }


        m_rtt->setMProjection( m_mProjection );
        m_rtt->setMView( m_mView );

        m_rtt->preRender();
    }

    virtual void render() override
    {
        m_rtt->render( m_mWorld );
    }

    virtual void postRender() override
    {
        m_rtt->postRender(); // let render to texture finish
        __super::preRender(); 

        // this applies warp
        m_pWarper->Render( m_rtt->getResource(), VWB_STATEMASK_STANDARD );

        __super::postRender();
    }
};
////< end VIOSO API code

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE   g_hInst = NULL;
vector<shared_ptr<OutputWindow>>    g_windows;
#ifdef _DEBUG
const UINT createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
const UINT createDeviceFlags = 0;
#endif

BOOL AddWindowWithRenderer(
    HMONITOR,
    HDC,
    LPRECT pR,
    LPARAM lp
)
{
    HINSTANCE hInstance = (HINSTANCE)lp;
    CString s; s.Format( _T("Display%i"), (int)g_windows.size() + 1 );
#ifdef CONFIG_NOWARP
    g_windows.push_back( make_shared<OutputWindow>( hInstance, s, pR->left, pR->top, pR->right - pR->left, pR->bottom - pR->top, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
#else
    g_windows.push_back( make_shared<VIOSOWarperWindow>( s, hInstance, pR->left, pR->top, pR->right - pR->left, pR->bottom - pR->top, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
#endif //def CONFIG_NOWARP
    return TRUE;
}

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( 
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow
)
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );
    UNREFERENCED_PARAMETER( nCmdShow );

    std::wstring channel = L"Display1";
    // set from cmdline
    std::wistringstream cmd(lpCmdLine);

    try
    {
    #if ( CONFIG_DISPLAYS == CONFIG_ENUMERATE ) // fill all desktop monitors
 
        ::EnumDisplayMonitors( 0, NULL, AddWindowWithRenderer, (LPARAM)hInstance );
        
    #elif ( CONFIG_DISPLAYS == CONFIG_MANUAL ) // manual config

        #ifdef CONFIG_NOWARP
            g_windows.push_back(make_shared<VIOSOWarperWindow>(hInstance, "Display1", 0, 0, 1280, 1600, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false));
            g_windows.push_back(make_shared<VIOSOWarperWindow>(hInstance, "Display2", 1280, 0, 1280, 1600, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false));
            g_windows.push_back(make_shared<VIOSOWarperWindow>(hInstance, "Display3", 7040, 0, 0, 0, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false));
        #else
            g_windows.push_back( make_shared<VIOSOWarperWindow>( "Display1", hInstance, 0, 0, 1280, 1600, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
            g_windows.push_back( make_shared<VIOSOWarperWindow>( "Display2", hInstance, 1280, 0, 1280, 1600, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
            g_windows.push_back( make_shared<VIOSOWarperWindow>( "Display3", hInstance, 7040, 0, 0, 0, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
        #endif //def CONFIG_NOWARP

    #else // from command line

        // preinitialize with defaults
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        cmd >> channel >> x >> y >> w >> h;

        #ifdef CONFIG_NOWARP
            g_windows.push_back(make_shared<OutputWindow>(hInstance, CString(channel.c_str()), x, y, w, h, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false));
            g_windows.back()->getRenderTarget()->setClearColor( RenderTarget::s_sky );
        #else
            g_windows.push_back( make_shared<VIOSOWarperWindow>( CString(channel.c_str()), hInstance, x, y, w, h, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, createDeviceFlags, false ) );
        #endif //def CONFIG_NOWARP
    #endif

        for (auto& window : g_windows)
        {
    #ifdef CONFIG_ASSIMP
            wstring path = L"cavemodel.dae";
            cmd >> path;
            window->addRenderer( make_shared< AssimpRenderer >( window->getDevice(), CStringA( path.c_str() ) ) );
    #endif
    #ifdef CONFIG_PNG
            wstring path = L"material_1.png";
            cmd >> path;
            window->addRenderer( make_shared< ImageRenderer >( window->getDevice(), CStringA( path.c_str() ) ) );
    #endif
    #ifdef CONFIG_CUBES
            window->addRenderer( make_shared< CubesRenderer >( window->getDevice() ) );
    #endif
        }

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
        OutputDebugStringA("\n");
        return -1;
    }
}
