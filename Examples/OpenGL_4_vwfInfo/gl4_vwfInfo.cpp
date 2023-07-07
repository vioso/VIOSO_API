#ifdef WIN32

#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>		// Header File For Windows
HINSTANCE	g_hInstance;		// Holds The Instance Of The Application
struct WndBase
{
    HDC			hDC = NULL;					// Private GDI Device Context
    HGLRC		hRC = NULL;					// Permanent Rendering Context
    HWND		hWnd = NULL;				// Holds Our Window Handle
};

#else

#include <X11/X.h>
#include <X11/Xlib.h>
#define ARRAYSIZE(arr) (sizeof(arr)/sizeof(arr[0]))

struct WndBase
{
    Display*    disp = nullptr:
    Window		win{};
    GLXContext	hRC = 0;					// Permanent Rendering Context
};
#endif // def WIN32

#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <string>
#include <sstream>
#include <memory>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "../../Include/VIOSOWarpBlend.hpp"

#define GL_EXT_DEFINE_AND_IMPLEMENT
#include "../../VIOSOWarpBlend/GL/GLext.h"

const int c_numTri = 30;
const char* s_configFile = "VIOSOWarpBlendGL.ini";
struct MyWindow : WndBase
{
	GLuint		iProg = GLuint(-1);			// the shader program
	GLuint		iVert = GLuint(-1);			// the vertex shader program
	GLuint		iFrag = GLuint(-1);			// the fragment shader
	GLuint		iVAData = GLuint(-1);		// the data buffer
	GLuint		iVAPos = GLuint(-1);		// the vertex array for position
	GLuint		iVACol = GLuint(-1);		// the vertex array for color
	GLuint		iVATex = GLuint(-1);		// the vertex array for texture
	GLuint		locVAPos = GLuint(-1);		// the location of the position input
	GLuint		locVACol = GLuint(-1);		// the location of the color input
	GLuint		locVATex = GLuint(-1);		// the location of the texture input
	GLuint		locMatProj = GLuint(-1);	// the location of the projection matrix
	GLuint		locMatView = GLuint(-1);	// the location of the view matrix
};
VWBmap< HWND, MyWindow > g_windows;

typedef char GLchar;

#pragma pack( push, 4 )
struct Vertex
{
	GLfloat x, y, z;
	GLfloat r, g, b, a;
	GLfloat u, v;
};
#pragma pack(pop)

typedef std::vector<Vertex> Vertices;
Vertices g_vertices;

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

GLvoid ReSizeGLScene( GLsizei width, GLsizei height );		// Resize And Initialize The GL Window

#ifdef WIN32
#include <crtdbg.h>
#endif

inline void logStr( const char* str )
{
	//if( NULL != VWB__logString )
	//	VWB__logString( 1, str );
	//else
	{
		FILE* f;
		{
			if( NOERROR == fopen_s( &f, "VIOSOWarpBlend.log", "a" ) )
			{
				fprintf( f, str );
				fclose( f );
			}
		}
	}
}

void APIENTRY glLog( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam )
{
	char str[2047];
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
	sprintf_s( str, "GLDEBUG: %s %s: %s\n", szType, GL_DEBUG_SEVERITY_LOW == severity ? "note" : ( GL_DEBUG_SEVERITY_MEDIUM == severity ? "warning" : "ERROR" ), message );
	logStr( str );
}

