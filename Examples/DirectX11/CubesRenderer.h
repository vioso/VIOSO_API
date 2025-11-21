// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#pragma once
#include "DX11Program.h"
class CubesRenderer : public Renderer
{
    static const int s_nCubes = 9;
    static const int s_nnCubes = 2 * s_nCubes * s_nCubes + 2 * s_nCubes * ( s_nCubes - 2 ) + 2 * ( s_nCubes - 2 ) * ( s_nCubes - 2 );
    static const char s_szShader[];
    CComPtr< ID3D11VertexShader > m_vs;
    CComPtr< ID3D11PixelShader > m_ps;
    CComPtr< ID3D11InputLayout > m_layout;
    CComPtr< ID3D11Buffer > m_vb;
    CComPtr< ID3D11Buffer > m_cb;
    VSConstantBufferA m_vsConstants;

public:
    CubesRenderer( ID3D11Device* dev )
    {
        // Compile the vertex shader
        CComPtr< ID3DBlob > codeBlob;
        CComPtr< ID3DBlob > errBlob;
        HRESULT hr = D3DCompile( s_szShader, strlen( s_szShader ), "cubes vertex shader", NULL, NULL, "VS", "vs_4_0", 0, 0, &codeBlob, &errBlob );
        if( FAILED( hr ) )
            throw std::exception( ( std::string( "failed to compile shader" ) + (char*)errBlob->GetBufferPointer() ).c_str() );

        // Create the vertex shader
        hr = dev->CreateVertexShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &m_vs );
        if( FAILED( hr ) )
            throw std::exception( "failed to create shader" );

        // Define the input layout
        UINT numElements = ARRAYSIZE( SimpleVertex::layout );

