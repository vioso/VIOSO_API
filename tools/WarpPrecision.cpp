// Measures how exactly the library reproduces the warp of a very wide content space. It is a development tool,
// not a part of the API, and no sample for integrators.
//
//   WarpPrecision wide.vwf [--api dx11|dx12|gl|all] [--row N] [--csv file] [--mirror]
//
// Make the file with MakeTestVwf, one channel of 3840x2160 in a content space of 153600 pixels, for example
//   MakeTestVwf make wide.vwf --size 3840x2160 --content 153600x2160 --content-offset 76800,0
// (the file is about 150 MB; delete it when you are done).
//
// The warper is used the way a media server does: bPartialInput is set, so the source texture holds only the part
// of the content space that the channel needs (optimalRect, here 3840 pixels wide), while the warp records are
// normalised to the whole content space. The source texture is R32G32_FLOAT, every texel holds its own position
// (x, y) in texel units, relative to optimalRect, so what the warper samples can be read from the output: the
// output pixel is the position, at which the warper looked into the content. The output is a float render target,
// the program reads one row back and compares it with the position the generator wrote into the warp (the
// generator maps every output pixel linearly: the content position is the pixel position, plus the offset).
//
// Reported, in content pixels:
//   max |error|   largest distance from that position (for the offset of the whole row, see "mean error")
//   steps         the output follows the warp in steps, if the warp texture is too coarse. "jumps" lists the smallest
//                 and the largest difference between neighbouring output pixels; the ideal is one everywhere.
//                 "equal" is the number of output pixels that are equal to their left neighbour (0 for a smooth line).
// The first and the last pixel of the row are reported on their own. At the last pixel the warper samples the
// source texture on the edge of its last texel; the hardware mixes in the border colour (black) with a weight of
// 1/256 there, so the value shows up 1/256 too small (3839.5 / 256 = 15 pixels). That is no error of the mapping.
// --mirror stores width - position in the texels and converts the output back: the same mixing then happens with a
// small value, and the error at the last pixel is gone.
// The OpenGL warp texture has always been float, so it should show no steps with the same measurement. If it does,
// the method is wrong.

#include <windows.h>
#include <d3d11.h>
#include <d3d12.h>
#include <wrl.h>
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include "VIOSOWarpBlend.hpp"

using Microsoft::WRL::ComPtr;
using namespace std;

struct Options
{
    string vwf;
    string api = "all";
    int row = -1; // -1: the middle row
    string csv;
    bool mirror = false; // texel i holds width - (i + 0.5): test for the border, see below
};

// One measured row: output.x in texel units of the source texture (relative to optimalRect.left)
struct Row
{
    vector<float> x;
    int rectLeft = 0, rectWidth = 0, contentWidth = 0;
    double offsetX = 0; // content position of the channel's first pixel
};

static void check( HRESULT hr, char const* what )
{
    if( FAILED( hr ) )
        throw runtime_error( string( what ) + " failed, HRESULT " + to_string( (unsigned)hr ) );
}

// ---- the evaluation -----------------------------------------------------------------------------------------------

static void evaluate( char const* api, Row const& r, string const& csv )
{
    size_t const n = r.x.size();
    // The generator maps pixel p to the content position offset + p + 0.5; the source texture starts at rectLeft, a
    // texel i holds i + 0.5, so the warper should sample exactly there.
    // The first and the last pixel of a row are judged on their own: the border of the warp texture is no
    // part of what is measured here.
    double maxErr = 0, sumErr = 0;
    float minJump = 1e30f, maxJump = -1e30f;
    size_t equal = 0;
    auto const ideal = [&]( size_t p ) { return r.offsetX + double( p ) + 0.5 - r.rectLeft; };
    for( size_t p = 1; p + 1 < n; p++ )
    {
        double const e = double( r.x[p] ) - ideal( p );
        sumErr += e;
        maxErr = max( maxErr, fabs( e ) );
        if( p > 1 )
        {
            float const j = r.x[p] - r.x[p - 1];
            minJump = min( minJump, j );
            maxJump = max( maxJump, j );
            if( 0 == j )
                equal++;
        }
    }
    printf( "%-5s content %d px, source texture %d px (optimalRect.left %d), row of %zu pixels\n", api, r.contentWidth, r.rectWidth, r.rectLeft, n );
    printf( "      max |error| %.4f px   mean error %+.4f px   (without the first and last pixel)\n", maxErr, sumErr / double( n - 2 ) );
    printf( "      jumps between neighbouring pixels: smallest %.4f, largest %.4f (ideal 1); %zu of %zu pixels equal to their left neighbour\n", minJump, maxJump, equal, n - 3 );
    printf( "      first pixel error %+.4f px, last pixel error %+.4f px\n", double( r.x[0] ) - ideal( 0 ), double( r.x[n - 1] ) - ideal( n - 1 ) );
    if( !csv.empty() )
    {
        string name = csv + "." + api + ".csv";
        if( FILE* f = fopen( name.c_str(), "w" ) )
        {
            fprintf( f, "pixel;sampled;ideal\n" );
            for( size_t p = 0; p != n; p++ )
                fprintf( f, "%zu;%.6f;%.6f\n", p, r.x[p], r.offsetX + double( p ) + 0.5 - r.rectLeft );
            fclose( f );
            printf( "      row written to %s\n", name.c_str() );
        }
    }
}

