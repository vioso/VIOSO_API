#include "AssimpRenderer.h"
#include "assimp/scene.h"
#include "assimp/cimport.h"
#include "assimp/postprocess.h"

#include "lpng16/png.hpp"

#include <exception>
#include <memory>
#include <filesystem>

#pragma comment( lib, "assimp/lib/assimp-vc142-mt")

using namespace Assimp;
using namespace std;

const unsigned int ppstepsdefault =
//    aiProcess_CalcTangentSpace | // calculate tangents and bitangents if possible
aiProcess_JoinIdenticalVertices | // join identical vertices/ optimize indexing
aiProcess_MakeLeftHanded | // Converts all the imported data to a left-handed coordinate space
aiProcess_ValidateDataStructure | // perform a full validation of the loader's output
aiProcess_ImproveCacheLocality | // improve the cache locality of the output vertices
//    aiProcess_RemoveRedundantMaterials | // remove redundant materials
aiProcess_FindDegenerates | // remove degenerated polygons from the import
aiProcess_FindInvalidData | // detect invalid model data, such as invalid normal vectors
aiProcess_GenUVCoords | // convert spherical, cylindrical, box and planar mapping to proper UVs
aiProcess_TransformUVCoords | // preprocess UV transformations (scaling, translation ...)
//    aiProcess_FindInstances | // search for instanced meshes and remove them by references to one master
//    aiProcess_LimitBoneWeights | // limit bone weights to 4 per vertex
//    aiProcess_OptimizeMeshes | // join small meshes, if possible;
//    aiProcess_SplitByBoneCount | // split meshes with too many bones. Necessary for our (limited) hardware skinning 
aiProcess_Triangulate |
aiProcess_SortByPType |
aiProcess_PreTransformVertices |
aiProcess_SplitLargeMeshes |
aiProcess_ConvertToLeftHanded |
aiProcess_EmbedTextures |
0;

const float smoothAngle = 80.f;

const bool nopointslines = false;

bool bWasFlipped /*= false*/;

const char AssimpRenderer::s_szShader[] = R"END(
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
    matrix Model;
	matrix View;
	matrix Projection;
}
Texture2D tex : register(t0);               
SamplerState samLin : register( s0 );

//-------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD0;
};

//-------------------------------------------------------------
// Vertex Shader
//-------------------------------------------------------------
PS_INPUT VS( VS_INPUT input )
{
    PS_INPUT output = (PS_INPUT)0;
    output.Pos = mul( input.Pos, World );
    output.Pos = mul( output.Pos, Model );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Color = input.Color;
    output.Tex = input.Tex;
    return output;
}

//-------------------------------------------------------------
// Pixel Shader
//-------------------------------------------------------------
float4 PS( PS_INPUT input) : SV_Target
{
    return input.Color + tex.Sample( samLin, input.Tex );
}
)END";


AssimpRenderer::AssimpRenderer( ID3D11Device* dev, char const* path )
{
	// Compile the vertex shader
	CComPtr< ID3DBlob > codeBlob;
	CComPtr< ID3DBlob > errBlob;
	HRESULT hr = D3DCompile( s_szShader, strlen( s_szShader ), "assimp vertex shader", NULL, NULL, "VS", "vs_4_0", 0, 0, &codeBlob, &errBlob );
	if( FAILED( hr ) )
		throw exception( ( string( "failed to compile shader" ) + (char*)errBlob->GetBufferPointer() ).c_str() );

	// Create the vertex shader
	hr = dev->CreateVertexShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &m_vs );
	if( FAILED( hr ) )
		throw exception( "failed to create shader" );

	// Define the input layout
	UINT numElements = ARRAYSIZE( SimpleVertexTex::layout );

	// Create the input layout
	hr = dev->CreateInputLayout( SimpleVertexTex::layout, numElements, codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), &m_layout );
	if( FAILED( hr ) )
		throw exception( "failed to create input layout" );

	// Compile the pixel shader
	codeBlob.Release();
	errBlob.Release();
	hr = D3DCompile( s_szShader, strlen( s_szShader ), "assimp pixel shader", NULL, NULL, "PS", "ps_4_0", 0, 0, &codeBlob, &errBlob );
	if( FAILED( hr ) )
		throw exception( ( string( "failed to compile shader" ) + (char*)errBlob->GetBufferPointer() ).c_str() );

	// Create the pixel shader
	hr = dev->CreatePixelShader( codeBlob->GetBufferPointer(), codeBlob->GetBufferSize(), NULL, &m_ps );
	if( FAILED( hr ) )
		throw exception( "failed to create pixel shader" );
	codeBlob.Release();
	errBlob.Release();

	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( VSConstantBufferB );
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = dev->CreateBuffer( &bd, NULL, &m_cb );
	if( FAILED( hr ) )
		throw exception( "failed to create constant buffer" );

	ZeroMemory( &m_vsConstants, sizeof( m_vsConstants ) );

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
		throw exception( "failed to create sampler state" );

	Load( path, dev );
	LoadBase( dev );
}

