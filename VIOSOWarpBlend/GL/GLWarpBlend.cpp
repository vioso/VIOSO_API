// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "GLWarpBlend.h"
#include "pixelshader.h"

#define GL_EXT_DEFINE_AND_IMPLEMENT
#include "GLext.h"
#include <filesystem>
#include <fstream>
#include "../VWF.h"
// check for windows platform
#if defined( WIN32 ) || defined( WIN64 )  // via VCPKG
	#include <stb_image.h>
#else // apt installs to subdir
	#include "stb/stb_image.h"
#endif
GLfloat GLWarpBlend::colBlack[4] = {0,0,0,0};
//save a texture to .bmp image
bool GLWarpBlend::savetex( std::filesystem::path filename ,GLint iTex )
{// get the image data
	bool bRet = false;
	GLenum err = glGetError();
	GLint oat;
	GLint ob0;
	glGetIntegerv( GL_ACTIVE_TEXTURE, &oat );
	glActiveTexture( GL_TEXTURE0 );
	glGetIntegerv( GL_TEXTURE_BINDING_2D, &ob0 );
	glBindTexture( GL_TEXTURE_2D, iTex );
	err = glGetError();
	GLint x,y;
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &x );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &y );
	err = glGetError();
	if( err == glGetError() )
	{

		long imageSize = x * y * 4;
		if (0 < imageSize)
		{
			unsigned char *data = new unsigned char[imageSize];
			if (data)
			{
				// note: requesting GL_BGRA_EXT format solves swizzling to BGRA for Windows Bitmaps
				glGetTexImage( GL_TEXTURE_2D, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data );
				if( GL_NO_ERROR == glGetError() )
				{
					// note: we need no row padding for 32bit bitmaps
					const VWB_uint pitch = 4 * x;
					BITMAPINFOHEADER hdr = { 0 };
					hdr.biSize = sizeof( hdr );
					hdr.biWidth = x;
					// note: the sign of biHeight specifies if the bitmap is upside down, positive is bottom-up like GL texture data
					hdr.biHeight = VWB_int( y );
					hdr.biPlanes = 1;
					hdr.biBitCount = 32;
					hdr.biSizeImage = pitch * y;

					BITMAPFILEHEADER fh = { 0 };
					fh.bfType = 'MB';
					fh.bfOffBits = sizeof( fh ) + hdr.biSize;
					fh.bfSize = fh.bfOffBits + hdr.biSizeImage;

					std::ofstream f( filename, std::ios::binary );
					if( f.is_open() )
					{
						f.write( reinterpret_cast<char*>(&fh), sizeof( fh ) );
						f.write( reinterpret_cast<char*>(&hdr), sizeof( hdr ) );
						f.write( reinterpret_cast<char*>(data), hdr.biSizeImage );
						bRet = true;
					}
				}
				delete[] data;
				data = NULL;
			}
		}
	}
	glBindTexture( GL_TEXTURE_2D, ob0 );
	glActiveTexture( oat );
	return bRet;
}

GLWarpBlend::GLWarpBlend()
	: m_texBlend( -1 )
	, m_texBlendX( -1 )
	, m_texWarp( -1 )
	, m_texBlack( -1 )
	, m_tex2ndBlend( -1 )
	, m_texDirectionalShading( -1 )
	, m_texBB( -1 )
	, m_texOverlay( -1 )
	, m_texOverlay2( -1 )

	, m_locCamPos( -1 )
	, m_locWarp( -1 )
	, m_locBorder( -1 )
	, m_locBlend( -1 )
	, m_locBlack( -1 )
	, m_loc2ndBlend( -1 )
	, m_locDirectionalShading( -1 )
	, m_locDoNotBlend( -1 )
	, m_locDoNoBlack( -1 )
	, m_locOffsScale( -1 )
	, m_locContent( -1 )
	, m_locContentBypass( -1 )
	, m_locMatView( -1 )
	, m_locDim(-1)
	, m_locSmooth(-1)
	, m_locBlackBias( -1 )
	, m_locParamsBicubic(-1)
	, m_locParamsDomeprojection(-1)

	, m_iVertexArray(-1)
	, m_iVertices(-1)
	, m_iIndices( -1 )

	, m_Program(-1)
	, m_ProgramBypass(-1)
	, m_nIndices( 0 )
{
	logStr( 1, "INFO: Start initializing OGL-Warper...\n" );

	#define GL_EXT_TEST
	if(
		#include "GLext.h"
		)
	{
		logStr( 2, "glXXX functions already present.\n" );
	}
	else
	{
		logStr( 2, "Loading glXXX functions...\n" );
		#define GL_EXT_INITIALIZE
		#include "GLext.h"
	}

	#define GL_EXT_TEST_VERBOSE
	if(
		#include "GLext.h"
		)
	{
		memcpy( &m_type4cc, "OGL\0", 4 );
	}
	else
	{
		throw (VWB_int)VWB_ERROR_NOT_IMPLEMENTED;
	}
}

GLWarpBlend::~GLWarpBlend()
{
	if( -1 != m_iIndices )
		glDeleteBuffers( 1, &m_iIndices );

	if( -1 != m_iVertices )
		glDeleteBuffers( 1, &m_iVertices );

	if( -1 != m_iVertexArray )
		glDeleteVertexArrays( 1, &m_iVertexArray );

	if( -1 != m_Program )
		glDeleteProgram( m_Program );

	if( -1 != m_ProgramBypass )
		glDeleteProgram( m_ProgramBypass );

	if( -1 != m_texOverlay )
		glDeleteTextures( 1, &m_texOverlay );
	if( -1 != m_texBB )
		glDeleteTextures( 1, &m_texBB );
	if( -1 != m_texWarp )
		glDeleteTextures( 1, &m_texWarp );
	if( -1 != m_texBlend )
		glDeleteTextures( 1, &m_texBlend );
	if( -1 != m_texBlendX )
		glDeleteTextures( 1, &m_texBlendX );
	if( -1 != m_texBlack )
		glDeleteTextures( 1, &m_texBlack );
	if( -1 != m_tex2ndBlend )
		glDeleteTextures( 1, &m_tex2ndBlend );
	if( -1 != m_texDirectionalShading )
		glDeleteTextures( 1, &m_texDirectionalShading );
	logStr( 1, "INFO: OGL-Warper uninitialized.\n" );
}

