// VIOSO API OpenGL example
// based on the NeHe tutorial
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#ifdef WIN32

#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>		// Header File For Windows

#else

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#define ARRAYSIZE(arr) (sizeof(arr)/sizeof(arr[0]))

#endif // def WIN32

#define GL_EXT_DEFINE_AND_IMPLEMENT
#include "../../VIOSOWarpBlend/GL/GLext.h"

#include <stdio.h>
#include <stdarg.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <filesystem>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtc/type_ptr.hpp"

using namespace std;
namespace fs = filesystem;

#define USE_VIOSO_API
const int c_numTri = 30;

#ifdef USE_VIOSO_API
#include "../../Include/VIOSOWarpBlend.hpp"

//fs::path configFile = "data/VIOSOWarpBlendGL.ini";
//fs::path configFile = "data/dynamic/config.xml";
fs::path configFile = "data/directionalShading/config.xml";

shared_ptr<VWB> pWarper;
#endif //USE_VIOSO_API

typedef char GLchar;

GLuint iProg = -1;				// the shader program
GLuint iVert = -1;				// the vertex shader program
GLuint iFrag = -1;				// the fragment shader
GLuint iVAData = -1;			// the data buffer
GLuint iVAPos = -1;				// the vertex array for position
GLuint iVACol = -1;				// the vertex array for color
GLuint iVATex = -1;				// the vertex array for texture
GLuint locVAPos = -1;			// the location of the position input
GLuint locVACol = -1;			// the location of the color input
GLuint locVATex = -1;			// the location of the texture input
GLuint locMatProj = -1;			// the location of the projection matrix
GLuint locMatView = -1;			// the location of the view matrix

#pragma pack( push, 4 )
struct Vertex
{
	GLfloat x, y, z;
	GLfloat r, g, b, a;
	GLfloat u, v;
};
#pragma pack( pop )
typedef vector<Vertex> Vertices;
Vertices vertices;

GLchar const* pszShaders[] = {
R"END(#version 330 core
uniform mat4 matView;
uniform mat4 matProj;
layout( location = 0 ) in vec3 in_position;
layout( location = 1 ) in vec4 in_color;
//layout( location = 2 ) in vec2 in_tex;
out vec4 vtx_color;
//out vec2 vtx_tex;

void main(void) {
	vec4 viewSpace = matView * vec4(in_position.xyz, 1.0);
	gl_Position = matProj * viewSpace;
	vtx_color = in_color;
//	vtx_tex = in_tex;
})END",
R"END(#version 330 core
precision highp float; // Video card drivers require this line to function properly
in vec4 vtx_color;
//in vec2 vtx_tex;
out vec4 out_color;
void main()
{
//	out_color = vec4(1.0,1.0,1.0,1.0);
	out_color = vtx_color;
})END"
};

GLvoid ReSizeGLScene( GLsizei width, GLsizei height );
GLvoid KillGLWindow( GLvoid );

inline void logStr( const char* str, ... )
{
	//if( NULL != VWB__logString )
	//	VWB__logString( 1, str );
	//else
	{
		FILE* f = nullptr;
		{
			#ifdef WIN32
			if( NOERROR == fopen_s( &f, "gl3_2.log", "a" ) )
			#else
			if( f = fopen( "gl3_2.log", "a" ) )
			#endif // def WIN32
			{
				va_list args;
				va_start( args, str );

				vfprintf( f, str, args );
				fclose( f );
				va_end( args );
			}
		}
	}
}

void APIENTRY glLog( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam )
{
	char const* szType = nullptr;
	switch( type )
	{
	case GL_DEBUG_TYPE_ERROR:
		szType = "FATAL";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		szType = "Deprecated";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		szType = "Undefined";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		szType = "Portability";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		szType = "Performance";
		break;
	case GL_DEBUG_TYPE_OTHER:
		szType = "Other";
		break;
	}
	logStr("GLDEBUG: %s %s: %s\n", szType, GL_DEBUG_SEVERITY_LOW == severity ? "note" : ( GL_DEBUG_SEVERITY_MEDIUM == severity ? "warning" : "ERROR" ), message );
}

