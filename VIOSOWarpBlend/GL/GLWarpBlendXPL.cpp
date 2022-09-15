#include "GLWarpBlendXPL.h"

GLWarpBlendXPL::GLWarpBlendXPL( IXPlaneRef* pXPL ): GLWarpBlend()
, XPLM( pXPL )
{
	XPLM->AddRef();
	logStr( 1, "INFO: Doing XPL-Overrride.\n" );
	memcpy( &m_type4cc, "XPL\0", 4 );
}

GLWarpBlendXPL::~GLWarpBlendXPL()
{
	XPLM->Release();
	logStr( 1, "INFO: XPL-Override finished.\n" );
}

VWB_ERROR GLWarpBlendXPL::Init( VWB_WarpBlendSet& wbs )
{
	VWB_ERROR err = VWB_Warper_base::Init( wbs );

	if( VWB_ERROR_NONE == err ) try
	{
		err = CreatePixelShader();
		if( VWB_ERROR_NONE != err )
			return err;

		XPLM->GenerateTextureNumbers( reinterpret_cast<int*>(&m_texWarp), 1 );
		XPLM->GenerateTextureNumbers( reinterpret_cast<int*>(&m_texBlend), 1 );
		XPLM->GenerateTextureNumbers( reinterpret_cast<int*>(&m_texBlack ), 1 );
		XPLM->GenerateTextureNumbers( reinterpret_cast<int*>(&m_texBB ), 1 );
		GLenum errGL = ::glGetError();
		if( GL_NO_ERROR != errGL )
		{
			logStr( 0, "ERROR: GLERROR %d at glGenTextures:\n", errGL );
			return VWB_ERROR_BLEND;
		}

		XPLM->BindTexture2d( m_texWarp, 0 );
		glActiveTexture( GL_TEXTURE0 );
		//glBindTexture( GL_TEXTURE_2D, m_texWarp );
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
			XPLM->BindTexture2d( m_texBlend, 0 );
			//glBindTexture( GL_TEXTURE_2D, m_texBlend );
			err = FillTexture( GL_RGBA16, wbs[calibIndex]->header.width, wbs[calibIndex]->header.height, GL_RGBA, GL_UNSIGNED_SHORT, wbs[calibIndex]->pBlend2, GL_LINEAR, GL_CLAMP_TO_BORDER );
			if( VWB_ERROR_NONE != err )
			{
				logStr( 0, "ERROR: %d failed to fill blend texture:", err );
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
			XPLM->BindTexture2d( m_texBlack, 0 );
			//glBindTexture( GL_TEXTURE_2D, m_texBlack );
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
				savetex( o, m_texBlack );
				logStr( 4, "Input texture (%dx%d) saved as \"%s\".", m_sizeIn.cx, m_sizeIn.cy, o );
			}
		}
		else
		{
			logStr( 3, "WARINIG: No blacklevel texture present.", err );
			bDoNoBlack = true;
		}

		logStr( 1, "SUCCESS: XPL-Warper initialized.\n" );

	}
	catch( VWB_ERROR e )
	{
		return e;
	}
	return err;
}


//VWB_ERROR GLWarpBlend::

VWB_ERROR GLWarpBlendXPL::Render( VWB_param inputTexture, VWB_uint stateMask )
{
	logStr( 4, "Render XPL" );
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

	// record current state
	if( ( ( VWB_STATEMASK_RASTERSTATE | VWB_STATEMASK_SAMPLER ) & stateMask ) && bUseGL110 )
	{
		glPushAttrib( GL_ALL_ATTRIB_BITS );
		glGetIntegerv( GL_MATRIX_MODE, &matrix_mode );
	}

	if( ( VWB_STATEMASK_VERTEX_SHADER | VWB_STATEMASK_PIXEL_SHADER ) & stateMask )
		glGetIntegerv( GL_CURRENT_PROGRAM, (GLint*)&( program ) );

	if( VWB_STATEMASK_SHADER_RESOURCE & stateMask )
	{
		if( bUseGL110 )
			glGetIntegerv( GL_CLIENT_ACTIVE_TEXTURE, &active_client_texture_unit );
		else
			glGetIntegerv( GL_VERTEX_ARRAY, &oldVA );


		glGetIntegerv( GL_ACTIVE_TEXTURE, &( active_texture_unit ) );
		glActiveTexture( GL_TEXTURE0 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding0 ) );

		glActiveTexture( GL_TEXTURE1 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding1 ) );

		glActiveTexture( GL_TEXTURE2 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding2 ) );

		glActiveTexture( GL_TEXTURE3 );
		glGetIntegerv( GL_TEXTURE_BINDING_2D, &( currentTexture2DBinding3 ) );
	}

	GLint viewport[4] = {0};
	glGetIntegerv(GL_VIEWPORT, viewport);

	if( -1 == iSrc )
	{
		glGetIntegerv( GL_UNPACK_ROW_LENGTH, &url );
		glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );

		glActiveTexture( GL_TEXTURE0 );
		XPLM->BindTexture2d( m_texBB, 0 );

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
				logStr( 3, "FAILED to copy input texture from current write buffer, glError = %#04x", res );
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
		XPLM->BindTexture2d( iSrc, 0 );
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &m_sizeIn.cx );
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &m_sizeIn.cy );
	}

	if (4 <= g_logLevel)
	{
		char o[MAX_PATH];
		strcpy_s( o, g_logFilePath );
		strcat_s( o, ".tex.in.bmp" );
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

				XPLM->SetGraphicsState( 0, 4, 0, 0, 0, 0, 0 );

				glDisable(GL_COLOR_MATERIAL);
				glDisable(GL_CLIP_PLANE0);
				glDisable(GL_CLIP_PLANE1);
				glDisable(GL_CLIP_PLANE2);
				glDisable(GL_CLIP_PLANE3);
				glDisable( GL_ALPHA_TEST );
				glDisable( GL_BLEND );
				glDisable( GL_CULL_FACE );
				glDisable( GL_DEPTH_TEST );
				glDepthMask( GL_FALSE );
				glDisableClientState( GL_TEXTURE_COORD_ARRAY );

				glEnable(GL_TEXTURE_2D);
				GLint opm[2];
				glGetIntegerv( GL_POLYGON_MODE, opm );
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
				glPolygonMode( opm[0], opm[1] );
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

void GLWarpBlendXPL::SetTexture( GLuint loc, GLuint tex, GLuint wrapMode ) const
{
	if( -1 != loc && -1 != tex )
	{
		::glUniform1i( loc, loc );
		XPLM->BindTexture2d( tex, loc );

		::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode );
		::glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode );
		::glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, colBlack );
		if( bUseGL110 )
			::glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	}
}
