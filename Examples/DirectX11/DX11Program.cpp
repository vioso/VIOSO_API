#include "DX11Program.h"
#include <dxgi1_2.h>
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib")
#pragma comment( lib, "d3dcompiler.lib" )
#ifdef _DEBUG
GUID const DXGI_DEBUG_D3D11 = { 0x4b99317b, 0xac39, 0x4aa6, 0xbb, 0xb, 0xba, 0xa0, 0x47, 0x84, 0x79, 0x8f };

#endif
#include <array>
using namespace std;


const UINT SimpleVertex::stride = sizeof(SimpleVertex);
const UINT SimpleVertexTex::stride = sizeof(SimpleVertexTex);
const D3D11_INPUT_ELEMENT_DESC SimpleVertex::layout[2] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
const D3D11_INPUT_ELEMENT_DESC SimpleVertexTex::layout[3] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

//--------------------------------------------------------------------------------------
// RenderTarget
//--------------------------------------------------------------------------------------
const FLOAT RenderTarget::s_black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
const FLOAT RenderTarget::s_sky[4] = { 0.59f, 0.86f, 1.0f, 1.0f };

void RenderTarget::setAsRTto( ID3D11DeviceContext* ctx )
{
    ctx->OMSetRenderTargets( 1, &m_rtv.p, m_dsv );
}