// create bypass shader
struct ShaderInit {
	GLuint numShaders;
	GLchar const* shaders[4];
	GLenum type;
	char const* debugName;
};

GLuint CompileFromSource( ShaderInit const& si ) {
	GLuint iS = glCreateShader( si.type );
	GLchar log[2048];
	glShaderSource( iS, si.numShaders, (const GLchar**)si.shaders, NULL );
	glCompileShader( iS );
	auto err = ::glGetError();
	glGetShaderInfoLog( iS, 2048, NULL, log );
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: %d at glCompileShader (%s) \"%s\".\n", err );
	}
	if( log[0] )
	{
		logStr( 0, "INFO: glCompileShader (%s) \"%s\":\n%s\n", si.type == GL_VERTEX_SHADER ? "vertex" : "fragment", si.debugName, log );
	}
	if( GL_NO_ERROR != err )
	{
		if( iS != -1 )
			glDeleteShader( iS );
		iS = -1;
	}
	return iS;
}

// create program from compiled shaders
GLuint CreateProgram( std::vector<GLuint> shaders, char const* debugName, std::vector<std::pair<GLuint*,const char*>> locations ) {
	//  { iVS, iFSB }, "bypass", { { m_locContentBypass, "samContent" }, { m_locSizeBypass, "size" } }
	auto iP = glCreateProgram();
	auto err = ::glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: %d at glCreateProgram %s.\n", err, debugName );
		return VWB_ERROR_SHADER;
	}
	for( int i = 0; i != shaders.size(); i++ ) {
		if( shaders[i] == -1 )
			continue;
		glAttachShader( iP, shaders[i] );
		err = ::glGetError();
		if( GL_NO_ERROR != err ) {
			logStr( 0, "ERROR: %d at glAttachShader #%d to program %s.\n", err, i, debugName );
			return VWB_ERROR_SHADER;
		}
	}

	glLinkProgram( iP );
	err = ::glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: %d at glLinkProgram %s.\n", err, debugName );
		return VWB_ERROR_SHADER;
	}

	GLint isLinked = 0;
	glGetProgramiv( iP, GL_LINK_STATUS, &isLinked );
	if( isLinked == GL_FALSE )
	{
		GLint maxLength = 0;
		glGetProgramiv( iP, GL_INFO_LOG_LENGTH, &maxLength );

		//The maxLength includes the NULL character
		std::vector<GLchar> infoLog( maxLength );
		glGetProgramInfoLog( iP, maxLength, &maxLength, infoLog.data() );

		//The program is useless now. So delete it.
		glDeleteProgram( iP );
		iP = -1;

		logStr( 0, "ERROR: %d at glLinkProgram bypass %s:\n%s\n", err, debugName, infoLog.data() );
		return VWB_ERROR_SHADER;
	}
	for( auto& loc : locations ) {
		*( loc.first ) = glGetUniformLocation( iP, loc.second );
	}
	return iP;
}