// the source texture: texel (i,j) holds (i+0.5, j+0.5)
static vector<float> makeSource( int w, int h, bool mirror )
{
    vector<float> d( size_t( w ) * h * 2 );
    for( int j = 0; j != h; j++ )
        for( int i = 0; i != w; i++ )
        {
            d[( size_t( j ) * w + i ) * 2 + 0] = mirror ? float( w - i ) - 0.5f : float( i ) + 0.5f;
            d[( size_t( j ) * w + i ) * 2 + 1] = float( j ) + 0.5f;
        }
    return d;
}

static void prepare( VWB_Warper& w, string const& vwf, bool leftHanded )
{
    strcpy_s( w.calibFile, vwf.c_str() );
    w.calibIndex = 0;
    w.bPartialInput = true; // the source texture holds only optimalRect
    w.bDoNotBlend = true;   // the output must be the source, nothing else
    w.bDoNoBlack = true;
    w.gamma = 1;
    float const lh[16]{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1 };
    float const rh[16]{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    memcpy( w.trans, leftHanded ? lh : rh, sizeof( lh ) );
}

static void fill( Row& row, VWB_Warper const& w, vector<VWB_WarpBlendHeader> const& headers )
{
    row.rectLeft = w.optimalRect.left;
    row.rectWidth = w.optimalRect.right - w.optimalRect.left;
    row.contentWidth = w.optimalRes.cx;
    // the generator's offset of the channel in the content space: vPartialCnt[0] is it, normalised
    row.offsetX = round( double( headers[0].header.vPartialCnt[0] ) * double( w.optimalRes.cx ) );
}

// ---- Direct3D 11 --------------------------------------------------------------------------------------------------

static Row measureDx11( Options const& opt, vector<VWB_WarpBlendHeader> const& headers )
{
    int const W = (int)headers[0].header.width, H = (int)headers[0].header.height;
    ComPtr<ID3D11Device> dev;
    ComPtr<ID3D11DeviceContext> ctx;
    D3D_FEATURE_LEVEL fl;
    check( D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &dev, &fl, &ctx ), "D3D11CreateDevice" );

    // the float render target, bound while the warper initialises: it reads size and viewport from there
    D3D11_TEXTURE2D_DESC td{};
    td.Width = W;
    td.Height = H;
    td.MipLevels = td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET;
    ComPtr<ID3D11Texture2D> rt;
    check( dev->CreateTexture2D( &td, nullptr, &rt ), "create render target" );
    ComPtr<ID3D11RenderTargetView> rtv;
    check( dev->CreateRenderTargetView( rt.Get(), nullptr, &rtv ), "create render target view" );
    ID3D11RenderTargetView* rtvs[]{ rtv.Get() };
    ctx->OMSetRenderTargets( 1, rtvs, nullptr );
    D3D11_VIEWPORT vp{ 0, 0, float( W ), float( H ), 0, 1 };
    ctx->RSSetViewports( 1, &vp );

    VWB warper( nullptr, dev.Get(), nullptr, "Display1" );
    prepare( warper.get(), opt.vwf, true );
    if( VWB_ERROR err = warper.Init(); VWB_ERROR_NONE != err )
        throw runtime_error( string( "Init failed: " ) + VWB::GetErrorCStr( err ) + ". See VIOSOWarpBlend.log." );

    Row row;
    fill( row, warper.get(), headers );
    int const cw = row.rectWidth, ch = warper.get().optimalRect.bottom - warper.get().optimalRect.top;

    vector<float> src = makeSource( cw, ch, opt.mirror );
    D3D11_TEXTURE2D_DESC sd{};
    sd.Width = cw;
    sd.Height = ch;
    sd.MipLevels = sd.ArraySize = 1;
    sd.Format = DXGI_FORMAT_R32G32_FLOAT;
    sd.SampleDesc.Count = 1;
    sd.Usage = D3D11_USAGE_IMMUTABLE;
    sd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA sdata{ src.data(), UINT( cw * 2 * sizeof( float ) ), 0 };
    ComPtr<ID3D11Texture2D> source;
    check( dev->CreateTexture2D( &sd, &sdata, &source ), "create source texture" );

    float const black[4]{ 0, 0, 0, 0 };
    ctx->ClearRenderTargetView( rtv.Get(), black );
    if( VWB_ERROR err = warper.Render( source.Get(), VWB_STATEMASK_DEFAULT | VWB_STATEMASK_VIEWPORT ); VWB_ERROR_NONE != err )
        throw runtime_error( string( "Render failed: " ) + VWB::GetErrorCStr( err ) );

    // one row to a staging texture
    int const y = opt.row >= 0 ? opt.row : H / 2;
    D3D11_TEXTURE2D_DESC st = td;
    st.Width = W;
    st.Height = 1;
    st.Usage = D3D11_USAGE_STAGING;
    st.BindFlags = 0;
    st.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    check( dev->CreateTexture2D( &st, nullptr, &staging ), "create staging texture" );
    D3D11_BOX box{ 0, UINT( y ), 0, UINT( W ), UINT( y + 1 ), 1 };
    ctx->CopySubresourceRegion( staging.Get(), 0, 0, 0, 0, rt.Get(), 0, &box );
    D3D11_MAPPED_SUBRESOURCE m{};
    check( ctx->Map( staging.Get(), 0, D3D11_MAP_READ, 0, &m ), "map staging texture" );
    row.x.resize( W );
    for( int p = 0; p != W; p++ )
        row.x[p] = ( (float const*)m.pData )[p * 4];
    ctx->Unmap( staging.Get(), 0 );
    return row;
}

