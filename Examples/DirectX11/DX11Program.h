#pragma once
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <tchar.h>
#include <Windows.h>
#include <atlbase.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
using namespace DirectX;

#include <memory>
#include <vector>
#include <string>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
    static const UINT stride;
    static const D3D11_INPUT_ELEMENT_DESC layout[2];
};

struct SimpleVertexTex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
    XMFLOAT2 Tex;
    //SimpleVertexTex( float x = 0.0f, float y = 0.0f, float z = 0.0f, float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f, float u = 0.0f, float v = 0.0f )
    //{
    //    Pos.x = x;
    //    Pos.y = y;
    //    Pos.z = z;
    //    Color.x = r;
    //    Color.y = g;
    //    Color.z = b;
    //    Color.w = a;
    //    Tex.x = u;
    //    Tex.y = v;
    //}
    static const UINT stride;
    static const D3D11_INPUT_ELEMENT_DESC layout[3];
};

struct VSConstantBufferA
{
    XMMATRIX mWorld;
    XMMATRIX mView;
    XMMATRIX mProjection;
};

struct VSConstantBufferB
{
    XMMATRIX mWorld;
    XMMATRIX mModel;
    XMMATRIX mView;
    XMMATRIX mProjection;
};

//--------------------------------------------------------------------------------------
// Classes
//--------------------------------------------------------------------------------------

class RenderTarget
{
public:
    static const FLOAT s_black[4];
    static const FLOAT s_sky[4];
protected:
    CComPtr< ID3D11RenderTargetView > m_rtv;
    CComPtr< ID3D11DepthStencilView > m_dsv;
    FLOAT m_clearColor[4] = { 0, 0, 0, 1 };
public:

    virtual void setAsRTto( ID3D11DeviceContext* ctx );
    virtual void clear( ID3D11DeviceContext* ctx );
    virtual void resize( ID3D11Device*, IDXGISwapChain* );

    D3D11_VIEWPORT getFullViewport() const;
    void setClearColor( const FLOAT (& clearColor)[4] ) { memcpy( m_clearColor, clearColor, sizeof( m_clearColor ) ); }
};

class BackBuffer : public RenderTarget
{
protected:
    void initBuffers( ID3D11Device* dev, IDXGISwapChain* sc, bool withDepth = false );
public:
    BackBuffer( ID3D11Device* dev, IDXGISwapChain* sc, bool withDepth = false );
    virtual void resize( ID3D11Device* dev, IDXGISwapChain* sc );
};

class RenderTexture : public RenderTarget
{
protected:
    CComPtr< ID3D11Texture2D > m_tex;
public:
    RenderTexture( ID3D11Device* dev, int width, int height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, bool withDepth = false );

    ID3D11Resource* getResource()
    {
        return m_tex.p;
    }
};

class Renderer
{
protected:
    UINT64 m_id;
    static UINT64 s_freeID;
public:
    Renderer() : m_id( ++s_freeID ) {}
    virtual void render( ID3D11DeviceContext* ctx, XMMATRIX const& world, XMMATRIX const& view, XMMATRIX const& projection ) = 0;
};


class GFXPipeline
{
public: 
    // Turn off culling, so we see the front and back of the triangle
    static const D3D11_RASTERIZER_DESC s_rasterDescWire;
    static const D3D11_RASTERIZER_DESC s_rasterDescSolid;
    static const D3D11_BLEND_DESC s_blendDescStd;

protected:
    CComPtr<ID3D11Device> m_dev;
    CComPtr<ID3D11DeviceContext> m_ic;

    std::vector< std::shared_ptr< Renderer > > m_renderers;
    CComPtr<ID3D11RasterizerState> m_rs;
    CComPtr<ID3D11BlendState> m_bs;
    XMMATRIX m_mView;
    XMMATRIX m_mProjection;

    std::shared_ptr< RenderTarget > m_rt;
    D3D11_VIEWPORT m_vp;

public:
    static bool SaveTex( ID3D11Device* dev, ID3D11DeviceContext* dc, LPCSTR path, ID3D11Texture2D* tex ) ;
    static ID3D11Device* createDevice( int createDeviceFlags );

    GFXPipeline( ID3D11Device* dev, D3D11_RASTERIZER_DESC const& rd = s_rasterDescSolid, D3D11_BLEND_DESC const& bd = s_blendDescStd );
    GFXPipeline( int createDeviceFlags, D3D11_RASTERIZER_DESC const& rd = s_rasterDescSolid, D3D11_BLEND_DESC const& bd = s_blendDescStd );
    ~GFXPipeline();

    void setMView( XMMATRIX const& m ) { m_mView = m; }
    XMMATRIX const& getMView() const { return m_mView; }
    XMMATRIX& getMView() { return m_mView; }

    ID3D11Device* getDevice() { return m_dev; }
    ID3D11DeviceContext* getContext() { return m_ic; }

    void setMProjection( XMMATRIX const& m ) { m_mProjection = m; }
    XMMATRIX const& getMProjection() const { return m_mProjection; }
    XMMATRIX& getMProjection() { return m_mProjection; }

    std::shared_ptr< RenderTarget >& getRenderTarget() { return m_rt; }

    void addRenderer( std::shared_ptr< Renderer > renderer );
    
    virtual void preRender();
    virtual void render( XMMATRIX const& world );
    virtual void postRender();
};


class OutputWindow : public GFXPipeline
{
protected:
    static ATOM s_wndclass;
    HWND m_hWnd;
    CComPtr<IDXGISwapChain> m_sc;

    static HWND createWindow( HINSTANCE hInstance, LPCTSTR windowName, int x, int y, int width, int height );
    static LRESULT CALLBACK  WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
public:
    XMMATRIX    m_mWorld;
public:
    OutputWindow( HINSTANCE hInstance, LPCTSTR windowName, int x, int y, int width, int height, int nCmdShow, DXGI_SWAP_EFFECT effect, int bufferCount, int createDeviceFlags = 0, bool withDepth = true );

    virtual LRESULT wndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

    virtual void preRender();

    virtual void render() { __super::render(m_mWorld); };

    virtual void postRender();
};

class RenderToTexture : public GFXPipeline
{
public:
    RenderToTexture( ID3D11Device* dev, int width, int height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, bool withDepth = false );
    // gets the texture resource
    ID3D11Resource* getResource() {
        return dynamic_cast< RenderTexture* >( m_rt.get() )->getResource();
    }
};


