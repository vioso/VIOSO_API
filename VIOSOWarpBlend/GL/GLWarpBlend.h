#ifndef VWB_GLWARPBLEND_HPP
#define VWB_GLWARPBLEND_HPP

#include "../common.h"
#include "GLext.h"
#include <string>

class GLWarpBlend : public VWB_Warper_base
{
public:

protected:
    //HGLRC					m_context;          // the gl render context
    //HDC						m_hDC;				// the device context
    GLuint				    m_texWarp;          // the warp lookup texture, in case of 3D it contains the real world 3D coordinates of the screen
	GLuint				    m_texBlend;         // the blend lookup texture
	GLuint				    m_texBlack;         // the black-level lookup texture
	GLuint					m_texBB;			// the back buffer copy, if demanded

	GLuint					m_locWarp;
	GLuint					m_locBorder;
	GLuint					m_locBlend;
	GLuint					m_locBlack;
	GLuint					m_locSize;
	GLuint					m_locDoNotBlend;
	GLuint					m_locDoNoBlack;
	GLuint					m_locOffsScale;
	GLuint					m_locContent;
	GLuint					m_locContentBypass;
	GLuint					m_locMatView;
	GLuint					m_locDim;
	GLuint					m_locSmooth;
	GLuint					m_locBlackBias;
	GLuint					m_locParams;
	GLuint					m_iVertexArray;
    GLuint					m_matrix;

	GLuint				    m_VertexShader;
    GLuint				    m_FragmentShader;
    GLuint				    m_FragmentShaderBypass;
    GLuint				    m_Program;
    GLuint				    m_ProgramBypass;
	GLuint					m_texOverlay;
	GLuint					m_locOverlay;


public:
 	GLWarpBlend();

	virtual ~GLWarpBlend(void);

	virtual VWB_ERROR Init( VWB_WarpBlendSet& wbs );

	VWB_MAT44f UpdateView( VWB_MAT44f const& igView, VWB_VEC3f& e );

	virtual VWB_ERROR GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj );
	virtual VWB_ERROR GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip );
	virtual VWB_ERROR GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pSymClip, bool symmetric, VWB_float aspect ) override;

	virtual VWB_ERROR SetViewProjection( VWB_float const* pView, VWB_float const* pProj );

	virtual VWB_ERROR Render( VWB_param inputTexture, VWB_uint stateMask );


protected:
	// Helper functions
    VWB_ERROR CreatePixelShader();
	VWB_ERROR FillTexture( GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* data, GLint filter, GLenum wrap );
    void    SetTexture(GLuint texLoc, GLuint tex, GLuint wrapMode = GL_CLAMP_TO_BORDER) const;
	static GLfloat colBlack[4];
	static bool savetex( char filename[MAX_PATH], GLint iTex );
};

#endif //ndef VWB_GLWARPBLEND_HPP
