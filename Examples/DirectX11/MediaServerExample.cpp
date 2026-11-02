// VIOSO API example: a media server with several outputs on one DirectX 11 device
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause
//
// What a media server has to do with a .vwf file that holds the channels of several projectors:
//   1. read the channel list (VWB_vwfInfo) and pick the channels this computer drives,
//   2. make one output (window and swap chain) and one warper per channel, each warper with its own calibIndex,
//   3. provide the content, the picture in "content space" that is spread over all projectors, and
//   4. every frame: bind the output's render target, VWB_render the content into it, present.
//
// The content is a test grid with lines in fixed distances in content pixels, labelled with the content coordinates,
// so any offset between channels is visible. Pass a PNG with --png to show your own picture instead.
//
// Usage: MediaServerExampleDX11 file.vwf [--list] [--channels A,B] [--host name] [--partial] [--png file] [--small]
//   --list      print the channels of the file and exit
//   --channels  only these channels, by name (default: all)
//   --host      only channels for this host name (the name or address stored in the file)
//   --partial   every output gets only the part of the content its channel needs (bPartialInput / optimalRect).
//               Needed for large setups, where the whole content space does not fit into one texture (16384 pixels).
//   --png       a picture for the whole content space
//   --small     show everything at 1/4 size
// ESC ends the program.
//
// NOTE: --partial relies on the content size and rect stored per channel (vCntDispPx, vPartialCnt). Exports from the calibration software
// currently have different vCntDispPx per channel, so the rect can be wrong with those files, until they are written
// consistently. Without --partial the whole content space is in one texture and this does not matter.

#include "DX11Program.h"
#include "WindowPlacement.h"
#include "../../Include/VIOSOWarpBlend.hpp"
#include "lpng16/png.hpp"
#include <cstdio>
#include <sstream>

using namespace std;

struct Options
{
    string vwf;
    vector<string> channels;
    string host, image;
    bool list = false, partial = false, preview = false;
};

// one projector: a window with a swap chain, and a warper that renders into it
struct Output
{
    string name;
    WindowHandle window;
    ComPtr<IDXGISwapChain> swapChain;
    unique_ptr<BackBuffer> backBuffer;
    unique_ptr<VWB> warper;
    ComPtr<ID3D11Texture2D> content; // the picture to warp, shared by all outputs unless --partial
    explicit Output( HWND hWnd ) : window( hWnd ) {}
};

//--------------------------------------------------------------------------------------
// Content: the grid or a PNG, as RGBA pixels, and the texture for it
//--------------------------------------------------------------------------------------

// A grid of the content rect (l,t,r,b) in content pixels: a line every 100 pixels, a bright one every 500, labelled.
static vector<uint32_t> makeGrid( int l, int t, int r, int b )
{
    int w = r - l, h = b - t;
    BITMAPINFO bi{ { sizeof( BITMAPINFOHEADER ), w, -h, 1, 32, BI_RGB } }; // negative height: top-down
    void* bits = nullptr;
    HDC dc = CreateCompatibleDC( nullptr );
    HBITMAP bmp = CreateDIBSection( dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0 );
    if( !bmp )
        throw runtime_error( "Failed to create a bitmap for the grid" );
    HGDIOBJ oldBmp = SelectObject( dc, bmp );
    RECT all{ 0, 0, w, h };
    HBRUSH bg = CreateSolidBrush( RGB( 24, 24, 40 ) );
    FillRect( dc, &all, bg );
    HPEN thin = CreatePen( PS_SOLID, 1, RGB( 70, 70, 110 ) ), bold = CreatePen( PS_SOLID, 3, RGB( 230, 230, 60 ) );
    HFONT font = CreateFontA( 22, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, "Arial" );
    SelectObject( dc, font );
    SetBkMode( dc, TRANSPARENT );
    SetTextColor( dc, RGB( 255, 255, 255 ) );
    for( int x = l - l % 100 ; x < r; x += 100 ) // l % 100 keeps the lines in place, whatever l is
    {
        SelectObject( dc, 0 == x % 500 ? bold : thin );
        MoveToEx( dc, x - l, 0, nullptr );
        LineTo( dc, x - l, h );
    }
    for( int y = t - t % 100; y < b; y += 100 )
    {
        SelectObject( dc, 0 == y % 500 ? bold : thin );
        MoveToEx( dc, 0, y - t, nullptr );
        LineTo( dc, w, y - t );
    }
    for( int y = t - t % 500; y < b; y += 500 ) // coordinates where the bold lines cross
        for( int x = l - l % 500; x < r; x += 500 )
        {
            string s = to_string( x ) + "," + to_string( y );
            TextOutA( dc, x - l + 6, y - t + 6, s.c_str(), (int)s.size() );
        }
    GdiFlush();

    vector<uint32_t> px( size_t( w ) * h );
    memcpy( px.data(), bits, px.size() * 4 );
    for( auto& p : px ) // GDI gives BGRX, we want RGBA
        p = 0xFF000000 | ( ( p & 0xFF ) << 16 ) | ( p & 0xFF00 ) | ( ( p >> 16 ) & 0xFF );
    SelectObject( dc, oldBmp );
    DeleteObject( bmp ); DeleteObject( bg ); DeleteObject( thin ); DeleteObject( bold ); DeleteObject( font );
    DeleteDC( dc );
    return px;
}