VWB_ERROR GLWarpBlend::CreateShaders()
{
	GLenum err = 0;

	ShaderInit const s[] = {
		{0,{},GL_VERTEX_SHADER,""}, // not used
		{3,{s_fragment_shader_header_v110,s_func_tex2D,s_bypass_fragment_shader},GL_FRAGMENT_SHADER,"VIOSO v110 bypass"},
		{3,{s_fragment_shader_header_v110,s_func_tex2D,s_warp_blend_fragment_shader},GL_FRAGMENT_SHADER,"VIOSO v110 2D"},
		{3,{s_fragment_shader_header_v110,s_func_tex2D,s_warp_blend_fragment_shader_3D},GL_FRAGMENT_SHADER,"VIOSO v110 3D"},
		{3,{s_fragment_shader_header_v110,s_func_tex2D_BC,s_bypass_fragment_shader},GL_FRAGMENT_SHADER,"VIOSO v110 bypass bicubic"},
		{3,{s_fragment_shader_header_v110,s_func_tex2D_BC,s_warp_blend_fragment_shader},GL_FRAGMENT_SHADER,"VIOSO v110 2D bicubic"},
		{3,{s_fragment_shader_header_v110,s_func_tex2D_BC,s_warp_blend_fragment_shader_3D},GL_FRAGMENT_SHADER,"VIOSO v110 3D bicubic"},

		{1,{s_szPasstrough_vertex_shader_v330},GL_VERTEX_SHADER,"Pass-through shader"},
		{3,{s_fragment_shader_header_v330,s_func_tex2D,s_bypass_fragment_shader},GL_FRAGMENT_SHADER},
		{3,{s_fragment_shader_header_v330,s_func_tex2D,s_warp_blend_fragment_shader},GL_FRAGMENT_SHADER},
		{3,{s_fragment_shader_header_v330,s_func_tex2D,s_warp_blend_fragment_shader_3D},GL_FRAGMENT_SHADER},
		{3,{s_fragment_shader_header_v330,s_func_tex2D_BC,s_bypass_fragment_shader},GL_FRAGMENT_SHADER},
		{3,{s_fragment_shader_header_v330,s_func_tex2D_BC,s_warp_blend_fragment_shader},GL_FRAGMENT_SHADER},
		{3,{s_fragment_shader_header_v330,s_func_tex2D_BC,s_warp_blend_fragment_shader_3D},GL_FRAGMENT_SHADER},

		{1,{s_sz_vertex_shader_v330_dp},GL_VERTEX_SHADER,"Domeprojection shader"},
		{3,{s_fragment_shader_header_v330_dp,s_func_tex2D,s_bypass_fragment_shader_dp},GL_FRAGMENT_SHADER,"DP bypass"},
		{3,{s_fragment_shader_header_v330_dp,s_func_tex2D,s_warp_blend_fragment_shader_dp},GL_FRAGMENT_SHADER,"DP"},
		{3,{s_fragment_shader_header_v330_dp,s_func_tex2D,s_warp_blend_fragment_shader_dp},GL_FRAGMENT_SHADER,"DP"},
		{3,{s_fragment_shader_header_v330_dp,s_func_tex2D_BC,s_bypass_fragment_shader_dp},GL_FRAGMENT_SHADER,"DP bypass bicubic"},
		{3,{s_fragment_shader_header_v330_dp,s_func_tex2D_BC,s_warp_blend_fragment_shader_dp},GL_FRAGMENT_SHADER,"DP bicubic"},
		{3,{s_fragment_shader_header_v330_dp,s_func_tex2D_BC,s_warp_blend_fragment_shader_dp},GL_FRAGMENT_SHADER,"DP bicubic"},
	};
	err = ::glGetError();
	if( GL_NO_ERROR != err )
		logStr( 1, "WARNING: %d before glCreateShader (vertex).\n", err );

	GLint iS = 0;
	GLuint iVS = -1;
	if( m_bDP ) {
		if( bUseGL110 ) {
			logStr( 0, "ERROR: Fixed pipeline openGL is not supported with someprojection shader.\n" );
			return VWB_ERROR_SHADER;
		}
		logStr( 2, "INFO: using #version 330 domeprojection shader.\n" );
		iS += 14;
		iVS = CompileFromSource( s[iS] );
	}
	else if( !bUseGL110 )
	{
		logStr( 2, "INFO: using #version 330 shader.\n" );
		iS += 7;
		iVS = CompileFromSource( s[iS] );
	} else {
		logStr( 2, "INFO: using #version 110 shader.\n" );
		// no vertex shader
	}

	// increment to bypass fragment shader
	iS++;

	// if bicubic, increment to bicubic bypass shader
	if( bBicubic )
		iS += 3;

	auto iFSB = CompileFromSource( s[iS] );
	if( -1 == iFSB )
		return VWB_ERROR_SHADER;

	m_ProgramBypass = CreateProgram( { iVS, iFSB }, "bypass", { { &m_locContentBypass, "samContent" } } );
	if( -1 == m_ProgramBypass )
		return VWB_ERROR_SHADER;
	glDeleteShader( iFSB );

	// increment to warp shader
	iS++;

	// if dynamic, increment to 3D shader
	if( m_bDynamicEye )
		iS++;

	auto iFS = CompileFromSource( s[iS] );
	if( -1 == iFSB )
		return VWB_ERROR_SHADER;
	m_Program = CreateProgram( { iVS,iFS }, "warp", {
		{ &m_locCamPos, "camPos" },
		{ &m_locContent, "samContent" },
		{ &m_locWarp, "samWarp" },
		{ &m_locBorder, "bBorder" },
		{ &m_locBlend, "samBlend" },
		{ &m_locBlack, "samBlack" },
		{ &m_loc2ndBlend, "samBlend2" },
		{ &m_locDirectionalShading, "samDirectionalShading" },
		{ &m_locDoNotBlend, "bDoNotBlend" },
		{ &m_locDoNoBlack, "bDoNoBlack" },
		{ &m_locOffsScale, "offsScale" },
		{ &m_locMatView, "matView" },
		{ &m_locDim, "dim" },
		{ &m_locSmooth, "range" },
		{ &m_locBlackBias, "blackBias" },
		{ &m_locParamsBicubic, "paramsBicubic" },
		{ &m_locParamsDomeprojection, "params" }
							   } );
	return VWB_ERROR_NONE;
	if( -1 == m_locContent || -1 == m_locContentBypass )
	{
		logStr( 0, "WARINIG: Shader content input missing.\n" );
		return VWB_ERROR_SHADER;
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR GLWarpBlend::FillTexture( GLuint& iTex, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid const* data, GLint filter, GLenum wrap, char const* name, bool save )
{
	if( nullptr == name )
		name = "unnamed";

	glGenTextures( 1, &iTex );
	glBindTexture( GL_TEXTURE_2D, iTex );
	GLenum err = ::glGetError();
	glTexImage2D( GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data );
	err = ::glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "Failed to create %s texture, error %d while glTexImage2D.\n", name, err );
		return VWB_ERROR_BLEND;
	}
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter );
	err = ::glGetError();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter );
	err = ::glGetError();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap );
	err = ::glGetError();
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap );
	err = ::glGetError();
	glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, colBlack );
	err = ::glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: Failed to create %s texture, error %d while setting parameters.\n", name, err );
		return VWB_ERROR_BLEND;
	}

	logStr( 2, "INFO: Succeeded to create %s texture (%dx%d).\n", name, width, height );

	// to save, we need allowance, data and log level >=4
	if( save && data && 4 <= g_logLevel )
	{
		std::filesystem::path o(g_logFilePath);
		o += ".tex.";
		o += name;
		o += +".bmp";
		if( savetex( o.string().c_str(), iTex ) )
			logStr( 4, "INFO: Saved %s texture (%dx%d) saved as \"%s\".", name, width, height, o.string().c_str() );
		else
			logStr( 4, "WARNING: Failed to save %s texture (%dx%d) saved as \"%s\".", name, width, height, o.string().c_str() );
	}

	return VWB_ERROR_NONE;
}