GLvoid ReSizeGLScene( GLsizei width, GLsizei height )		// Resize And Initialize The GL Window
{
	if( height == 0 )										// Prevent A Divide By Zero By
	{
		height = 1;										// Making Height Equal One
	}

	glViewport( 0, 0, width, height );						// Reset The Current Viewport
}

bool InitGL( GLvoid )										// All Setup For OpenGL Goes Here
{
	GLenum err = glGetError();

	#if defined( _DEBUG)
	glDebugMessageCallback( &glLog, nullptr );
	glEnable( GL_DEBUG_OUTPUT );							// Enables Debug output
	glDebugMessageCallback( &glLog, nullptr );
	#endif

	glShadeModel( GL_SMOOTH );							// Enable Smooth Shading
	glEnable( GL_DEPTH_TEST );							// Enables Depth Testing
	glDepthFunc( GL_LEQUAL );								// The Type Of Depth Testing To Do
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );	// Really Nice Perspective Calculations

	// vertex shader
	iVert = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( iVert, 1, &pszShaders[0], NULL );
	glCompileShader( iVert );
	char log[2000] = { 0 };
	glGetShaderInfoLog( iVert, 2000, NULL, log );
	if( log[0] )
	{
		logStr( log );
		return false;
	}

	// fragment shader
	iFrag = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( iFrag, 1, &pszShaders[1], NULL );
	glCompileShader( iFrag );
	glGetShaderInfoLog( iFrag, 2000, NULL, log );
	if( log[0] )
	{
		logStr( log );
		return false;
	}
	err = glGetError();

	iProg = glCreateProgram();
	glAttachShader( iProg, iVert );
	glAttachShader( iProg, iFrag );
	glLinkProgram( iProg );
	err = glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( "Failed to link shader program" );
		return false;
	}
	GLint isLinked = 0;
	glGetProgramiv( iProg, GL_LINK_STATUS, &isLinked );
	if( isLinked == GL_FALSE )
	{
		GLint maxLength = 0;
		glGetProgramiv( iProg, GL_INFO_LOG_LENGTH, &maxLength );

		//The maxLength includes the NULL character
		vector<GLchar> infoLog( maxLength );
		glGetProgramInfoLog( iProg, maxLength, &maxLength, &infoLog[0] );

		//The program is useless now. So delete it.
		glDeleteProgram( iProg );
		iProg = -1;

		logStr( log, "ERROR: %d at glLinkProgram:\n%s\n", err, &infoLog[0] );
		return false;
	}

	locMatProj = glGetUniformLocation( iProg, "matProj" );
	locMatView = glGetUniformLocation( iProg, "matView" );
	locVAPos = glGetAttribLocation( iProg, "in_position" );
	locVACol = glGetAttribLocation( iProg, "in_color" );
	locVATex = glGetAttribLocation( iProg, "in_tex" );
	glUseProgram( iProg );

	err = glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( "Failed to create vertex position buffer" );
		return false;
	}

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
							vertices.push_back( Vertex{ fx + corners[faces[i][j]][0], fy + corners[faces[i][j]][1], fz + corners[faces[i][j]][2], col[0], col[1], col[2], 1.0f, uv[i][j][0], uv[i][j][1] } );
						}
						vertices.push_back( Vertex{ fx + corners[faces[i][0]][0], fy + corners[faces[i][0]][1], fz + corners[faces[i][0]][2], col[0], col[1], col[2], 1.0f, uv[i][0][0], uv[i][0][1] } );
						for( int j = 2; j != 4; j++ )
						{
							vertices.push_back( Vertex{ fx + corners[faces[i][j]][0], fy + corners[faces[i][j]][1], fz + corners[faces[i][j]][2], col[0], col[1], col[2], 1.0f, uv[i][j][0], uv[i][j][1] } );
						}
						//break;
					}
				}
			}
		}
	}

	glGenBuffers( 1, &iVAData );
	glGenVertexArrays( 1, &iVAPos );
	glBindVertexArray( iVAPos );
	glGenVertexArrays( 1, &iVACol );
	glBindVertexArray( iVACol );
	glGenVertexArrays( 1, &iVATex );
	glBindVertexArray( iVATex );
	glBindBuffer( GL_ARRAY_BUFFER, iVAData );
	glBufferData( GL_ARRAY_BUFFER, (int)( sizeof( Vertex ) * vertices.size() ), &vertices[0], GL_STATIC_DRAW );

	err = glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( "Failed to set vertex position buffer" );
		return false;
	}
	return true;										// Initialization Went OK
}