// The rect (l,t,r,b) of the PNG, which is a picture for a content space of size cw x ch.
static vector<uint32_t> cropPng( string const& path, int cw, int ch, int l, int t, int r, int b, int& w, int& h )
{
    CPng img( path.c_str() );
    vector<uint8_t> rgba( size_t( img.m_width ) * img.m_height * 4 );
    if( img.m_colorType == PNG_COLOR_TYPE_RGBA && img.m_bitDepth == 8 ) rgba.swap( img.m_imageData );
    else if( img.m_colorType == PNG_COLOR_TYPE_RGB && img.m_bitDepth == 8 ) img.convertRGB8toRGBA8( rgba.data() );
    else if( img.m_colorType == PNG_COLOR_TYPE_RGB && img.m_bitDepth == 16 ) img.convertRGB16toRGBA8( rgba.data() );
    else if( img.m_colorType == PNG_COLOR_TYPE_RGBA && img.m_bitDepth == 16 ) img.convertRGBA16toRGBA8( rgba.data() );
    else throw runtime_error( "The PNG must be RGB or RGBA with 8 or 16 bits" );

    double sx = double( img.m_width ) / cw, sy = double( img.m_height ) / ch; // the PNG may have another size than the content
    int x0 = int( l * sx ), y0 = int( t * sy );
    w = max( 1, int( ( r - l ) * sx ) );
    h = max( 1, int( ( b - t ) * sy ) );
    vector<uint32_t> px( size_t( w ) * h );
    for( int y = 0; y != h; y++ )
        memcpy( &px[size_t( y ) * w], &rgba[( size_t( min( y0 + y, (int)img.m_height - 1 ) ) * img.m_width + x0 ) * 4], size_t( min( w, (int)img.m_width - x0 ) ) * 4 );
    return px;
}

// the content for the content rect (l,t,r,b) of a content space of size cw x ch, as a texture
static ComPtr<ID3D11Texture2D> makeContent( ID3D11Device* dev, Options const& opt, int cw, int ch, int l, int t, int r, int b )
{
    int w = r - l, h = b - t;
    vector<uint32_t> px;
    if( opt.image.empty() )
        px = makeGrid( l, t, r, b );
    else
        px = cropPng( opt.image, cw, ch, l, t, r, b, w, h );
    if( w > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || h > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION )
        throw runtime_error( "The content (" + to_string( w ) + "x" + to_string( h ) + ") does not fit into a texture. Use --partial." );

    D3D11_TEXTURE2D_DESC desc{ (UINT)w, (UINT)h, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
    D3D11_SUBRESOURCE_DATA data{ px.data(), UINT( w * 4 ), 0 };
    ComPtr<ID3D11Texture2D> tex;
    if( FAILED( dev->CreateTexture2D( &desc, &data, &tex ) ) )
        throw runtime_error( "Failed to create the content texture" );
    return tex;
}

//--------------------------------------------------------------------------------------
// Outputs: window, swap chain, back buffer
//--------------------------------------------------------------------------------------

static LRESULT CALLBACK wndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    if( WM_DESTROY == msg || ( WM_CHAR == msg && 27 == wParam ) ) // ESC
        PostQuitMessage( 0 );
    return DefWindowProcA( hWnd, msg, wParam, lParam );
}