GLvoid KillGLWindow( MyWindow& wnd )								// Properly Kill The Window
{
	if( wnd.hRC )											// Do We Have A Rendering Context?
	{
		wglMakeCurrent(NULL, NULL);

		if( !wglDeleteContext( wnd.hRC ) )						// Are We Able To Delete The RC?
		{
			MessageBox( NULL, "Release Rendering Context Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION );
		}
		wnd.hRC = NULL;										// Set RC To NULL
	}

	if( wnd.hDC && !ReleaseDC( wnd.hWnd, wnd.hDC ) )					// Are We Able To Release The DC
	{
		MessageBox( NULL, "Release Device Context Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION );
		wnd.hDC = NULL;										// Set DC To NULL
	}

	if( wnd.hWnd && !DestroyWindow( wnd.hWnd ) )					// Are We Able To Destroy The Window?
	{
		MessageBox( NULL, "Could Not Release hWnd.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION );
		wnd.hWnd = NULL;										// Set hWnd To NULL
	}

}

GLvoid ReSizeGLScene( GLsizei width, GLsizei height )		// Resize And Initialize The GL Window
{
	if( height == 0 )										// Prevent A Divide By Zero By
	{
		height = 1;										// Making Height Equal One
	}

	glViewport( 0, 0, width, height );						// Reset The Current Viewport
}

int InitGL( MyWindow& wnd )										// All Setup For OpenGL Goes Here
{
	GLenum err = glGetError();

	#define GL_EXT_INITIALIZE
	#include "../../VIOSOWarpBlend/GL/GLext.h"

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
	wnd.iVert = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( wnd.iVert, 1, &pszShaders[0], NULL );
	glCompileShader( wnd.iVert );
	char log[2000] = { 0 };
	glGetShaderInfoLog( wnd.iVert, 2000, NULL, log );
	if( log[0] )
	{
		logStr( log );
		return FALSE;
	}

	// fragment shader
	wnd.iFrag = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( wnd.iFrag, 1, &pszShaders[1], NULL );
	glCompileShader( wnd.iFrag );
	glGetShaderInfoLog( wnd.iFrag, 2000, NULL, log );
	if( log[0] )
	{
		logStr( log );
		return FALSE;
	}
	err = glGetError();

	wnd.iProg = glCreateProgram();
	glAttachShader( wnd.iProg, wnd.iVert );
	glAttachShader( wnd.iProg, wnd.iFrag );
	glLinkProgram( wnd.iProg );
	err = glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( "Failed to link shader program" );
		return FALSE;
	}
	GLint isLinked = 0;
	glGetProgramiv( wnd.iProg, GL_LINK_STATUS, &isLinked );
	if( isLinked == GL_FALSE )
	{
		GLint maxLength = 0;
		glGetProgramiv( wnd.iProg, GL_INFO_LOG_LENGTH, &maxLength );

		//The maxLength includes the NULL character
		std::vector<GLchar> infoLog( maxLength );
		glGetProgramInfoLog( wnd.iProg, maxLength, &maxLength, &infoLog[0] );

		//The program is useless now. So delete it.
		glDeleteProgram( wnd.iProg );
		wnd.iProg = -1;

		char log[1024];
		sprintf_s( log, "ERROR: %d at glLinkProgram:\n%s\n", err, &infoLog[0] );
		logStr( log );
		return FALSE;
	}

	wnd.locMatProj = glGetUniformLocation( wnd.iProg, "matProj" );
	wnd.locMatView = glGetUniformLocation( wnd.iProg, "matView" );
	wnd.locVAPos = glGetAttribLocation( wnd.iProg, "in_position" );
	wnd.locVACol = glGetAttribLocation( wnd.iProg, "in_color" );
	wnd.locVATex = glGetAttribLocation( wnd.iProg, "in_tex" );
	glUseProgram( wnd.iProg );

	err = glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( "Failed to create vertex position buffer" );
		return FALSE;
	}

	glGenBuffers( 1, &wnd.iVAData );
	glGenVertexArrays( 1, &wnd.iVAPos );
	glBindVertexArray( wnd.iVAPos );
	glGenVertexArrays( 1, &wnd.iVACol );
	glBindVertexArray( wnd.iVACol );
	glGenVertexArrays( 1, &wnd.iVATex );
	glBindVertexArray( wnd.iVATex );
	glBindBuffer( GL_ARRAY_BUFFER, wnd.iVAData );
	glBufferData( GL_ARRAY_BUFFER, (int)( sizeof( Vertex ) * g_vertices.size() ), g_vertices.data(), GL_STATIC_DRAW );

	err = glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( "Failed to set vertex position buffer" );
		return FALSE;
	}
	return TRUE;										// Initialization Went OK
}