        // Create the input layout
        hr = dev->CreateInputLayout( SimpleVertex::layout, numElements, codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), &m_layout );
        if( FAILED( hr ) )
            throw std::exception( "failed to create input layout" );

        // Compile the pixel shader
        codeBlob.Release();
        errBlob.Release();
        hr = D3DCompile( s_szShader, strlen( s_szShader ), "cubes pixel shader", NULL, NULL, "PS", "ps_4_0", 0, 0, &codeBlob, &errBlob );
        if( FAILED( hr ) )
            throw std::exception( ( std::string( "failed to compile shader" ) + (char*)errBlob->GetBufferPointer() ).c_str() );

        // Create the pixel shader
        hr = dev->CreatePixelShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &m_ps );
        if( FAILED( hr ) )
            throw std::exception( "failed to create pixel shader" );
        codeBlob.Release();
        errBlob.Release();

        // render some cubes around 0,0,0
        static const float sz = 10.0f / ( 3 * s_nCubes - 1 );
        static const int gg = s_nCubes / 2;
        std::vector<SimpleVertex> vertices;
        vertices.reserve( s_nnCubes * 36 );

        // cube corners calculated from cube center
        //     E/-----/|F   
        //     /  6  / |      ^y / z
        //   A|-----|B2|      | /
        //   5|  3  | / G     |---->x
        //    |-----|/          
        //   D   1   C
        static const float corners[8][3] =
        {
            { -sz,  sz, -sz }, //A 0
            {  sz,  sz, -sz	}, //B 1
            {  sz, -sz, -sz	}, //C 2
            { -sz, -sz, -sz	}, //D 3
            { -sz,  sz,  sz	}, //E 4
            {  sz,  sz,  sz	}, //F 5
            {  sz, -sz,  sz	}, //G 6
            { -sz, -sz,  sz	}  //H 7
        };

        // faces
        //  1: quad DCGH tri DCG DGH
        //  2: quad BFGC tri BFG BGC
        //  3: quad ABCD tri ABC ACD
        //  4: quad FEHG tri FEH FHG
        //  5: quad EADH tri EAD EDH
        //  6: quad AEFB tri AEF AFB
        static const int faces[6][4] = {
            { 3, 2, 6, 7 }, // face "1"
            { 1, 5, 6, 2 }, // face "2"
            { 0, 1, 2, 3 }, // face "3"
            { 5, 4, 7, 6 }, // face "4"
            { 4, 0, 3, 7 }, // face "5"
            { 0, 4, 5, 2 }  // face "6"
        };

        // faces for uv's, faces are packed into a rectangular image:
        //  1 2 3       0.0,0.0 - 0.3333,0.5 ; 0.3333,0.0 - 0.6666,0.5 ; 0.66666,0.0 - 1.0,0.5
        //  4 5 6  ==>  0.5,0.0 - 0.3333,1.0 ; 0.3333,0.5 - 0.6666,1.0 ; 0.66666,0.5 - 1.0,1.0
        static const float uv[6][4][2] = {
            { { 0.0f, 0.0f }, { 0.33333f, 0.0f }, { 0.3333f, 0.5f }, { 0.0f, 0.5f } },
            { { 0.33333f, 0.0f }, { 0.66666f, 0.0f }, { 0.66666f, 0.5f }, { 0.333333f, 0.5f } },
            { { 0.66666f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.5f }, { 0.66666f, 0.5f } },
            { { 0.0f, 0.5f }, { 0.33333f, 0.5f }, { 0.3333f, 1.0f }, { 0.0f, 1.0f } },
            { { 0.33333f, 0.5f }, { 0.66666f, 0.5f }, { 0.66666f, 1.0f }, { 0.333333f, 1.0f } },
            { { 0.66666f, 0.5f }, { 1.0f, 0.5f }, { 1.0f, 1.0f }, { 0.66666f, 1.0f } }
        };

        for( int z = 0; z != s_nCubes; z++ )
            //int z = 0;
        {
            float fz = ( z - gg ) * 3.0f * sz;
            for( int y = 0; y != s_nCubes; y++ )
                //int y = 0;
            {
                float fy = ( y - gg ) * 3.0f * sz;
                for( int x = 0; x != s_nCubes; x++ )
                    //int x = 0;
                {
                    float fx = ( x - gg ) * 3.0f * sz;
                    if( ( 0 == x ) || ( 2 * gg == x ) || ( 0 == y ) || ( 2 * gg == y ) || ( 0 == z ) || ( 2 * gg == z ) )
                    {
                        const float col[3] = { float( x ) / ( s_nCubes - 1 ), float( y ) / ( s_nCubes - 1 ), float( z ) / ( s_nCubes - 1 ) };
                        for( int i = 0; i != ARRAYSIZE( uv ); i++ )
                        {
                            for( int j = 0; j != 3; j++ )
                            {
                                vertices.push_back( SimpleVertex{ {fx + corners[faces[i][j]][0], fy + corners[faces[i][j]][1], fz + corners[faces[i][j]][2]}, {col[0], col[1], col[2], 1.0f}/*, {uv[i][j][0], uv[i][j][1] }*/ } );
                            }
                            vertices.push_back( SimpleVertex{ {fx + corners[faces[i][0]][0], fy + corners[faces[i][0]][1], fz + corners[faces[i][0]][2] }, {col[0], col[1], col[2], 1.0f}/*, {uv[i][0][0], uv[i][0][1] }*/ } );
                            for( int j = 2; j != 4; j++ )
                            {
                                vertices.push_back( SimpleVertex{ {fx + corners[faces[i][j]][0], fy + corners[faces[i][j]][1], fz + corners[faces[i][j]][2] }, {col[0], col[1], col[2], 1.0f} /*, {uv[i][j][0], uv[i][j][1] }*/ } );
                            }
                            //break;
                        }
                    }
                }
            }
        }

        D3D11_BUFFER_DESC bd;
        ZeroMemory( &bd, sizeof( bd ) );
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof( SimpleVertex ) * s_nnCubes * 36;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA InitData;
        ZeroMemory( &InitData, sizeof( InitData ) );
        InitData.pSysMem = &vertices.front();
        hr = dev->CreateBuffer( &bd, &InitData, &m_vb );
        if( FAILED( hr ) )
            throw std::exception( "failed to create vertex buffer" );

        // Create the constant buffer
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof( VSConstantBufferA );
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = dev->CreateBuffer( &bd, NULL, &m_cb );
        if( FAILED( hr ) )
            throw std::exception( "failed to create constant buffer" );

        ZeroMemory( &m_vsConstants, sizeof( m_vsConstants ) );
    }

    virtual void render( ID3D11DeviceContext* ctx, XMMATRIX const& world, XMMATRIX const& view, XMMATRIX const& projection )
    {

        // Set the input layout
        ctx->IASetInputLayout( m_layout );

        // Set vertex buffer
        UINT stride = sizeof( SimpleVertex );
        UINT offset = 0;
        ctx->IASetVertexBuffers( 0, 1, &m_vb.p, &stride, &offset );

        // Set primitive topology
        ctx->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

        // update and set constant buffer
        m_vsConstants.mWorld = XMMatrixTranspose( world );
        m_vsConstants.mView = XMMatrixTranspose( view );
        m_vsConstants.mProjection = XMMatrixTranspose( projection );
        ctx->UpdateSubresource( m_cb, 0, NULL, &m_vsConstants, 0, 0 );
        ctx->VSSetConstantBuffers( 0, 1, &m_cb.p );

        // set shader
        ctx->VSSetShader( m_vs, NULL, 0 );
        ctx->PSSetShader( m_ps, NULL, 0 );

        // draw
        ctx->Draw( s_nnCubes * 36, 0 );
    }
};

const char CubesRenderer::s_szShader[] = R"END(
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
};

//-------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

//-------------------------------------------------------------
// Vertex Shader
//-------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Color = input.Color;

    return output;
}

//-------------------------------------------------------------
// Pixel Shader
//-------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
    return input.Color;
}
)END";