void RenderTarget::clear( ID3D11DeviceContext* ctx )
{
    ctx->ClearRenderTargetView( m_rtv, m_clearColor );
    if( m_dsv )
        ctx->ClearDepthStencilView( m_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
}

D3D11_VIEWPORT RenderTarget::getFullViewport() const
{
    D3D11_VIEWPORT vp = { 0 };
    CComPtr<ID3D11Resource> res;
    m_rtv->GetResource( &res );
    CComPtr<ID3D11Texture2D> tex;
    if( SUCCEEDED( res->QueryInterface( &tex ) ) )
    {
        D3D11_TEXTURE2D_DESC desc = { 0 };
        tex->GetDesc( &desc );
        vp.Width = (FLOAT)desc.Width;
        vp.Height = (FLOAT)desc.Height;
        vp.MaxDepth = 1.0f;
    }
    return vp;
}

void RenderTarget::resize( ID3D11Device*, IDXGISwapChain* ) {
    // TODO
}

//--------------------------------------------------------------------------------------
// BackBuffer
//--------------------------------------------------------------------------------------

void BackBuffer::initBuffers( ID3D11Device* dev, IDXGISwapChain* sc, bool withDepth )
{
    if( nullptr == sc )
        throw std::exception( "sc parameter must not be NULL" );
    D3D11_TEXTURE2D_DESC desc = { 0 };
    CComPtr<ID3D11Texture2D> res;
    if( FAILED( sc->GetBuffer( 0, __uuidof( res ), (void**)&res ) ) )
        throw exception( "failed to get buffer from swapchain" );

    res->GetDesc( &desc );

    if( FAILED( dev->CreateRenderTargetView( res, nullptr, &m_rtv ) ) )
        throw std::exception( "failed to create render target view for render texture" );

    // Create the depth stencil view if specified
    if( withDepth )
    {
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        desc.MiscFlags = 0;
        CComPtr< ID3D11Texture2D > ds;
        if( FAILED( dev->CreateTexture2D( &desc, NULL, &ds ) ) )
            throw std::exception( "failed to create depth stencil texture" );
        if( FAILED( dev->CreateDepthStencilView( ds, nullptr, &m_dsv ) ) )
            throw std::exception( "failed to create depth stencil view" );
    }
}

BackBuffer::BackBuffer( ID3D11Device* dev, IDXGISwapChain* sc, bool withDepth )
{
    initBuffers( dev, sc, withDepth );
}

void BackBuffer::resize( ID3D11Device* dev, IDXGISwapChain* sc )
{
    m_rtv.Release();
    bool withDepth = nullptr != m_dsv;
    sc->ResizeBuffers( 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0 );
    initBuffers( dev, sc, withDepth );
}

    
//--------------------------------------------------------------------------------------
// RenderTexture
//--------------------------------------------------------------------------------------

RenderTexture::RenderTexture( ID3D11Device* dev, int width, int height, DXGI_FORMAT format, bool withDepth )
{
    if( nullptr == dev )
        throw std::exception( "dev parameter must not be NULL" );

    D3D11_TEXTURE2D_DESC desc = { 0 };
    desc.Width = width;
    desc.Height = height;
    desc.Format = format;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	if( FAILED( dev->CreateTexture2D( &desc, nullptr, &m_tex ) ) )
		throw std::exception( "failed to create render texture" );

	if( FAILED( dev->CreateRenderTargetView( m_tex, nullptr, &m_rtv ) ) )
		throw std::exception( "failed to create render target view for render texture" );

    D3D11_SHADER_RESOURCE_VIEW_DESC srDesc{};
    srDesc.Format = desc.Format;
    srDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
    srDesc.Texture2D.MipLevels = 1;
    srDesc.Texture2D.MostDetailedMip = 0;

    if( FAILED( dev->CreateShaderResourceView( m_tex, &srDesc, &m_srv ) ) )
        throw exception( "failed to create shader resource view" );

    // Create the depth stencil view if specified
    if( withDepth )
    {
		desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		CComPtr< ID3D11Texture2D > ds;
		if( FAILED( dev->CreateTexture2D( &desc, NULL, &ds ) ) )
			throw std::exception( "failed to create depth stencil texture" );

		if( FAILED( dev->CreateDepthStencilView( ds, nullptr, &m_dsv ) ) )
			throw std::exception( "failed to create depth stencil texture" );
    }
}

//--------------------------------------------------------------------------------------
// Renderer
//--------------------------------------------------------------------------------------
UINT64 Renderer::s_freeID = 0;

//--------------------------------------------------------------------------------------
// GFXPipeline
//--------------------------------------------------------------------------------------

const D3D11_RASTERIZER_DESC GFXPipeline::s_rasterDescWire{
    D3D11_FILL_WIREFRAME,
    D3D11_CULL_NONE,
    FALSE,
    0, 0.0f, 0.0f,
    TRUE, FALSE, FALSE, FALSE };
const D3D11_RASTERIZER_DESC GFXPipeline::s_rasterDescSolid{
    D3D11_FILL_SOLID,
    D3D11_CULL_BACK,
    FALSE,
    0, 0.0f, 0.0f,
    TRUE, FALSE, FALSE, FALSE };
const D3D11_RASTERIZER_DESC GFXPipeline::s_rasterDescSolidNoCull{
    D3D11_FILL_SOLID,
    D3D11_CULL_NONE,
    FALSE,
    0, 0.0f, 0.0f,
    TRUE, FALSE, FALSE, FALSE };
const D3D11_BLEND_DESC GFXPipeline::s_blendDescStd{ FALSE, FALSE, {{ FALSE, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, 0xF }} };


ID3D11Device* GFXPipeline::createDevice( int createDeviceFlags )
{
    ID3D11Device* dev = nullptr;
    D3D_DRIVER_TYPE const driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };

    D3D_FEATURE_LEVEL const featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };


    HRESULT hr = E_FAIL;
    for( UINT driverTypeIndex = 0; driverTypeIndex < ARRAYSIZE( driverTypes ); driverTypeIndex++ )
    {
        hr = D3D11CreateDevice( NULL, driverTypes[driverTypeIndex], NULL, createDeviceFlags, featureLevels, ARRAYSIZE( featureLevels ),
                                D3D11_SDK_VERSION, &dev, nullptr, nullptr );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        throw exception( "failed to create D3D11 device", hr );

    return dev;
}


GFXPipeline::GFXPipeline( ID3D11Device* dev, D3D11_RASTERIZER_DESC const& rd, D3D11_BLEND_DESC const& bd)
: m_dev( dev )
, m_mView{ XMMatrixIdentity() }
, m_mProjection{ XMMatrixPerspectiveFovLH( XM_PIDIV2, 16.0f / 9, 0.25f, 1024.0f ) }
, m_vp{}
{
    if( nullptr == dev )
        throw std::exception( "dev must not be NULL" );

    m_dev->GetImmediateContext( &m_ic );

    HRESULT hr = dev->CreateRasterizerState( &rd, &m_rs );
    if( FAILED( hr ) )
        throw exception( "failed to create rasterizer state", hr );

    hr = m_dev->CreateBlendState( &bd, &m_bs );
    if( FAILED( hr ) )
        throw exception( "failed to create blend state", hr );

}