VWB_ERROR GLWarpBlend::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = VWB_Warper_base::Init( wbs );

	if( VWB_ERROR_NONE == err ) try
	{
		err = CreateShaders();
		if( VWB_ERROR_NONE != err )
			return err;

		// create vertex array
		glGenVertexArrays( 1, &m_iVertexArray );
		glBindVertexArray( m_iVertexArray );

		if( m_bDP ) {
			if( !wbs[calibIndex]->pMesh ) {
				logStr( 0, "FATAL: No mesh present.\n", err );
				return VWB_ERROR_WARP;
			}

			// create and fill vertex buffer
			glGenBuffers( 1, &m_iVertices );
			std::vector<Vertex> vertices( wbs[calibIndex]->pMesh->nVtx );
			for( VWB_uint i = 0; i != wbs[calibIndex]->pMesh->nVtx; i++ ) {
				vertices[i].x = wbs[calibIndex]->pMesh->vtx[i].pos[0];
				vertices[i].y = wbs[calibIndex]->pMesh->vtx[i].pos[1];
				vertices[i].z = wbs[calibIndex]->pMesh->vtx[i].pos[2];
				vertices[i].u = wbs[calibIndex]->pMesh->vtx[i].uv[0];
				vertices[i].v = wbs[calibIndex]->pMesh->vtx[i].uv[1];
				vertices[i].nx = wbs[calibIndex]->pMesh->vtx[i].n[0];
				vertices[i].ny = wbs[calibIndex]->pMesh->vtx[i].n[1];
				vertices[i].nz = wbs[calibIndex]->pMesh->vtx[i].n[2];
				vertices[i].tx = wbs[calibIndex]->pMesh->vtx[i].t[0];
				vertices[i].ty = wbs[calibIndex]->pMesh->vtx[i].t[1];
				vertices[i].tz = wbs[calibIndex]->pMesh->vtx[i].t[2];
			}
			glBindBuffer(GL_ARRAY_BUFFER, m_iVertices);
			glBufferData(GL_ARRAY_BUFFER,
						  vertices.size() * sizeof(Vertex),
						  vertices.data(),
						  GL_STATIC_DRAW);

			GLenum errGL = ::glGetError();
			if( GL_NO_ERROR != errGL ) {
				logStr( 0, "ERROR: GLERROR %d at glGenBuffers for vertex buffer.\n", errGL );
				return VWB_ERROR_BLEND;
			}

			// set vertex layout
			glEnableVertexAttribArray( 0 ); // pos
			glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (GLvoid*)offsetof( Vertex, x ) );
			glEnableVertexAttribArray( 1 ); // texture coord
			glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (GLvoid*)offsetof( Vertex, u ) );
			glEnableVertexAttribArray( 2 ); // normal
			glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (GLvoid*)offsetof( Vertex, nx ) );
			glEnableVertexAttribArray( 3 ); // tangent
			glVertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), (GLvoid*)offsetof( Vertex, tx ) );
			errGL = ::glGetError();
			if( GL_NO_ERROR != errGL ) {
				logStr( 0, "ERROR: GLERROR %d at glVertexAttribPointer for vertex buffer.\n", errGL );
				return VWB_ERROR_BLEND;
			}

			// create and fill index buffer
			glGenBuffers( 1, &m_iIndices );
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iIndices);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
						  wbs[calibIndex]->pMesh->nIdx * sizeof(wbs[calibIndex]->pMesh->idx[0]),
						  wbs[calibIndex]->pMesh->idx,
						  GL_STATIC_DRAW);
			errGL = ::glGetError();
			if( GL_NO_ERROR != errGL ) {
				logStr( 0, "ERROR: GLERROR %d at glGenBuffers for index buffer.\n", errGL );
				return VWB_ERROR_BLEND;
			}

			m_nIndices = wbs[calibIndex]->pMesh->nIdx;
		}
		// the VIOSO warper uses static quad from within the shader via gl_VertexID, so we skip creating buffers

		glBindVertexArray( 0 );

		// create textures
		glActiveTexture( GL_TEXTURE0 );

		if( m_bDP ) {
			// secondary blend
			if( wbs[calibIndex]->p2ndBlend ) {
				err = FillTexture( m_tex2ndBlend, GL_RGBA8, wbs[calibIndex]->header.width, wbs[calibIndex]->header.height, GL_RGBA, GL_UNSIGNED_BYTE, wbs[calibIndex]->p2ndBlend, GL_NEAREST, GL_CLAMP_TO_BORDER, "secondary blend", true );
				if( VWB_ERROR_NONE != err )
					return err;
			}
			// directional shading
			if( wbs[calibIndex]->pDirectional ) {
				err = FillTexture( m_texDirectionalShading, GL_RGBA, wbs[calibIndex]->directionalSz.cx, wbs[calibIndex]->directionalSz.cy, GL_RGBA, GL_UNSIGNED_BYTE, wbs[calibIndex]->pDirectional, GL_LINEAR, GL_CLAMP_TO_BORDER, "directional shading", true );
				if( VWB_ERROR_NONE != err )
					return err;
			}
		} else {
			// warp
			if( !wbs[calibIndex]->pWarp ) {
				logStr( 0, "FATAL: No warp texture present.\n", err );
				return VWB_ERROR_WARP;
			}
			err = FillTexture( m_texWarp, ( wbs[calibIndex]->header.flags & FLAG_WARPFILE_HEADER_3D ) ? GL_RGB32F : GL_RG32F, wbs[calibIndex]->header.width, wbs[calibIndex]->header.height, GL_RGBA, GL_FLOAT, wbs[calibIndex]->pWarp, GL_NEAREST, GL_CLAMP_TO_BORDER, "warp", true );
			if( VWB_ERROR_NONE != err )
				return err;
		}

		// blend
		if( wbs[calibIndex]->pBlend2 ) {
			err = FillTexture( m_texBlend, GL_RGBA16, wbs[calibIndex]->header.width, wbs[calibIndex]->header.height, GL_RGBA, GL_UNSIGNED_SHORT, wbs[calibIndex]->pBlend2, GL_LINEAR, GL_CLAMP_TO_BORDER, "blend", true );
			if( VWB_ERROR_NONE != err )
				return err;

			VWB_BlendRecord2* pX = new VWB_BlendRecord2[wbs[calibIndex]->header.width * wbs[calibIndex]->header.height];
			std::copy( wbs[calibIndex]->pBlend2, wbs[calibIndex]->pBlend2 + (ptrdiff_t)wbs[calibIndex]->header.width * (ptrdiff_t)wbs[calibIndex]->header.height, pX );
			int w = 0, h = 0;
			auto logo = stbi_load_from_memory( LOGO, sizeof( LOGO ), &w, &h, NULL, 4 );
			if( !logo ) {
				logStr( 0, "FATAL: No internal blend texture present.", err );
				return VWB_ERROR_BLEND;
			}
			alphablend( pX, wbs[calibIndex]->header.width, wbs[calibIndex]->header.height, logo, w, h );
			err = FillTexture( m_texBlendX, GL_RGBA16, wbs[calibIndex]->header.width, wbs[calibIndex]->header.height, GL_RGBA, GL_UNSIGNED_SHORT, pX, GL_LINEAR, GL_CLAMP_TO_BORDER, "blend with logo", true );
			delete[] pX;
			stbi_image_free( logo );
			if( VWB_ERROR_NONE != err )
				return err;
		} else if( !bDoNotBlend ) {
			logStr( 3, "WARINIG: No blend texture present.", err );
			bDoNotBlend = true;
		}

		// blacklevel
		if( wbs[calibIndex]->pBlack ) {
			err = FillTexture( m_texBlack, GL_RGBA8, wbs[calibIndex]->header.width, wbs[calibIndex]->header.height, GL_RGBA, GL_UNSIGNED_BYTE, wbs[calibIndex]->pBlack, GL_LINEAR, GL_CLAMP_TO_BORDER, "blacklevel", true );
			if( VWB_ERROR_NONE != err )
				return err;
		} else if( !bDoNoBlack ) {
			logStr( 3, "WARINIG: No blacklevel texture present.", err );
			bDoNoBlack = true;
		}

		logStr( 1, "SUCCESS: OGL-Warper initialized.\n" );

	} catch( VWB_ERROR e )
	{
		return e;
	}
	return err;
}

