// VIOSO API
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#pragma once
#include "DX11Program.h"
#include "lpng16/png.hpp"

class ImageRenderer : public Renderer
{
    static const char s_szShader[];
    CComPtr< ID3D11VertexShader > m_vs;
    CComPtr< ID3D11PixelShader > m_ps;
    CComPtr< ID3D11SamplerState > m_ss;
    CComPtr< ID3D11ShaderResourceView > m_tex;
public:	
	ImageRenderer( ID3D11Device* dev, char const* path )
    {
        // Compile the vertex shader
        CComPtr< ID3DBlob > codeBlob;
        CComPtr< ID3DBlob > errBlob;
        HRESULT hr = D3DCompile( s_szShader, strlen( s_szShader ), "image vertex shader", NULL, NULL, "VS", "vs_4_0", 0, 0, &codeBlob, &errBlob );
        if( FAILED( hr ) )
            throw std::exception( ( std::string( "failed to compile shader" ) + (char*)errBlob->GetBufferPointer() ).c_str() );

        // Create the vertex shader
        hr = dev->CreateVertexShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &m_vs );
        if( FAILED( hr ) )
            throw std::exception( "failed to create shader" );

        // Compile the pixel shader
        codeBlob.Release();
        errBlob.Release();
        hr = D3DCompile( s_szShader, strlen( s_szShader ), "image pixel shader", NULL, NULL, "PS", "ps_4_0", 0, 0, &codeBlob, &errBlob );
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
        std::string pps( path );
        if( '\"' == pps[0] )
        {
            pps.erase( 0, 1 );
            pps.erase( pps.size() - 1, 1 );
        }
        CPng png( pps.c_str() );
        std::vector<uint8_t> data;

        // create texture
        D3D11_TEXTURE2D_DESC texDesc = {
           (UINT)png.m_width,//UINT Width;
           (UINT)png.m_height,//UINT Height;
           1,//UINT MipLevels;
           1,//UINT ArraySize;
           DXGI_FORMAT_R8G8B8A8_UNORM,
           {1,0},//DXGI_SAMPLE_DESC SampleDesc;
           D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;

//                    D3D11_BIND_RENDER_TARGET | 
                    D3D11_BIND_SHADER_RESOURCE | //UINT BindFlags;
                    0,

                    0,//UINT CPUAccessFlags;

//                  D3D11_RESOURCE_MISC_GENERATE_MIPS | //UINT MiscFlags;
                    0
        };
        
        if( png.m_colorType == PNG_COLOR_TYPE_RGBA && png.m_bitDepth == 8 )
        {
            data.swap( png.m_imageData );
        }
        else if( png.m_colorType == PNG_COLOR_TYPE_RGB && png.m_bitDepth == 8 )
        {
            data.resize( size_t( texDesc.Width ) * texDesc.Height * 4 );
            png.convertRGB8toRGBA8( data.data() );
        }
        else if( png.m_colorType == PNG_COLOR_TYPE_RGB && png.m_bitDepth == 16 )
        {
            data.resize( size_t( texDesc.Width ) * texDesc.Height * 4 );
            png.convertRGB16toRGBA8( data.data() );
        }
        else if( png.m_colorType == PNG_COLOR_TYPE_RGBA && png.m_bitDepth == 16 )
        {
            data.resize( size_t( texDesc.Width ) * texDesc.Height * 4 );
            png.convertRGBA16toRGBA8( data.data() );
        }
        else
            throw std::exception( "unknown png texture format" );

        D3D11_SUBRESOURCE_DATA sbd{
            data.data(),
            4 * texDesc.Width,
            4 * texDesc.Width * texDesc.Height
        };

        CComPtr<ID3D11Texture2D> tex;
        hr = dev->CreateTexture2D( &texDesc, &sbd, &tex );
        if( FAILED( hr ) )
            throw std::exception( "failed to create texture" );

        D3D11_SHADER_RESOURCE_VIEW_DESC srDesc{};
        srDesc.Format = texDesc.Format;
        srDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
        srDesc.Texture2D.MipLevels = 1;
        srDesc.Texture2D.MostDetailedMip = 0;

        hr = dev->CreateShaderResourceView( tex, &srDesc, &m_tex );
        if( FAILED( hr ) )
            throw std::exception( "failed to create shader resource view" );
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
    }

};
const char ImageRenderer::s_szShader[] = R"END(
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
    //return float4( input.Tex.x, input.Tex.y, 1, 1 );
}
)END";