GFXPipeline::~GFXPipeline()
{
    m_rt.reset();
    m_renderers.clear();
    m_rs.Release();
    m_bs.Release();
    m_ic.Release();

    #ifdef _DEBUG
    {
        CComPtr<IDXGIDebug> dbg;
        decltype( DXGIGetDebugInterface )* fn = nullptr;
        HMODULE h = GetModuleHandleA( "Dxgidebug.dll" );
        if( h )
            fn = (decltype( DXGIGetDebugInterface )*)GetProcAddress( h, "DXGIGetDebugInterface" );
        if( fn )
            fn( __uuidof( IDXGIDebug), (void**)&dbg );
        if( dbg )
            dbg->ReportLiveObjects( DXGI_DEBUG_D3D11, DXGI_DEBUG_RLO_DETAIL );
        m_dev.Release();
        OutputDebugStringA( "After release:\n" );
        if( dbg )
            dbg->ReportLiveObjects( DXGI_DEBUG_D3D11, DXGI_DEBUG_RLO_DETAIL );
    }
    #else //dev _DEBUG
        m_dev.Release();
    #endif //dev _DEBUG
}

GFXPipeline::GFXPipeline( int createDeviceFlags, D3D11_RASTERIZER_DESC const& rd, D3D11_BLEND_DESC const& bd )
    :GFXPipeline( createDevice( createDeviceFlags ), rd, bd )
{
}