// ---- OpenGL -------------------------------------------------------------------------------------------------------

#define GL_RG32F 0x8230
#define GL_RG 0x8227
#define GL_RGBA32F 0x8814
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_FRAMEBUFFER 0x8D40
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
typedef void( APIENTRY* PFNGENFB )( GLsizei, GLuint* );
typedef void( APIENTRY* PFNBINDFB )( GLenum, GLuint );
typedef void( APIENTRY* PFNFBTEX )( GLenum, GLenum, GLenum, GLuint, GLint );
typedef GLenum( APIENTRY* PFNCHECKFB )( GLenum );

static Row measureGl( Options const& opt, vector<VWB_WarpBlendHeader> const& headers )
{
    int const W = (int)headers[0].header.width, H = (int)headers[0].header.height;

    // a window nobody sees, only for the context
    WNDCLASSA wc{};
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = GetModuleHandleA( nullptr );
    wc.lpszClassName = "WarpPrecisionGL";
    RegisterClassA( &wc );
    HWND hWnd = CreateWindowA( wc.lpszClassName, "WarpPrecision", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr );
    HDC hDC = GetDC( hWnd );
    PIXELFORMATDESCRIPTOR pfd{ sizeof( pfd ), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32 };
    pfd.cDepthBits = 24;
    int pf = ChoosePixelFormat( hDC, &pfd );
    if( !pf || !SetPixelFormat( hDC, pf, &pfd ) )
        throw runtime_error( "no OpenGL pixel format" );
    HGLRC hRC = wglCreateContext( hDC );
    if( !hRC || !wglMakeCurrent( hDC, hRC ) )
        throw runtime_error( "no OpenGL context" );

    auto glGenFramebuffers = (PFNGENFB)wglGetProcAddress( "glGenFramebuffers" );
    auto glBindFramebuffer = (PFNBINDFB)wglGetProcAddress( "glBindFramebuffer" );
    auto glFramebufferTexture2D = (PFNFBTEX)wglGetProcAddress( "glFramebufferTexture2D" );
    auto glCheckFramebufferStatus = (PFNCHECKFB)wglGetProcAddress( "glCheckFramebufferStatus" );
    if( !glGenFramebuffers || !glBindFramebuffer || !glFramebufferTexture2D || !glCheckFramebufferStatus )
        throw runtime_error( "OpenGL framebuffer objects are missing" );

    // float render target: texture in a framebuffer object
    GLuint rtTex = 0, fbo = 0;
    glGenTextures( 1, &rtTex );
    glBindTexture( GL_TEXTURE_2D, rtTex );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, W, H, 0, GL_RGBA, GL_FLOAT, nullptr );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glGenFramebuffers( 1, &fbo );
    glBindFramebuffer( GL_FRAMEBUFFER, fbo );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rtTex, 0 );
    if( GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus( GL_FRAMEBUFFER ) )
        throw runtime_error( "the float framebuffer is not complete" );
    glViewport( 0, 0, W, H );

    Row row;
    {
        VWB warper( nullptr, nullptr, nullptr, "Display1" );
        prepare( warper.get(), opt.vwf, false );
        if( VWB_ERROR err = warper.Init(); VWB_ERROR_NONE != err )
            throw runtime_error( string( "Init failed: " ) + VWB::GetErrorCStr( err ) + ". See VIOSOWarpBlend.log." );
        fill( row, warper.get(), headers );
        int const cw = row.rectWidth, ch = warper.get().optimalRect.bottom - warper.get().optimalRect.top;

        vector<float> src = makeSource( cw, ch, opt.mirror );
        GLuint srcTex = 0;
        glGenTextures( 1, &srcTex );
        glBindTexture( GL_TEXTURE_2D, srcTex );
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RG32F, cw, ch, 0, GL_RG, GL_FLOAT, src.data() );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glBindTexture( GL_TEXTURE_2D, 0 );

        glClearColor( 0, 0, 0, 0 );
        glClear( GL_COLOR_BUFFER_BIT );
        if( VWB_ERROR err = warper.Render( (VWB_param)(uintptr_t)srcTex, VWB_STATEMASK_STANDARD ); VWB_ERROR_NONE != err )
            throw runtime_error( string( "Render failed: " ) + VWB::GetErrorCStr( err ) );

        int const y = opt.row >= 0 ? opt.row : H / 2;
        vector<float> px( size_t( W ) * 4 );
        glReadPixels( 0, y, W, 1, GL_RGBA, GL_FLOAT, px.data() ); // GL counts rows from the bottom, so does the render target
        row.x.resize( W );
        for( int p = 0; p != W; p++ )
            row.x[p] = px[size_t( p ) * 4];
        glDeleteTextures( 1, &srcTex );
    }
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    glDeleteTextures( 1, &rtTex );
    wglMakeCurrent( nullptr, nullptr );
    wglDeleteContext( hRC );
    ReleaseDC( hWnd, hDC );
    DestroyWindow( hWnd );
    return row;
}