int LoadNode( aiScene const& scene, aiNode const& node, ID3D11Device* dev, aiMatrix4x4 const& preTransform, AssimpRenderer::MeshInfoList& l )
{
	aiMatrix4x4 trans = preTransform * node.mTransformation;
	for( unsigned int i = 0; i != node.mNumMeshes; i++ )
	{
		unsigned int& im = node.mMeshes[i];

		// checking for triangles, all meshes should be triangles, as we requested a triangulation in first place
		if( scene.mMeshes[im]->mPrimitiveTypes == aiPrimitiveType_TRIANGLE )
		{
			AssimpRenderer::MeshInfo mi;
			// set already transposed
			mi.modelViewT = XMMatrixSet(
				trans.a1,
				trans.b1,
				trans.c1,
				trans.d1,
				trans.a2,
				trans.b2,
				trans.c2,
				trans.d2,
				trans.a3,
				trans.b3,
				trans.c3,
				trans.d3,
				trans.a4,
				trans.b4,
				trans.c4,
				trans.d4
			);

			aiMesh& mesh = *scene.mMeshes[im];
			mi.topo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

			// create vertices
			{
				std::vector<SimpleVertexTex> vertices( mesh.mNumVertices );

				for( unsigned int j = 0; j != mesh.mNumVertices; j++ )
				{
					SimpleVertexTex& v = vertices[j];
					v.Pos.x = mesh.mVertices[j].x;
					v.Pos.y = mesh.mVertices[j].y;
					v.Pos.z = mesh.mVertices[j].z;
				}
				if( mesh.HasTextureCoords( 0 ) )
				{
					for( unsigned int j = 0; j != mesh.mNumVertices; j++ )
					{
						SimpleVertexTex& v = vertices[j];
						v.Tex.x = mesh.mTextureCoords[0][j].x;
						v.Tex.y = mesh.mTextureCoords[0][j].y;
						v.Color.x = 0;
						v.Color.y = 0;
						v.Color.z = 0;
						v.Color.w = 0;
					}

					mi.texIndex = mesh.mMaterialIndex;
				}
				else
				{
					for( unsigned int j = 0; j != mesh.mNumVertices; j++ )
					{
						SimpleVertexTex& v = vertices[j];
						v.Tex.x = 0;
						v.Tex.y = 0;
						v.Color.x = 0.8f;
						v.Color.y = 0.8f;
						v.Color.z = 0.8f;
						v.Color.w = 1.0f;
					}
				}

				// create vertex buffer
				D3D11_BUFFER_DESC bd;
				ZeroMemory( &bd, sizeof( bd ) );
				bd.Usage = D3D11_USAGE_DEFAULT;
				bd.ByteWidth = sizeof( SimpleVertexTex ) * (UINT)vertices.size();
				bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				bd.CPUAccessFlags = 0;
				D3D11_SUBRESOURCE_DATA InitData{ vertices.data() };
				HRESULT hr = dev->CreateBuffer( &bd, &InitData, &mi.vb );
				if( FAILED( hr ) )
					throw exception( "failed to create vertex buffer" );
			}


			// create indices/faces
			{
				// accumulate 
				for( unsigned int j = 0; j != mesh.mNumFaces; j++ )
					mi.numIndices += mesh.mFaces[j].mNumIndices;
				vector<unsigned int> indices( mi.numIndices );
				auto in = indices.begin();
				for( unsigned int j = 0; j != mesh.mNumFaces; j++ )
				{
					for( unsigned int k = 0; k != mesh.mFaces[j].mNumIndices; k++ )
						*( in++ ) = mesh.mFaces[j].mIndices[k];
				}

				// create index buffer
				D3D11_BUFFER_DESC bd;
				ZeroMemory( &bd, sizeof( bd ) );
				bd.Usage = D3D11_USAGE_DEFAULT;
				bd.ByteWidth = sizeof( unsigned int ) * (UINT)indices.size();
				bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
				bd.CPUAccessFlags = 0;
				D3D11_SUBRESOURCE_DATA InitData{ indices.data() };
				HRESULT hr = dev->CreateBuffer( &bd, &InitData, &mi.ib );
				if( FAILED( hr ) )
					throw exception( "failed to create vertex buffer" );
			}

			l.push_back( mi );

		}
	}
	for( unsigned int j = 0; j != node.mNumChildren; j++ )
	{
		LoadNode( scene, *node.mChildren[j], dev, trans, l );
	}
	return 1;
}