bool GFXPipeline::SaveTex( ID3D11Device* dev, ID3D11DeviceContext* dc, LPCSTR path, ID3D11Texture2D* tex )
{
    bool ret = false;
    // create CPU accessable texture
    D3D11_TEXTURE2D_DESC descI = { 0 };
    D3D11_TEXTURE2D_DESC desc = { 0 };
    tex->GetDesc( &descI );
    fprintf( stdout, "Input texture dump desc:\n"
             "Width           = %i\n"
             "Height          = %i\n"
             "MipLevels       = %i\n"
             "ArraySize       = %i\n"
             "Format          = %i\n"
             "SampleDesc      = {Count = %i, Quality = %i}\n"
             "Usage           = %i\n"
             "BindFlags       = %i\n"
             "CPUAccessFlags  = %i\n"
             "MiscFlags       = %i\n"
             , descI.Width
             , descI.Height
             , descI.MipLevels
             , descI.ArraySize
             , descI.Format
             , descI.SampleDesc.Count
             , descI.SampleDesc.Quality
             , descI.Usage
             , descI.BindFlags
             , descI.CPUAccessFlags
             , descI.MiscFlags
    );


    desc.Format = descI.Format;
    desc.Width = descI.Width;
    desc.Height = descI.Height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

    CComPtr<ID3D11Texture2D> pTexMem = NULL;
    HRESULT hr = dev->CreateTexture2D( &desc, NULL, &pTexMem );
    if( SUCCEEDED( hr ) )
    {
        dc->CopyResource( pTexMem, tex );
        D3D11_MAPPED_SUBRESOURCE res = { 0 };
        if( SUCCEEDED( dc->Map( pTexMem, 0, D3D11_MAP_READ, 0, &res ) ) )
        {
            if( DXGI_FORMAT_R8G8B8A8_UNORM == desc.Format ||
                DXGI_FORMAT_R8G8B8A8_TYPELESS == desc.Format ||
                DXGI_FORMAT_R8G8B8A8_UNORM_SRGB == desc.Format ||
                DXGI_FORMAT_R8G8B8A8_UINT == desc.Format ||
                DXGI_FORMAT_R8G8B8A8_SNORM == desc.Format ||
                DXGI_FORMAT_R8G8B8A8_SINT == desc.Format
                )
            {
                const LONG pitch = 4 * desc.Width;
                BITMAPINFOHEADER hdr = { 0 };
                hdr.biSize = sizeof( hdr );
                hdr.biWidth = desc.Width;
                hdr.biHeight = -LONG( desc.Height );
                hdr.biPlanes = 1;
                hdr.biBitCount = 32;
                hdr.biSizeImage = pitch * desc.Height;

                BITMAPFILEHEADER fh = { 0 };
                fh.bfType = 'MB';
                fh.bfOffBits = sizeof( fh ) + hdr.biSize;
                fh.bfSize = fh.bfOffBits + hdr.biSizeImage;

                // swivel RGBA to BGRA
                unsigned char t = 0;
                const LONG padd = res.RowPitch - pitch;
                for( unsigned char* px = ( unsigned char* )res.pData, *pxE = px + res.RowPitch * desc.Height;
                     px != pxE; px += padd )
                {
                    for( const unsigned char* pxLE = px + pitch; px != pxLE; px += 4 )
                    {
                        t = px[0];
                        px[0] = px[2];
                        px[2] = t;
                    }
                }


                FILE* f = NULL;
                if( NO_ERROR == fopen_s( &f, path, "wb" ) )
                {
                    fwrite( &fh, sizeof( fh ), 1, f );
                    fwrite( &hdr, sizeof( hdr ), 1, f );
                    for( unsigned char* ln = ( unsigned char* )res.pData, *lnE = ln + res.RowPitch * desc.Height; ln != lnE; ln += res.RowPitch )
                        fwrite( ln, pitch, 1, f );
                    fclose( f );
                    ret = true;
                    fprintf( stdout, "Texture dumped to %s", path );
                }
                else
                {
                    fprintf( stdout, "Could not write file %s", path );
                }
            }
            else if( DXGI_FORMAT_R16G16B16A16_TYPELESS == desc.Format ||
                     DXGI_FORMAT_R16G16B16A16_UNORM == desc.Format ||
                     DXGI_FORMAT_R16G16B16A16_UINT == desc.Format ||
                     DXGI_FORMAT_R16G16B16A16_SNORM == desc.Format ||
                     DXGI_FORMAT_R16G16B16A16_SINT == desc.Format
                     )
            {
                const LONG pitch = 4 * desc.Width;

                BITMAPINFOHEADER hdr = { 0 };
                hdr.biSize = sizeof( hdr );
                hdr.biWidth = desc.Width;
                hdr.biHeight = -LONG( desc.Height );
                hdr.biPlanes = 1;
                hdr.biBitCount = 32;
                hdr.biSizeImage = desc.Height * pitch;

                BITMAPFILEHEADER fh = { 0 };
                fh.bfType = 'MB';
                fh.bfOffBits = sizeof( fh ) + hdr.biSize;
                fh.bfSize = fh.bfOffBits + hdr.biSizeImage;

                // swivel RGBA to BGRA
                unsigned char* pDst = new unsigned char[hdr.biSizeImage];
                unsigned char* pxD = pDst;
                for( UINT y = 0; y != desc.Height; y++ )
                {
                    const unsigned short* pxS = (unsigned short*)( ( (unsigned char*)res.pData ) + ptrdiff_t( y ) * res.RowPitch );
                    for( const unsigned short* pxLE = pxS + ptrdiff_t( 4 ) * desc.Width; pxS != pxLE; pxS += 4, pxD += 4 )
                    {
                        pxD[0] = unsigned char( pxS[1] >> 8 );
                        pxD[1] = unsigned char( pxS[2] >> 8 );
                        pxD[2] = unsigned char( pxS[0] >> 8 );
                        pxD[3] = unsigned char( pxS[3] >> 8 );
                    }
                }

                FILE* f = NULL;
                if( NO_ERROR == fopen_s( &f, path, "wb" ) )
                {
                    fwrite( &fh, sizeof( fh ), 1, f );
                    fwrite( &hdr, sizeof( hdr ), 1, f );
                    fwrite( res.pData, hdr.biSizeImage, 1, f );
                    fclose( f );
                    ret = true;
                    fprintf( stdout, "Texture dumped to %s", path );
                }
                else
                {
                    fprintf( stdout, "Could not write file %s", path );
                }

                delete[] pDst;
            }
            else
            {
                fprintf( stdout, "Could not dump texture unknown format %i.", desc.Format );
            }
            dc->Unmap( pTexMem, 0 );
        }
        else
        {
            fprintf( stdout, "Could not map dump texture." );
        }
    }
    else
    {
        fprintf( stdout, "Could not create dump texture (%08x).", hr );
    }
    return ret;
}

void GFXPipeline::addRenderer( std::shared_ptr<Renderer> renderer )
{
    m_renderers.push_back( renderer );
}

void GFXPipeline::preRender()
{
    m_rt->setAsRTto( m_ic );
    m_rt->clear( m_ic );
    m_ic->RSSetViewports( 1, &m_vp );
    m_ic->RSSetState( m_rs );
    m_ic->OMSetBlendState( m_bs, {}, 0xffffffff );
}

void GFXPipeline::render( XMMATRIX const& world )
{
    for( auto& renderer : m_renderers )
        renderer->render( m_ic, world, m_mView, m_mProjection );
}

void GFXPipeline::postRender()
{
}