bool DrawGLScene()
{
	//#ifdef WIN32
	//OutputDebugStringA( "." );
	//#endif
	glm::mat4x4 view0, proj0;
	glm::mat4x4 view1, proj1;
	glm::mat4x4 view2, proj2;

	glm::mat4x4 view;
	glm::mat4x4 proj;

#ifdef USE_VIOSO_API
	//< start VIOSO API code
	glm::vec3 eye = { 0,0,0 };
	glm::vec3 rot = { 0,0,0 };

	if( pWarper )
	{

		{
			// either use this
			pWarper->GetViewProj( glm::value_ptr( eye ), glm::value_ptr( rot ), glm::value_ptr( view0 ), glm::value_ptr( proj0 ) );
		}

		{
			// or use this, which yields exact same result
			GLfloat clip[6];
			pWarper->GetViewClip( glm::value_ptr( eye ), glm::value_ptr( rot ), glm::value_ptr( view1 ), clip );
			proj1 = glm::frustumRH( -clip[0], clip[2], -clip[3], clip[1], clip[4], clip[5] );
		}

		{
			// or use this, which yields also exact same result
			GLfloat clip[6];
			GLfloat pos[3];
			GLfloat dir[3];

			pWarper->GetPosDirClip( &eye.x, &rot.x, pos, dir, clip );
			view2 = glm::transpose(
				glm::yawPitchRoll( -dir[1], dir[0], -dir[2] )
			);
			view2[0].w = pos[0];
			view2[1].w = pos[1];
			view2[2].w = pos[2];
			proj2 = glm::frustumRH( -clip[0], clip[2], -clip[3], clip[1], clip[4], clip[5] );
		}
		static int c = 0;
		if( c == 0 ) {
			view = view0;
			proj = proj0;
			c = 1;
		} else if( c == 1 ) {
			view = view1;
			proj = proj1;
			c = 2;
		} else {
			view = view2;
			proj = proj2;
			c = 0;
		}
	} else {
		view = glm::mat4(1); // identity
		proj = glm::frustum( -0.64f, 0.64f, -0.36f, 0.36f, 0.125f, 1000.125f ); // 16:10 frustum
	}

	//< end VIOSO API code
	#else
	view = glm::mat4(1); // identity
	proj = glm::frustum( -0.64f, 0.64f, -0.36f, 0.36f, 0.125f, 1000.125f ); // 16:10 frustum
	//GLFrustumRH( reinterpret_cast<GLfloat( & )[16]>( proj ), -0.64f, 0.64f, -0.36f, 0.36f, 0.125f, 1000.125f ); // 16:10 frustum
	#endif //def USE_VIOSO_API

	glUseProgram(iProg);
	/* Set background colour to light blue sky color */
	glClearColor( 0.6f, 0.8f, 1, 1.0f );

	/* Clear background with BLACK colour */
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glUniformMatrix4fv( locMatProj, 1, GL_FALSE, glm::value_ptr(proj) );
	glUniformMatrix4fv( locMatView, 1, GL_FALSE, glm::value_ptr(view) );

	glBindVertexArray( iVAPos );
	glBindBuffer( GL_ARRAY_BUFFER, iVAData );

	glEnableVertexAttribArray( locVAPos );
	glVertexAttribPointer( locVAPos, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void*)offsetof( Vertex, x ) ); // position

	if( -1 != locVACol )
	{
		glEnableVertexAttribArray( locVACol );
		glVertexAttribPointer( locVACol, 4, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void*)offsetof( Vertex, r ) ); // color
	}

	if( -1 != locVATex )
	{
		glEnableVertexAttribArray( locVATex );
		glVertexAttribPointer( locVATex, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void*)offsetof( Vertex, u ) ); // color
	}
	/* Actually draw the triangle, giving the number of vertices provided by invoke glDrawArrays
	   while telling that our data is a triangle and we want to draw 0-3 vertexes
	*/
	glDrawArrays( GL_TRIANGLES, 0, (int)vertices.size() );
	glDisableVertexAttribArray( locVAPos );
	if( -1 != locVACol )
	{
		glDisableVertexAttribArray( locVACol );
	}
	if( -1 != locVATex )
	{
		glDisableVertexAttribArray( locVATex );
	}
	#ifdef USE_VIOSO_API
	//< start VIOSO API code
	if( pWarper )
	{
		pWarper->Render( VWB_UNDEFINED_GL_TEXTURE, VWB_STATEMASK_PIXEL_SHADER | VWB_STATEMASK_SHADER_RESOURCE );
	}
	//< end VIOSO API code
	#endif //def USE_VIOSO_API

	return true;
}


