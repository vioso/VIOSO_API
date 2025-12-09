// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#ifndef VWB_GLWARPBLEND_HPP
#define VWB_GLWARPBLEND_HPP

#include "../WarperBase.h"
#include "../common.h"
#include "GLext.h"
#include <string>

class GLWarpBlend : public VWB_Warper_base
{
public:
#pragma pack(push, 4)
	typedef struct Vertex {
		VWB_float x, y, z;
		VWB_float u, v;
		VWB_float nx, ny, nz;
		VWB_float tx, ty, tz;
	} Vertex;
#pragma pack(pop)
protected:
    //HGLRC					m_context;          // the gl render context
    //HDC						m_hDC;				// the device context
    GLuint				    m_texWarp;          // the warp lookup texture, in case of 3D it contains the real world 3D coordinates of the screen
	GLuint				    m_texBlend;         // the blend lookup texture
	GLuint				    m_texBlendX;         // the blend lookup texture
	GLuint				    m_texBlack;         // the black-level lookup texture
	GLuint				    m_tex2ndBlend;         // the secondary blend lookup texture
	GLuint				    m_texDirectionalShading;         // the directional shading lookup texture
	GLuint					m_texBB;			// the back buffer copy, if demanded
	GLuint					m_texOverlay; // the overlay texture
	GLuint					m_texOverlay2; // 2nd overlay texture for double buffering

	GLuint					m_locCamPos; // camera position for DP shader
	GLuint					m_locWarp; // warp texture location
	GLuint					m_locBorder; // border on/off, blend on/off, blacklevel on/off
	GLuint					m_locBlend; // blend texture location
	GLuint					m_locBlack; // blacklevel texture location
	GLuint					m_loc2ndBlend; // secondary blend texture location
	GLuint					m_locDirectionalShading; // directional shading texture location
	GLuint					m_locDoNotBlend; // do not blend flag
	GLuint					m_locDoNoBlack; // do not blacklevel flag
	GLuint					m_locOffsScale; // offset and scale for content uv
	GLuint					m_locContent; // content texture location
	GLuint					m_locContentBypass; // content texture location for bypass shader
	GLuint					m_locMatView; // view-projection matrix location
	GLuint					m_locDim; // content dimensions location
	GLuint					m_locSmooth; // smoothing flag location
	GLuint					m_locBlackBias;	// black level bias location
	GLuint					m_locParamsBicubic; // params location for bicubic sampler
	GLuint					m_locParamsDomeprojection; // params location for domeprojection shaders

	GLuint					m_iVertexArray; // the vertex array object
	GLuint					m_iVertices; // the vertex buffer object
	GLuint					m_iIndices; // the index buffer object

	GLuint				    m_Program; // the normal shader program
	GLuint				    m_ProgramBypass; // the bypass shader program
	GLsizei					m_nIndices;			// number of indices in the index buffer
	
public:
 	GLWarpBlend();

	virtual ~GLWarpBlend(void);
	virtual VWB_ERROR Init( VWB_WarpBlendSet& wbs ) override;
	virtual VWB_ERROR GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj ) override;
	virtual VWB_ERROR GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip ) override;
	virtual VWB_ERROR GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pSymClip, bool symmetric, VWB_float aspect ) override;
	virtual VWB_ERROR SetViewProjection( VWB_float const* pView, VWB_float const* pProj ) override;
	virtual VWB_ERROR Render( VWB_param inputTexture, VWB_uint stateMask ) override; 

	/// @brief Calculates the IG's view matrix and updates the internal view-projection matrix based on input view matrix and eye position
	/// Note: m_mVP is only the view part after this step, projection must be multiplied later
	/// @param [IN] igView the view matrix, this is usually the reference to own m_mViewIG, but might be different. This is because requesting a symmetric view might change the view direction.
	/// @param [OUT] e the resulting eye position calculated from the view matrix + the current rotation with respect to the internal eye offset
	/// @return the new view matrix
	VWB_MAT44f UpdateView( VWB_MAT44f const& igView, VWB_VEC3f& e );
protected:
	// Helper functions
 
    /// @brief create shaders
	/// @return VWB_ERROR_NONE on success, VWB_ERROR_SHADER otherwise @see VWB_ERROR
    VWB_ERROR CreateShaders();

	/// @brief fill a texture with data
	/// @param iTex the texture handle
	/// @param internalFormat the internal format, like GL_R8 to GL_RGBA32F 
	/// @param width the width of the texture
	/// @param height the height of the texture
	/// @param format the base format, like GL_RED to GL_RGBA setting the number of channels / components
	/// @param type the base type, like GL_UNSIGNED_BYTE to GL_FLOAT
	/// @param data a pointer to the data to fill the texture with, might be NULL to create an empty texture
	/// @param filter the texture filter, like GL_LINEAR or GL_NEAREST
	/// @param wrap the texture wrap mode, like GL_CLAMP_TO_BORDER, GL_REPEAT or GL_MIRRORED_REPEAT
	/// @param name a name for debug purposes
	/// @param save set to true, to save the texture to a file for debug purposes, if debug level is 4 or higher. The filename is constructed from the name parameter and the log file path.
	/// @return VWB_ERROR_NONE on success, VWB_ERROR_BLEND otherwise @see VWB_ERROR
	VWB_ERROR FillTexture( GLuint& iTex, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid const* data, GLint filter, GLenum wrap, char const* name = "unnamed", bool save = false );

	/// @brief bind a texture to a texture location with given parameters
	/// @param texLoc the texture location in the shader
	/// @param tex the texture handle
	/// @param wrapMode the texture wrap mode, like GL_CLAMP_TO_BORDER, GL_REPEAT or GL_MIRRORED_REPEAT
	/// @param filter the texture filter, like GL_LINEAR or GL_NEAREST
    void    SetTexture(GLuint texLoc, GLuint unit, GLuint tex, GLuint wrapMode = GL_CLAMP_TO_BORDER, GLuint filter = GL_LINEAR ) const;
	static GLfloat colBlack[4];
	static bool savetex( std::filesystem::path filename, GLint iTex );
	virtual void UpdateOverlayTexture( int type, int w, int h, void const* data ) override;
};

#endif //ndef VWB_GLWARPBLEND_HPP
