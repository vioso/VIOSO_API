#include <windows.h>		// Header File For Windows
#include <stdio.h>
#include <tchar.h>
#include <exception>
#include <sstream>
#include "DisplayInfo.h"

//#define USE_VIOSO_API
const int c_numTri = 30;
const float c_rad = 3;

#ifdef USE_VIOSO_API
#include "../../Include/VIOSOWarpBlend.hpp"

LPCTSTR s_configFile = _T("VIOSOWarpBlendGL.ini");
std::shared_ptr<VWB> pWarper;
#endif //USE_VIOSO_API

#include "VK.h"

using namespace std;

mat4x4 g_mWorld;
std::unique_ptr<VK::OutputWindow> g_wnd;

#pragma warning( push )
#pragma warning( disable : 4309 )
#pragma warning( disable : 4838 )
#include "triangleVS.h"
#include "triangleFS.h"
#pragma warning( pop )

class TriangleRenderer : public VK::Renderer
{
	VK::ShaderModuleH m_sm;
	//VK::VertexBufferLoc<VertexWCol> m_vb;
	//static const vector< VertexWCol > s_vertices;

public:
	TriangleRenderer( VK::GFX const& gfx, VK::RenderTarget const& rt );
	TriangleRenderer( TriangleRenderer const& ) = delete;
	TriangleRenderer( TriangleRenderer&& other ) noexcept 
	: VK::Renderer( move( other ) )
	, m_sm( move( other.m_sm ) )
//	, m_vb( move( other.m_vb ) ) 
	{}
	virtual void preRender( VK::GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection );
	virtual void render( VK::GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection );
	virtual void postRender( VK::GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection );
};
//const vector<VK::Renderer::VertexWCol> TriangleRenderer::s_vertices( {
//		VertexWCol{ 0.0f, -0.5f, 0, 1, 1, 1, 1, 1 }, // CW
//		VertexWCol{ 0.5f,  0.5f, 0, 1, 1, 1, 1, 1 },
//		VertexWCol{-0.5f,  0.5f, 0, 1, 1, 1, 1, 1 }
//	} );

TriangleRenderer::TriangleRenderer( VK::GFX const& gfx, VK::RenderTarget const& rt )	
	: VK::Renderer(
		gfx, 
		{VK_DYNAMIC_STATE_VIEWPORT,	VK_DYNAMIC_STATE_SCISSOR},
		{ 
			make_shared < VK::ShaderModule>( gfx.getDevice(), (uint32_t const*)triangleVS_bytecode, sizeof(triangleVS_bytecode) ), 
			make_shared < VK::ShaderModule>( gfx.getDevice(), (uint32_t const*)triangleFS_bytecode, sizeof(triangleFS_bytecode), VK_SHADER_STAGE_FRAGMENT_BIT )  }
		,
		rt, nullptr, nullptr, {},
		1 )
{
}

void TriangleRenderer::preRender( VK::GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection )
{
	__super::preRender( gfx, world, view, projection );
	//mat4x4 M;
	//mat4x4_mul( M, view, projection );
	//mat4x4_mul( m_mub.mVP, world, M );
}

void TriangleRenderer::render( VK::GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection )
{
	__super::render( gfx, world, view, projection );
	vkCmdDraw( m_cbs[0], 3, 1, 0, 0 );
}

void TriangleRenderer::postRender( VK::GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection )
{
	__super::postRender( gfx, world, view, projection );
}

/// <summary>
/// ///////////////////////////////////////////////////////////////////////////////////////////////
/// </summary>

class CubesRenderer : public VK::Renderer
{
	struct Constants {
		mat4x4 mVP;
	};

	vector<VertexWColTex> makeVertexBuffer();
	Constants& m_mub;
public:
	CubesRenderer( VK::GFX const& gfx, std::vector<std::unique_ptr<VK::Image>> const& attachments );
	CubesRenderer( CubesRenderer const& ) = delete;
	CubesRenderer( CubesRenderer&& other ) noexcept 
		: VK::Renderer( move( other ))
		, m_mub( other.m_mub )
	{}
	virtual void preRender(  VK::GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection );
	virtual void render(     VK::GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection );
	virtual void postRender( VK::GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection );
};