#ifdef WIN32

HDC			hDC = NULL;		// Private GDI Device Context
HGLRC		hRC = NULL;		// Permanent Rendering Context
HWND		hWnd = NULL;		// Holds Our Window Handle
HINSTANCE	hInstance = 0;		// Holds The Instance Of The Application
#include <crtdbg.h>

#include <string>
#include <windows.h>
#include <shellscalingapi.h>

inline std::u8string wstring_to_u8string(const std::wstring& wstr) {
	if (wstr.empty()) return std::u8string();

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::u8string utf8(size_needed, u8'\0');
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1,
						 reinterpret_cast<char*>(utf8.data()), size_needed,
						 nullptr, nullptr);

	utf8.pop_back(); // remove null terminator
	return utf8;
}

LRESULT CALLBACK WndProc( HWND	hWnd,			// Handle For This Window
						  UINT	uMsg,			// Message For This Window
						  WPARAM	wParam,			// Additional Message Information
						  LPARAM	lParam )			// Additional Message Information
{
	switch( uMsg )									// Check For Windows Messages
	{
	case WM_SYSCOMMAND:							// Intercept System Commands
		{
			switch( wParam )							// Check System Calls
			{
			case SC_SCREENSAVE:					// Screensaver Trying To Start?
			case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;							// Prevent From Happening
			}
		}
		break;									// Exit

	case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage( 0 );						// Send A Quit Message
			return 0;								// Jump Back
		}

	case WM_KEYDOWN:							// Did We Receive Key?
		{
			if( VK_ESCAPE == wParam )
			{
				PostQuitMessage( 0 );						// Send A Quit Message
				return 0;								// Jump Back
			}
		}
		break;

	case WM_SIZE:								// Resize The OpenGL Window
		{
			ReSizeGLScene( LOWORD( lParam ), HIWORD( lParam ) );  // LoWord=Width, HiWord=Height
			return 0;								// Jump Back
		}
		break;
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

/*	This Code Creates Our OpenGL Window in Windows.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*/
BOOL CreateGLWindow( wchar_t const* title, int posX, int posY, int width, int height, int bits )
{

	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		rc = { posX, posY, posX + width, posY + height };				// Grabs Rectangle Upper Left / Lower Right Values

	if( 0 >= width || 0 >= height )
	{
		MONITORINFO mi = { 0 }; mi.cbSize = sizeof( mi );

		if( !GetMonitorInfo( MonitorFromPoint( *(POINT*)&rc, MONITOR_DEFAULTTONULL ), &mi ) )
		{
			cout << "Window Creation Error.";
			return FALSE;
		}
		rc = mi.rcMonitor;
	}

	hInstance = GetModuleHandle( NULL );				// Grab An Instance For Our Window
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc = (WNDPROC)WndProc;					// WndProc Handles Messages
	wc.cbClsExtra = 0;									// No Extra Window Data
	wc.cbWndExtra = 0;									// No Extra Window Data
	wc.hInstance = hInstance;							// Set The Instance
	wc.hIcon = LoadIcon( NULL, IDI_WINLOGO );			// Load The Default Icon
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );			// Load The Arrow Pointer
	wc.hbrBackground = NULL;									// No Background Required For GL
	wc.lpszMenuName = NULL;									// We Don't Want A Menu
	wc.lpszClassName = L"OpenGL";								// Set The Class Name

	if( !RegisterClass( &wc ) )									// Attempt To Register The Window Class
	{
		cout << "Failed To Register The Window Class.";
		return FALSE;											// Return FALSE
	}

	dwExStyle = WS_EX_APPWINDOW;								// Window Extended Style
	dwStyle = WS_POPUP;										// Windows Style

	AdjustWindowRectEx( &rc, dwStyle, FALSE, dwExStyle );		// Adjust Window To True Requested Size

	// Create The Window
	if( !( hWnd = CreateWindowEx( dwExStyle,							// Extended Style For The Window
								  L"OpenGL",							// Class Name
								  title,								// Window Title
								  dwStyle |							// Defined Window Style
								  WS_CLIPSIBLINGS |					// Required Window Style
								  WS_CLIPCHILDREN,					// Required Window Style
								  rc.left, rc.top,								// Window Position
								  rc.right - rc.left,	// Calculate Window Width
								  rc.bottom - rc.top,	// Calculate Window Height
								  NULL,								// No Parent Window
								  NULL,								// No Menu
								  hInstance,							// Instance
								  NULL ) ) )								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		cout << "Window Creation Error.";
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd =				// pfd Tells Windows How We Want Things To Be
	{
		sizeof( PIXELFORMATDESCRIPTOR ),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		(BYTE)bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		24,											// 16Bit Z-Buffer (Depth Buffer)  
		8,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};

	if( !( hDC = GetDC( hWnd ) ) )							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		cout << "Can't Create A GL Device Context.";
		return FALSE;								// Return FALSE
	}

	if( !( PixelFormat = ChoosePixelFormat( hDC, &pfd ) ) )	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		cout << "Can't Find A Suitable PixelFormat.";
		return FALSE;								// Return FALSE
	}

	if( !SetPixelFormat( hDC, PixelFormat, &pfd ) )		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		cout << "Can't Set The PixelFormat.";
		return FALSE;								// Return FALSE
	}


	if( !( hRC = wglCreateContext( hDC ) ) )				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		cout << "Can't Create A GL Rendering Context.";
		return FALSE;								// Return FALSE
	}

	if( !wglMakeCurrent( hDC, hRC ) )					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		cout << "Can't Activate The GL Rendering Context.";
		return FALSE;								// Return FALSE
	}

	ShowWindow( hWnd, SW_SHOW );						// Show The Window
	SetForegroundWindow( hWnd );						// Slightly Higher Priority
	SetFocus( hWnd );									// Sets Keyboard Focus To The Window

	#define GL_EXT_INITIALIZE
	#include "../../VIOSOWarpBlend/GL/GLext.h"

	if( !InitGL() )									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		cout << "Initialization Failed.";
		return FALSE;								// Return FALSE
	}

	ReSizeGLScene( rc.right - rc.left, rc.bottom - rc.top );					// Set Up Our Perspective GL Screen

	glEnable( GL_DEBUG_OUTPUT );
	return TRUE;									// Success
}

