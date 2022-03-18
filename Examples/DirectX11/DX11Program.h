#pragma once
#include <sdkddkver.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <tchar.h>
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
using namespace DirectX;

#include <memory>
#include <vector>
#include <string>
#include <atlbase.h>

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct VSConstantBuffer
{
    XMMATRIX mWorld;
    XMMATRIX mView;
    XMMATRIX mProjection;
};

//--------------------------------------------------------------------------------------
// Classes
//--------------------------------------------------------------------------------------

class RenderTarget
{
protected:
    CComPtr< ID3D11RenderTargetView > m_rtv;
    CComPtr< ID3D11DepthStencilView > m_dsv;
public:
    static const FLOAT s_black[4];
    static const FLOAT s_sky[4];

    virtual void setAsRTto( ID3D11DeviceContext* ctx );
    virtual void clear( ID3D11DeviceContext* ctx, const FLOAT color[4] = s_black );
    D3D11_VIEWPORT getFullViewport() const;
    virtual void resize( ID3D11Device*, IDXGISwapChain* );
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
protected:
    CComPtr<ID3D11Device> m_dev;
    CComPtr<ID3D11DeviceContext> m_ic;
    std::shared_ptr< RenderTarget > m_rt;
    std::vector< std::shared_ptr< Renderer > > m_renderers;
    D3D11_VIEWPORT m_vp = { 0 };
    XMMATRIX m_mView = XMMatrixIdentity();
    XMMATRIX m_mProjection = XMMatrixPerspectiveFovLH( XM_PIDIV2, 16.0f / 9, 0.25f, 1024.0f );
public:

    void setMView( XMMATRIX const& m ) { m_mView = m; }
    XMMATRIX const& getMView() const { return m_mView; }
    XMMATRIX& getMView() { return m_mView; }

    ID3D11Device* getDevice() { return m_dev; }
    ID3D11DeviceContext* getContext() { return m_ic; }

    void setMProjection( XMMATRIX const& m ) { m_mProjection = m; }
    XMMATRIX const& getMProjection() const { return m_mProjection; }
    XMMATRIX& getMProjection() { return m_mProjection; }

    std::shared_ptr< RenderTarget >& getRenderTarget() { return m_rt;  }

    virtual void addRenderer( std::shared_ptr< Renderer > renderer );
    virtual void preRender();
    virtual void render( XMMATRIX const& world );
    virtual void postRender();
};


class OutputWindow : public GFXPipeline
{
protected:
    static ATOM s_wndclass;
    HWND m_hWnd;
    D3D_DRIVER_TYPE    m_driverType;
    D3D_FEATURE_LEVEL  m_featureLevel;
    CComPtr<IDXGISwapChain> m_sc;

    static LRESULT CALLBACK  WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
    virtual LRESULT wndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

public:
    OutputWindow( HINSTANCE hInstance, LPCTSTR windowName, int x, int y, int width, int height, int nCmdShow, DXGI_SWAP_EFFECT effect, int bufferCount, int createDeviceFlags = 0, bool withDepth = true );

    virtual void postRender();
};

class RenderToTexture : public GFXPipeline
{
public:
    RenderToTexture( ID3D11Device* dev, ID3D11DeviceContext* ic, int width, int height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, bool withDepth = false );
};


