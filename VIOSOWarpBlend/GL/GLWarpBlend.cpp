#include "GLWarpBlend.h"
#include "pixelshader.h"

#define GL_EXT_DEFINE_AND_IMPLEMENT
#include "GLext.h"

GLfloat GLWarpBlend::colBlack[4] = {0,0,0,0};
//save a texture to .tif image
bool GLWarpBlend::savetex (char filename[MAX_PATH],GLint iTex)
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
				glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, data);// split x and y sizes into bytes
				if( GL_NO_ERROR == glGetError() )
				{
					const LONG pitch = 4 * x;
					BITMAPINFOHEADER hdr = { 0 };
					hdr.biSize = sizeof( hdr );
					hdr.biWidth = x;
					hdr.biHeight = LONG( y );
					hdr.biPlanes = 1;
					hdr.biBitCount = 32;
					hdr.biSizeImage = pitch * y;

					BITMAPFILEHEADER fh = { 0 };
					fh.bfType = 'MB';
					fh.bfOffBits = sizeof( fh ) + hdr.biSize;
					fh.bfSize = fh.bfOffBits + hdr.biSizeImage;

					FILE* f = NULL;
					if( NO_ERROR == fopen_s( &f, filename, "wb" ) )
					{
						fwrite( &fh, sizeof( fh ), 1, f );
						fwrite( &hdr, sizeof( hdr ), 1, f );
						fwrite( data, hdr.biSizeImage, 1, f );
						fclose( f );
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

GLWarpBlend::GLWarpBlend(): 
	m_texBlend( -1 ),
	m_texWarp( -1 ),
	m_texBlack( -1 ), 
	m_texBB( -1 ),
	
	m_locWarp( -1 ),
	m_locBorder( -1 ),
	m_locBlend( -1 ),
	m_locBlack( -1 ),
	m_locDoNotBlend( -1 ),
	m_locDoNoBlack( -1 ),
	m_locContent( -1 ),
	m_locContentBypass( -1 ),
	m_locMatView( -1 ),
	m_locDim(-1),
	m_locSmooth(-1),
	m_locBlackBias( -1 ),
	m_locParams(-1),
	m_iVertexArray(-1),
	
	m_FragmentShader( -1 ),
	m_FragmentShaderBypass( -1 ),
	m_VertexShader( -1 ),
	m_Program(-1),
	m_ProgramBypass(-1)
{
	logStr( 1, "INFO: Start initializing OGL-Warper...\n" );

#ifndef WIN32
	// FF 10.8.21: use GLEW and initialize it manually because the GLext.h header
	// doesn't seem to work correctly with current Linuxen.
	GLenum err=glewInit();
	if(err!=GLEW_OK)
	{
	  //Problem: glewInit failed, something is seriously wrong.
	  logStr( 1, "glewInit failed.\n" );
	}
#endif

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
	if( -1 != m_iVertexArray )
		glDeleteVertexArrays( 1, &m_iVertexArray );

	if( -1 != m_Program )
		glDeleteProgram( m_Program );

	if( -1 != m_FragmentShader )
		glDeleteShader( m_FragmentShader );

	if( -1 != m_ProgramBypass )
		glDeleteProgram( m_ProgramBypass );

	if( -1 != m_FragmentShaderBypass )
		glDeleteShader( m_FragmentShaderBypass );

	if( -1 != m_VertexShader )
		glDeleteShader( m_VertexShader );

	if( -1 != m_texBB )
		glDeleteTextures( 1, &m_texBB );
	if( -1 != m_texWarp )
		glDeleteTextures( 1, &m_texWarp );
	if( -1 != m_texBlend )
		glDeleteTextures( 1, &m_texBlend );
	if( -1 != m_texBlack )
		glDeleteTextures( 1, &m_texBlack );
	logStr( 1, "INFO: OGL-Warper uninitialized.\n" );
}

VWB_ERROR GLWarpBlend::CreatePixelShader()
{
	// compile shaders
	struct ShaderInit {
		GLuint numShaders;
		GLchar const* shaders[4];
	};

	GLenum err = 0;

	ShaderInit const s[] = {
		{1,{s_szPasstrough_vertex_shader_v110}},
		{3,{s_fragment_shader_header_v110,s_func_tex2D,s_bypass_fragment_shader}},
		{3,{s_fragment_shader_header_v110,s_func_tex2D,s_warp_blend_fragment_shader}},
		{3,{s_fragment_shader_header_v110,s_func_tex2D,s_warp_blend_fragment_shader_3D}},
		{3,{s_fragment_shader_header_v110,s_func_tex2D_BC,s_bypass_fragment_shader}},
		{3,{s_fragment_shader_header_v110,s_func_tex2D_BC,s_warp_blend_fragment_shader}},
		{3,{s_fragment_shader_header_v110,s_func_tex2D_BC,s_warp_blend_fragment_shader_3D}},

		{1,{s_szPasstrough_vertex_shader_v330}},
		{3,{s_fragment_shader_header_v330,s_func_tex2D,s_bypass_fragment_shader}},
		{3,{s_fragment_shader_header_v330,s_func_tex2D,s_warp_blend_fragment_shader}},
		{3,{s_fragment_shader_header_v330,s_func_tex2D,s_warp_blend_fragment_shader_3D}},
		{3,{s_fragment_shader_header_v330,s_func_tex2D_BC,s_bypass_fragment_shader}},
		{3,{s_fragment_shader_header_v330,s_func_tex2D_BC,s_warp_blend_fragment_shader}},
		{3,{s_fragment_shader_header_v330,s_func_tex2D_BC,s_warp_blend_fragment_shader_3D}},
	};
	GLchar log[2048];
	GLint iS = 0;

	if( !bUseGL110 )
	{
		logStr( 2, "INFO: using #version 330 shader.\n" );
		iS += 7;
		err = ::glGetError();
		if( GL_NO_ERROR != err )
		{
			logStr( 1, "WARNING: %d before glCreateShader (vertex).\n", err );
		}
		m_VertexShader = glCreateShader( GL_VERTEX_SHADER );
		glShaderSource( m_VertexShader, s[iS].numShaders, (const GLchar**)s[iS].shaders, NULL );
		glCompileShader( m_VertexShader );
		err = ::glGetError();
		glGetShaderInfoLog( m_VertexShader, 2048, NULL, log );
		if( log[0] )
		{
			logStr( 0, "ERROR: glCompileShader (vertex):\n%s\n", log );
			return VWB_ERROR_SHADER;
		}
		if( GL_NO_ERROR != err )
		{
			logStr( 0, "ERROR: %d at glCompileShader (vertex).\n", err );
			return VWB_ERROR_SHADER;
		}
		glGenVertexArrays( 1, &m_iVertexArray );
	}
	else
		logStr( 2, "INFO: using #version 110 shader.\n" );


	m_FragmentShaderBypass = glCreateShader( GL_FRAGMENT_SHADER );
	iS++;

	if( bBicubic )
		iS += 3;

	glShaderSource( m_FragmentShaderBypass, s[iS].numShaders, (const GLchar**)s[iS].shaders, NULL );
	glCompileShader( m_FragmentShaderBypass );
	err = ::glGetError();
	glGetShaderInfoLog( m_FragmentShaderBypass, 2048, NULL, log );
	if( log[0] )
	{
		logStr( 0, "ERROR: glCompileShader (fragment bypass):\n%s\n", log );
		return VWB_ERROR_SHADER;
	}
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: %d at glCompileShader (fragment bypass).\n", err );
		return VWB_ERROR_SHADER;
	}

	m_ProgramBypass = glCreateProgram();
	err = ::glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: %d at glCreateProgram bypass.\n", err );
		return VWB_ERROR_SHADER;
	}
	if( -1 != m_VertexShader )
	{
		glAttachShader( m_ProgramBypass, m_VertexShader );
		err = ::glGetError();
		if( GL_NO_ERROR != err )
		{
			logStr( 0, "ERROR: %d at glAttachShader (vertex bypass).\n", err );
			return VWB_ERROR_SHADER;
		}
	}
	glAttachShader( m_ProgramBypass, m_FragmentShaderBypass );
	err = ::glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: %d at glAttachShader (fragment bypass).\n", err );
		return VWB_ERROR_SHADER;
	}

	glLinkProgram( m_ProgramBypass );
	err = ::glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: %d at glLinkProgram.\n", err );
		return VWB_ERROR_SHADER;
	}

	GLint isLinked = 0;
	glGetProgramiv( m_ProgramBypass, GL_LINK_STATUS, &isLinked );
	if( isLinked == GL_FALSE )
	{
		GLint maxLength = 0;
		glGetProgramiv( m_ProgramBypass, GL_INFO_LOG_LENGTH, &maxLength );

		//The maxLength includes the NULL character
		std::vector<GLchar> infoLog( maxLength );
		glGetProgramInfoLog( m_ProgramBypass, maxLength, &maxLength, &infoLog[0] );

		//The program is useless now. So delete it.
		glDeleteProgram( m_ProgramBypass );
		m_ProgramBypass = -1;

		logStr( 0, "ERROR: %d at glLinkProgram bypass:\n%s\n", err, infoLog );
		return VWB_ERROR_SHADER;
	}
	m_locContentBypass = glGetUniformLocation( m_ProgramBypass, "samContent" );

	m_FragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
	iS++;
	if( m_bDynamicEye )
		iS++;

	err = GL_INVALID_VALUE;
	err = GL_INVALID_OPERATION;
	glShaderSource( m_FragmentShader, s[iS].numShaders, (const GLchar**)s[iS].shaders, NULL );
	err = ::glGetError();
	glCompileShader( m_FragmentShader );
	err = ::glGetError();
	glGetShaderInfoLog( m_FragmentShader, 2048, NULL, log );
	if( log[0] )
	{
		logStr( 0, "ERROR: glCompileShader (fragment):\n%s\n", log );
		return VWB_ERROR_SHADER;
	}
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: %d at glCompileShader (fragment).\n", err );
		return VWB_ERROR_SHADER;
	}

	m_Program = glCreateProgram();
	err = ::glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: %d at glCreateProgram.\n", err );
		return VWB_ERROR_SHADER;
	}
	if( -1 != m_VertexShader )
	{
		glAttachShader( m_Program, m_VertexShader );
		err = ::glGetError();
		if( GL_NO_ERROR != err )
		{
			logStr( 0, "ERROR: %d at glAttachShader (vertex).\n", err );
			return VWB_ERROR_SHADER;
		}
	}
	glAttachShader( m_Program, m_FragmentShader );
	err = ::glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: %d at glAttachShader (fragment).\n", err );
		return VWB_ERROR_SHADER;
	}

	glLinkProgram( m_Program );
	err = ::glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: %d at glLinkProgram.\n", err );
		return VWB_ERROR_SHADER;
	}

	glGetProgramiv( m_Program, GL_LINK_STATUS, &isLinked );
	if( isLinked == GL_FALSE )
	{
		GLint maxLength = 0;
		glGetProgramiv( m_Program, GL_INFO_LOG_LENGTH, &maxLength );

		//The maxLength includes the NULL character
		std::vector<GLchar> infoLog( maxLength );
		glGetProgramInfoLog( m_Program, maxLength, &maxLength, &infoLog[0] );

		//The program is useless now. So delete it.
		glDeleteProgram( m_Program );
		m_ProgramBypass = -1;

		logStr( 0, "ERROR: %d at glLinkProgram bypass:\n%s\n", err, infoLog );
		return VWB_ERROR_SHADER;
	}

	m_locSize = glGetUniformLocation( m_Program, "size" );
	m_locContent = glGetUniformLocation( m_Program, "samContent" );
	m_locWarp = glGetUniformLocation( m_Program, "samWarp" );
	m_locBorder = glGetUniformLocation( m_Program, "bBorder" );
	m_locBlend = glGetUniformLocation( m_Program, "samBlend" );
	m_locBlack = glGetUniformLocation( m_Program, "samBlack" );
	m_locDoNotBlend = glGetUniformLocation( m_Program, "bDoNotBlend" );/*-*/
	m_locDoNoBlack = glGetUniformLocation( m_Program, "bDoNoBlack" );
	m_locOffsScale = glGetUniformLocation( m_Program, "offsScale" );
	m_locMatView = glGetUniformLocation( m_Program, "matView" );
	m_locDim = glGetUniformLocation( m_Program, "dim" );
	m_locSmooth = glGetUniformLocation( m_Program, "range" );
	m_locBlackBias = glGetUniformLocation( m_Program, "blackBias" );
	m_locParams = glGetUniformLocation( m_Program, "params" );

	if( -1 == m_locContent || -1 == m_locContentBypass )
	{
		logStr( 0, "WARINIG: Shader content input missing.\n" );
		//return VWB_ERROR_SHADER;
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR GLWarpBlend::FillTexture( GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* data, GLint filter, GLenum wrap )
{
	GLenum err = ::glGetError();
	glTexImage2D( GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data );
	err = ::glGetError();
	if( GL_NO_ERROR != err )
	{
		logStr( 0, "ERROR: %d at glTexImage2D:\n", err );
		return VWB_ERROR_WARP;
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
		logStr( 0, "ERROR: %d at glTexParameteri:\n", err );
		return VWB_ERROR_WARP;
	}

	return VWB_ERROR_NONE;
}

VWB_ERROR GLWarpBlend::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = VWB_Warper_base::Init( wbs );

	if( VWB_ERROR_NONE == err ) try
	{
		err = CreatePixelShader();
		if( VWB_ERROR_NONE != err )
			return err;

		glGenTextures( 1, &m_texWarp );
		glGenTextures( 1, &m_texBlend );
		glGenTextures( 1, &m_texBlack );
		glGenTextures( 1, &m_texBB );
		GLenum errGL = ::glGetError();
		if( GL_NO_ERROR != errGL )
		{
			logStr( 0, "ERROR: GLERROR %d at glGenTextures:\n", errGL );
			return VWB_ERROR_BLEND;
		}

		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, m_texWarp );
		err = FillTexture( ( wbs[calibIndex]->header.flags & FLAG_WARPFILE_HEADER_3D ) ? GL_RGB32F : GL_RG32F, wbs[calibIndex]->header.width, wbs[calibIndex]->header.height, GL_RGBA, GL_FLOAT, wbs[calibIndex]->pWarp, GL_NEAREST, GL_CLAMP_TO_BORDER );
		if( VWB_ERROR_NONE != err )
		{
			logStr( 0, "ERROR: %d failed to fill warp texture:\n", err );
			return err;
		}
		if( 4 <= g_logLevel )
		{
			char o[MAX_PATH];
			strcpy_s( o, g_logFilePath );
			strcat_s( o, ".tex.warp.bmp" );
			savetex( o, m_texWarp );
			logStr( 4, "Warp texture (%dx%d) saved as \"%s\".", wbs[calibIndex]->header.width, wbs[calibIndex]->header.height, o );
		}

		if( wbs[calibIndex]->pBlend2 )
		{
			glBindTexture( GL_TEXTURE_2D, m_texBlend );
			err = FillTexture( GL_RGBA16, wbs[calibIndex]->header.width, wbs[calibIndex]->header.height, GL_RGBA, GL_UNSIGNED_SHORT, wbs[calibIndex]->pBlend2, GL_LINEAR, GL_CLAMP_TO_BORDER );
			if( VWB_ERROR_NONE != err )
			{
				logStr( 0, "ERROR: %d failed to fill blend texture:\n", err );
				return VWB_ERROR_BLEND;
			}
			if( 4 <= g_logLevel )
			{
				char o[MAX_PATH];
				strcpy_s( o, g_logFilePath );
				strcat_s( o, ".tex.blend.bmp" );
				savetex( o, m_texBlend );
				logStr( 4, "Blend texture (%dx%d) saved as \"%s\".", wbs[calibIndex]->header.width, wbs[calibIndex]->header.height, o );
			}
		}
		else
		{
			logStr( 3, "WARINIG: No blend texture present.", err );
			bDoNotBlend = true;
		}

		if( wbs[calibIndex]->pBlack )
		{
			glBindTexture( GL_TEXTURE_2D, m_texBlack );
			err = FillTexture( GL_RGBA8, wbs[calibIndex]->header.width, wbs[calibIndex]->header.height, GL_RGBA, GL_UNSIGNED_BYTE, wbs[calibIndex]->pBlack, GL_LINEAR, GL_CLAMP_TO_BORDER );
			if( VWB_ERROR_NONE != err )
			{
				logStr( 0, "ERROR: %d failed to fill blend texture:\n", err );
				return VWB_ERROR_BLEND;
			}
			if( 4 <= g_logLevel )
			{
				char o[MAX_PATH];
				strcpy_s( o, g_logFilePath );
				strcat_s( o, ".tex.black.bmp" );
				savetex( o, m_texBlend );
				logStr( 4, "Input texture (%dx%d) saved as \"%s\".", m_sizeIn.cx, m_sizeIn.cy, o );
			}
		}
		else
		{
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
	GLint                       active_texture_unit = -1;
	GLint						active_client_texture_unit = -1;
	GLint						oldVA = -1;

	//TODO record current state
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
			glGetIntegerv( GL_VERTEX_ARRAY, &oldVA );


	    glGetIntegerv( GL_ACTIVE_TEXTURE, &( active_texture_unit ) );
		glActiveTexture (GL_TEXTURE0 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding0 ) );

		glActiveTexture( GL_TEXTURE1 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding1 ) );

		glActiveTexture( GL_TEXTURE2 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding2 ) );

		glActiveTexture( GL_TEXTURE3 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding3 ) );
	}

	GLint viewport[4] = { 0 };
	glGetIntegerv( GL_VIEWPORT, viewport );

	if( -1 == iSrc )
	{
		glGetIntegerv( GL_UNPACK_ROW_LENGTH, &url );
		glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );

		::glActiveTexture( GL_TEXTURE0 );
		::glBindTexture( GL_TEXTURE_2D, m_texBB );

		GLint oldRB = GL_BACK;
		GLint WB = GL_BACK;
		glGetIntegerv( GL_READ_BUFFER, &oldRB );
		glGetIntegerv( GL_DRAW_BUFFER, &WB );
		//gl
		glReadBuffer( WB );


		if( viewport[2] != m_sizeIn.cx || viewport[3] != m_sizeIn.cy )
		{
			res = glGetError();
			VWB_ERROR rr = FillTexture( GL_RGB, viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, NULL, GL_LINEAR, GL_CLAMP_TO_BORDER );
			if( VWB_ERROR_NONE != rr )
			{
				logStr( 0, "ERROR: %d failed to fill content clone texture:\n", rr );
				return VWB_ERROR_BLEND;
			}
			else
				logStr( 2, "clone texture (%dx%d) created.", viewport[2], viewport[3] );

			m_sizeIn.cx = viewport[2];
			m_sizeIn.cy = viewport[3];
		}
		glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, viewport[0], viewport[1], viewport[2], viewport[3] );
		res = glGetError();
		if( GL_NO_ERROR == res )
		{
			logStr( 4, "Copied input texture from current write buffer" );
		}
		else
		{
			static bool bOnce = true;
			if( bOnce )
			{
				logStr( 3, "FAILED to copy input texture from current write buffer, glError = %04x", res );
				bOnce = false;
			}
		}

		if( oldRB != GL_BACK )
			glReadBuffer( oldRB );
		iSrc = m_texBB;
	}
	else
	{
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, iSrc );
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &m_sizeIn.cx );
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &m_sizeIn.cy );
	}

	if( 4 <= g_logLevel )
	{
		char o[MAX_PATH];
		strcpy_s( o, g_logFilePath );
		strcat_s( o, ".texin.tif" );
		savetex( o, iSrc );
		logStr( 4, "Input texture (%dx%d) saved as \"%s\".", m_sizeIn.cx, m_sizeIn.cy, o );
	}

	// set own params
	::glUseProgram(m_Program);
    res = ::glGetError();
	if( GL_NO_ERROR == res )
	{
		// Set the texturing modes
		SetTexture( m_locWarp, m_texWarp);
		SetTexture( m_locBlend, m_texBlend );
		SetTexture( m_locBlack, m_texBlack );
		SetTexture( m_locContent, iSrc );

		//res = glGetError();
		//if( GL_NO_ERROR == res )
		{
			if (m_bDynamicEye)
				glUniformMatrix4fv( m_locMatView, 1, GL_TRUE, m_mVP );
			else
				glUniform1i( m_locBorder, m_bBorder );
			if( bBicubic )
				glUniform4f( m_locParams, (GLfloat)m_sizeIn.cx, (GLfloat)m_sizeIn.cy, 1.0f/m_sizeIn.cx, 1.0f/m_sizeIn.cy );
			glUniform1i( m_locDoNotBlend, bDoNotBlend );
			glUniform1i( m_locDoNoBlack, bDoNoBlack );
			if(bPartialInput)
				glUniform4f( m_locOffsScale, 
					(GLfloat)optimalRect.left / (GLfloat)optimalRes.cx,
					(GLfloat)optimalRect.top / (GLfloat)optimalRes.cy,
					(GLfloat)optimalRes.cx / ((GLfloat)optimalRect.right - (GLfloat)optimalRect.left ),
					(GLfloat)optimalRes.cy / ((GLfloat)optimalRect.bottom - (GLfloat)optimalRect.top )
				);
			else
				glUniform4f( m_locOffsScale,  0.0f, 0.0f, 1.0f, 1.0f );
			glUniform4f( m_locBlackBias, 
						 m_blackBias.x, m_blackBias.y, m_blackBias.z, m_blackBias.w );

			if( VWB_STATEMASK_VIEWPORT & stateMask )
			{
				glViewport( 0, 0, m_sizeIn.cx, m_sizeIn.cy );
			}

			if( VWB_STATEMASK_CLEARBACKBUFFER & stateMask )
			{
				glClearColor( 0, 0, 0, 1 );
				glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
			}

			if (bUseGL110)
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

				GLfloat x1 = ( -0.5f 			  ) / viewport[2];
				GLfloat y1 = ( -0.5f 			  ) / viewport[3];
				GLfloat x2 = ( 0.5f + viewport[2] ) / viewport[2];
				GLfloat y2 = ( 0.5f + viewport[3] ) / viewport[3];

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
					glTexCoord2f(0,0);        glVertex2f(x1,y1);
					glTexCoord2f(0,1);        glVertex2f(x1,y2);
					glTexCoord2f(1,1);        glVertex2f(x2,y2);
					glTexCoord2f(1,0);        glVertex2f(x2,y1);
				glEnd();

				glPopMatrix();
				glMatrixMode( GL_TEXTURE );
				glPopMatrix();
				glMatrixMode( GL_MODELVIEW );
				glPopMatrix();
			}
			else
			{
				glBindVertexArray(m_iVertexArray);
				glUniform4f( m_locSize, (GLfloat)viewport[2], (GLfloat)viewport[3],1.0f/viewport[2], 1.0f/viewport[3] );
				glDisable(GL_DEPTH_TEST); 
				glDisable(GL_CLIP_PLANE0);
				glDisable(GL_CLIP_PLANE1);
				glDisable(GL_CLIP_PLANE2);
				glDisable(GL_CLIP_PLANE3);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4 );
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

		glActiveTexture(active_texture_unit);
	}

	if( ( VWB_STATEMASK_VERTEX_SHADER | VWB_STATEMASK_PIXEL_SHADER ) & stateMask )
		glUseProgram(program);

	if( ( ( VWB_STATEMASK_RASTERSTATE | VWB_STATEMASK_SAMPLER ) & stateMask ) && bUseGL110 )
	{
		glMatrixMode( matrix_mode );
		glPopAttrib();
	}

	if( -1 != url )
		glPixelStorei( GL_UNPACK_ROW_LENGTH, url );
    return VWB_ERROR_NONE;
}
	
void GLWarpBlend::SetTexture( GLuint loc, GLuint tex, GLuint wrapMode ) const
{
	if( -1 != loc && -1 != tex )
	{
		::glUniform1i( loc, loc );
		::glActiveTexture( GL_TEXTURE0 + loc );
//		::glClientActiveTexture( GL_TEXTURE0 + loc );
		::glBindTexture( GL_TEXTURE_2D, tex );

		::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode );
		::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode );
		::glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, colBlack );
		if( bUseGL110 )
			::glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_MODULATE );
	}
}