//--------------------------------------------------------------------------------------
// OutputWindow
//--------------------------------------------------------------------------------------
ATOM OutputWindow::s_wndclass = 0;

LRESULT CALLBACK  OutputWindow::WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    OutputWindow* that = (OutputWindow*)GetWindowLongPtr( hWnd, GWLP_USERDATA );
    if( that )
        return that->wndProc( hWnd, msg, wParam, lParam );
    return DefWindowProc( hWnd, msg, wParam, lParam );
}

LRESULT OutputWindow::wndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( msg )
    {
    case WM_PAINT:
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    case WM_CHAR:
        switch( wParam )
        {
        case 27:
            PostQuitMessage( 0 );
            break;
        default:
            return DefWindowProc( hWnd, msg, wParam, lParam );
        }
        break;
    case WM_SIZE:
        if( m_dev && m_sc )
            m_rt->resize( m_dev, m_sc );
        break;
    default:
        return DefWindowProc( hWnd, msg, wParam, lParam );
    }

    return 0;
}

#include <stdarg.h>
const std::string toString( const char* fmt, ... )
{
    va_list params;
    va_start( params, fmt );
    std::string s( size_t( _vscprintf( fmt, params ) ) + 1, '\0' );
    vsprintf_s( s.data(), s.length(), fmt, params );
    va_end( params );
    return s;
}

void rotOnSpot( XMMATRIX& m, XMVECTOR const& axis, float angle )
{
    m *= XMMatrixRotationNormal( axis, angle );
   
#ifdef _DEBUG
    OutputDebugStringA( toString( 
        "mWorld:\n"
        " %-05.3f % -05.3f % -05.3f % -05.3f\n"
        " %-05.3f % -05.3f % -05.3f % -05.3f\n"
        " %-05.3f % -05.3f % -05.3f % -05.3f\n"
        " %-05.3f % -05.3f % -05.3f % -05.3f\n"
        ,
        m.r[0].m128_f32[0], m.r[0].m128_f32[1], m.r[0].m128_f32[2], m.r[0].m128_f32[3],
        m.r[1].m128_f32[0], m.r[1].m128_f32[1], m.r[1].m128_f32[2], m.r[1].m128_f32[3],
        m.r[2].m128_f32[0], m.r[2].m128_f32[1], m.r[2].m128_f32[2], m.r[2].m128_f32[3],
        m.r[3].m128_f32[0], m.r[3].m128_f32[1], m.r[3].m128_f32[2], m.r[3].m128_f32[3]
        ).c_str() );
#endif //def _DEBUG
}

void translate( XMMATRIX& m, XMVECTOR const& axis, float dist )
{
    m.r[3].m128_f32[0] += axis.m128_f32[0] * dist;
    m.r[3].m128_f32[1] += axis.m128_f32[1] * dist;
    m.r[3].m128_f32[2] += axis.m128_f32[2] * dist;

#ifdef _DEBUG
    OutputDebugStringA( toString(
        "mWorld:\n"
        " %-05.3f % -05.3f % -05.3f % -05.3f\n"
        " %-05.3f % -05.3f % -05.3f % -05.3f\n"
        " %-05.3f % -05.3f % -05.3f % -05.3f\n"
        " %-05.3f % -05.3f % -05.3f % -05.3f\n"
        ,
        m.r[0].m128_f32[0], m.r[0].m128_f32[1], m.r[0].m128_f32[2], m.r[0].m128_f32[3],
        m.r[1].m128_f32[0], m.r[1].m128_f32[1], m.r[1].m128_f32[2], m.r[1].m128_f32[3],
        m.r[2].m128_f32[0], m.r[2].m128_f32[1], m.r[2].m128_f32[2], m.r[2].m128_f32[3],
        m.r[3].m128_f32[0], m.r[3].m128_f32[1], m.r[3].m128_f32[2], m.r[3].m128_f32[3]
    ).c_str() );
#endif //def _DEBUG
}