// ---- Direct3D 12 --------------------------------------------------------------------------------------------------

static Row measureDx12( Options const& opt, vector<VWB_WarpBlendHeader> const& headers )
{
    int const W = (int)headers[0].header.width, H = (int)headers[0].header.height;
    ComPtr<ID3D12Device> dev;
    check( D3D12CreateDevice( nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( &dev ) ), "D3D12CreateDevice" );
    D3D12_COMMAND_QUEUE_DESC qd{};
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ComPtr<ID3D12CommandQueue> queue;
    check( dev->CreateCommandQueue( &qd, IID_PPV_ARGS( &queue ) ), "create command queue" );
    ComPtr<ID3D12CommandAllocator> ca;
    check( dev->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS( &ca ) ), "create command allocator" );
    ComPtr<ID3D12GraphicsCommandList> cl;
    check( dev->CreateCommandList( 0, D3D12_COMMAND_LIST_TYPE_DIRECT, ca.Get(), nullptr, IID_PPV_ARGS( &cl ) ), "create command list" );

    auto const heap = []( D3D12_HEAP_TYPE t ) { return D3D12_HEAP_PROPERTIES{ t, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 }; };
    auto const buffer = []( UINT64 size ) {
        D3D12_RESOURCE_DESC d{};
        d.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        d.Width = size;
        d.Height = d.DepthOrArraySize = d.MipLevels = 1;
        d.SampleDesc.Count = 1;
        d.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        return d;
    };
    auto const texture = []( UINT w, UINT h, DXGI_FORMAT f, D3D12_RESOURCE_FLAGS flags ) {
        D3D12_RESOURCE_DESC d{};
        d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        d.Width = w;
        d.Height = h;
        d.DepthOrArraySize = d.MipLevels = 1;
        d.Format = f;
        d.SampleDesc.Count = 1;
        d.Flags = flags;
        return d;
    };
    auto const barrier = []( ID3D12Resource* r, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to ) {
        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition.pResource = r;
        b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        b.Transition.StateBefore = from;
        b.Transition.StateAfter = to;
        return b;
    };

    // the float render target and the descriptor for it
    D3D12_HEAP_PROPERTIES hDefault = heap( D3D12_HEAP_TYPE_DEFAULT ), hUpload = heap( D3D12_HEAP_TYPE_UPLOAD ), hReadback = heap( D3D12_HEAP_TYPE_READBACK );
    D3D12_RESOURCE_DESC rtd = texture( W, H, DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET );
    ComPtr<ID3D12Resource> rt;
    check( dev->CreateCommittedResource( &hDefault, D3D12_HEAP_FLAG_NONE, &rtd, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS( &rt ) ), "create render target" );
    D3D12_DESCRIPTOR_HEAP_DESC hd{ D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1 };
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    check( dev->CreateDescriptorHeap( &hd, IID_PPV_ARGS( &rtvHeap ) ), "create descriptor heap" );
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    dev->CreateRenderTargetView( rt.Get(), nullptr, rtv );

    VWB warper( nullptr, queue.Get(), nullptr, "Display1" );
    prepare( warper.get(), opt.vwf, true );
    warper.get().D3D12RTVF = DXGI_FORMAT_R32G32B32A32_FLOAT; // the pipeline state is made for this format
    if( VWB_ERROR err = warper.Init(); VWB_ERROR_NONE != err )
        throw runtime_error( string( "Init failed: " ) + VWB::GetErrorCStr( err ) + ". See VIOSOWarpBlend.log." );

    Row row;
    fill( row, warper.get(), headers );
    UINT const cw = row.rectWidth, ch = warper.get().optimalRect.bottom - warper.get().optimalRect.top;

    // the source texture, uploaded with the first commands
    vector<float> src = makeSource( cw, ch, opt.mirror );
    D3D12_RESOURCE_DESC sd = texture( cw, ch, DXGI_FORMAT_R32G32_FLOAT, D3D12_RESOURCE_FLAG_NONE );
    ComPtr<ID3D12Resource> source;
    check( dev->CreateCommittedResource( &hDefault, D3D12_HEAP_FLAG_NONE, &sd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS( &source ) ), "create source texture" );
    UINT const pitch = ( cw * 8 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1 ) / D3D12_TEXTURE_DATA_PITCH_ALIGNMENT * D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
    D3D12_RESOURCE_DESC ud = buffer( UINT64( pitch ) * ch );
    ComPtr<ID3D12Resource> upload;
    check( dev->CreateCommittedResource( &hUpload, D3D12_HEAP_FLAG_NONE, &ud, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS( &upload ) ), "create upload buffer" );
    {
        D3D12_RANGE none{ 0, 0 };
        char* p = nullptr;
        check( upload->Map( 0, &none, (void**)&p ), "map upload buffer" );
        for( UINT j = 0; j != ch; j++ )
            memcpy( p + size_t( j ) * pitch, &src[size_t( j ) * cw * 2], size_t( cw ) * 8 );
        upload->Unmap( 0, nullptr );
    }
    D3D12_TEXTURE_COPY_LOCATION dst{}, from{};
    dst.pResource = source.Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    from.pResource = upload.Get();
    from.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    from.PlacedFootprint.Footprint = { DXGI_FORMAT_R32G32_FLOAT, cw, ch, 1, pitch };
    cl->CopyTextureRegion( &dst, 0, 0, 0, &from, nullptr );
    D3D12_RESOURCE_BARRIER toRead = barrier( source.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );
    cl->ResourceBarrier( 1, &toRead );

    // render into the float target, with our own command list as the example does
    float const black[4]{ 0, 0, 0, 0 };
    cl->ClearRenderTargetView( rtv, black, 0, nullptr );
    VWB_D3D12_RENDERINPUT in{ source.Get(), rt.Get(), (UINT64)rtv.ptr, { 0, 0, 0, 0, 0, 0 }, cl.Get() };
    if( VWB_ERROR err = warper.Render( &in, VWB_STATEMASK_DEFAULT_D3D12 ); VWB_ERROR_NONE != err )
        throw runtime_error( string( "Render failed: " ) + VWB::GetErrorCStr( err ) );

    // one row into a read-back buffer
    UINT const rowBytes = W * 16; // a multiple of 256 for the usual widths; checked below
    if( rowBytes % D3D12_TEXTURE_DATA_PITCH_ALIGNMENT )
        throw runtime_error( "the width of the channel must be a multiple of 16 pixels for the read-back" );
    D3D12_RESOURCE_DESC bd = buffer( rowBytes );
    ComPtr<ID3D12Resource> readback;
    check( dev->CreateCommittedResource( &hReadback, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS( &readback ) ), "create read-back buffer" );
    D3D12_RESOURCE_BARRIER toCopy = barrier( rt.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE );
    cl->ResourceBarrier( 1, &toCopy );
    int const y = opt.row >= 0 ? opt.row : H / 2;
    D3D12_TEXTURE_COPY_LOCATION rbDst{}, rbSrc{};
    rbDst.pResource = readback.Get();
    rbDst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    rbDst.PlacedFootprint.Footprint = { DXGI_FORMAT_R32G32B32A32_FLOAT, UINT( W ), 1, 1, rowBytes };
    rbSrc.pResource = rt.Get();
    rbSrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_BOX box{ 0, UINT( y ), 0, UINT( W ), UINT( y + 1 ), 1 };
    cl->CopyTextureRegion( &rbDst, 0, 0, 0, &rbSrc, &box );
    check( cl->Close(), "close command list" );
    ID3D12CommandList* lists[]{ cl.Get() };
    queue->ExecuteCommandLists( 1, lists );

    ComPtr<ID3D12Fence> fence;
    check( dev->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &fence ) ), "create fence" );
    HANDLE ev = CreateEventA( nullptr, FALSE, FALSE, nullptr );
    check( queue->Signal( fence.Get(), 1 ), "signal fence" );
    check( fence->SetEventOnCompletion( 1, ev ), "wait for the GPU" );
    WaitForSingleObject( ev, INFINITE );
    CloseHandle( ev );

    D3D12_RANGE all{ 0, rowBytes };
    float* p = nullptr;
    check( readback->Map( 0, &all, (void**)&p ), "map read-back buffer" );
    row.x.resize( W );
    for( int i = 0; i != W; i++ )
        row.x[i] = p[i * 4];
    readback->Unmap( 0, nullptr );
    return row;
}

