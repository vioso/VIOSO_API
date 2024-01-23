#ifndef VWB_GLWARPBLENDXPL_HPP
#define VWB_GLWARPBLENDXPL_HPP

#include "GLWarpBlend.h"
#include "../../Include/VWBTypes.h"

class GLWarpBlendXPL : public GLWarpBlend
{
protected:
	IXPlaneRef* XPLM;
public:
 	GLWarpBlendXPL( IXPlaneRef* pXPL );
	virtual ~GLWarpBlendXPL(void);
	virtual VWB_ERROR Init( VWB_WarpBlendSet& wbs );
	virtual VWB_ERROR Render( VWB_param inputTexture, VWB_uint stateMask );
	void    SetTexture( GLuint texLoc, GLuint tex, GLuint wrapMode = GL_CLAMP_TO_BORDER, GLuint filter = GL_LINEAR ) const;
};

#endif //ndef VWB_GLWARPBLENDXPL_HPP