static unique_ptr<Output> makeOutput( ID3D11Device* dev, string const& name, RECT const& rc )
{
    static const char* c_class = "MediaServerExampleOutput";
    static bool registered = false;
    if( !registered )
    {
        WNDCLASSEXA wc{ sizeof( wc ), CS_HREDRAW | CS_VREDRAW, wndProc, 0, 0, GetModuleHandle( nullptr ), nullptr, LoadCursor( nullptr, IDC_ARROW ), nullptr, nullptr, c_class };
        registered = 0 != RegisterClassExA( &wc );
    }
    HWND hWnd = CreateWindowExA( WS_EX_APPWINDOW, c_class, name.c_str(), WS_POPUP | WS_VISIBLE, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                                 nullptr, nullptr, GetModuleHandle( nullptr ), nullptr );
    if( !hWnd )
        throw runtime_error( "Failed to create the window for " + name );
    auto out = make_unique<Output>( hWnd );
    out->name = name;

    // the swap chain has to come from the factory of the device
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc = { UINT( rc.right - rc.left ), UINT( rc.bottom - rc.top ), { 60, 1 }, DXGI_FORMAT_R8G8B8A8_UNORM };
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc = { 1, 0 };
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    ComPtr<IDXGIDevice> dxgiDev;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory> factory;
    if( FAILED( dev->QueryInterface( IID_PPV_ARGS( &dxgiDev ) ) ) || FAILED( dxgiDev->GetAdapter( &adapter ) ) ||
        FAILED( adapter->GetParent( IID_PPV_ARGS( &factory ) ) ) || FAILED( factory->CreateSwapChain( dev, &sd, &out->swapChain ) ) )
        throw runtime_error( "Failed to create the swap chain for " + name );
    out->backBuffer = make_unique<BackBuffer>( dev, out->swapChain.Get(), false );
    return out;
}

//--------------------------------------------------------------------------------------
// The program
//--------------------------------------------------------------------------------------

static Options parseArguments( int argc, char** argv )
{
    Options opt;
    for( int i = 1; i < argc; i++ )
    {
        string a = argv[i];
        if( "--list" == a ) opt.list = true;
        else if( "--partial" == a ) opt.partial = true;
        else if( "--small" == a ) opt.preview = true;
        else if( "--host" == a && i + 1 < argc ) opt.host = argv[++i];
        else if( "--png" == a && i + 1 < argc ) opt.image = argv[++i];
        else if( "--channels" == a && i + 1 < argc )
        {
            stringstream ss( argv[++i] );
            for( string name; getline( ss, name, ',' ); )
                opt.channels.push_back( name );
        }
        else if( a.size() > 1 && '-' == a[0] ) throw invalid_argument( "Unknown or incomplete option " + a );
        else opt.vwf = a;
    }
    if( opt.vwf.empty() )
        throw invalid_argument( "Usage: MediaServerExampleDX11 file.vwf [--list] [--channels A,B] [--host name] [--partial] [--png file] [--small]" );
    return opt;
}

