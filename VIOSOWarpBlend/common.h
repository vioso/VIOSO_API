#define SAFERELEASE( x ) if( x ){ x->Release(); x = NULL; }

#if !defined( VWB_common_h )
#define VWB_common_h

#include "Platform.h"

// C RunTime Header Files
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#ifdef WIN32
#include <crtdbg.h>
#endif
#else
#include <stdlib.h>
#endif //def _DEBUG

#ifdef WIN32
#include <malloc.h>
#endif
#include <memory.h>
#include <stdio.h>
#include <map>
#include <string>

#include "../Include/VIOSOWarpBlend.h"
#include "logging.h"
#include "PathHelper.h"
#include "VWF.h"
#include "resource.h"
#define STRINGIFY(s) #s
#define STRVER( ma, mi, x ) STRINGIFY(ma.mi.x)
#define VWB_VERSTR STRVER(VWB_Version_MAJ,VWB_Version_MIN,VWB_Version_MAI)
#include "mmath.h"

#include "../Include/EyePointProvider.h"

#if defined( WIN32 )
extern VWB_int g_error;
extern bool g_bCurEnabled;
extern HCURSOR g_hCur;
extern SIZE	g_dimCur;
extern POINT g_hotCur;
typedef int (WINAPI *FPtrInt_BOOL)(BOOL bShow);
extern FPtrInt_BOOL ShowSystemCursor;

size_t copyCursorBitmapToMappedTexture( HBITMAP hbmMask, HBITMAP hbmColor, BITMAP& bmMask, BITMAP& bmColor, void* data, int pitch );
#endif

#ifndef MIN
	#define MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
	#define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif

inline void spliceVec( VWB_double& outX, VWB_double& outY, VWB_double& outZ, VWB_double const& inX, VWB_double const& inY, VWB_double const& inZ, VWB_uint sw );

inline std::string fmt( const char* format, ... )
{
	char buf[256];
	va_list args;
	va_start( args, format );
	const auto r = std::vsnprintf( buf, sizeof buf, format, args );
	if( r < 0 )
	{
		va_end( args );
		return {};
	}
	else
	{
		const size_t len = r;
		if( len < sizeof( buf ) )
		{
			va_end( args );
			return { buf, len };
		}

		std::string s( len, '\0' );
		#if __cplusplus >= 201703L
		std::vsnprintf( s.data(), len + 1, format, args );
		#else
		std::vsnprintf( &s[0], len + 1, format, args );
		#endif
		va_end( args );
		return s;
	}
}


class VWB_Warper_base : public VWB_Warper 
{
protected:
	VWB_uint		m_type4cc;				/// the class type
	VWB_size		m_sizeMap;				/// the size of the mappings
	VWB_size		m_sizeIn;				/// the back buffer / input texture size
	bool			m_bDynamicEye;			/// indicates dymaic eye-point correction
	bool			m_bBorder;				/// indicates use of border correction
	bool			m_bRH;					/// indicates right hand system
	VWB_MAT44f		m_mBaseI;				/// the inverted base transformation, calculated from trans
	VWB_MAT44f		m_mViewIG;				/// the view matrix of the static frustum for the IG, this contains translation and rotation offsets
	VWB_MAT44f		m_mVP;					/// the view-projection matrix to be put into the shader
	EyePoint		m_ep;					/// the current eye
	VWB_VEC4f		m_viewSizes;			/// the calculated view size, x - left, y - top, z - right, w - bottom
	VWB_VEC4f       m_blackBias;			/// the black bias values
											/// [0] is black offset scale,
											/// [1] is black cut; range 0..1, set to 0 to clip values darker as blacklevel offset, 1 will lift all values above black level, default is 1
											/// [2] is uplift-downscale; range 0..1, set to 0 to clip whites that are shifted out of range, set to 1 to scale values to stay in range, default is 1
											/// [3] is not used, set to 0
	std::string		m_overlay;				/// a path or ndi locator to show instead of IG input
#ifdef WIN32
	HMODULE			m_hmEPP;					/// the eye point provider module handle
#else
    void* m_hmEPP;
#endif //def WIN32
	void*	 m_hEPP; // eye point provider handle pointer
	pfn_CreateEyePointReceiver	 m_fnEPPCreate; /// eye point provider create function pointer
	pfn_ReceiveEyePoint	m_fnEPPGet;			/// eye point provider getter function pointer
	pfn_DeleteEyePointReceiver m_fnEPPRelease;	/// eye point provider release function pointer

#ifdef _NDI_DISP
	void*	m_ndi_recv;						/// a ndi receiver
	void*	m_ndi_frame;						/// a ndi frame, if set, it will be shown instead of the host application's input
#endif //def _NDI_DISP
	
public:
	VWB_Warper_base();						/// constructor
	virtual ~VWB_Warper_base();				/// virtual destructor

