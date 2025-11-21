// Example source file for VIOSO API
// using DX11 graphics interface
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "DX11Program.h"

#include "resource.h"
#include "../../Include/StringConversions.h"
#include "../../VIOSOWarpBlend/ParamMap.h"

#include "../../Include/VIOSOWarpBlend.hpp"
#include "ImageRenderer.h"
#include "CubesRenderer.h"
#include "AssimpRenderer.h"
#include <numeric>
using namespace std;
using namespace VWBUtil;

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
    VIOSOWarperWindow( LPCTSTR channelName, HINSTANCE hInstance, int x, int y, int width, int height, int nCmdShow, DXGI_SWAP_EFFECT effect, int bufferCount, int createDeviceFlags, bool withDepth, LPCTSTR iniFile = _T( "VIOSOWarpBlend.ini" ), VWB_int logLevel = 2, LPCTSTR logFile = nullptr )
    : OutputWindow( hInstance, channelName, x, y, width, height, nCmdShow, effect, bufferCount, createDeviceFlags, withDepth )
    {

        try {
            m_pWarper = make_unique<VWB>("", m_dev, iniFile, std::filesystem::path(channelName).u8string().c_str(), logLevel, logFile );
        }
        catch (VWB_ERROR& err)
        {
            throw exception( (string( "Creating Warper failed with error ") + to_string( int(err)) ).c_str() );
        }

        m_rtt = make_unique< MyRenderToTexture >( m_dev, (int)m_vp.Width, (int)m_vp.Height, DXGI_FORMAT_R8G8B8A8_UNORM, true );
    }

    virtual ~VIOSOWarperWindow()
    {
        m_pWarper.reset();
        m_rtt.reset();
    }

    VWB_ERROR init() noexcept
    {
        return m_pWarper->Init();
    }

    VWB& getWarper() {
        return *m_pWarper.get();
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
        // dynamic eye-point scenarios.
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
ParamMap<wchar_t> g_params;
vector<shared_ptr<OutputWindow>>    g_windows;
#ifdef _DEBUG
const UINT g_createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
const UINT g_createDeviceFlags = 0;
#endif

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
    UNREFERENCED_PARAMETER( hInstance );
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( nCmdShow );

	try {
        g_params = ParamMap( lpCmdLine, {
            false, L'F',
            { // define parameters with their defaults
                { L"help", {} }, // help is a flag
                { L"render", { -1, true, { L"cubes" } } }, // render needs at least one argument, is mandatory but initialized with { L"cubes" }
                { L"ini", { 1, true, { L"VIOSOWarpBlend.ini" } } }, // ini needs one argument
                { L"nowarp", {} },
                { L"small", {} },
                { L"mode", { -1, true, { L"monitor", L"Display1", L"0" } } },
            },
            { // define how switches map to parameters
                { L'h', { L"help" } },
                { L's', { L"small" } },
                { L'c', { L"render", { L"cubes" } } },
                { L'm', { L"render", { L"model" } } },
                { L'p', { L"render", { L"image" } } },
                { L'i', { L"ini" } },
                { L'n', { L"nowarp" } },
                { L'F', { L"mode", { L"file" } } },
                { L'I', { L"mode", { L"inifile" } } },
                { L'M', { L"mode", { L"monitor" } } },
                { L'A', { L"mode", { L"allmonitors" } } },
                { L'W', { L"mode", { L"window" } } },
            },
    	} );

        if( g_params.is( L"help" ) )
        {
            auto msg = R"END(VIOSO VWF tester
Usage:
  VWFTester [-s] [--option]
  to show a vwf file with cubes just call
  VWFTester file.vwf
  Switches:
    -h or --help: show help

    -c or --render cubes: render some colored cubes
    -m modelfile or --render model modelfile: renders a 3D model from a file
    -p pngfile or --render image pngfile: render a png file

    -i inifile or --ini inifile: use this warpblend ini file for settings, can be empty "" to use internal defaults, if not set, it uses VIOSOWarpBlend.ini
    -n or --nowarp: switch on unwarped output
    -s or --small: switch on preview mode for --file and --inifile mode

    -F file [file]... or --mode file [file]...: parse some vwf files and shows all outputs in their original place
    -I or --mode inifile: parse the inifile specified in default section of given ini
    -M [# [Name [# Name]...]] or --mode monitor [# [Name [# Name]...]]: Brings up the monitors # from Windows display configuration, use 0 to use primary. If no Name is found, it is named DISPLAY#
    -A Name or --mode allmonitors [Name]: Brings up all connected monitors like -M.
    -W Name x y w h [Name x y w h]... or --mode window Name x y w h [Name x y w h]...: Brings up some windows at given position named Name.

(c) VIOSO GmbH 2024
)END";
        
            MessageBoxA( 0, msg, "Help", MB_OK );
        }
        else
        {
            // first we decide, how we produce our window(s)
            auto const& mode = g_params[L"mode"];
            auto const& render = g_params[L"render"];
            bool noWarp = g_params.is( L"nowarp" );
            bool preview = g_params.is( L"small" );
            wstring ini = g_params[L"ini"].empty() ? L"" : g_params[L"ini"][0];
            if( g_params[L"mode"][0] == L"inifile" && 1 < g_params[L"mode"].size() )
                ini = g_params[L"mode"][1];
            if( mode[0] == L"file" )
            {
                // analyze vwf
                VWB::loadDll();
                VWB::SetCryptoKey( (uint8_t*)"blalaberbla" );
                wstring accu = accumulate( mode.begin() + 2, mode.end(), mode[1], []( wstring s, wstring const& o ) { return move(s) + L',' + o; } );

                std::vector<VWB_WarpBlendHeader> set = VWB::VwfInfo( accu.c_str() );
                for( size_t i = 0; i != set.size(); i++ )
                {
                    auto const& h = set[i]; 
                    wstring chName = h.header.name[0] ? to_wstring( h.header.name ) : wstring( _T("Display") ) + to_wstring( i );
                    int l = ( int )h.header.offsetX, t = ( int )h.header.offsetY, wi = h.header.width, hi = h.header.height;
                    if( preview )
                    {
                        l /= 4;
                        t /= 4;
                        hi /= 4;
                        wi /= 4;
                    }
                    if( noWarp )
                        g_windows.emplace_back( make_shared<OutputWindow>( hInstance, chName.c_str(), l, t, wi, hi, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, g_createDeviceFlags, false));
                    else
                    {
                        auto ptr = make_shared<VIOSOWarperWindow>( chName.c_str(), hInstance, l, t, wi, hi, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, g_createDeviceFlags, false, ini.c_str() );
                        auto& w = ptr->getWarper().get();
                        strcpy_s( w.calibFile, accu.c_str() );
                        w.calibIndex = ( int )i;
                        w.calibSplit[0] = 0;
                        w.calibSplit[1] = 0;
                        w.calibSplit[2] = 0;
                        w.calibSplit[3] = 0;
                        if( VWB_ERROR_NONE != ptr->init() )
                            throw exception( ( string( "Init of channel " ) + to_string( chName ) + " failed." ).c_str() );
                        g_windows.push_back( ptr );
                    }
                }
				VWB::unloadDll();
            }
            else if( mode[0] == L"inifile" )
            {
                VWB vwb( "", VWB_DUMMYDEVICE, ini, u8"default" );
                auto set = VWB::VwfInfo( vwb.get().calibFile );
                if( set.empty() )
                    throw invalid_argument( string( "Could not find vwf file " ) + vwb.get().calibFile + " from default section in " + to_string(ini));
                for( size_t i = 0; i != set.size(); i++ )
                {
                    auto const& h = set[i];
                    tstring chName = h.header.name[0] ? to_wstring( h.header.name ) : wstring( _T( "Display" ) ) + to_wstring( i );
                    int l = ( int )h.header.offsetX, t = ( int )h.header.offsetY, wi = h.header.width, hi = h.header.height;
                    if( preview )
                    {
                        l /= 4;
                        t /= 4;
                        hi /= 4;
                        wi /= 4;
                    }
                    if( noWarp )
                        g_windows.emplace_back( make_shared<OutputWindow>( hInstance, chName.c_str(), l, t, wi, hi, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, g_createDeviceFlags, false ) );
                    else
                    {
                        auto ptr = make_shared<VIOSOWarperWindow>( chName.c_str(), hInstance, l, t, wi, hi, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, g_createDeviceFlags, false, ini.c_str() );
                        auto& w = ptr->getWarper().get();
                        strcpy_s( w.calibFile, h.path );
                        w.calibIndex = ( int )i;
                        w.calibSplit[0] = 0;
                        w.calibSplit[1] = 0;
                        w.calibSplit[2] = 0;
                        w.calibSplit[3] = 0;
                        if( VWB_ERROR_NONE != ptr->init() )
                            throw exception( ( string( "Init of channel " ) + to_string( chName ) + " failed." ).c_str() );
                        g_windows.push_back( ptr );
                    }
                }
            }
            else if( mode[0] == L"monitor" )
            {
                struct Info {
                    wstring const& ini;
                    bool noWarp;
                    map<int, wstring> monitors;
                } info{ ini, noWarp };
                for( auto it = mode.begin() + 1; it != mode.end(); )
                {
                    try { 
                        int num = stoi( *(it++) );
                        if( it != mode.end() )
                        {
                            info.monitors.emplace( num, *(it++) );
                        }
                        else
                        {
                            info.monitors.emplace( num, wstring( L"DISPLAY" ) + to_wstring( num ) );
                        }

                    } catch( logic_error& ) {}
                }
                if( info.monitors.empty() )
                    info.monitors.emplace( 0, L"DISPLAY" );
                ::EnumDisplayMonitors( 0, NULL, []( HMONITOR h, HDC, LPRECT pR, LPARAM lp ) {
                    Info& info = *( Info* )lp;
                    MONITORINFOEX mie{ sizeof( MONITORINFOEX ) };
                    GetMonitorInfo( h, &mie );
                    int num = _wtoi( mie.szDevice + 11 );
                    auto it = info.monitors.find( num );
                    if( info.monitors.end() == it && ( mie.dwFlags & MONITORINFOF_PRIMARY ) )
                    {
                        it = info.monitors.find( 0 );
                    }
                    if( info.monitors.end() != it )
                    { // primary (0) or numer equal
                        if( info.noWarp )
                            g_windows.emplace_back( make_shared<OutputWindow>( g_hInst, it->second.c_str(), pR->left, pR->top, pR->right - pR->left, pR->bottom - pR->top, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, g_createDeviceFlags, false ) );
                        else
                        {
                            auto ptr = make_shared<VIOSOWarperWindow>( it->second.c_str(), g_hInst, pR->left, pR->top, pR->right - pR->left, pR->bottom - pR->top, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, g_createDeviceFlags, false, info.ini.c_str() );
                            if( VWB_ERROR_NONE != ptr->init() )
                                throw exception( ( string( "Init of channel " ) + to_string( it->second ) + " failed." ).c_str() );
                            g_windows.push_back( ptr );
                        }
                        info.monitors.erase( it );
                    }
                    if( info.monitors.empty() )
                        return FALSE; // stop enumerating, all requested monitors are created
                    return TRUE; // keep enumerating
                }, ( LPARAM )&info );
            }
            else if( mode[0] == L"allmonitors" )
            {
                struct Info {
                    wstring const& ini;
                    wstring const& name;
                    bool noWarp;
                } info{ ini, 1 < mode.size() && !mode[1].empty() ? mode[1] : L"DISPLAY", noWarp };
                ::EnumDisplayMonitors( 0, NULL, []( HMONITOR h, HDC, LPRECT pR, LPARAM lp) {
                    Info& info = *( Info* )lp;
                    MONITORINFOEX mie{ sizeof( MONITORINFOEX ) };
                    GetMonitorInfo( h, &mie );
                    wstring chName = info.name + ( mie.szDevice + 11 );
                    if( info.noWarp )
                        g_windows.emplace_back( make_shared<OutputWindow>( g_hInst, chName.c_str(), pR->left, pR->top, pR->right - pR->left, pR->bottom - pR->top, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, g_createDeviceFlags, false ) );
                    else
                    {
                        auto ptr = make_shared<VIOSOWarperWindow>( chName.c_str(), g_hInst, pR->left, pR->top, pR->right - pR->left, pR->bottom - pR->top, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, g_createDeviceFlags, false, info.ini.c_str() );
                        if( VWB_ERROR_NONE != ptr->init() )
                            throw exception( ( string( "Init of channel " ) + to_string( chName ) + " failed." ).c_str() );
                        g_windows.push_back( ptr );
                    }
                    return TRUE; // keep enumerating
                }, (LPARAM)&info );
            }
            else if( mode[0] == L"window" )
            {
                auto it = mode.begin() + 1;
                while( it != mode.end() )
                {
                    wstring chName = *it;
                    int x = 0, y = 0, w = 800, h = 600;
                    try {
                        if( ++it != mode.end() ) 
                        {
                            x = stoi( *it );
                            if( ++it != mode.end() )
                            {
                                y = stoi( *it );
                                if( ++it != mode.end() )
                                {
                                    w = stoi( *it );
                                    if( ++it != mode.end() ) h = stoi( *it++ );
                                }
                            }
                        }
                    }
                    catch( logic_error&)
                    {
                        // just ignore and continue
                    }
                    if( noWarp )
                        g_windows.emplace_back( make_shared<OutputWindow>( hInstance, chName.c_str(), x, y, w, h, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, g_createDeviceFlags, false ) );
                    else
                    {
                        auto ptr = make_shared<VIOSOWarperWindow>( chName.c_str(), hInstance, x, y, w, h, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, g_createDeviceFlags, false, ini.c_str() );
                        if( VWB_ERROR_NONE != ptr->init() )
                            throw exception( ( string( "Init of channel " ) + to_string( chName ) + " failed." ).c_str() );
                        g_windows.push_back( ptr );
                    }

                }
                if( mode.size() == 1 )
                {
                    if( noWarp )
                        g_windows.emplace_back( make_shared<OutputWindow>( hInstance, L"Display1", 0, 0, 800, 600, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, g_createDeviceFlags, false));
                    else
                    {
                        auto ptr = make_shared<VIOSOWarperWindow>( L"Display1", hInstance, 0, 0, 800, 600, SW_SHOW, DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, 2, g_createDeviceFlags, false, ini.c_str() );
                        if( VWB_ERROR_NONE != ptr->init() )
                            throw exception( "Init of channel Display1 failed." );
                        g_windows.push_back( ptr );
                    }
                }
            }
            else
            {
                throw invalid_argument( string( "Unknown --mode " + to_string( mode[0] ) ) );
            }

            if( render[0] == L"cubes" )
            {
                for( auto& wnd : g_windows )
                {
                    wnd->addRenderer( make_shared< CubesRenderer >( wnd->getDevice() ) );
                }
            }
            else if( render[0] == L"model" )
            {
                if( render.size() < 2 || render[1].empty() )
                    throw invalid_argument("Path to 3D model file missing.");
                for( auto& wnd : g_windows )
                {
                    for( auto s = render.begin() + 1; s != render.end(); s++ )
                        wnd->addRenderer( make_shared< AssimpRenderer >( wnd->getDevice(), *s ) );
                }
            }
            else if( render[0] == L"image" )
            {
                if( render.size() != 2 || render[1].empty() )
                    throw invalid_argument( "Path to image file missing." );

                for( auto& wnd : g_windows )
                {
                    for( auto s = render.begin() + 1; s != render.end(); s++ )
                        wnd->addRenderer( make_shared< ImageRenderer >( wnd->getDevice(), to_string( *s ).c_str() ) );
                }
            }
            else if( render[0] == L"imageROI" )
            {
                throw invalid_argument( "Not implemented yet." );
            }
        }

        MSG msg = { 0 };
        if( !g_windows.empty() ) {
            // Main message loop
            while( WM_QUIT != msg.message ) {
                if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                } else {
                    for( auto& wnd : g_windows ) {
                        wnd->preRender();
                        wnd->render();
                        wnd->postRender();
                    }
                }
            }
        } else {
            MessageBoxA( 0, std::string("No outputs.\nFor help use -h or --help." ).c_str(), "Error", MB_OK );
        }
        g_windows.clear();

        #ifdef _DEBUG
        {
            if( HMODULE h = GetModuleHandleA( "Dxgidebug.dll" ) )
            {
                if( auto fn = ( decltype( DXGIGetDebugInterface )* )GetProcAddress( h, "DXGIGetDebugInterface" ) )
                {
                    if( IDXGIDebug* dbg; SUCCEEDED( fn( __uuidof( IDXGIDebug ), ( void** )( IDXGIDebug** )&dbg ) ) )
                    {
                        dbg->ReportLiveObjects( DXGI_DEBUG_D3D11, DXGI_DEBUG_RLO_DETAIL );
                        dbg->Release();
                    }
                }
            }
        }
        #endif //dev _DEBUG

        return ( int )msg.wParam;
    }
    catch( VWB_ERROR e ) {
        MessageBoxA( 0, ( std::string("VWB ERROR: ") + VWB::GetErrorCStr(e) + "\nFor help use -h or --help." ).c_str(), "Library Parameters", MB_OK );
        return -3;
    }
	catch( std::invalid_argument& e )
	{
		MessageBoxA( 0, ( std::string(e.what()) + "\nFor help use -h or --help." ).c_str(), "Invalid Parameters", MB_OK );
		return -2;
	}
    catch( std::exception& e )
    {
        MessageBoxA( 0, e.what(), "Exception", MB_OK );
        return -1;
    }
}