bool DrawGLScene( VWBmap< HGLRC, MyWindow >::mapped_type::element_type& wnd )
{
	if(!wnd.w)
		return false;

	wglMakeCurrent( wnd.hDC, wnd.hRC );

	glm::mat4x4 view, proj;

	glm::vec3 eye = { 0,0,0 };
	glm::vec3 rot = { 0,0,0 };
	wnd.w.GetViewProj( glm::value_ptr( eye ), glm::value_ptr( rot ), glm::value_ptr( view ), glm::value_ptr( proj ) );



	/* Set background colour to light blue sky color */
	glClearColor( 0.6f, 0.8f, 1, 1.0f );

	/* Clear background with BLACK colour */
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glUseProgram( wnd.iProg );
	glUniformMatrix4fv( wnd.locMatProj, 1, GL_FALSE, glm::value_ptr(proj) );
	glUniformMatrix4fv( wnd.locMatView, 1, GL_FALSE, glm::value_ptr(view) );

	glBindVertexArray( wnd.iVAPos );
	glBindBuffer( GL_ARRAY_BUFFER, wnd.iVAData );

	glEnableVertexAttribArray( wnd.locVAPos );
	glVertexAttribPointer( wnd.locVAPos, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void*)offsetof( Vertex, x ) ); // position

	if( -1 != wnd.locVACol )
	{
		glEnableVertexAttribArray( wnd.locVACol );
		glVertexAttribPointer( wnd.locVACol, 4, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void*)offsetof( Vertex, r ) ); // color
	}

	if( -1 != wnd.locVATex )
	{
		glEnableVertexAttribArray( wnd.locVATex );
		glVertexAttribPointer( wnd.locVATex, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (void*)offsetof( Vertex, u ) ); // color
	}
	/* Actually draw the triangle, giving the number of vertices provided by invoke glDrawArrays
	   while telling that our data is a triangle and we want to draw 0-3 vertexes
	*/
	glDrawArrays( GL_TRIANGLES, 0, (int)g_vertices.size() );
	glDisableVertexAttribArray( wnd.locVAPos );
	if( -1 != wnd.locVACol )
	{
		glDisableVertexAttribArray( wnd.locVACol );
	}
	if( -1 != wnd.locVATex )
	{
		glDisableVertexAttribArray( wnd.locVATex );
	}

	wnd.w.Render( VWB_UNDEFINED_GL_TEXTURE, VWB_STATEMASK_PIXEL_SHADER | VWB_STATEMASK_SHADER_RESOURCE );

	return true;
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
		break;									// Exit
	}

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

	case WM_SIZE:								// Resize The OpenGL Window
	{
		auto wnd = g_windows.find(hWnd);
		if(wnd != g_windows.end())
		{
			MyWindow const* wnd = g_windows[hWnd].get();
			wglMakeCurrent(wnd->hDC, wnd->hRC);
			ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));  // LoWord=Width, HiWord=Height
		}
		return 0;								// Jump Back
	}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

bool LoadVertices()
{
	// render some cubes around 0,0,0
	static const int s_nCubes = 9;
	static const int s_nnCubes = 2 * s_nCubes * s_nCubes + 2 * s_nCubes * ( s_nCubes - 2 ) + 2 * ( s_nCubes - 2 ) * ( s_nCubes - 2 );
	static const float sz = 10.0f / ( 3 * s_nCubes - 1 );
	static const int gg = s_nCubes / 2;
	g_vertices.clear();
	g_vertices.reserve( s_nnCubes );

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
							g_vertices.push_back( Vertex{ fx + corners[faces[i][j]][0], fy + corners[faces[i][j]][1], fz + corners[faces[i][j]][2], col[0], col[1], col[2], 1.0f, uv[i][j][0], uv[i][j][1] } );
						}
						g_vertices.push_back( Vertex{ fx + corners[faces[i][0]][0], fy + corners[faces[i][0]][1], fz + corners[faces[i][0]][2], col[0], col[1], col[2], 1.0f, uv[i][0][0], uv[i][0][1] } );
						for( int j = 2; j != 4; j++ )
						{
							g_vertices.push_back( Vertex{ fx + corners[faces[i][j]][0], fy + corners[faces[i][j]][1], fz + corners[faces[i][j]][2], col[0], col[1], col[2], 1.0f, uv[i][j][0], uv[i][j][1] } );
						}
						//break;
					}
				}
			}
		}
	}
	return true;
}