GLvoid KillGLWindow( GLvoid )								// Properly Kill The Window
{
	#ifdef USE_VIOSO_API
	if( pWarper )
		pWarper.reset();
	#endif //def USE_VIOSO_API

	if( hRC )											// Do We Have A Rendering Context?
	{
		if( !wglMakeCurrent( NULL, NULL ) )					// Are We Able To Release The DC And RC Contexts?
		{
			cout << "Release Of DC And RC Failed.";
		}

		if( !wglDeleteContext( hRC ) )						// Are We Able To Delete The RC?
		{
			cout << "Release Rendering Context Failed.";
		}
		hRC = NULL;										// Set RC To NULL
	}

	if( hDC && !ReleaseDC( hWnd, hDC ) )					// Are We Able To Release The DC
	{
		cout << "Release Device Context Failed.";
		hDC = NULL;										// Set DC To NULL
	}

	if( hWnd && !DestroyWindow( hWnd ) )					// Are We Able To Destroy The Window?
	{
		cout << "Could Not Release hWnd.";
		hWnd = NULL;										// Set hWnd To NULL
	}

	if( !UnregisterClass( L"OpenGL", hInstance ) )			// Are We Able To Unregister Class
	{
		cout << "Could Not Unregister Class.";
		hInstance = NULL;									// Set hInstance To NULL
	}
}