    /** initialize maps
	* @param wb	the warp blend set
    * @return			error code, VWB_ERROR_NONE on success, otherwise @see VWB_ERROR*/
	virtual VWB_ERROR Init( VWB_WarpBlendSet& wbs );		// 

	/** set the new dynamic eye position and calculate viewports
    * @param [opt_INOUT] eye, if eye point provider present, receives, else set the new eye position
    * @param [opt_INOUT] rot, if eye point provider present, receives, else set the new eye rotation angles
    * @param [opt_OUT] pView, if not NULL it gets the updated view matrix to translate and rotate into the viewer's perspective
	* @param [opt_OUT] pProj, if not NULL it gets the updated projection matrix
	* @param [OUT]			pClip	it gets the updated clip planes, left, top, right, bottom, near, far
	* @param [OUT]			pPos    it gets the updated relative position: x,y,z
	* @param [OUT]			pDir	it gets the updated relative direction: euler angles around x,y,z rotation order is y,x,z
	* @param [OUT]			pSymFov	it gets the updated symmetric frustum: hFov, vFov, near, far
	* @return VWB_ERROR_NONE on success, otherwise @see VWB_ERROR
	* @remarks If EyePointProvider is used, the eye point is set by calling it's getEye function. eye and rot are set to that if not NULL.
	* Else, if eye and rot are not NULL, values taken from here.
	* Else eye and rot are set to 0-vectors.
	* The internal view and projection matrices are calculated to render. You should set pView and pProj to get these matrices for rendering, if updated.*/
	VWB_ERROR UpdateEye( VWB_float* eye, VWB_float* rot );
	virtual VWB_ERROR GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj );		// 
	virtual VWB_ERROR GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip );		// 
	virtual VWB_ERROR GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric = false, VWB_float aspect = 0 );
	virtual VWB_ERROR GetScreenplane( VWB_float* pTL, VWB_float* pTR, VWB_float* pBL, VWB_float* pBR );

	/** sets internal projection and view matrix
    * @param view    the view matrix
    * @param proj    the projection matrix
    * @return VWB_ERROR_NONE on success, otherwise @see VWB_ERROR */
	virtual VWB_ERROR SetViewProjection( VWB_float const* pView, VWB_float const* pProj )=0;

    /** render a warped and blended source texture into the current back buffer
    * @param [in,opt] inputTexture    the source texture, if set to NULL, a backbuffer copy is used as input
    * @return VWB_ERROR_NONE on success, otherwise @see VWB_ERROR */
	virtual VWB_ERROR Render( VWB_param inputTexture, VWB_uint stateMask );

	virtual VWB_ERROR getWarpBlend( VWB_WarpBlend const*& wb );
	virtual VWB_ERROR getShaderVPMatrix( VWB_float* pMVP );
	virtual VWB_ERROR getWarpMesh( VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh );

	/// read the .ini file
	VWB_ERROR ReadIniFile( char const* szConfigFile, char const* szChannelName );
	char const* GetType() const { return (char const*)&m_type4cc; };
	VWB_size getMappingSize() { return m_sizeMap; }
protected:
	/// set all default values to the VWB_Warper struct
	void Defaults();
	/// update view parameters from warp bland set
	VWB_ERROR AutoView( VWB_WarpBlend const& wb );
	/// update view parameters from warp bland set
	VWB_ERROR FixWraparound( VWB_WarpBlend& wb );
	// calculate clipping planes
	void getClip( VWB_VEC3f const& e, VWB_float* pClip );
};

class Dummywarper : public VWB_Warper_base
{
protected:
	 VWB_WarpBlend m_wb;
public:
 	Dummywarper();

	virtual ~Dummywarper(void);

	virtual VWB_ERROR Init( VWB_WarpBlendSet& wbs );

	VWB_MAT44f UpdateView( VWB_MAT44f const& igView, VWB_VEC3f& e );
	virtual VWB_ERROR GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj );
	virtual VWB_ERROR GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip );
	virtual VWB_ERROR GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pSymClip, bool symmetric, VWB_float aspect ) override;

	virtual VWB_ERROR SetViewProjection( VWB_float const* pView, VWB_float const* pProj );

    virtual VWB_ERROR Render( VWB_param inputTexture, VWB_uint stateMask );  

	virtual VWB_ERROR getWarpBlend( VWB_WarpBlend const*& wb );
	virtual VWB_ERROR getWarpMesh( VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh );
};

#if defined( __GNU__ )
  void __attribute__ ((constructor)) my_init(void);
  void __attribute__ ((destructor)) my_fini(void);
#endif

#endif //!defined( VWB_common_h )