bool RegisterWndClass()
{
	WNDCLASS wc{
		CS_HREDRAW | CS_VREDRAW | CS_OWNDC,	// Redraw On Size, And Own DC For Window.
		(WNDPROC)WndProc,					// WndProc Handles Messages
		0,									// No Extra Window Data
		0,									// No Extra Window Data
		g_hInstance,							// Set The Instance
		LoadIcon(NULL, IDI_WINLOGO),			// Load The Default Icon
		LoadCursor(NULL, IDC_ARROW),			// Load The Arrow Pointer
		NULL,									// No Background Required For GL
		NULL,									// We Don't Want A Menu
		"OpenGL"								// Set The Class Name
	};
	if( !RegisterClass( &wc ) )									// Attempt To Register The Window Class
	{
		MessageBox( NULL, "Failed To Register The Window Class.", "ERROR", MB_OK | MB_ICONEXCLAMATION );
		return false;											// Return FALSE
	}
	return true;
}

bool unregisterWndClass()
{
	return FALSE != UnregisterClass( "OpenGL", g_hInstance );
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*/

BOOL CreateGLWindow( MyWindow& wnd, char const* title, int posX, int posY, int width, int height, int bits )
{

	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		rc = { posX, posY, posX + width, posY + height };				// Grabs Rectangle Upper Left / Lower Right Values
	if( 0 >= width || 0 >= height )
	{
		MONITORINFO mi = { 0 }; mi.cbSize = sizeof( mi );

		if( !GetMonitorInfo( MonitorFromPoint( *(POINT*)&rc, MONITOR_DEFAULTTONULL ), &mi ) )
		{
			MessageBox( NULL, "Window Creation Error.", "ERROR", MB_OK | MB_ICONEXCLAMATION );
			return FALSE;
		}
		rc = mi.rcMonitor;
	}


	dwExStyle = WS_EX_APPWINDOW;								// Window Extended Style
	dwStyle = WS_POPUP;										// Windows Style

	AdjustWindowRectEx( &rc, dwStyle, FALSE, dwExStyle );		// Adjust Window To True Requested Size

	
	// Create The Window
	if( !( wnd.hWnd = CreateWindowEx( dwExStyle,							// Extended Style For The Window
								  "OpenGL",							// Class Name
								  title,								// Window Title
								  dwStyle |							// Defined Window Style
								  WS_CLIPSIBLINGS |					// Required Window Style
								  WS_CLIPCHILDREN,					// Required Window Style
								  rc.left, rc.top,								// Window Position
								  rc.right - rc.left,	// Calculate Window Width
								  rc.bottom - rc.top,	// Calculate Window Height
								  NULL,								// No Parent Window
								  NULL,								// No Menu
								  g_hInstance,							// Instance
								  NULL ) ) )								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow(wnd);								// Reset The Display
		MessageBox( NULL, "Window Creation Error.", "ERROR", MB_OK | MB_ICONEXCLAMATION );
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

	if( !( wnd.hDC = GetDC( wnd.hWnd ) ) )							// Did We Get A Device Context?
	{
		KillGLWindow( wnd );								// Reset The Display
		MessageBox( NULL, "Can't Create A GL Device Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION );
		return FALSE;								// Return FALSE
	}

	if( !( PixelFormat = ChoosePixelFormat( wnd.hDC, &pfd ) ) )	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow( wnd );								// Reset The Display
		MessageBox( NULL, "Can't Find A Suitable PixelFormat.", "ERROR", MB_OK | MB_ICONEXCLAMATION );
		return FALSE;								// Return FALSE
	}

	if( !SetPixelFormat( wnd.hDC, PixelFormat, &pfd ) )		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow( wnd );								// Reset The Display
		MessageBox( NULL, "Can't Set The PixelFormat.", "ERROR", MB_OK | MB_ICONEXCLAMATION );
		return FALSE;								// Return FALSE
	}

	if( !( wnd.hRC = wglCreateContext( wnd.hDC ) ) )				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow( wnd );								// Reset The Display
		MessageBox( NULL, "Can't Create A GL Rendering Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION );
		return FALSE;								// Return FALSE
	}

	if( !wglMakeCurrent( wnd.hDC, wnd.hRC ) )					// Try To Activate The Rendering Context
	{
		KillGLWindow( wnd );								// Reset The Display
		MessageBox( NULL, "Can't Activate The GL Rendering Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION );
		return FALSE;								// Return FALSE
	}

	ShowWindow( wnd.hWnd, SW_SHOW );						// Show The Window
	SetForegroundWindow( wnd.hWnd );						// Slightly Higher Priority
	SetFocus( wnd.hWnd );									// Sets Keyboard Focus To The Window

	if( !InitGL(wnd) )									// Initialize Our Newly Created GL Window
	{
		KillGLWindow( wnd );								// Reset The Display
		MessageBox( NULL, "Initialization Failed.", "ERROR", MB_OK | MB_ICONEXCLAMATION );
		return FALSE;								// Return FALSE
	}
	
	std::string sTitle( title );
	sTitle += " ";
	sTitle += (char const*)glGetString( GL_VERSION );
	SetWindowTextA( wnd.hWnd, sTitle.c_str() );

	ReSizeGLScene( rc.right - rc.left, rc.bottom - rc.top );					// Set Up Our Perspective GL Screen
	return TRUE;									// Success
}

int WINAPI WinMain( HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow )			// Window Show State
{
	MSG		msg{};									// Windows Message Structure
	BOOL	done = FALSE;								// Bool Variable To Exit Loop
	g_hInstance = hInstance;				// Grab An Instance For Our Window

	char const* calibFile = lpCmdLine;
	if( 0 == calibFile[0] )
		calibFile = "JK_ccs.vwf";

	if(!LoadVertices())
		return -1;

	VWB_WarpBlendHeaderSet set;
	if( VWB_ERROR_NONE != VWB::VwfInfo( (const char*)nullptr, calibFile, &set) )
		return -1;

	char hostname[] = "localhost";

	RegisterWndClass();

	for( size_t i = 0; i != set.size(); i++ )
	{
		// match host
		if( set[i]->header.hostname[0] && _stricmp( hostname, set[i]->header.hostname ) ) // hostname set but different
			continue;

		MyWindow wnd{};
		// Create Our OpenGL Window
		if( !CreateGLWindow( wnd, "NeHe's Solid Object Tutorial", (int)set[i]->header.offsetX, (int)set[i]->header.offsetY, (int)set[i]->header.width, (int)set[i]->header.height, 32 ) )
		{
			return 0;									// Quit If Window Was Not Created	
		}

		std::shared_ptr<VWBX<MyWindow >> w;
		try {
			w = g_windows.emplace( wnd.hWnd, std::make_shared<VWBX<MyWindow>>( wnd, "", nullptr, s_configFile, set[i]->header.name ) ).first->second;
		}
		catch( VWB_ERROR )
		{
			return FALSE;
		}
		strcpy_s( w->w.get().calibFile, calibFile );
		w->w.get().calibIndex = (int)i;
		if( VWB_ERROR_NONE != w->w.Init() )
			return FALSE;
	}
	// release memory
	VWB::VwfInfo( (const char*)nullptr, nullptr, &set);

	while( !done )									// Loop That Runs While done=FALSE
	{
		if( !PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) // we are active, Is There A Message Waiting?
		{
			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if(g_windows.empty())
				done = TRUE;
			for( auto& w : g_windows )
			{
				if( !DrawGLScene( *w.second ) )
				{
					done = TRUE;							// ESC or DrawGLScene Signalled A Quit
				}
				else									// Not Time To Quit, Update Screen
				{
					SwapBuffers( w.second->hDC );					// Swap Buffers (Double Buffering)
				}
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
	for(auto& w : g_windows)
	{
		// we need to make a copy of the window data, as we need the context intact to release resources gracefully
		MyWindow wnd = *w.second;
		// release the warper
		w.second.reset();
		// Kill The Window
		KillGLWindow(wnd);
	}
	unregisterWndClass();
	return int( msg.wParam );							// Exit The Program
}