/// dpi awareness for this application, make sure to call before creating any windows
/// Microsoft recommends using the application manifest rather than this call.
HRESULT beDPIAware( PROCESS_DPI_AWARENESS pda ) {
	HRESULT hr = E_FAIL;
	typedef HRESULT( STDAPICALLTYPE* PFN_Set_Process_Dpi_Awareness )( _In_ PROCESS_DPI_AWARENESS value );
	auto hDll = ::LoadLibraryW( L"Shcore.dll" );
	if( hDll ) {
		auto SetProcessDpiAwareness = ( PFN_Set_Process_Dpi_Awareness )::GetProcAddress( hDll, "SetProcessDpiAwareness" );
		if( SetProcessDpiAwareness ) {
			hr = SetProcessDpiAwareness( pda );
		}
		::FreeLibrary( hDll );
	}
	return hr;
}

int wmain( int argc, wchar_t* argv[] )
{
	beDPIAware( PROCESS_PER_MONITOR_DPI_AWARE );

	MSG		msg;									// Windows Message Structure
	BOOL	done = FALSE;							// Bool Variable To Exit Loop

	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
	wstring channel = L"0";
	if( 1 < argc )
		configFile = argv[1];
	if( 2 < argc )
		channel = argv[2];
	if( 3 < argc )
		x = _wtoi( argv[3] );
	if( 4 < argc )
		y = _wtoi( argv[4] );
	if( 5 < argc )
		w = _wtoi( argv[5] );
	if( 6 < argc )
		h = _wtoi( argv[6] );

	// Create Our OpenGL Window
	if( !CreateGLWindow( L"NeHe's Solid Object Tutorial", x, y, w, h, 32 ) )
	{
		return 0;									// Quit If Window Was Not Created	
	}

	#ifdef USE_VIOSO_API
	try {
		pWarper = make_shared<VWB>( L"", nullptr, configFile, channel.c_str(), 3 );
	}
	catch( VWB_ERROR )
	{
		return FALSE;
	}
	if( VWB_ERROR_NONE != pWarper->Init() )
		return FALSE;

	float tl[3];
	float tr[3];
	float bl[3];
	float br[3];
	if( VWB_ERROR_NONE == pWarper->GetScreenplane( tl, tr, bl, br ) )
	{
		// review screen corners
		tl;
	}
	#endif //def USE_VIOSO_API


	while( !done )									// Loop That Runs While done=FALSE
	{
		if( !PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) // we are active, Is There A Message Waiting?
		{
			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if( !DrawGLScene() )	// Active?  Was There A Quit Received?
			{
				done = TRUE;							// ESC or DrawGLScene Signalled A Quit
			}
			else									// Not Time To Quit, Update Screen
			{
				SwapBuffers( hDC );					// Swap Buffers (Double Buffering)
			}
		}
		else if(
			WM_QUIT != msg.message
			) // do messages
		{
			TranslateMessage( &msg );				// Translate The Message
			DispatchMessage( &msg );				// Dispatch The Message
		}
		else
		{
			done = TRUE;
		}
	}

	// Shutdown
	KillGLWindow();									// Kill The Window
	return int( msg.wParam );							// Exit The Program
}
#else