aiString getTexNameFrom( aiMaterial const& m )
{
	for( unsigned int i = 0; i != m.mNumProperties; i++ )
	{
		if( m.mProperties[i]->mKey == aiString("$tex.file") )
		{
			return *(aiString*)m.mProperties[i]->mData;
		}
	}
	return aiString( "" );
}

HRESULT AssimpRenderer::Load( char const* path, ID3D11Device* dev )
{
	HRESULT hr = S_OK;
	unique_ptr<aiPropertyStore, void( * )( aiPropertyStore* )> props( aiCreatePropertyStore(), &aiReleasePropertyStore );
	if( props )
	{
		aiSetImportPropertyInteger( props.get(), AI_CONFIG_IMPORT_TER_MAKE_UVS, 1 );
		aiSetImportPropertyFloat( props.get(), AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, smoothAngle );
		aiSetImportPropertyInteger( props.get(), AI_CONFIG_PP_SBP_REMOVE, nopointslines ? aiPrimitiveType_LINE | aiPrimitiveType_POINT : 0 );

		aiSetImportPropertyInteger( props.get(), AI_CONFIG_GLOB_MEASURE_TIME, 1 );
		//aiSetImportPropertyInteger(props,AI_CONFIG_PP_PTV_KEEP_HIERARCHY,1);

		// Call ASSIMPs C-API to load the file
		filesystem::path pp( path );

		string pps = pp.string();
		if( '\"' == pps[0] )
		{
			pps.erase( 0, 1 );
			pps.erase( pps.size() - 1, 1 );
		}
		unique_ptr<aiScene, void( * )( aiScene const* )> pScene(
			(aiScene*)aiImportFileExWithProperties(
				pps.c_str(),
				ppstepsdefault,
				nullptr, props.get()
			),
			&aiReleaseImport
		);
		if( pScene )
		{
			aiScene& scene = *pScene;
			if( scene.mRootNode )
			{
				aiMatrix4x4 I; aiIdentityMatrix4( &I );
				LoadNode( *pScene, *scene.mRootNode, dev, I, m_meshes );

				// load textures
				D3D11_TEXTURE2D_DESC texDesc = {
					(UINT)0,//UINT Width;
					(UINT)0,//UINT Height;
					1,//UINT MipLevels;
					1,//UINT ArraySize;
					DXGI_FORMAT_UNKNOWN,
					{1,0},//DXGI_SAMPLE_DESC SampleDesc;
					D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;

//                    D3D11_BIND_RENDER_TARGET | 
					D3D11_BIND_SHADER_RESOURCE | //UINT BindFlags;
					0,

					0,//UINT CPUAccessFlags;

//                  D3D11_RESOURCE_MISC_GENERATE_MIPS | //UINT MiscFlags;
					0
				};

				D3D11_SHADER_RESOURCE_VIEW_DESC srDesc;
				srDesc.Format = texDesc.Format;
				srDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
				srDesc.Texture2D.MipLevels = 1;
				srDesc.Texture2D.MostDetailedMip = 0;
				D3D11_SUBRESOURCE_DATA sd{};
				std::vector<uint8_t> data;

				for( auto& mesh : m_meshes )
				{
					if( mesh.texIndex != UINT_MAX && mesh.texIndex < scene.mNumMaterials )
					{
						if( m_textures.size() < size_t( mesh.texIndex ) + 1 )
							m_textures.resize( size_t( mesh.texIndex ) + 1 );
						if( nullptr == m_textures[mesh.texIndex] )
						{
							aiMaterial& material = *scene.mMaterials[mesh.texIndex];
							aiString texPath = getTexNameFrom( material );

							CComPtr<ID3D11Texture2D> tex;

							if( texPath.data[0] == '*' ) // there is a texture index
							{
								int i = atoi( &texPath.data[1] );

								if( 0 == strcmp( "rgba8888", scene.mTextures[i]->achFormatHint ) )
								{
									texDesc.Width = pScene->mTextures[i]->mWidth;
									texDesc.Height = pScene->mTextures[i]->mHeight;
									texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
									sd.pSysMem = pScene->mTextures[i]->pcData;
									sd.SysMemPitch = 4 * texDesc.Width;
									sd.SysMemSlicePitch = sd.SysMemPitch * texDesc.Height;
								}
								else if( 0 == strcmp( "png", scene.mTextures[i]->achFormatHint ) )
								{
									CPng png( (uint8_t*)scene.mTextures[i]->pcData, scene.mTextures[i]->mWidth );

									texDesc.Width = png.m_width;
									texDesc.Height = png.m_height;
									texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
									sd.SysMemPitch = 4 * texDesc.Width;
									sd.SysMemSlicePitch = sd.SysMemPitch * texDesc.Height;

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
										throw exception( "unknown png texture format" );

									sd.pSysMem = data.data();
								}
								else
								{
									i = i; // what other formats are common?
									throw exception( "unknown texture format" );
								}
								if( sd.pSysMem )
								{
									hr = dev->CreateTexture2D( &texDesc, &sd, &tex );
									if( FAILED( hr ) )
										throw exception( "failed to create texture" );
								}
							}
							else // load from file
							{
								throw exception( "load texture from file not implemented" );
							}

							if( tex )
							{
								srDesc.Format = texDesc.Format;
								hr = dev->CreateShaderResourceView( tex, &srDesc, &m_textures[mesh.texIndex] );
								if( FAILED( hr ) )
									throw exception( "failed to create shader resource view" );
							}

						}

					}
				}
			}
		}
		else
		{
			throw exception( aiGetErrorString() );
		}
	}
	return hr;
}

