// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "VWBTypes.h"
#include "mmath.h"
#include "EyePointProvider.h"
#include "licCM.h"
#include "logging.h"
#include "common.h"

#include "dpLogo.h"

#include <functional>

class VWB_Warper_base : public VWB_Warper 
{
public:
	typedef struct ImageBuffer {
		int type;
		int width;
		int height;
		void* data;
	} ImageBuffer;
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
	EyePoint		m_ep;					/// the current eye, maybe axis-swapped or negated
	VWB_VEC3f		m_eye;					/// this is the effective eypoint in screen coordinates, this is used to find the direction for directional shading
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
	bool m_bUTF8; /// threat chars as UTF-8, this is set by VWB_createW and VWB_createU
	std::filesystem::path m_configPath;  // the base path to load mappings from
	bool m_bDP;  // domeplrojection mode
	std::shared_ptr< const VWBLic::LicenseInfo > m_licenseInfo; // license info
	std::array<ImageBuffer,2> m_overlayBuffers; // double buffered overlay images
	std::atomic_int m_currentOverlayBuffer; // index of the current overlay buffer
	std::atomic_bool m_bOverlayUpdated; // indicates that overlay buffer has been updated

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
	virtual VWB_ERROR GetViewProjection( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pProj ){ return VWB_ERROR_NOT_IMPLEMENTED; } 
	virtual VWB_ERROR GetViewClip( VWB_float* eye, VWB_float* rot, VWB_float* pView, VWB_float* pClip ) { return VWB_ERROR_NOT_IMPLEMENTED; } 
	virtual VWB_ERROR GetPosDirClip( VWB_float* eye, VWB_float* rot, VWB_float* pPos, VWB_float* pDir, VWB_float* pClip, bool symmetric = false, VWB_float aspect = 0 ) { return VWB_ERROR_NOT_IMPLEMENTED; } 
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
	virtual VWB_ERROR Render2( VWB_param inputTexture, VWB_uint stateMask );

	virtual VWB_ERROR getWarpBlend( VWB_WarpBlend const*& wb );
	virtual VWB_ERROR getShaderVPMatrix( VWB_float* pMVP );
	virtual VWB_ERROR getWarpMesh( VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh );

	/// read the config file file
	VWB_ERROR ReadConfigFile( std::filesystem::path const& configFile, char const* szChannelName );
	/// read from domeprojection XML
	VWB_ERROR ReadXMLFile( std::filesystem::path const& configFile, char const* szChannelName );
	/// read from VIOSO ini
	VWB_ERROR ReadIniFile( std::filesystem::path const& configFile, char const* szChannelName );

	char const* GetType() const { return (char const*)&m_type4cc; };
	VWB_size getMappingSize() { return m_sizeMap; }
	void setUTF8(bool utf8) { m_bUTF8 = utf8; }
	bool isUTF8() const { return m_bUTF8; }
	bool isDP() const { return m_bDP; }
	bool isRH() const { return m_bRH; }
	bool isDynamic() const { return m_bDynamicEye; }
	std::filesystem::path const& getConfigPath() const { return m_configPath; }
	int getInfoFlags() const {
		int flags = 0;
		if( m_type4cc != 0 )
			flags |= VWB_WARPER_INFO_FLAGS_CREATED;
		if( m_sizeMap.cx != 0 && m_sizeMap.cy != 0 )
			flags |= VWB_WARPER_INFO_FLAGS_INITIALIZED;
		if( m_bDynamicEye )
			VWB_WARPER_INFO_FLAGS_DYNAMIC_EYEPOINT;
		if( m_bBorder )
			flags |= VWB_WARPER_INFO_FLAGS_BORDER;
		if( m_bRH )
			flags |= VWB_WARPER_INFO_FLAGS_RIGHT_HANDED;
		if( m_bUTF8 )
			flags |= VWB_WARPER_INFO_FLAGS_UTF8;
		if( m_bDP )
			flags |= VWB_WARPER_INFO_FLAGS_DOMEPROJECTION;

		return flags;
	}
	// thread-safe overlay image update
	void UpdateOverlayImage( ImageBuffer const& imgBuf );
protected:
	/// set all default values to the VWB_Warper struct
	static void Default( VWB_Warper& w );
	static VWB_Warper GetDefaultWarper();
	/// update view parameters from warp bland set
	VWB_ERROR AutoView( VWB_WarpBlend const& wb );
	/// update view parameters from warp bland set
	VWB_ERROR FixWraparound( VWB_WarpBlend& wb );
	// calculate clipping planes
	void getClip( VWB_VEC3f const& e, VWB_float* pClip );

	/// virtual overlay texture update
	virtual void UpdateOverlayTexture( int type, int w, int h, void const* data ) {};

};