Display* dpy{ nullptr };
Window root{ 0 };
GLint att[]{ GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
XVisualInfo* vi{ nullptr };
Colormap                cmap;
XSetWindowAttributes    swa;
Window                  win;
GLXContext              hRC = 0;
XWindowAttributes       gwa;
XEvent                  xev;


GLvoid KillGLWindow()
{
	if( hRC )
	{
		glXMakeCurrent( dpy, None, NULL );
		glXDestroyContext( dpy, hRC );
		hRC = 0;
	}
	if( win )
		XDestroyWindow( dpy, win );
	win = 0;
	if( dpy )
		XCloseDisplay( dpy );
	dpy = nullptr;
}

bool CreateGLWindow( char const* title, int posX, int posY, int width, int height, int bits )
{
	dpy = XOpenDisplay( NULL );
	if( nullptr == dpy )
	{
		logStr( "Can't connect to X server.\n" );
		return false;
	}
	root = DefaultRootWindow( dpy );
    if( width == 0 || height == 0 )
    {
        auto sc = ScreenOfDisplay(dpy,0);
        width = sc->width;
        height = sc->height;
    }
	vi = glXChooseVisual( dpy, 0, att );
	if( nullptr == vi )
	{
		logStr( "No appropriatevisual found.\n" );
		return false;
	}
	cmap = XCreateColormap( dpy, root, vi->visual, AllocNone );
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask;
	win = XCreateWindow( dpy, root, posX, posY, width, height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa );
	XMapWindow( dpy, win );
	XStoreName( dpy, win, title );

	hRC = glXCreateContext( dpy, vi, NULL, GL_TRUE );
	if( !hRC )
	{
		KillGLWindow();
		return false;
	}
	glXMakeCurrent( dpy, win, hRC );
	glEnable( GL_DEPTH_TEST );
	return true;
}

int main( int argc, char* argv[] )
{
	int x = 0;
	int y = 0;
	int w = 0;
	int h = 0;
	u8string channel = u8"0";
	if( 1 < argc )
		configFile = argv[1];
	if( 2 < argc )
		channel = (char8_t*)argv[2];
	if( 3 < argc )
		x = atoi( argv[3] );
	if( 4 < argc )
		y = atoi( argv[4] );
	if( 5 < argc )
		w = atoi( argv[5] );
	if( 6 < argc )
		h = atoi( argv[6] );

	// Create Our OpenGL Window
	if( !CreateGLWindow( "NeHe's Solid Object Tutorial", x, y, w, h, 32 ) )
		return 2;									// Quit If Window Was Not Created	

	#define GL_EXT_INITIALIZE
	#include "../../VIOSOWarpBlend/GL/GLext.h"

#ifdef USE_VIOSO_API
    try {
        pWarper = make_shared<VWB>( "", nullptr, configFile, channel.c_str(), 3 );
    }
    catch( VWB_ERROR )
    {
        return 0;
    }
    if( VWB_ERROR_NONE != pWarper->Init() )
        return 0;

    float tl[3];
    float tr[3];
    float bl[3];
    float br[3];
    if( VWB_ERROR_NONE == pWarper->GetScreenplane( tl, tr, bl, br ) )
    {
        // review screen corners
        tl;
    }
#endif //def USE_VIOSO_API


	if( !InitGL() )
		return 1;

	while( true )
	{
		XNextEvent( dpy, &xev );
		if( xev.type == Expose )
		{
			XGetWindowAttributes( dpy, win, &gwa );
			glViewport( 0, 0, gwa.width, gwa.height );
			DrawGLScene();
			glXSwapBuffers( dpy, win );
		}
		else if( xev.type == KeyPress )
		{
			KillGLWindow();
			break;
		}
	}
	return 0;
}
#endif //dev WIN32