vector<CubesRenderer::VertexWColTex> CubesRenderer::makeVertexBuffer()
{
	vector<VertexWColTex> vertices;

	// render some cubes around 0,0,0
	static const int s_nCubes = 9;
	static const int s_nnCubes = 2 * s_nCubes * s_nCubes + 2 * s_nCubes * ( s_nCubes - 2 ) + 2 * ( s_nCubes - 2 ) * ( s_nCubes - 2 );
	static const float sz = 10.0f / ( 3 * s_nCubes - 1 );
	static const int gg = s_nCubes / 2;
	vertices.clear();
	vertices.reserve( s_nnCubes );

	// cube corners calculated from cube center
	//     E/-----/|F       ^y
	//     /  6  / |        |
	//   A|-----|B2|        |---->x
	//   5|  3  | / G      /
	//    |-----|/        /z     
	//   D   1   C
	static const float corners[8][3] =
	{
		{ -sz,  sz,  sz }, //A 0
		{  sz,  sz,  sz	}, //B 1
		{  sz, -sz,  sz	}, //C 2
		{ -sz, -sz,  sz	}, //D 3
		{ -sz,  sz, -sz	}, //E 4
		{  sz,  sz, -sz	}, //F 5
		{  sz, -sz, -sz	}, //G 6
		{ -sz, -sz, -sz	}  //H 7
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
					const float col[3] = { float( x ) / ( s_nCubes - 1 ), float( y ) / ( s_nCubes - 1 ), float( s_nCubes - z ) / ( s_nCubes - 1 ) };
					for( int i = 0; i != ARRAYSIZE( uv ); i++ )
					{
						for( int j = 0; j != 3; j++ )
						{
							vertices.push_back( VertexWColTex( fx + corners[faces[i][j]][0], fy + corners[faces[i][j]][1], fz + corners[faces[i][j]][2], col[0], col[1], col[2], 1.0f, uv[i][j][0], uv[i][j][1] ) );
						}
						vertices.push_back( VertexWColTex( fx + corners[faces[i][0]][0], fy + corners[faces[i][0]][1], fz + corners[faces[i][0]][2], col[0], col[1], col[2], 1.0f, uv[i][0][0], uv[i][0][1] ) );
						for( int j = 2; j != 4; j++ )
						{
							vertices.push_back( VertexWColTex( fx + corners[faces[i][j]][0], fy + corners[faces[i][j]][1], fz + corners[faces[i][j]][2], col[0], col[1], col[2], 1.0f, uv[i][j][0], uv[i][j][1] ) );
						}
						//break;
					}
				}
			}
		}
	}

	return vertices;
}

#if 0
CubesRenderer::CubesRenderer(
	VK::GFX const& gfx,
	std::vector<std::unique_ptr<VK::Image>> const& attachments )
	: VK::Renderer(
		gfx,
		{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR },
		{ 
			make_shared<VK::ShaderModule>( gfx.createVSPassthrough() ),
			make_shared<VK::ShaderModule>( gfx.createFSPassthrough() ) 
		},
		attachments, make_unique<VK::MappedUniformBuffer<Constants>>( gfx, Constants{} ), nullptr, {},
	1 )
	, m_mub( *(Constants*)m_ub.get() )
{}

void CubesRenderer::preRender( VK::GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection )
{
	__super::preRender( gfx, world, view, projection );
	mat4x4 M;
	mat4x4_mul( M, view, projection );
	mat4x4_mul( m_mub.mVP, world, M );
}

void CubesRenderer::render( VK::GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection )
{
	__super::render( gfx, world, view, projection );
	vkCmdDraw( m_cbs[0], m_vb->getVertexCount(), 1, 0, 0 );
}

void CubesRenderer::postRender( VK::GFX const& gfx, mat4x4 const& world, mat4x4 const& view, mat4x4 const& projection )
{
	__super::postRender( gfx, world, view, projection );
}
#endif
int WINAPI WinMain( HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow )			// Window Show State
{
	try {
		//testDisplayInfo();

		VK::test();

		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;
		int gpu = -1;
		mat4x4_identity( g_mWorld );

		std::string channel = "Display1";
		std::istringstream cmd( lpCmdLine );
		cmd >> channel >> x >> y >> w >> h >> gpu;

		g_wnd = std::make_unique<VK::OutputWindow>(hInstance, "VULKAN Demo", x, y, w, h, SW_SHOW, VK_PRESENT_MODE_FIFO_KHR, 3, VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, true, gpu, true );

		g_wnd->addRenderer( make_shared<TriangleRenderer>( *g_wnd, g_wnd->getRT() ) );
		// Main message loop
		MSG msg = { 0 };
		while( WM_QUIT != msg.message )
		{
			if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
			{
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
			else
			{
				try {
					g_wnd->preRender( g_mWorld );
					g_wnd->render( g_mWorld );
					g_wnd->postRender( g_mWorld );
				}
				catch( VK::device_lost&  )
				{

				}
				catch( VK::out_of_date&  )
				{

				}
			}
		}

		return (int)msg.wParam;
	}
	catch( exception& e )
	{
		logStr( e.what() );
		UNREFERENCED_PARAMETER( e );
	}
	return -1;
}