inline VWB_MAT44f GLWarpBlend::UpdateView( VWB_MAT44f const& igView, VWB_VEC3f& e )
{
	VWB_MAT44f V; // return value

	// rotation matrix from angles
	VWB_MAT44f R;
	if( m_bRH )
		R = VWB_MAT44f::R( (VWB_float)m_ep.pitch, (VWB_float)m_ep.yaw, (VWB_float)m_ep.roll );
	else
		R = VWB_MAT44f::R_LH( (VWB_float)m_ep.pitch, (VWB_float)m_ep.yaw, (VWB_float)m_ep.roll );

	// reverse eye vector
	e = VWB_VEC3f( -(float)m_ep.x, -(float)m_ep.y, -(float)m_ep.z );
	
	// add platform-rotated eye offset
	if( 0 != this->eye[0] || 0 != this->eye[1] || 0 != this->eye[2] )
		e -= R * VWB_VEC3f::ptr( this->eye );

	// translate to local coordinates
	e = igView * e;

	VWB_MAT44f T = VWB_MAT44f::T( e );
	m_mVP = T * igView * m_mBaseI;

	if( bTurnWithView )
		V = igView * R;
	else
		V = T * igView;

	return V;
}

void GLWarpBlend::UpdateOverlayTexture( int type, int w, int h, void const* data ) {
	// we try to fill 2nd overlay texture
	if( -1 == m_texOverlay2 ) {
		glGenTextures( 1, &m_texOverlay2 );
	}
	// get width and height and format from the texture
	glBindTexture( GL_TEXTURE_2D, m_texOverlay2 );
	GLenum currentInternalFormat;
	GLuint currentWidth;
	GLuint currentHeight;
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, (GLint*)&currentInternalFormat );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, (GLint*)&currentWidth );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, (GLint*)&currentHeight );

	// ToDo: translate type to channel lyaout
	GLenum newFormat = GL_RGBA;
	// ToDo: translate type to channel format
	GLenum newType = GL_UNSIGNED_BYTE;
	// ToDo: translate to internal format
	GLenum newInternalFormat = GL_RGBA8;

	if( currentWidth != (GLuint)w || currentHeight != (GLuint)h || currentInternalFormat != newInternalFormat ) {
		// we need to re-create the texture
		glTexImage2D( GL_TEXTURE_2D, 0, newInternalFormat, w, h, 0, newFormat, newType, data );
	} else {
		// we can update the texture
		glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, w, h, newFormat, newType, data );
	}

	// we swap handles
	std::swap( m_texOverlay, m_texOverlay2 );
}

// set up the view matrix,
// use the same matrices as in your program, construct a view matrix relative to the actual screen
// use the same units (usually millimeters) for the screen and the scene
VWB_ERROR GLWarpBlend::GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj )
{
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret )
	{
		VWB_MAT44f P; // the projection matrix to return 

		VWB_VEC3f e;
		VWB_MAT44f V = UpdateView( m_mViewIG, e );

		VWB_float clip[6];
		getClip( e, clip );

		if( m_bRH )
			P = VWB_MAT44f::GLFrustumRH( clip );
		else
			P = VWB_MAT44f::GLFrustumLH( clip );

		m_mVP = P * m_mVP;

		if( pView )
		{
			V.Transposed().SetPtr( pView );
		}
		if( pProj )
		{
			P.Transposed().SetPtr( pProj );
		}
	}
	return ret;
}

// set up the view matrix,
// use the same matrices as in your program, construct a view matrix relative to the actual screen
// use the same units (usually millimeters) for the screen and the scene
VWB_ERROR GLWarpBlend::GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip )
{
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret )
	{
		VWB_MAT44f P;
		VWB_VEC3f e;
		VWB_MAT44f V = UpdateView( m_mViewIG, e );

		VWB_float clip[6];
		getClip( e, clip );

		if( m_bRH )
			P = VWB_MAT44f::GLFrustumRH( clip );
		else
			P = VWB_MAT44f::GLFrustumLH( clip );

		m_mVP = P * m_mVP;

		if( pView )
		{
			V.Transposed().SetPtr( pView );
		}

		if( pClip )
		{
			memcpy( pClip, clip, sizeof( clip ) );
		}
	}
	return ret;
}