void OutputWindow::preRender()
{
    BYTE keys[256];
    if( GetKeyboardState( keys ) )
    {
		if( 0x80 & keys['W'] ) // forward
			translate( m_mWorld, XMVectorSet( 0, 0, 1, 1 ), -0.2f );
		if( 0x80 & keys['S'] ) // backward
			translate( m_mWorld, XMVectorSet( 0, 0, 1, 1 ), 0.2f );
		if( 0x80 & keys['A'] ) // strave left
			translate( m_mWorld, XMVectorSet( 1, 0, 0, 1 ), 0.2f );
		if( 0x80 & keys['D'] ) // strave right
			translate( m_mWorld, XMVectorSet( 1, 0, 0, 1 ), -0.2f );
		if( 0x80 & keys['R'] ) // fly up
            translate( m_mWorld, XMVectorSet( 0, 1, 0, 1 ), -0.2f );
        if( 0x80 & keys['F'] ) // fly down
			translate( m_mWorld, XMVectorSet( 0, 1, 0, 1 ), 0.2f );
		if( 0x80 & keys[VK_LEFT] ) // turn left
            rotOnSpot( m_mWorld, XMVectorSet( 0, 1, 0, 1 ), 0.1f );
        if( 0x80 & keys[VK_RIGHT] ) // turn right
            rotOnSpot( m_mWorld, XMVectorSet( 0, 1, 0, 1 ), -0.1f );
        if( 0x80 & keys[VK_UP] ) // turn up
            rotOnSpot( m_mWorld, XMVectorSet( 1, 0, 0, 1 ), 0.1f );
        if( 0x80 & keys[VK_DOWN] ) // turn down
            rotOnSpot( m_mWorld, XMVectorSet( 1, 0, 0, 1 ), -0.1f );
        if( 0x80 & keys[VK_OEM_PERIOD] ) // roll cw
            rotOnSpot( m_mWorld, XMVectorSet( 0, 0, 1, 1 ), 0.1f );
        if( 0x80 & keys[VK_OEM_COMMA] ) // roll ccw
            rotOnSpot( m_mWorld, XMVectorSet( 0, 0, 1, 1 ), -0.1f );

    }
    __super::preRender();
}

HWND OutputWindow::createWindow( HINSTANCE hInstance, LPCTSTR windowName, int x, int y, int width, int height )
{
    if( !s_wndclass )
    {
        // Register class
        WNDCLASSEX wcex = {
            sizeof( WNDCLASSEX ),
            CS_HREDRAW | CS_VREDRAW,
            WndProc,
            0,0,
            hInstance,
            LoadIcon( hInstance, (LPCTSTR)_T( "directx.ico" ) ),
            LoadCursor( NULL, IDC_ARROW ),
            (HBRUSH)( COLOR_WINDOW + 1 ),
            NULL,
            _T( "OutputWindowClass" ),
            LoadIcon( hInstance, (LPCTSTR)_T( "directx.ico" ) )
        };
        s_wndclass = RegisterClassEx( &wcex );
        if( !s_wndclass )
            throw exception( "failed to register window class" );
    }

    // Create window
    DWORD dwExStyle = WS_EX_APPWINDOW;								// Window Extended Style
    DWORD dwStyle = WS_POPUP;										// Windows Style
    RECT rc = { x, y, x + width, y + height };
    if( 0 == width )
    {
        MONITORINFO mi = { 0 }; mi.cbSize = sizeof( mi );
        const POINT _p0 = { x,y };
        if( GetMonitorInfo( MonitorFromPoint( _p0, MONITOR_DEFAULTTONULL ), &mi ) )
        {
            rc = mi.rcMonitor;
        }
        AdjustWindowRectEx( &rc, dwStyle, FALSE, dwExStyle );
    }
    HWND hWnd = CreateWindowEx(
        dwExStyle, _T( "OutputWindowClass" ), windowName, dwStyle,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInstance, NULL );
    if( !hWnd )
        throw exception( "failed to create window" );

    return hWnd;
}

OutputWindow::OutputWindow( HINSTANCE hInstance, LPCTSTR windowName, int x, int y, int width, int height, int nCmdShow, DXGI_SWAP_EFFECT effect, int bufferCount, int createDeviceFlags, bool withDepth )
: m_hWnd( createWindow( hInstance, windowName, x, y, width, height ) )
, GFXPipeline( createDeviceFlags, s_rasterDescSolid )
, m_mWorld( XMMatrixIdentity() )
{
    SetWindowLongPtr( m_hWnd, GWLP_USERDATA, (LONG_PTR)this );
    ShowWindow( m_hWnd, nCmdShow );

    DXGI_SWAP_CHAIN_DESC sd;
    memset( &sd, 0, sizeof( sd ) );
    sd.BufferCount = bufferCount;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = effect;

    CComPtr<IDXGIFactory> fac;
    HRESULT hr = CreateDXGIFactory( __uuidof( IDXGIFactory), (void**)&fac );
    
    if( FAILED( hr ) )
        throw exception( "failed to create factory", hr );

    hr = fac->CreateSwapChain( m_dev, &sd, &m_sc );

    if( FAILED( hr ) )
        throw exception( "failed to create swapchain", hr );

    m_rt = make_shared<BackBuffer>( m_dev, m_sc, withDepth );
    m_vp = m_rt->getFullViewport();
}

