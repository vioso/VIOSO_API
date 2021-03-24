#include "DX11Program.h"
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d3dcompiler.lib" )
using namespace std;

//--------------------------------------------------------------------------------------
// RenderTarget
//--------------------------------------------------------------------------------------
const FLOAT RenderTarget::s_black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
const FLOAT RenderTarget::s_sky[4] = { 0.59f, 0.86f, 1.0f, 1.0f };

void RenderTarget::setAsRTto( ID3D11DeviceContext* ctx )
{
    ctx->OMSetRenderTargets( 1, &m_rtv.p, m_dsv );
}

void RenderTarget::clear( ID3D11DeviceContext* ctx, const FLOAT color[4] )
{
    ctx->ClearRenderTargetView( m_rtv, color );
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

void RenderTarget::resize( ID3D11Device*, IDXGISwapChain* ) {}

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
            throw std::exception( "failed to create depth stencil texture" );
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

void GFXPipeline::addRenderer( std::shared_ptr<Renderer> renderer )
{
    m_renderers.push_back( renderer );
}

void GFXPipeline::preRender()
{
    m_rt->setAsRTto( m_ic );
    m_rt->clear( m_ic );
    m_ic->RSSetViewports( 1, &m_vp );
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
        if( 27 == wParam )
            PostQuitMessage( 0 );
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

OutputWindow::OutputWindow( HINSTANCE hInstance, LPCTSTR windowName, int x, int y, int width, int height, int nCmdShow, DXGI_SWAP_EFFECT effect, int bufferCount, int createDeviceFlags, bool withDepth )
: m_hWnd( 0 )
, m_driverType( D3D_DRIVER_TYPE_NULL )
, m_featureLevel( D3D_FEATURE_LEVEL_11_0 )
{

    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, (LPCTSTR)_T( "directx.ico" ) );
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = _T( "OutputWindowClass" );
    wcex.hIconSm = LoadIcon( wcex.hInstance, (LPCTSTR)_T( "directx.ico" ) );
    if( !RegisterClassEx( &wcex ) )
        throw exception( "failed to register window class" );

    // Create window
    RECT rc = { x, y, x + width, y + height };
    if( 0 == width )
    {
        MONITORINFO mi = { 0 }; mi.cbSize = sizeof( mi );
        static const POINT _p0 = { x,y };
        if( GetMonitorInfo( MonitorFromPoint( _p0, MONITOR_DEFAULTTONULL ), &mi ) )
        {
            rc = mi.rcMonitor;
        }
        AdjustWindowRect( &rc, WS_POPUPWINDOW, FALSE );
    }
    m_hWnd = CreateWindow(
        _T( "OutputWindowClass" ), windowName, WS_POPUPWINDOW,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, hInstance, NULL );
    if( ! m_hWnd )
        throw exception( "failed to create window" );

    SetWindowLongPtr( m_hWnd, GWLP_USERDATA, (LONG_PTR)this );

    ShowWindow( m_hWnd, nCmdShow );

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
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

    HRESULT hr;
    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        m_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, m_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &m_sc, &m_dev, &m_featureLevel, &m_ic );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        throw exception( "failed to create D3D11 device", hr );

    m_rt = make_shared<BackBuffer>( m_dev, m_sc, withDepth );
    m_vp = m_rt->getFullViewport();
    m_mProjection = XMMatrixPerspectiveFovLH( XM_PIDIV2, m_vp.Width / m_vp.Height, 0.25f, 1024.0f );
}

void OutputWindow::postRender()
{
    __super::postRender();
    m_sc->Present( 1, 0 );
}

//--------------------------------------------------------------------------------------
// RenderToTexture
//--------------------------------------------------------------------------------------

RenderToTexture::RenderToTexture( ID3D11Device* dev, ID3D11DeviceContext* ic, int width, int height, DXGI_FORMAT format, bool withDepth )
{
    if( nullptr == dev )
        throw std::exception( "dev must not be NULL" );
	dev->QueryInterface( &m_dev );
    if( nullptr == ic )
        throw std::exception( "ic must not be NULL" );
    ic->QueryInterface( &m_ic );
    m_rt = make_shared<RenderTexture>( dev, width, height, format, withDepth );
    m_vp = m_rt->getFullViewport();
}