VWB_ERROR GLWarpBlend::GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric, VWB_float aspect )
{
	VWB_ERROR ret = UpdateEye( eye, rot );
	if( VWB_ERROR_NONE == ret )
	{

		VWB_MAT44f P;
		VWB_VEC3f e;

		VWB_MAT44f V = UpdateView( m_mViewIG, e );

		VWB_float clip[6];
		getClip( e, clip );

		if( symmetric )
		{
			VWB_MAT44f ig = m_mViewIG;
			if( m_bRH )
				MakeSymmetricRH( ig, clip );
			else
				MakeSymmetricLH( ig, clip );
			V = UpdateView( ig, e );
		}

		if( pDir )
		{
			// extract rotation angles from upper View matrix
			VWB_VEC3f::ptr( pDir ) = V.Upper().GetR();
			if( !m_bRH )
				VWB_VEC3f::ptr( pDir ) *= -1;
		}

		if( pPos )
		{
			pPos[0] = V._14;
			pPos[1] = V._24;
			pPos[2] = V._34;
		}

		if( 0 != aspect )
		{
			VWB_float a = ( clip[0] + clip[2] ) / ( clip[1] + clip[3] );
			if( aspect > a ) // we need to make frustum wider
			{
				a = aspect / a;
				clip[0] *= a;
				clip[2] *= a;
			}
			else // we need to make frustum higher
			{
				a /= aspect;
				clip[1] *= a;
				clip[3] *= a;
			}
		}

		if( m_bRH )
			P = VWB_MAT44f::GLFrustumRH( clip );
		else
			P = VWB_MAT44f::GLFrustumLH( clip );

		m_mVP = P * m_mVP;

		if( pClip )
		{
			memcpy( pClip, clip, sizeof( clip ) );
		}
	}

	return ret;
}

VWB_ERROR GLWarpBlend::SetViewProjection( VWB_float const* pView, VWB_float const* pProj )
{
	if( pView && pProj )
		m_mVP = VWB_MAT44f::ptr( pProj ) * VWB_MAT44f::ptr( pView ) * m_mBaseI;
	else
		return VWB_ERROR_PARAMETER;
	return VWB_ERROR_NONE;
}