HRESULT AssimpRenderer::LoadBase( ID3D11Device* dev )
{
	MeshInfo mi;
	mi.modelViewT = XMMatrixSet( 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 );
	mi.topo = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	mi.texIndex = UINT_MAX;
	std::vector<SimpleVertexTex> vertices;

	vertices.emplace_back( 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f );
	vertices.emplace_back( 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f );

	vertices.emplace_back( 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f );
	vertices.emplace_back( 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f );

	vertices.emplace_back( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f );
	vertices.emplace_back( 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f );

	// create vertex buffer
	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof( bd ) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( SimpleVertexTex ) * (UINT)vertices.size();
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData{ vertices.data() };
	HRESULT hr = dev->CreateBuffer( &bd, &InitData, &mi.vb );
	if( FAILED( hr ) )
		throw exception( "failed to create vertex buffer" );

	vector<unsigned int> indices( { 0, 1 , 2, 3, 4, 5 } );    // create index buffer
	ZeroMemory( &bd, sizeof( bd ) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( unsigned int ) * (UINT)indices.size();
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices.data();
	hr = dev->CreateBuffer( &bd, &InitData, &mi.ib );
	if( FAILED( hr ) )
		throw exception( "failed to create vertex buffer" );

	mi.numIndices = (UINT)indices.size();

	m_meshes.push_back( mi );
	return S_OK;
}

void AssimpRenderer::render( ID3D11DeviceContext* ctx, XMMATRIX const& world, XMMATRIX const& view, XMMATRIX const& projection )
{
	// Set the input layout
	ctx->IASetInputLayout( m_layout );

	// set sampler state
	ctx->PSSetSamplers( 0, 1, &m_ss.p );

	// update and set constant buffer
	m_vsConstants.mWorld = XMMatrixTranspose( world );
	m_vsConstants.mView = XMMatrixTranspose( view );
	m_vsConstants.mProjection = XMMatrixTranspose( projection );
	m_vsConstants.mModel = XMMatrixSet( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ); // reset to make the loop update the CB

	// set shader
	ctx->VSSetShader( m_vs, NULL, 0 );
	ctx->PSSetShader( m_ps, NULL, 0 );

	// draw
	for( auto& mesh : m_meshes )
	{
		// Set vertex buffer
		ctx->IASetVertexBuffers( 0, 1, &mesh.vb.p, &SimpleVertexTex::stride, &mesh.vertexOffset );
		// set index buffer
		ctx->IASetIndexBuffer( mesh.ib, DXGI_FORMAT_R32_UINT, 0 );
		// Set primitive topology
		ctx->IASetPrimitiveTopology( mesh.topo );

		ID3D11ShaderResourceView * const _empty = nullptr;
		// set texture
		if( UINT_MAX != mesh.texIndex && !m_textures.empty() && m_textures[mesh.texIndex] )
			ctx->PSSetShaderResources( 0, 1, &m_textures[mesh.texIndex].p );
		else
			ctx->PSSetShaderResources( 0, 1, &_empty );

		if( memcmp( m_vsConstants.mModel.r, mesh.modelViewT.r, sizeof( m_vsConstants.mModel ) ) )
		{
			m_vsConstants.mModel = mesh.modelViewT;
			ctx->UpdateSubresource( m_cb, 0, NULL, &m_vsConstants, 0, 0 );
			ctx->VSSetConstantBuffers( 0, 1, &m_cb.p );
		}

		ctx->DrawIndexed( mesh.numIndices, mesh.indexOffset, mesh.vertexOffset );
	}

}