void OutputWindow::postRender()
{
    __super::postRender();
    m_sc->Present( 1, 0 );
}

//--------------------------------------------------------------------------------------
// RenderToTexture
//--------------------------------------------------------------------------------------

RenderToTexture::RenderToTexture( ID3D11Device* dev, int width, int height, DXGI_FORMAT format, bool withDepth )
    : GFXPipeline( dev )
{
    m_rt = make_shared<RenderTexture>( dev, width, height, format, withDepth );
    m_vp = m_rt->getFullViewport();
}

size_t RenderToTexture::copyToBuffer( void* buff, size_t nBytes, D3D11_TEXTURE2D_DESC* pDesc ) const
{
    size_t s = 0;

    // get desc
    D3D11_TEXTURE2D_DESC descI = { 0 };
    auto tex = dynamic_cast< RenderTexture* >( m_rt.get() )->getTexture();
    tex->GetDesc( &descI );
    // get bytes per pixel
    size_t nBPP = 0;
    if( DXGI_FORMAT_R32G32B32A32_TYPELESS == descI.Format ||
        DXGI_FORMAT_R32G32B32A32_FLOAT == descI.Format ||
        DXGI_FORMAT_R32G32B32A32_UINT == descI.Format ||
        DXGI_FORMAT_R32G32B32A32_SINT == descI.Format
        )
    {
        nBPP = 16;
    }
    else if( DXGI_FORMAT_R8G8B8A8_UNORM == descI.Format ||
             DXGI_FORMAT_R8G8B8A8_TYPELESS == descI.Format ||
             DXGI_FORMAT_R8G8B8A8_UNORM_SRGB == descI.Format ||
             DXGI_FORMAT_R8G8B8A8_UINT == descI.Format ||
             DXGI_FORMAT_R8G8B8A8_SNORM == descI.Format ||
             DXGI_FORMAT_R8G8B8A8_SINT == descI.Format
             )
    {
        nBPP = 4;
    }
    else if( DXGI_FORMAT_R16G16B16A16_TYPELESS == descI.Format ||
             DXGI_FORMAT_R16G16B16A16_UNORM == descI.Format ||
             DXGI_FORMAT_R16G16B16A16_UINT == descI.Format ||
             DXGI_FORMAT_R16G16B16A16_SNORM == descI.Format ||
             DXGI_FORMAT_R16G16B16A16_SINT == descI.Format
             )
    {
        nBPP = 8;
    }
    else
    {
        return 0;
    }
    // get total size
    s = nBPP * descI.Width * descI.Height;

    // create desc for CPU accessable texture
    D3D11_TEXTURE2D_DESC desc = { 0 };
    desc.Format = descI.Format;
    desc.Width = descI.Width;
    desc.Height = descI.Height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    // copy desc
    if( pDesc )
        memcpy( pDesc, &desc, sizeof( *pDesc ) );

    // return if buffer is not set
    if( !buff )
        return s;
    // or not big enough, which is an error
    if( nBytes < s )
        return 0;

    // create CPU accessable texture
    ID3D11Texture2D* pTexMem = NULL;
    HRESULT hr = m_dev->CreateTexture2D( &desc, NULL, &pTexMem );
    if( SUCCEEDED( hr ) )
    {
        // map texture
        m_ic->CopyResource( pTexMem, tex );
        D3D11_MAPPED_SUBRESOURCE res = { 0 };
        if( SUCCEEDED( m_ic->Map( pTexMem, 0, D3D11_MAP_READ, 0, &res ) ) )
        {
            // copy content
            for( UINT y = 0; y != desc.Height; y++ )
            {
                memcpy( ( char* )buff + nBPP * y * desc.Width, ( char* )res.pData + y * res.RowPitch, nBPP * desc.Width );
            }
            m_ic->Unmap( pTexMem, 0 );
        }
        else
        {
            return 0;
        }
        pTexMem->Release();
    }
    return s;
}