VWB_ERROR GLWarpBlend::Render( VWB_param inputTexture, VWB_uint stateMask )
{
	VWB_Warper_base::Render( inputTexture, stateMask );
	logStr( 4, "Render GL" );
	if( VWB_STATEMASK_STANDARD == stateMask )
		stateMask = VWB_STATEMASK_DEFAULT;

	if( -1 == m_Program )
		return VWB_ERROR_GENERIC;

	GLenum res = GL_NO_ERROR;
	GLint iSrc = (GLint)(long long)inputTexture;
	GLint url = -1;

	GLint                       matrix_mode = -1;
	GLuint                      program = -1;
	GLint                       currentTexture2DBinding0 = -1;
	GLint                       currentTexture2DBinding1 = -1;
	GLint                       currentTexture2DBinding2 = -1;
	GLint                       currentTexture2DBinding3 = -1;
	GLint                       currentTexture2DBinding4 = -1;
	GLint                       currentTexture2DBinding5 = -1;
	GLint                       active_texture_unit = -1;
	GLint						active_client_texture_unit = -1;
	GLint						oldVA = -1;

	// record current state
	if( ( ( VWB_STATEMASK_RASTERSTATE | VWB_STATEMASK_SAMPLER ) & stateMask ) && bUseGL110 )
	{
		glPushAttrib( GL_ALL_ATTRIB_BITS );
		glGetIntegerv( GL_MATRIX_MODE, &matrix_mode );
	}

	if( ( VWB_STATEMASK_VERTEX_SHADER | VWB_STATEMASK_PIXEL_SHADER ) & stateMask )
		glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&(program));

	if( VWB_STATEMASK_SHADER_RESOURCE & stateMask )
	{
		if(bUseGL110)
			glGetIntegerv(GL_CLIENT_ACTIVE_TEXTURE,&active_client_texture_unit);
		else
			glGetIntegerv( GL_VERTEX_ARRAY_BINDING, &oldVA );


	    glGetIntegerv( GL_ACTIVE_TEXTURE, &( active_texture_unit ) );
		glActiveTexture (GL_TEXTURE0 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding0 ) );

		glActiveTexture( GL_TEXTURE1 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding1 ) );

		glActiveTexture( GL_TEXTURE2 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding2 ) );

		glActiveTexture( GL_TEXTURE3 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding3 ) );

		glActiveTexture( GL_TEXTURE4 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding4 ) );

		glActiveTexture( GL_TEXTURE5 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding5 ) );
	}

	GLint viewport[4] = { 0 };
	glGetIntegerv( GL_VIEWPORT, viewport );

	if( -1 == m_texOverlay ) {
		if( -1 == iSrc ) {
			glGetIntegerv( GL_UNPACK_ROW_LENGTH, &url );
			glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );

			::glActiveTexture( GL_TEXTURE0 );

			if( viewport[2] != m_sizeIn.cx || viewport[3] != m_sizeIn.cy ) {
				res = glGetError();

				VWB_ERROR rr = FillTexture( m_texBB, GL_RGB, viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, nullptr, GL_LINEAR, GL_CLAMP_TO_BORDER, "content clone" );
				if( VWB_ERROR_NONE != rr ) 
					return rr;

				m_sizeIn.cx = viewport[2];
				m_sizeIn.cy = viewport[3];
			} else {
				::glBindTexture( GL_TEXTURE_2D, m_texBB );
			}

			GLint oldRB = GL_BACK;
			GLint WB = GL_BACK;
			glGetIntegerv( GL_READ_BUFFER, &oldRB );
			glGetIntegerv( GL_DRAW_BUFFER, &WB );
			glReadBuffer( WB );
			res = glGetError(); // clear error flag
			if( GL_NO_ERROR != res ) {
				static bool bOnce = true;
				if( bOnce ) {
					logStr( 3, "FAILED to set read buffer, glError = %#04x", res );
					bOnce = false;
				}
				return VWB_ERROR_PARAMETER;
			}
			glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, viewport[0], viewport[1], viewport[2], viewport[3] );
			res = glGetError();
			if( GL_NO_ERROR == res ) {
				logStr( 4, "Copied input texture from current write buffer" );
			} else {
				static bool bOnce = true;
				if( bOnce ) {
					logStr( 3, "FAILED to copy input texture from current write buffer, glError = %#04x", res );
					bOnce = false;
				}
				return VWB_ERROR_PARAMETER;
			}

			if( oldRB != GL_BACK )
				glReadBuffer( oldRB );
			iSrc = m_texBB;
		}
	} else {
		iSrc = m_texOverlay;
	}

	if( 4 <= g_logLevel )
	{
		std::filesystem::path o(g_logFilePath);
		o += ".tex.in.bmp";
		savetex(o.string().c_str(), iSrc);
		logStr(4, "Input texture (%dx%d) saved as \"%s\".", m_sizeIn.cx, m_sizeIn.cy, o.string().c_str());
	}

	// set own params
	if( -1 != m_texOverlay )
		::glUseProgram( m_ProgramBypass );
	else
		::glUseProgram(m_Program);
    res = ::glGetError();
	if( GL_NO_ERROR == res )
	{
		// Set the texturing modes
		if( -1 == m_texOverlay )
		{ // no overlay
			if( m_bDP ) {
				SetTexture( m_loc2ndBlend, 4, m_tex2ndBlend, GL_CLAMP, GL_NEAREST );
				SetTexture( m_locDirectionalShading, 5, m_texDirectionalShading, GL_CLAMP, GL_NEAREST );
			} else {
				SetTexture( m_locWarp, 1, m_texWarp, GL_CLAMP, GL_NEAREST );
			}

			if( m_licenseInfo->isValid )
				SetTexture( m_locBlend, 2, m_texBlend, GL_CLAMP, GL_NEAREST );
			else
				SetTexture( m_locBlend, 2, m_texBlendX, GL_CLAMP, GL_NEAREST );

			SetTexture( m_locBlack, 3, m_texBlack, GL_CLAMP, GL_NEAREST );
			SetTexture( m_locContent, 0, iSrc, bFixWraparound ? GL_REPEAT : GL_CLAMP_TO_BORDER );
		}
		else // in overlay mode, we set overlay texture as the source texture 
			SetTexture( m_locContentBypass, 0, iSrc );

		static const VWB_MAT44f I = VWB_MAT44f::I();
		if( -1 == m_texOverlay )
		{ // no overlay
			if( m_bDP ) {
				// set special uniforms for domeprojection shaders
				GLfloat params[16]{
					/* 0*/gamma,
					/* 1*/1, // always do warp
					/* 2*/-1 != m_texBlend && !bDoNotBlend ? 1.f : 0.f,
					/* 3*/-1 != m_texBlack && !bDoNoBlack  ? 1.f : 0.f,
					/* 4*/-1 != m_tex2ndBlend ? 1.f : 0.f,
					/* 5*/inputGamma,
					/* 6*/1.0f / outputGamma,
					/* 7*/-1 != m_texDirectionalShading ? 1.f : 0.f,
					/* 8*/bFlipWarpmeshTexcoords ? 1.f : 0.f
				};

				if( m_bDynamicEye ) {
					glUniformMatrix4fv( m_locMatView, 1, GL_TRUE, m_mVP );
				} else {
					glUniformMatrix4fv( m_locMatView, 1, GL_TRUE, I );
				}
				// using eye coordinates here, enables us to do directional shading on 2D warp, just set the eye according to a normalized plane x = -1..1, y = -1..1, z = 1
				glUniform4f( m_locCamPos, m_ep.x, m_ep.y, m_ep.z, 1.f );
				glUniform4fv( m_locParamsDomeprojection, 4, params );
			} else {
				// set special uniforms for vioso shader
				if( m_bDynamicEye )
					glUniformMatrix4fv( m_locMatView, 1, GL_TRUE, m_mVP );
				else
					glUniform1i( m_locBorder, m_bBorder );
				glUniform1i( m_locDoNotBlend, bDoNotBlend );
				glUniform1i( m_locDoNoBlack, bDoNoBlack );
				if( bPartialInput )
					glUniform4f( m_locOffsScale,
								 (GLfloat)optimalRect.left / (GLfloat)optimalRes.cx,
								 (GLfloat)optimalRect.top / (GLfloat)optimalRes.cy,
								 (GLfloat)optimalRes.cx / ( (GLfloat)optimalRect.right - (GLfloat)optimalRect.left ),
								 (GLfloat)optimalRes.cy / ( (GLfloat)optimalRect.bottom - (GLfloat)optimalRect.top )
					);
				else
					glUniform4f( m_locOffsScale, 0.0f, 0.0f, 1.0f, 1.0f );

				float bb[4] = {
					m_blackBias.x * blackScale,
					m_blackBias.y,
					m_blackBias.z,
					inputGamma };

				if( blackDarkAdjust <= 0.5f )
					bb[1] *= blackDarkAdjust * 2.0f;
				else
					bb[1] += ( blackDarkAdjust - 0.5f ) * 2.0f * ( 1.0f - m_blackBias.y );
				if( blackBrightAdjust <= 0.5f )
					bb[2] *= blackBrightAdjust * 2.0f;
				else
					bb[2] += ( blackBrightAdjust - 0.5f ) * 2.0f * ( 1.0f - m_blackBias.z );
				bb[2] *= bb[1];
				bb[3] = inputGamma;
				glUniform4fv( m_locBlackBias, 1, bb );
			}
			if( bBicubic )
				glUniform4f( m_locParamsBicubic, (GLfloat)m_sizeIn.cx, (GLfloat)m_sizeIn.cy, 1.0f / m_sizeIn.cx, 1.0f / m_sizeIn.cy );
		} else {
			if( m_bDP ) {
				// in overlay mode, we need to set uniforms for the dp vertex shader
				GLfloat params[16]{}; // all zero		}
				glUniformMatrix4fv( m_locMatView, 1, GL_TRUE, I ); // put identity
				glUniform4f( m_locCamPos, 0.f, 0.f, 0.f, 1.0f );
				glUniform4fv( m_locParamsDomeprojection, 4, params );
			}
			if( bBicubic )
				glUniform4f( m_locParamsBicubic, (GLfloat)m_sizeIn.cx, (GLfloat)m_sizeIn.cy, 1.0f / m_sizeIn.cx, 1.0f / m_sizeIn.cy );
		}

		if( VWB_STATEMASK_VIEWPORT & stateMask )
		{
			glViewport( 0, 0, m_sizeIn.cx, m_sizeIn.cy );
		}

		if( VWB_STATEMASK_CLEARBACKBUFFER & stateMask )
		{
			glClearColor( 0, 0, 0, 1 );
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		}

		if( bUseGL110 )
		{
			glMatrixMode( GL_MODELVIEW );
			glPushMatrix();
			glLoadIdentity();

			glMatrixMode( GL_TEXTURE );
			glPushMatrix();
			glLoadIdentity();

			glMatrixMode( GL_PROJECTION );
			glPushMatrix();
			glLoadIdentity();
			glOrtho(0, 1, 1, 0, -100, 100);

			glDisable(GL_FOG); 
			glDisable(GL_TEXTURE_1D);
			glDisable(GL_LIGHTING); 
			glDisable(GL_LIGHT0); 
			glDisable(GL_ALPHA_TEST); 
			glDisable(GL_BLEND); 
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST); 
			glDepthMask(GL_TRUE); 
			glDepthMask(GL_FALSE); 

			glDisable(GL_COLOR_MATERIAL);
			glDisable(GL_CLIP_PLANE0);
			glDisable(GL_CLIP_PLANE1);
			glDisable(GL_CLIP_PLANE2);
			glDisable(GL_CLIP_PLANE3);
			glDisableClientState( GL_TEXTURE_COORD_ARRAY );

			glEnable(GL_TEXTURE_2D);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );

			res = ::glGetError();
			// draw quad
			glBegin(GL_QUADS);

				// no mid-pixel correction needed, it is done in the warp texture already
				//GLfloat x1 = ( -0.5f 			  ) / viewport[2];
				//GLfloat y1 = ( -0.5f 			  ) / viewport[3];
				//GLfloat x2 = ( 0.5f + viewport[2] ) / viewport[2];
				//GLfloat y2 = ( 0.5f + viewport[3] ) / viewport[3];

				glTexCoord2f(0,0); glVertex2f(0,0);
				glTexCoord2f(0,1); glVertex2f(0,1);
				glTexCoord2f(1,1); glVertex2f(1,1);
				glTexCoord2f(1,0); glVertex2f(1,0);
			glEnd();

			glPopMatrix();
			glMatrixMode( GL_TEXTURE );
			glPopMatrix();
			glMatrixMode( GL_MODELVIEW );
			glPopMatrix();
			glMatrixMode( GL_PROJECTION );
			glPopMatrix();
		}
		else
		{
			glBindVertexArray( m_iVertexArray );
			glDisable( GL_DEPTH_TEST );
			glDisable( GL_CLIP_PLANE0 );
			glDisable( GL_CLIP_PLANE1 );
			glDisable( GL_CLIP_PLANE2 );
			glDisable( GL_CLIP_PLANE3 );
			if( m_bDP ) {
				glDrawElements(GL_TRIANGLES, m_nIndices, GL_UNSIGNED_INT, 0);
			} else {
				glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
			}
		}
	}

	if( VWB_STATEMASK_SHADER_RESOURCE & stateMask )
	{
		glActiveTexture(GL_TEXTURE0);
		if(bUseGL110)
			glClientActiveTexture(active_client_texture_unit);
		else
			glBindVertexArray(oldVA);
		glBindTexture(GL_TEXTURE_2D, currentTexture2DBinding0 );

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, (currentTexture2DBinding1));

		glActiveTexture( GL_TEXTURE2 );
		glBindTexture( GL_TEXTURE_2D, ( currentTexture2DBinding2 ) );

		glActiveTexture( GL_TEXTURE3 );
		glBindTexture( GL_TEXTURE_2D, ( currentTexture2DBinding3 ) );

		glActiveTexture( GL_TEXTURE4 );
		glBindTexture( GL_TEXTURE_2D, ( currentTexture2DBinding4 ) );

		glActiveTexture( GL_TEXTURE5 );
		glBindTexture( GL_TEXTURE_2D, ( currentTexture2DBinding5 ) );

		glActiveTexture(active_texture_unit);
	}

	if( ( VWB_STATEMASK_VERTEX_SHADER | VWB_STATEMASK_PIXEL_SHADER ) & stateMask )
		glUseProgram(program);

	if( ( ( VWB_STATEMASK_RASTERSTATE | VWB_STATEMASK_SAMPLER ) & stateMask ) && bUseGL110 )
	{
		glMatrixMode( matrix_mode );
		glPopAttrib();
	}

	if( 0 != url && -1 != url )
		glPixelStorei( GL_UNPACK_ROW_LENGTH, url );
    return VWB_ERROR_NONE;
}
	
void GLWarpBlend::SetTexture( GLuint loc, GLuint unit, GLuint tex, GLuint wrapMode, GLuint filter ) const
{
	if( -1 != loc && -1 != tex )
	{
		::glUniform1i( loc, unit );
		::glActiveTexture( GL_TEXTURE0 + unit );
		::glBindTexture( GL_TEXTURE_2D, tex );

		::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter );
		::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter );
		::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode );
		::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode );
		::glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, colBlack );
		if( bUseGL110 )
			::glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_MODULATE );
	}
}