static int run( Options const& opt )
{
    // 1. The channels of the file. Relative paths are resolved against the folder of the VIOSOWarpBlend library.
    vector<VWB_WarpBlendHeader> all = VWB::VwfInfo( opt.vwf.c_str() );
    if( all.empty() )
        throw runtime_error( "Could not read any channel from \"" + opt.vwf + "\". Is the file missing or not a .vwf?" );
    printf( "  #  name                           host                 position    size        content size\n" );
    for( size_t i = 0; i != all.size(); i++ )
    {
        auto const& h = all[i].header;
        printf( "%3d  %-30.30s %-20.20s %5.0f,%-5.0f %4dx%-5d %dx%d\n", (int)i, h.name, h.hostname, h.offsetX, h.offsetY, h.width, h.height,
                h.vCntDispPx[4] ? int( 1 / h.vCntDispPx[4] ) : 0, h.vCntDispPx[5] ? int( 1 / h.vCntDispPx[5] ) : 0 );
    }
    if( opt.list )
        return 0;

    // 2. The channels this computer drives. The calibIndex of a channel is its position in the list.
    vector<int> chosen;
    for( size_t i = 0; i != all.size(); i++ )
    {
        string name = all[i].header.name;
        bool byName = opt.channels.empty() || opt.channels.end() != find( opt.channels.begin(), opt.channels.end(), name );
        bool byHost = opt.host.empty() || opt.host == all[i].header.hostname;
        if( byName && byHost )
            chosen.push_back( (int)i );
    }
    for( auto const& name : opt.channels ) // a typo must not go unnoticed
        if( all.end() == find_if( all.begin(), all.end(), [&]( auto const& h ) { return name == h.header.name; } ) )
            throw runtime_error( "There is no channel named \"" + name + "\" in the file. Use --list to see the names." );
    if( chosen.empty() )
        throw runtime_error( "No channel left after the selection." );

    // 3. One device for all outputs, so they can share the content texture.
    ComPtr<ID3D11Device> dev = GFXPipeline::createDevice( 0 );
    ComPtr<ID3D11DeviceContext> ctx;
    dev->GetImmediateContext( &ctx );

    vector<VWB_WarpBlendHeader> headers;
    for( int i : chosen )
        headers.push_back( all[i] );
    Placement placement( headers, opt.preview ); // where the windows go, shrinks the arrangement if it does not fit on the desktop

    vector<unique_ptr<Output>> outputs;
    for( size_t n = 0; n != chosen.size(); n++ )
    {
        auto const& h = headers[n].header;
        auto out = makeOutput( dev.Get(), h.name, placement.rect( headers[n] ) );
        // the window title tells, if the layout was changed
        SetWindowTextW( out->window, ( wstring( h.name, h.name + strlen( h.name ) ) + placement.note ).c_str() );

        // The warper reads size and viewport of the output from the render target that is bound while it initialises.
        out->backBuffer->setAsRTto( ctx.Get() );

        // No .ini file: the channel comes from the program. Create the warper with the channel's name, set what the
        // .ini file would hold, then Init. With an index >= 0 only the data of this channel is loaded.
        out->warper = make_unique<VWB>( nullptr, dev.Get(), nullptr, h.name );
        VWB_Warper& w = out->warper->get();
        strcpy_s( w.calibFile, opt.vwf.c_str() );
        w.calibIndex = chosen[n];
        static const float c_leftHanded[16]{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1 }; // base for Direct3D
        memcpy( w.trans, c_leftHanded, sizeof( c_leftHanded ) );
        w.bPartialInput = opt.partial; // the source texture holds only optimalRect of the content space
        if( VWB_ERROR err = out->warper->Init(); VWB_ERROR_NONE != err )
            throw runtime_error( string( "Init of channel \"" ) + h.name + "\" failed: " + VWB::GetErrorCStr( err ) + ". See VIOSOWarpBlend.log next to the library." );
        outputs.push_back( move( out ) );
    }

    // 4. The content. Init has worked out the content size (optimalRes) and the part this channel needs (optimalRect).
    int cw = 0, ch = 0;
    for( auto& o : outputs )
    {
        VWB_Warper& w = o->warper->get();
        cw = max( cw, (int)w.optimalRes.cx ); // equal for all channels of a consistent file
        ch = max( ch, (int)w.optimalRes.cy );
        printf( "%s: content %dx%d, rect [%d,%d,%d,%d]\n", o->name.c_str(), w.optimalRes.cx, w.optimalRes.cy, w.optimalRect.left, w.optimalRect.top, w.optimalRect.right, w.optimalRect.bottom );
    }
    ComPtr<ID3D11Texture2D> whole;
    if( !opt.partial )
        whole = makeContent( dev.Get(), opt, cw, ch, 0, 0, cw, ch );
    for( auto& o : outputs )
    {
        auto const& rc = o->warper->get().optimalRect;
        o->content = opt.partial ? makeContent( dev.Get(), opt, o->warper->get().optimalRes.cx, o->warper->get().optimalRes.cy, rc.left, rc.top, rc.right, rc.bottom ) : whole;
    }

    // 5. Every frame, for every output.
    MSG msg{};
    while( WM_QUIT != msg.message )
    {
        if( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
            continue;
        }
        for( auto& o : outputs )
        {
            o->backBuffer->setAsRTto( ctx.Get() ); // the warper draws into the render target that is bound now
            o->backBuffer->clear( ctx.Get() );     // black, where the mapping has no content

            // VWB_STATEMASK_DEFAULT restores vertex buffer, input layout, primitive topology and raster state. Shaders,
            // resources and samplers are not restored, set them again before your next draw call.
            // VWB_STATEMASK_VIEWPORT sets the viewport to the size of the render target and restores yours afterwards.
            o->warper->Render( o->content.Get(), VWB_STATEMASK_DEFAULT | VWB_STATEMASK_VIEWPORT );

            // only the first output waits for the vertical sync, otherwise the frame rate drops with every output
            o->swapChain->Present( o == outputs.front() ? 1 : 0, 0 );
        }
    }
    return 0;
}

int main( int argc, char** argv )
{
    setvbuf( stdout, nullptr, _IONBF, 0 ); // show the messages at once, also when redirected
    try
    {
        return run( parseArguments( argc, argv ) );
    }
    catch( exception& e )
    {
        fprintf( stderr, "%s\n", e.what() );
        MessageBoxA( nullptr, e.what(), "VIOSO media server example", MB_OK | MB_ICONERROR );
        return 1;
    }
}
