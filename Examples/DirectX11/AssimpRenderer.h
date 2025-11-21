// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#pragma once
#include "DX11Program.h"
#include <vector>
#include <filesystem>

class AssimpRenderer :
    public Renderer
{
public:
    typedef struct MeshInfo {
        XMMATRIX modelViewT; // store transposed matrix here
        UINT numIndices;
        UINT indexOffset;
        UINT vertexOffset;
        D3D_PRIMITIVE_TOPOLOGY topo;
        UINT texIndex;
        CComPtr< ID3D11Buffer > vb;
        CComPtr< ID3D11Buffer > ib;
        MeshInfo() :modelViewT{}, numIndices{ 0U }, indexOffset{ 0 }, vertexOffset{ 0U }, topo{ D3D_PRIMITIVE_TOPOLOGY_UNDEFINED }, texIndex{ UINT_MAX } {}
    } MeshInfo;
    typedef std::vector<MeshInfo> MeshInfoList;
    typedef std::vector< CComPtr< ID3D11ShaderResourceView >> TextureList;

protected:
    static const char s_szShader[];
    CComPtr< ID3D11VertexShader > m_vs;
    CComPtr< ID3D11PixelShader > m_ps;
    CComPtr< ID3D11InputLayout > m_layout;
    CComPtr< ID3D11Buffer > m_cb;
    CComPtr< ID3D11SamplerState > m_ss;
    VSConstantBufferB m_vsConstants;
    MeshInfoList m_meshes;
    TextureList m_textures;

public:
    AssimpRenderer( ID3D11Device* dev, std::filesystem::path const& path = "");
    HRESULT Load( std::filesystem::path const& path, ID3D11Device* dev );
    HRESULT LoadBase( ID3D11Device* dev );
    HRESULT LoadQuad( ID3D11Device* dev );
    virtual void render(ID3D11DeviceContext* ctx, XMMATRIX const& world, XMMATRIX const& view, XMMATRIX const& projection);
};

