#pragma once
#include "DX11Program.h"

class TexRenderer : public Renderer
{
    static const char s_szShader[];
    CComPtr< ID3D11VertexShader > m_vs;
    CComPtr< ID3D11PixelShader > m_ps;
    CComPtr< ID3D11SamplerState > m_ss;
    CComPtr< ID3D11ShaderResourceView > m_tex;
public:	
	TexRenderer( ID3D11Device* dev, ID3D11ShaderResourceView* tex )
    {
        // Compile the vertex shader
        CComPtr< ID3DBlob > codeBlob;
        CComPtr< ID3DBlob > errBlob;
        HRESULT hr = D3DCompile( s_szShader, strlen( s_szShader ), "tex vertex shader", NULL, NULL, "VS", "vs_4_0", 0, 0, &codeBlob, &errBlob );
        if( FAILED( hr ) )
            throw std::exception( ( std::string( "failed to compile shader" ) + (char*)errBlob->GetBufferPointer() ).c_str() );

        // Create the vertex shader
        hr = dev->CreateVertexShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &m_vs );
        if( FAILED( hr ) )
            throw std::exception( "failed to create shader" );

        // Compile the pixel shader
        codeBlob.Release();
        errBlob.Release();
        hr = D3DCompile( s_szShader, strlen( s_szShader ), "tex pixel shader", NULL, NULL, "PS", "ps_4_0", 0, 0, &codeBlob, &errBlob );
        if( FAILED( hr ) )
            throw std::exception( ( std::string( "failed to compile shader" ) + (char*)errBlob->GetBufferPointer() ).c_str() );

        // Create the pixel shader
        hr = dev->CreatePixelShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &m_ps );
        if( FAILED( hr ) )
            throw std::exception( "failed to create pixel shader" );
        codeBlob.Release();
        errBlob.Release();

        // create sampler state
        D3D11_SAMPLER_DESC sd{
          D3D11_FILTER_MIN_MAG_MIP_LINEAR,
          D3D11_TEXTURE_ADDRESS_WRAP,
          D3D11_TEXTURE_ADDRESS_WRAP,
          D3D11_TEXTURE_ADDRESS_WRAP,
          0, 8,
          D3D11_COMPARISON_ALWAYS,
          { 0,0,0,0 },
          0, D3D11_FLOAT32_MAX
        };
        hr = dev->CreateSamplerState( &sd, &m_ss );
        if( FAILED( hr ) )
            throw std::exception( "failed to create sampler state" );

        tex->QueryInterface( &m_tex );
    }

    virtual void render( ID3D11DeviceContext* ctx, XMMATRIX const&, XMMATRIX const&, XMMATRIX const& )
    {

        // Set primitive topology
        ctx->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP );

        // set shader
        ctx->VSSetShader( m_vs, NULL, 0 );
        ctx->PSSetShader( m_ps, NULL, 0 );

        // set sampler state
        ctx->PSSetSamplers( 0, 1, &m_ss.p );

        // set texture resource
        ctx->PSSetShaderResources( 0, 1, &m_tex.p );

        // draw
        ctx->Draw( 4, 0 );

        // release texture resource
        ID3D11ShaderResourceView* _empty = nullptr;
        ctx->PSSetShaderResources( 0, 1, &_empty );
    }

};
const char TexRenderer::s_szShader[] = R"END(
Texture2D tex : register(t0);               
SamplerState samLin : register( s0 );

//-------------------------------------------------------------
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

//-------------------------------------------------------------
// Vertex Shader
//-------------------------------------------------------------
PS_INPUT VS( uint i : SV_VERTEXID )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Tex = float2( i & 1, i >> 1 );
    output.Pos = float4( ( output.Tex.x - 0.5f ) * 2, -( output.Tex.y - 0.5f ) * 2, 0, 1 );
    return output;
}

//-------------------------------------------------------------
// Pixel Shader
//-------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
    return tex.Sample( samLin, input.Tex );
}
)END";