// ---- main ---------------------------------------------------------------------------------------------------------

static int usage()
{
    fprintf( stderr, "usage: WarpPrecision file.vwf [--api dx11|dx12|gl|all] [--row N] [--csv file] [--mirror]\n" );
    return 2;
}

int main( int argc, char** argv )
{
    setvbuf( stdout, nullptr, _IONBF, 0 );
    Options opt;
    for( int i = 1; i < argc; i++ )
    {
        string a = argv[i];
        if( "--api" == a && i + 1 < argc )
            opt.api = argv[++i];
        else if( "--row" == a && i + 1 < argc )
            opt.row = atoi( argv[++i] );
        else if( "--mirror" == a )
            opt.mirror = true;
        else if( "--csv" == a && i + 1 < argc )
            opt.csv = argv[++i];
        else if( a.rfind( "--", 0 ) != 0 && opt.vwf.empty() )
            opt.vwf = a;
        else
            return usage();
    }
    if( opt.vwf.empty() )
        return usage();
    try
    {
        vector<VWB_WarpBlendHeader> headers = VWB::VwfInfo( opt.vwf.c_str() );
        if( headers.size() != 1 )
            throw runtime_error( "The file must hold exactly one channel (made by MakeTestVwf make --size ... --content ...)." );
        printf( "%s: channel %s, %dx%d pixels\n", opt.vwf.c_str(), headers[0].header.name, (int)headers[0].header.width, (int)headers[0].header.height );
        auto const undoMirror = [&]( Row r ) {
            if( opt.mirror )
                for( float& x : r.x )
                    x = float( r.rectWidth ) - x;
            return r;
        };
        if( "dx11" == opt.api || "all" == opt.api )
            evaluate( "DX11", undoMirror( measureDx11( opt, headers ) ), opt.csv );
        if( "dx12" == opt.api || "all" == opt.api )
            evaluate( "DX12", undoMirror( measureDx12( opt, headers ) ), opt.csv );
        if( "gl" == opt.api || "all" == opt.api )
            evaluate( "GL", undoMirror( measureGl( opt, headers ) ), opt.csv );
    }
    catch( exception& e )
    {
        fprintf( stderr, "%s\n", e.what() );
        return 1;
    }
    catch( VWB_ERROR e )
    {
        fprintf( stderr, "VWB_ERROR %d\n", (int)e );
        return 1;
    }
    return 0;
}
