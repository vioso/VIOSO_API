// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "WarperBase.h"
#include "VWF.h"
#include "tinyxml2/tinyxml2.h"
#include "PathHelper.h"

#include <span>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <limits>

using namespace std::chrono_literals;
using namespace std;
using namespace VWBUtil;
namespace fs = std::filesystem;

#if defined( WIN32 )
static char const* _libExt = ".dll";
#elif defined( __APPLE__ )
// no eye provider
static char const* _libExt = "";
#else
static char const* _libExt = ".so";
#endif // defined( WIN32 )

constexpr VWB_size _size0 = { 0,0 };
static bool __isFirstInstance = false;

VWB_ERROR invertWB( VWB_WarpBlend const& in, VWB_WarpBlend& out );

VWB_Warper_base::VWB_Warper_base()
	: VWB_Warper( GetDefaultWarper() )
	, m_type4cc( 0 )
	, m_sizeMap( _size0 )
	, m_sizeIn( _size0 )
	, m_bDynamicEye( false )
	, m_bBorder( false )
	, m_mBaseI( VWB_MAT44f::I() )
	, m_mViewIG( VWB_MAT44f::I() )
	, m_bRH( true )
	, m_mVP( VWB_MAT44f::I() )
	, m_ep{}
	, m_viewSizes( 1, 1, 1, 1 )
	, m_blackBias( 0, 1, 1, 0 )
	, m_hmEPP( 0 )
	, m_hEPP( NULL )
	, m_fnEPPCreate( NULL )
	, m_fnEPPGet( NULL )
	, m_fnEPPRelease( NULL )
	, m_bUTF8( false )
	, m_bDP( false )
{}

VWB_Warper_base::~VWB_Warper_base() {
	if( m_hmEPP ) {
		if( m_hEPP && m_fnEPPRelease )
			m_fnEPPRelease( m_hEPP );
	#ifdef WIN32
		::FreeLibrary( m_hmEPP );
	#else
	#endif
	}
}

VWB_ERROR VWB_Warper_base::Init( VWB_WarpBlendSet& wbs ) {
	VWB_ERROR ret = VWB_ERROR_NONE;
	// determine screen
	if( 0 > calibIndex ) {
		int iScreen = -calibIndex;
		std::string prefix = std::format("D{} ", iScreen);

		auto it = std::find_if(wbs.begin(), wbs.end(),
								[&](auto* wb) {
									std::string_view name = wb->header.name;
									return name.starts_with(prefix);
								});

		if (it != wbs.end()) {
			int idx = static_cast<int>(std::distance(wbs.begin(), it));
			logStr(2, "AdapterOrdinal %d (%s) resolved to calibIndex %d.\n",
					-calibIndex,
					(*it)->header.name,
					idx);
			calibIndex = idx;
		} else {
			logStr(0, "ERROR: Could not find mapping of display D%u.", iScreen);
			return VWB_ERROR_GENERIC;
		}
	}

#ifdef WIN32
	ShowSystemCursor = reinterpret_cast<FPtrInt_BOOL>( ::GetProcAddress( ::GetModuleHandleA( "user32.dll" ), "ShowSystemCursor" ) );
	if( nullptr == ShowSystemCursor ) {
		logStr( 1, "WARNING: Could not find ShowSystemCursor function." );
	}
#endif //def WIN32


	if( (VWB_int)wbs.size() <= calibIndex ) {
		logStr( 0, "ERROR: calibIndex out of range. There is(are) only %u dataset(s).", (VWB_uint)wbs.size() );
		return VWB_ERROR_GENERIC;
	}

	VWB_WarpBlend& wb = *wbs[calibIndex];

	if( !wb.pWarp && !wb.pMesh ) {
		logStr( 0, "ERROR: no warp in mapping." );
		return VWB_ERROR_GENERIC;
	}

	int oRes[2] = {
		int( 1.0f / wb.header.vCntDispPx[4] ),
		int( 1.0f / wb.header.vCntDispPx[5] )
	};
	int oRect[4] = {
		int( wb.header.vPartialCnt[0] * oRes[0] ),
		int( wb.header.vPartialCnt[1] * oRes[1] ),
		int( wb.header.vPartialCnt[2] * oRes[0] ),
		int( wb.header.vPartialCnt[3] * oRes[1] )
	};

	if( 0 >= oRect[2] - oRect[0] ||
		0 >= oRect[3] - oRect[1] ||
		0 >= oRes[0] || 0 >= oRes[1] ) {
		logStr( 1, "Warning: Invalid content bounds. Recalculating..." );
		if( !wb.pWarp || VWB_ERROR_NONE != CalculateBounds( wb, oRes[0], oRes[1], oRect[0], oRect[1], oRect[2], oRect[3] ) ) {
			oRect[0] = 0;
			oRect[1] = 0;
			oRect[2] = wb.header.width;
			oRect[3] = wb.header.height;
			oRes[0] = wb.header.width;
			oRes[1] = wb.header.height;
		}
	}

	logStr( 2, "Mapping Info:\n"
			"    Hostname: \"%s\"\n"
			"  Devicename: \"%s\"\n"
			"   SplitInfo: (%i,%i) of (%i,%i)\n"
			"  Resolution: %dx%d\n"
			"    Position: %d,%d\n"
			"  optimalRes: [%d,%d]\n"
			" optimalRect: [%d,%d,%d,%d]\n",
			wb.header.hostname,
			wb.header.name,
			int( wb.header.splitColumnIndex ), int( wb.header.splitRowIndex ), int( wb.header.splitColumns ), int( wb.header.splitRows ),
			wb.header.width, wb.header.height,
			(int)wb.header.offsetX, (int)wb.header.offsetY,
			oRes[0], oRes[1],
			oRect[0], oRect[1], oRect[2], oRect[3]
	);

	if( wb.pWarp ) {
		CalculateBounds( wb, oRes[0], oRes[1], oRect[0], oRect[1], oRect[2], oRect[3] );
		logStr( 2, "Recalculated Bounds:\n"
				"  optimalRes: [%d,%d]\n"
				" optimalRect: [%d,%d,%d,%d]\n",
				oRes[0], oRes[1],
				oRect[0], oRect[1], oRect[2], oRect[3]
		);
	}

	if( calibSplit[0] ) {
		ret = SplitVWF( wb, calibSplit );
		if( ret != VWB_ERROR_NONE && ret != VWB_ERROR_FALSE ) {
			logStr( 0, "ERROR: Failed to split mapping" );
			return ret;
		}
		logStr( 2, "Splitted map [%i,%i,%i,%i], new resolution (%i,%i), new position %d,%d.",
				calibSplit[0], calibSplit[1], calibSplit[2], calibSplit[3],
				wb.header.width, wb.header.height,
				(int)wb.header.offsetX, (int)wb.header.offsetY );
	}


	m_bBorder = 0 != ( FLAG_WARPFILE_HEADER_BORDER & wb.header.flags );
	m_bDynamicEye = 0 != ( FLAG_WARPFILE_HEADER_3D & wb.header.flags );
	if( m_bDynamicEye )
		logStr( 2, "3D data found in mapping. Using DYNAMIC EYE settings.\n" );
	else
		logStr( 2, "2D data found in mapping. Using FIXED EYE settings.\n" );

	m_sizeMap.cx = wbs[calibIndex]->header.width;
	m_sizeMap.cy = wbs[calibIndex]->header.height;
	m_blackBias.x = wbs[calibIndex]->header.blackScale;
	m_blackBias.y = wbs[calibIndex]->header.blackDark;
	m_blackBias.z = wbs[calibIndex]->header.blackBright;
	m_blackBias.w = 1.0f; // the gamma value

	VWB_MAT44f B( trans );
	m_mBaseI = B.Inverted();

	m_bRH = 0 < VWB_VEC3f( B.Z() ).dot( VWB_VEC3f( B.X() ) * VWB_VEC3f( B.Y() ) );
	logStr( 2, "%s-handedness detected.\n", m_bRH ? "right" : "left" );

	ret = PrepareForUse( wb, gamma );
	if( VWB_ERROR_NONE != ret ) {
		return ret;
	}

	if( wb.pWarp && 0 == ( wb.header.flags & FLAG_WARPFILE_HEADER_3D ) ) {
		// in case of 2D mapping
		// calculate bounds, init with opposite extremum
		wb.header.vPartialCnt[0] = 1; // left
		wb.header.vPartialCnt[1] = 1; // top
		wb.header.vPartialCnt[2] = 0; // right
		wb.header.vPartialCnt[3] = 0; // bottom
		VWB_BlendRecord2* pB = wb.pBlend2;
		for( VWB_WarpRecord* pW = wb.pWarp, *pWE = wb.pWarp + ptrdiff_t( m_sizeMap.cx ) * m_sizeMap.cy; pW != pWE; pW++, pB++ ) {
			if( 0.5f <= pW->z && ( 0 < pB->r || 0 < pB->g || 0 < pB->b ) ) {
				if( wb.header.vPartialCnt[0] > pW->x )
					wb.header.vPartialCnt[0] = pW->x;
				if( wb.header.vPartialCnt[1] > pW->y )
					wb.header.vPartialCnt[1] = pW->y;
				if( wb.header.vPartialCnt[2] < pW->x )
					wb.header.vPartialCnt[2] = pW->x;
				if( wb.header.vPartialCnt[3] < pW->y )
					wb.header.vPartialCnt[3] = pW->y;
			}
		}
		if( wb.header.vPartialCnt[2] <= wb.header.vPartialCnt[0] ||
			wb.header.vPartialCnt[3] <= wb.header.vPartialCnt[1] ) {
			wb.header.vPartialCnt[0] = 0;
			wb.header.vPartialCnt[1] = 0;
			wb.header.vPartialCnt[2] = 0;
			wb.header.vPartialCnt[3] = 0;
			logStr( 1, "WARNING: Could not calculate content bounds." );
		} else {
			wb.header.vPartialCnt[4] = ( wb.header.vPartialCnt[2] - wb.header.vPartialCnt[0] ) / ( wb.header.vPartialCnt[3] - wb.header.vPartialCnt[1] );
		}
	}

	// initialize eye provider, in case it is stated
	if( eyeProvider[0] ) {
		char p[MAX_PATH] = { 0 };
		VWBUtil::copy( p, eyeProvider );
	#ifdef WIN32
		m_hmEPP = LoadLibraryA( p );
		if( 0 == m_hmEPP ) {
			logStr( 0, "ERROR: Could not find eye point provider module: \"%s\".\n", p );
			return VWB_ERROR_PARAMETER;
		}
		m_fnEPPCreate = (pfn_CreateEyePointReceiver)GetProcAddress( m_hmEPP, "CreateEyePointReceiver" );
		m_fnEPPGet = (pfn_ReceiveEyePoint)GetProcAddress( m_hmEPP, "ReceiveEyePoint" );
		m_fnEPPRelease = (pfn_DeleteEyePointReceiver)GetProcAddress( m_hmEPP, "DeleteEyePointReceiver" );
		if( NULL == m_fnEPPCreate || NULL == m_fnEPPGet || NULL == m_fnEPPRelease ) {
			logStr( 0, "ERROR: Could not load eye point provider methods from \"%s\".\n", p );
			return VWB_ERROR_GENERIC;
		}
	#else
		//#error implement!
		logStr( 0, "ERROR: Not implemented. Could not load eye point provider methods from \"%s\".\n", p );
		return VWB_ERROR_NOT_IMPLEMENTED;
	#endif //def WIN32

		m_hEPP = m_fnEPPCreate( eyeProviderParam );
		if( NULL == m_hEPP ) {
			logStr( 0, "ERROR: Could not initialize eye point provider \"%s\".\n", p );
			return VWB_ERROR_GENERIC;
		}
		logStr( 1, "Eye point provider \"%s\" successfully initialized.\n", p );
	}

	// do auto calculations
	if( m_bDynamicEye ) {
		if( bAutoView ) { // TODO: add autoview for grids as well
			VWB_ERROR res = AutoView( *wbs[calibIndex] );		// viewport calculation
			if( VWB_ERROR_NONE != res ) {
				logStr( 0, "ERROR: Autoview returns error %d.\n", res );
				optimalRes.cx = wb.header.width;
				optimalRes.cy = wb.header.height;
				optimalRect.left = 0;
				optimalRect.top = 0;
				optimalRect.right = wb.header.width;
				optimalRect.bottom = wb.header.height;
			}
		} else {
			optimalRes.cx = wb.header.width;
			optimalRes.cy = wb.header.height;
			optimalRect.left = 0;
			optimalRect.top = 0;
			optimalRect.right = wb.header.width;
			optimalRect.bottom = wb.header.height;
		}
	} else {
		if( !m_bDP && bAutoView ) { // TODO: add autoview for grids as well
			if( bFixWraparound ) {
				VWB_ERROR res = FixWraparound( *wbs[calibIndex] );
				if( VWB_ERROR_NONE != res )
					logStr( 0, "ERROR: FixWraparound returns error %d.\n", res );
			}

			if( 0 != wb.header.vCntDispPx[4] && 0 != wb.header.vCntDispPx[5] ) {
				optimalRes.cx = (VWB_int)( 1.0f / wb.header.vCntDispPx[4] );
				optimalRes.cy = (VWB_int)( 1.0f / wb.header.vCntDispPx[5] );
			} else {
				optimalRes.cx = wb.header.width;
				optimalRes.cy = wb.header.height;
				logStr( 2, "WARNING: AutoView could not calculate optimal resolution due to missing information in warp file\n" );
			}
			if( 0 != ( wb.header.vPartialCnt[2] - wb.header.vPartialCnt[0] ) &&
				0 != ( wb.header.vPartialCnt[3] - wb.header.vPartialCnt[1] ) ) {
				optimalRect.left = (VWB_int)( wb.header.vPartialCnt[0] * optimalRes.cx );
				optimalRect.top = (VWB_int)( wb.header.vPartialCnt[1] * optimalRes.cy );
				optimalRect.right = (VWB_int)( wb.header.vPartialCnt[2] * optimalRes.cx );
				optimalRect.bottom = (VWB_int)( wb.header.vPartialCnt[3] * optimalRes.cy );
			} else {
				optimalRect.left = 0;
				optimalRect.top = 0;
				optimalRect.right = wb.header.width;
				optimalRect.bottom = wb.header.height;
				logStr( 2, "WARNING: AutoView could not calculate optimal partial rect due to missing information in warp file\n" );
			}
			logStr( 1, "INFO: AutoView for channel [%s] yields:\n"
					"optimalRes=[%d, %d]\n"
					"optimalRect=[%d, %d, %d, %d]\n",
					channel,
					optimalRes.cx, optimalRes.cy,
					optimalRect.left, optimalRect.top, optimalRect.right, optimalRect.bottom );
		} else if( !m_bDP ) { // still no fixwraparound for grids
			if( bFixWraparound ) {
				VWB_ERROR res = FixWraparound( *wbs[calibIndex] );
				if( VWB_ERROR_NONE != res )
					logStr( 0, "ERROR: FixWraparound returns error %d.\n", res );
			}

			if( optimalRes.cx <= 0 || optimalRes.cy <= 0 ) {
				optimalRes.cx = wb.header.width;
				optimalRes.cy = wb.header.height;
			}
			if( optimalRect.right - optimalRect.left <= 0 ||
				optimalRect.bottom - optimalRect.top <= 0 ) {
				// if no rect is given, use the whole image
				// this is the case for 2D mappings, where no partial rects are defined
				optimalRect.left = 0;
				optimalRect.top = 0;
				optimalRect.right = wb.header.width;
				optimalRect.bottom = wb.header.height;
			}
		}
	}

	// translate world to local IG's coordinates
	// translation/rotation to VIOSO's eye point is done via base matrix later

	// we have to inverse rotate and translate
	m_mViewIG = m_bRH ? VWB_MAT44f::R( VWB_VEC3f::ptr( dir ) * VWB_float( M_PI / 180.0 ) ) : VWB_MAT44f::R_LH( VWB_VEC3f::ptr( dir ) * VWB_float( M_PI / 180.0 ) );

	// Remember the sizes of the frustum on screenDist
	m_viewSizes.x = tan( DEG2RAD( fov[0] ) ) * screenDist; // left
	m_viewSizes.y = tan( DEG2RAD( fov[1] ) ) * screenDist; // top
	m_viewSizes.z = tan( DEG2RAD( fov[2] ) ) * screenDist; // right
	m_viewSizes.w = tan( DEG2RAD( fov[3] ) ) * screenDist; // bottom

	if( !VWBLic::init<VWBLicCM>( 5s, 107 ) ) { // 107 is the product code for API/SDK
		logStr( 0, "ERROR: License initialization failed.\n" );
		return VWB_ERROR_LICENSE;
	}
	m_licenseInfo = VWBLic::acquireChannel( pluginId, 10s );
	if( !m_licenseInfo ) {
		logStr( 0, "ERROR: License acquisition failed.\n" );
		return VWB_ERROR_LICENSE;
	}
	return ret;
}

void spliceVec( VWB_double& outX, VWB_double& outY, VWB_double& outZ, VWB_double const& inX, VWB_double const& inY, VWB_double const& inZ, VWB_uint sw ) {
	if( sw & 0x002 )
		outX = inY;
	else if( sw & 0x004 )
		outX = inZ;
	else
		outX = inX;
	if( sw & 0x001 )
		outX *= -1;

	if( sw & 0x020 )
		outY = inX;
	else if( sw & 0x040 )
		outY = inZ;
	else
		outY = inY;
	if( sw & 0x010 )
		outY *= -1;

	if( sw & 0x200 )
		outZ = inX;
	else if( sw & 0x400 )
		outZ = inY;
	else
		outZ = inZ;
	if( sw & 0x100 )
		outZ *= -1;

}

VWB_ERROR VWB_Warper_base::UpdateEye( VWB_float* eye, VWB_float* rot ) {
	// receive the current car parameters
	if( m_hEPP && m_fnEPPGet ) // try eye porvider
	{
		m_fnEPPGet( m_hEPP, &m_ep );
		if( eye ) {
			eye[0] = (float)m_ep.x;
			eye[1] = (float)m_ep.y;
			eye[2] = (float)m_ep.z;
		}
		if( rot ) {
			rot[0] = (float)m_ep.pitch;
			rot[1] = (float)m_ep.yaw;
			rot[2] = (float)m_ep.roll;
		}

		logStr( 4, "INFO: EyePointReceiver %s input for channel [%s]: pos=[%01.3f,%01.3f,%01.3f] dir=[%01.3f,%01.3f,%01.3f].\n",
				eyeProviderParam, channel,
				m_ep.x, m_ep.y, m_ep.z,
				m_ep.pitch, m_ep.yaw, m_ep.roll );
	} else { // use given
		if( eye )
			spliceVec( m_ep.x, m_ep.y, m_ep.z, eye[0], eye[1], eye[2], ( splice >> 16 ) );
		else {
			m_ep.x = 0;
			m_ep.y = 0;
			m_ep.z = 0;
		}

		if( rot )
			spliceVec( m_ep.pitch, m_ep.yaw, m_ep.roll, rot[0], rot[1], rot[2], splice );
		else {
			m_ep.yaw = 0;
			m_ep.pitch = 0;
			m_ep.roll = 0;
		}

		if( NULL != eye && NULL != rot )
			logStr( 4, "INFO: Eyepoint from call for channel [%s]: pos=[%01.3f,%01.3f,%01.3f] dir=[%01.3f,%01.3f,%01.3f].\n",
					channel,
					eye[0], eye[1], eye[2],
					rot[0], rot[1], rot[2] );
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::ReadConfigFile( std::filesystem::path const& configFile, char const* szChannelName ) {
	if( !configFile.empty() ) {
		Default( *this );

		if( szChannelName && szChannelName[0] )
			VWBUtil::copy( channel, szChannelName );
		else
			VWBUtil::copy( channel, "default" );

		if( configFile.has_extension() && ( configFile.extension() == ".xml" || configFile.extension() == ".XML" ) )
			return ReadXMLFile( configFile, szChannelName );
		else
			return ReadIniFile( configFile, szChannelName );
	}
	return VWB_ERROR_FALSE;
}

VWB_ERROR VWB_Warper_base::ReadXMLFile( std::filesystem::path const& configFile, char const* szChannelName )  {
	m_bDP = true; // switch to domeprojection mode
	bAutoView = false; // switch off autoview for domeprojection, as the mesh covers more than the screen, so the target area might be smaller and is already given
	// in domeprojection exports, the channel is an integer, so we try to parse it here
	std::istringstream ss( channel );
	// skip everything, that is not a number, this includes whitespaces and all non-digit characters, so a "Display 01" becomes 1
	while( ss.good() && !isdigit( ss.peek() ) )
		ss.get();
	int iChannel = 0;
	ss >> iChannel;
	// stringify it again
	std::string strChannel = std::to_string( iChannel );
	hMonitor = 0x10000 + (VWB_int)iChannel;
	using namespace tinyxml2;

	calibFile[0] = 0; // no calib file, we fill with maps later

	auto p = MkPath( configFile, ".xml" );
	if( m_bUTF8 )
		VWBUtil::copy( reinterpret_cast<char8_t*>(path), sizeof(path), p.u8string() );
	else
		VWBUtil::copy( path, p.string() );

	m_configPath = p.parent_path();

	// we assume the xml-file to be utf-8 encoded, even though TinyXML2 does not care about or check the encoding
	tinyxml2::XMLDocument doc;

	FILE* f = nullptr;
#ifdef _WIN32
	// On Windows, use _wfopen_s with wide strings
	if( 0 != _wfopen_s(&f, p.c_str(), L"rb" ) )
		f = nullptr;
#else
	// On POSIX systems, path.c_str() returns UTF-8 encoded string
	f = fopen( p.c_str(), "rb" );
#endif
	if( !f ) {
		logStr( 0, "Error open \"%s\"", path );
		return VWB_ERROR_INI_LOAD;
	}
	// strip BOM
	if( fgetc( f ) != 0xEF ||
		fgetc( f ) != 0xBB ||
		fgetc( f ) != 0xBF )
		fseek( f, 0, 0 );

	auto res = doc.LoadFile( f );
	fclose( f );

	if( XML_SUCCESS != res ) {
		logStr( 0, "Error parsing \"%s\"", path );
		return VWB_ERROR_INI_LOAD;
	}

	XMLElement* root = doc.FirstChildElement("dpCorrection");
	if (!root)
	{
		logStr( 0, "Invalid xml. Missing root tag <dpCorrection>");
		return VWB_ERROR_INI_LOAD;
	}

	XMLElement* element = nullptr;
	if( element = root->FirstChildElement( "type" ) ) {
		m_bDynamicEye = ( dpDynamicCorrection == (dpCorrectionType)atoi( element->FirstChild()->Value() ) );
	} 
	if( element = root->FirstChildElement( "gamma" ) ) {
		gamma = (float)atof(element->FirstChild()->Value());
	} 
	if( element = root->FirstChildElement( "input-gamma" ) ) {
		inputGamma = (float)atof(element->FirstChild()->Value());
	} 
	if( element = root->FirstChildElement( "output-gamma" ) ) {
		outputGamma = (float)atof(element->FirstChild()->Value());
	} 
	if( element = root->FirstChildElement( "logFile" ) ) {
		g_logFilePath = MkPath( (char8_t*)element->GetText(), ".log", m_configPath );
	} 
	if( element = root->FirstChildElement( "logLevel" ) ) {
		g_logLevel = atoi( element->FirstChild()->Value() );
	}
#ifdef _DEBUG
	else
		g_logLevel = 3;
#endif
	if( __isFirstInstance ) {
		if( ( element = root->FirstChildElement( "bLogClear" ) ) && atoi( element->FirstChild()->Value() ) ) {
			logClear();
		} else {
			logStr( 1, "--- BEGIN SESSION --- BEGIN SESSION --- BEGIN SESSION --- BEGIN SESSION ---\n" );
		}
		__isFirstInstance = false;
	}
	if( element = root->FirstChildElement( "bTurnWithView" ) ) {
		bTurnWithView = atoi( element->FirstChild()->Value() ) != 0;
	}
	if( element = root->FirstChildElement( "bDoNotBlend" ) ) {
		bDoNotBlend = atoi( element->FirstChild()->Value() ) != 0;
	}
	if( element = root->FirstChildElement( "eyePointProvider" ) ) {
		VWBUtil::copy( eyeProvider, (char const*)MkPath( (char8_t const*)element->GetText(), _libExt, m_configPath ).u8string().c_str() );
	}
	if( element = root->FirstChildElement( "eyePointProviderParam" ) ) {
		VWBUtil::copy( eyeProviderParam, element->GetText() );
	}
	if( element = root->FirstChildElement( "near" ) ) {
		nearDist = (float)atof( element->GetText() );
	}
	if( element = root->FirstChildElement( "far" ) ) {
		farDist = (float)atof( element->GetText() );
	}
	if( element = root->FirstChildElement( "screen" ) ) {
		screenDist = (float)atof( element->GetText() );
	}
	if( element = root->FirstChildElement( "eye" ) ) {
		copyMat( eye, GetMat( element->GetText(), 3, 1 ).value_or( createMat( eye ) ) );
	}
	if( element = root->FirstChildElement( "dir" ) ) {
		copyMat( dir, GetMat( element->GetText(), 3, 1 ).value_or( createMat( dir ) ) );
	}
	if( element = root->FirstChildElement( "fov" ) ) {
		copyMat( fov, GetMat( element->GetText(), 4, 1 ).value_or( createMat( fov ) ) );
	}
	if( element = root->FirstChildElement( "trans" ) ) {
		logStr( 1, "WARN: base and trans matrix can't be set, as it is predefined by domeprojection\n" );
	} else if( element = root->FirstChildElement( "base" ) ) {
		logStr( 1, "WARN: base and trans matrix can't be set, as it is predefined by domeprojection\n" );
	}
	if( element = root->FirstChildElement( "optimalRes" ) ) {
		auto res = GetMat( element->GetText(), 2, 1 );
		if( res ) {
			optimalRes.cx = (int)( res.value()[0] );
			optimalRes.cy = (int)( res.value()[1] );
		}
	}
	if( element = root->FirstChildElement( "optimalRect" ) ) {
		auto res = GetMat( element->GetText(), 4, 1 );
		if( res ) {
			optimalRect.left = (int)( res.value()[0] );
			optimalRect.top = (int)( res.value()[1] );
			optimalRect.right = (int)( res.value()[2] );
			optimalRect.bottom = (int)( res.value()[3] );
		}
	}
	if( element = root->FirstChildElement( "mouseMode" ) ) {
		mouseMode = atoi( element->GetText() );
	}
	if( element = root->FirstChildElement( "autoViewC" ) ) {
		autoViewC = (VWB_float)atof( element->GetText() );
	}
	if( element = root->FirstChildElement( "bAutoView" ) ) {
		bAutoView = atoi( element->GetText() ) != 0;
	}
	if( element = root->FirstChildElement( "port" ) ) {
		port = (unsigned short)atoi( element->GetText() );
	}
	if( element = root->FirstChildElement( "heartBeatPort" ) ) {
		heartBeatPort = (unsigned short)atoi( element->GetText() );
	}
	if( element = root->FirstChildElement( "addr" ) ) {
		VWBUtil::copy( addr, element->GetText() );
	}
	if( element = root->FirstChildElement( "bDoNoBlack" ) ) {
		bDoNoBlack = atoi( element->GetText() ) != 0;
	}
	if( element = root->FirstChildElement( "calibSplit" ) ) {
		copyMatAs( calibSplit, GetMat( element->GetText(), 4, 1, false ).value_or( createMat<VWB_float>( calibSplit ) ) );
	}
	if( element = root->FirstChildElement( "overrideStatemask" ) ) {
		overrideStatemask = atoi( element->GetText() );
	}
	if( element = root->FirstChildElement( "bFixWraparound" ) ) {
		bFixWraparound = atoi( element->GetText() ) != 0;
	}
	if( element = root->FirstChildElement( "D3D12RTVF" ) ) {
		D3D12RTVF = atoi( element->GetText() ) != 0;
	}
	if( element = root->FirstChildElement( "blackScale" ) ) {
		blackScale = (float)atof( element->GetText() );
	}
	if( element = root->FirstChildElement( "blackDarkAdjust" ) ) {
		blackDarkAdjust = (float)atof( element->GetText() );
	}
	if( element = root->FirstChildElement( "blackBrightAdjust" ) ) {
		blackBrightAdjust = (float)atof( element->GetText() );
	}
	if( element = root->FirstChildElement( "bFlipWarpmeshTexcoords" ) ) {
		bFlipWarpmeshTexcoords = atoi( element->GetText() ) != 0;
	}
	if( element = root->FirstChildElement( "bFlipWarpmeshVertices" ) ) {
		bFlipWarpmeshVertices = atoi( element->GetText() ) != 0;
	}
	if( element = root->FirstChildElement( "pluginId" ) ) {
		pluginId = atoi( element->GetText() ) != 0;
	}
	if( element = root->FirstChildElement( "hMonitor" ) ) {
		hMonitor = atoi( element->GetText() );
	}

	if( element = root->FirstChildElement( "debugBreak" ) ) {
	#ifdef WIN32
		MessageBoxA( NULL, "BREAK", "DEBUG", MB_OK );
	#else
	#endif
	}
	for( element = root->FirstChildElement( "channel" ); element; element = element->NextSiblingElement() ) {
		auto attr = element->Attribute( "id");
		if( attr && attr[0] && !strChannel.empty() && strChannel == attr ) {
			constexpr array names{
				"warpmap",
				"blending",
				"blacklevel",
				"whitelevel",
				"secondary-blending",
				"shape",
				"frustum",
				"target",
				"directional-shading"
			};
			vector<char8_t const*> files; files.reserve( names.size() );

			for( auto name : names ) {
				if( attr = element->Attribute( name ) )
					files.push_back( (char8_t const*)attr );
				else
					files.push_back( nullptr );
			}
			if( attr = element->Attribute( "input-gamma" ) ) {
				inputGamma = (float)atof( attr );
			}
			if( attr = element->Attribute( "output-gamma" ) ) {
				outputGamma = (float)atof( attr );
			}
			if( attr = element->Attribute( "monitor" ) ) {
				hMonitor = (float)atof( attr );
			}
			if( attr = element->Attribute( "pluginId" ) ) {
				pluginId = (float)atof( attr );
			}

			if( attr = element->Attribute( "screen" ) ) {
				screenDist = (float)atof( attr );
			}
			if( attr = element->Attribute( "eye" ) ) {
				copyMat( eye, GetMat( attr, 3, 1 ).value_or( createMat( eye ) ) );
			}
			if( attr = element->Attribute( "dir" ) ) {
				copyMat( dir, GetMat( attr, 3, 1 ).value_or( createMat( dir ) ) );
			}
			if( attr = element->Attribute( "fov" ) ) {
				copyMat( fov, GetMat( attr, 4, 1 ).value_or( createMat( fov ) ) );
			}

			ostringstream oss;
			if( files[0] ) {
				auto fp = MkPath( files[0], "", m_configPath );
				if( m_bUTF8 )
					oss << (char const*)fp.u8string().c_str(); // in utf8 mode, we keep utf8
				else
					oss << fp.string(); // in non-utf8 mode, we convert to local codepage
			}
			for( auto fs : span( files.begin() + 1, files.end() ) ) {
				oss << ",";
				if( fs ) {
					auto fp = MkPath( fs, "", m_configPath ); // we construct the filepath from utf8
					if( m_bUTF8 )
						oss << (char const*)fp.u8string().c_str(); // in utf8 mode, we keep utf8
					else
						oss << fp.string(); // in non-utf8 mode, we convert to local codepage
				}
			}
			copy( calibFile, oss.str() );
			calibIndex = 0;
			break;
		}
	}
	if( calibFile[0] == 0 ) {
		logStr( 1, "WARN: this configuration file does not contain a channel named \"%s\"\n", szChannelName );
		return VWB_ERROR_FALSE;
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::ReadIniFile( std::filesystem::path const& configFile, char const* szChannelName ) {
	auto p = MkPath( configFile, ".ini" );
	if( m_bUTF8 )
		VWBUtil::copy( path, (const char*)p.u8string().c_str() );
	else
		VWBUtil::copy( path, p.string() );

	m_configPath = p.parent_path();

	{ // try to open file
		std::ifstream ifs( p );
		if( !ifs.is_open() ) {
			logStr( 0, "Error open \"%s\"", path );
			return VWB_ERROR_INI_LOAD;
		}
	}
	std::string sDef;
	int iDef;
	float fDef;

	if( m_bUTF8 ) {
		auto res = GetIniString( channel, "logFile", p );
		if( !res.has_value() )
			res = GetIniString( "default", "logFile", p );
		if( res.has_value() ) {
			g_logFilePath = (char8_t const*)res.value().c_str();
			if( g_logFilePath.is_relative() )
				g_logFilePath = m_configPath / g_logFilePath;
		}
	} else {
		auto res = GetIniString( channel, "logFile", p );
		if( !res.has_value() )
			res = GetIniString( "default", "logFile", p );
		if( res.has_value() ) {
			g_logFilePath = res.value();
			if( g_logFilePath.is_relative() )
				g_logFilePath = m_configPath / g_logFilePath;
		}
	}
	iDef = GetIniInt( "default", "logLevel", g_logLevel, p );
	g_logLevel = GetIniInt( channel, "logLevel", iDef, p );

	if( __isFirstInstance ) {
		iDef = GetIniInt( "default", "bLogClear", 0, p );
		if( 1 == GetIniInt( channel, "bLogClear", iDef, p ) )
			logClear();
		else
			logStr( 1, "--- BEGIN SESSION --- BEGIN SESSION --- BEGIN SESSION --- BEGIN SESSION ---\n" );
		__isFirstInstance = false;
	}

	iDef = GetIniInt( "default", "bTurnWithView", bTurnWithView, p );
	bTurnWithView = 0 != GetIniInt( channel, "bTurnWithView", iDef, p );

	iDef = GetIniInt( "default", "bDoNotBlend", bDoNotBlend, p );
	bDoNotBlend = 0 != GetIniInt( channel, "bDoNotBlend", iDef, p );

	sDef = GetIniString( "default", "eyePointProvider", p ).value_or( eyeProvider );
	sDef = GetIniString( channel, "eyePointProvider", p ).value_or( sDef );

	if( m_bUTF8 )
		VWBUtil::copy( eyeProvider, (char const*)MkPath( (char8_t const*)sDef.c_str(), _libExt, m_configPath ).u8string().c_str() );
	else
		VWBUtil::copy( eyeProvider, MkPath( sDef, _libExt, m_configPath ).string() );

	sDef = GetIniString( "default", "eyePointProviderParam", p ).value_or( eyeProviderParam );
	VWBUtil::copy( eyeProviderParam, GetIniString( channel, "eyePointProviderParam", p ).value_or(sDef) );

	sDef = GetIniString( "default", "calibFile", p ).value_or( calibFile );
	VWBUtil::copy( calibFile, GetIniString( channel, "calibFile", p ).value_or( sDef ) );

	iDef = GetIniInt( channel, "calibAdapterOrdinal", -1, p );
	iDef = -1 * ( GetIniInt( channel, "calibAdapterOrdinal", iDef, p ) );
	if( iDef < 0 ) {
		calibIndex = iDef;
	} else {
		iDef = GetIniInt( "default", "calibIndex", calibIndex, p );
		calibIndex = GetIniInt( channel, "calibIndex", iDef, p );
	}
	fDef = GetIniFloat( "default", "near", nearDist, p );
	nearDist = GetIniFloat( channel, "near", fDef, p );

	fDef = GetIniFloat( "default", "far", farDist, path );
	farDist = GetIniFloat( channel, "far", fDef, path );

	fDef = GetIniFloat( "default", "screen", screenDist, path );
	screenDist = GetIniFloat( channel, "screen", fDef, path );

	std::vector<VWB_float> vDef( 16, 0.0f );

	vDef = GetIniMat( "default", "eye", 3, 1, p ).value_or( std::vector( eye, eye + 3 ) );
	copyMat( eye, GetIniMat( channel, "eye", 3, 1, p ).value_or( vDef ) );

	vDef = GetIniMat( "default", "dir", 3, 1, p ).value_or( std::vector( dir, dir + 3 ) );
	copyMat( eye, GetIniMat( channel, "dir", 3, 1, p ).value_or( vDef ) );

	vDef = GetIniMat( "default", "fov", 4, 1, p ).value_or( std::vector( fov, fov + 4 ) );
	copyMat( fov, GetIniMat( channel, "fov", 4, 1, p ).value_or( vDef ) );

	auto res = GetIniMat( channel, "trans", 4, 4, p, false );
	if( !res.has_value() ) {
		res = GetIniMat( channel, "base", 4, 4, p, true );
		if( !res.has_value() ) {
			res = GetIniMat( "default", "trans", 4, 4, p, false );
			if( !res.has_value() ) {
				res = GetIniMat( "default", "base", 4, 4, p, true );
			}
		}
	}
	copyMat( trans, res.value_or( std::vector( trans, trans + 16 ) ) );

	iDef = GetIniInt( "default", "mode", -1, p );
	iDef = GetIniInt( channel, "mode", iDef, p );
	if( -1 != iDef ) {
		switch( iDef ) {
		case 1:
			splice = 0x00000122;
			break;
		case 2:
			splice = 0x00000133;
			break;
		default:
			splice = 0;
		}
	} else {
		iDef = GetIniInt( "default", "splice", splice, p );
		splice = GetIniInt( channel, "splice", iDef, p );
	}

	iDef = GetIniInt( "default", "bBicubic", -1, p );
	iDef = GetIniInt( channel, "bBicubic", iDef, p );
	if( -1 == iDef ) {
		iDef = GetIniInt( "default", "bicubic", bBicubic, p );
		bBicubic = 0 != GetIniInt( channel, "bicubic", iDef, p );
	} else
		bBicubic = 0 != iDef;

	iDef = GetIniInt( "default", "bUseGL110", bUseGL110, p );
	bUseGL110 = 0 != GetIniInt( channel, "bUseGL110", iDef, p );

	iDef = GetIniInt( "default", "bPartialInput", bPartialInput, p );
	bPartialInput = 0 != GetIniInt( channel, "bPartialInput", iDef, p );

	iDef = GetIniInt( "default", "mouseMode", mouseMode, p );
	mouseMode = GetIniInt( channel, "mouseMode", iDef, p );

	fDef = GetIniFloat( "default", "autoViewC", 1.0f, p );
	autoViewC = GetIniFloat( channel, "autoViewC", fDef, p );

	iDef = GetIniInt( "default", "bAutoView", bAutoView, p );
	bAutoView = 0 != GetIniInt( channel, "bAutoView", iDef, p );

	vDef = GetIniMat( "default", "optimalRes", 2, 1, p ).value_or( std::vector( { VWB_float(optimalRes.cx), VWB_float(optimalRes.cy) } ) );
	vDef = GetIniMat( channel, "optimalRes", 2, 1, p ).value_or( vDef );
	optimalRes.cx = VWB_int(vDef[0]); optimalRes.cy = VWB_int(vDef[1]);

	vDef = GetIniMat( "default", "optimalRect", 2, 1, p ).value_or( std::vector( { VWB_float(optimalRect.left), VWB_float(optimalRect.top), VWB_float(optimalRect.right), VWB_float(optimalRect.bottom) } ) );
	vDef = GetIniMat( channel, "optimalRect", 2, 1, p ).value_or( vDef );
	optimalRect.left = VWB_int(vDef[0]); optimalRect.top = VWB_int(vDef[1]); optimalRect.right = VWB_int(vDef[2]); optimalRect.bottom = VWB_int(vDef[3]);

	gamma = GetIniFloat( "default", "gamma", gamma, p );
	gamma = GetIniFloat( channel, "gamma", gamma, p );

	iDef = GetIniInt( "default", "port", port, p );
	port = GetIniInt( channel, "port", iDef, p );

	iDef = GetIniInt( "default", "heartBeatPort", heartBeatPort, p );
	heartBeatPort = GetIniInt( channel, "heartBeatPort", iDef, p );

	sDef = GetIniString( "default", "addr", p ).value_or( addr );
	VWBUtil::copy( addr, GetIniString( channel, "addr", p ).value_or( sDef ) );

	iDef = GetIniInt( "default", "bDoNoBlack", bDoNoBlack, p );
	bDoNoBlack = 0 != GetIniInt( channel, "bDoNoBlack", iDef, p );

	vDef = GetIniMat( "default", "calibSplit", 4, 1, p ).value_or( createMat<VWB_float>( calibSplit ) );
	copyMatAs<VWB_word>( calibSplit, GetIniMat( channel, "calibSplit", 4, 1, p ).value_or( vDef ) );

	iDef = GetIniInt( "default", "overrideStatemask", overrideStatemask, p );
	overrideStatemask = GetIniInt( channel, "overrideStatemask", iDef, p );

	iDef = GetIniInt( "default", "bFixWraparound", bFixWraparound, p );
	bFixWraparound = 0 != GetIniInt( channel, "bFixWraparound", iDef, p );

	D3D12RTVF = GetIniInt( "default", "D3D12RTVF", D3D12RTVF, p );
	D3D12RTVF = GetIniInt( channel, "D3D12RTVF", D3D12RTVF, p );

	fDef = GetIniFloat( "default", "blackScale", blackScale, p );
	blackScale = GetIniFloat( channel, "blackScale", fDef, p );

	fDef = GetIniFloat( "default", "inputGamma", inputGamma, p );
	inputGamma = GetIniFloat( channel, "inputGamma", fDef, p );

	fDef = GetIniFloat( "default", "outputGamma", outputGamma, p );
	outputGamma = GetIniFloat( channel, "outputGamma", fDef, p );

	fDef = GetIniFloat( "default", "blackDarkAdjust", blackDarkAdjust, p );
	blackDarkAdjust = GetIniFloat( channel, "blackDarkAdjust", fDef, p );

	fDef = GetIniFloat( "default", "blackBrightAdjust", blackBrightAdjust, p );
	blackBrightAdjust = GetIniFloat( channel, "blackBrightAdjust", fDef, p );

	iDef = GetIniInt( "default", "bFlipWarpmeshTexcoords", bFlipWarpmeshTexcoords, p );
	bFlipWarpmeshTexcoords = 0 != GetIniInt( channel, "bFlipWarpmeshTexcoords", iDef, p );

	iDef = GetIniInt( "default", "bFlipWarpmeshVertices", bFlipWarpmeshVertices, p );
	bFlipWarpmeshTexcoords = 0 != GetIniInt( channel, "bFlipWarpmeshVertices", iDef, p );

	iDef = GetIniInt( "default", "pluginId", bFlipWarpmeshVertices, p );
	pluginId = 0 != GetIniInt( channel, "pluginId", iDef, p );

	iDef = GetIniInt( "default", "hMonitor", bFlipWarpmeshVertices, p );
	hMonitor = 0 != GetIniInt( channel, "hMonitor", iDef, p );

	iDef = GetIniInt( "default", "debugBreak", 0, p );
	if( GetIniInt( channel, "debugBreak", iDef, p ) ) {
	#ifdef WIN32
		MessageBoxA( NULL, "BREAK", "DEBUG", MB_OK );
	#else
	#endif
	}
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::GetScreenplane( VWB_float* pTL, VWB_float* pTR, VWB_float* pBL, VWB_float* pBR ) {
	if( !( pTL && pTR && pBL && pBR ) )
		return VWB_ERROR_PARAMETER;

	pBL[0] = pTL[0] = -m_viewSizes[0];
	pTR[1] = pTL[1] = m_viewSizes[1];
	pBR[0] = pTR[0] = m_viewSizes[2];
	pBR[1] = pBL[1] = -m_viewSizes[3];
	pTL[2] = pTR[2] = pBL[2] = pBR[2] = m_bRH ? -screenDist : screenDist;
	VWB_MAT44f V = m_mViewIG.Inverted();
	VWB_VEC3f::ptr( pTL ) = V * VWB_VEC3f::ptr( pTL );
	VWB_VEC3f::ptr( pTR ) = V * VWB_VEC3f::ptr( pTR );
	VWB_VEC3f::ptr( pBL ) = V * VWB_VEC3f::ptr( pBL );
	VWB_VEC3f::ptr( pBR ) = V * VWB_VEC3f::ptr( pBR );
	return VWB_ERROR_NONE;
}

void VWB_Warper_base::getClip( VWB_VEC3f const& e, VWB_float* pClip ) {
	// x points right and y points up in NDC, same goes for input value "e"
	// z is depending on handedness, LH z points forward, RH rearward
	// screenDist is always positive, so we need to take handedness into account
	VWB_float dd = nearDist / ( screenDist + ( m_bRH ? -e.z : e.z ) );
	pClip[0] = ( m_viewSizes[0] - e.x ) * dd; // left
	pClip[1] = ( m_viewSizes[1] + e.y ) * dd; // top
	pClip[2] = ( m_viewSizes[2] + e.x ) * dd; // right
	pClip[3] = ( m_viewSizes[3] - e.y ) * dd; // bottom
	pClip[4] = nearDist;
	pClip[5] = farDist;
}

/*static*/ void VWB_Warper_base::Default( VWB_Warper& w ) {
	memset( &w, 0, sizeof( VWB_Warper ) );
	VWBUtil::copy( w.calibFile, "vioso.vwf" );
	VWBUtil::copy( w.channel, "channel 1" );
	VWBUtil::copy( w.addr, "0.0.0.0" );
	w.nearDist = 0.125f;
	w.farDist = 20000.0f;
	w.fov[0] = 35;
	w.fov[1] = 30;
	w.fov[2] = 35;
	w.fov[3] = 30;
	w.trans[0] = 1;
	w.trans[5] = 1;
	w.trans[10] = 1;
	w.trans[15] = 1;
	w.screenDist = 1;
	w.autoViewC = 1;
	w.bAutoView = true;
	w.gamma = 1;
	w.D3D12RTVF = 28;
	w.blackScale = 1.0f;
	w.inputGamma = 2.2f;
	w.blackDarkAdjust = 0.5f;
	w.blackBrightAdjust = 0.5f;
	w.outputGamma = 2.2f;
}

/*static*/ VWB_Warper VWB_Warper_base::GetDefaultWarper() {
	VWB_Warper w{};
	VWBUtil::copy( w.calibFile, "vioso.vwf" );
	VWBUtil::copy( w.channel, "channel 1" );
	VWBUtil::copy( w.addr, "0.0.0.0" );
	w.nearDist = 0.125f;
	w.farDist = 20000.0f;
	w.fov[0] = 35;
	w.fov[1] = 30;
	w.fov[2] = 35;
	w.fov[3] = 30;
	w.trans[0] = 1;
	w.trans[5] = 1;
	w.trans[10] = 1;
	w.trans[15] = 1;
	w.screenDist = 1;
	w.autoViewC = 1;
	w.bAutoView = true;
	w.gamma = 1;
	w.D3D12RTVF = 28;
	w.blackScale = 1.0f;
	w.inputGamma = 2.2f;
	w.blackDarkAdjust = 0.5f;
	w.blackBrightAdjust = 0.5f;
	w.outputGamma = 2.2f;
	return w;
}

VWB_ERROR VWB_Warper_base::AutoView( VWB_WarpBlend const& wb ) {
	// test if blend2
	if( !( FLAG_WARPFILE_HEADER_BLENDV2 & wb.header.flags ) )
		return VWB_ERROR_PARAMETER;

	if( !wb.pWarp && !wb.pMesh )
		return VWB_ERROR_FALSE;

	// check base matrix, if left or right handed...
	// global base matrix
	VWB_MAT44d B = VWB_MAT44d( VWB_MAT44f::ptr( trans ) );
	VWB_MAT44d Bi = B.Inverted();

	VWB_VEC3d dx, dy, dtl, dtr, dbl, dbr; // the main coordinate axes to-be.
	if( wb.pWarp ) {
		// find x and y axis from scan
		// find corners
		VWB_WarpRecord* pW = wb.pWarp;
		VWB_BlendRecord2* pB = wb.pBlend2;
		int wh = wb.header.width / 2;
		int hh = wb.header.height / 2;
		VWB_WarpRecord* ptl = NULL, * ptr = NULL, * pbl = NULL, * pbr = NULL; // the extremal corners
		int ltl = INT_MAX, ltr = INT_MIN, lbl = INT_MAX, lbr = INT_MIN; // the extremal corner's distance to projector centre on projector
	#ifdef _DEBUG
		struct ImageCorners {
			struct px {
				int x, y;
			} tl, tr, bl, br;
		} imgCrn;
	#endif
		for( int y = hh - wb.header.height; y != hh; y++ ) {
			for( int x = wh - wb.header.width; x != wh; x++, pW++, pB++ ) {
				if( 1 == pW->w && // map contains valid value
					0 != pB->a && // not masked
					0 != ( pB->r + pB->g + pB->b ) && // not entirely blended black
					( 0 != pW->x || 0 != pW->y || 0 != pW->z ) ) // we skip (0,0,0)
				{
					//int sq = x * x + y * y;
					int d1 = x + y;
					int d2 = x - y;

					// top left
					if( d1 < ltl ) {
						ltl = d1;
						ptl = pW;
					#ifdef _DEBUG
						imgCrn.tl.x = x;
						imgCrn.tl.y = y;
					#endif
					}

					// top right
					if( d2 > ltr ) {
						ltr = d2;
						ptr = pW;
					#ifdef _DEBUG
						imgCrn.tr.x = x;
						imgCrn.tr.y = y;
					#endif
					}

					// bottom left
					if( d2 < lbl ) {
						lbl = d2;
						pbl = pW;
					#ifdef _DEBUG
						imgCrn.bl.x = x;
						imgCrn.bl.y = y;
					#endif
					}

					//bottom right
					if( d1 > lbr ) {
						lbr = d1;
						pbr = pW;
					#ifdef _DEBUG
						imgCrn.br.x = x;
						imgCrn.br.y = y;
					#endif
					}
				}
			}
		}

		//TODO reduce the chance of picking an outlier by using corrected average.
		// but the chance of an outlier is really low, because the data is adjusted by the Calibrator
		if( NULL == ptl || NULL == ptr || NULL == pbl || NULL == pbr ) {
			logStr( 1, "WARINIG: AutoView cannot find corners of screen.\n" );
			return VWB_ERROR_GENERIC;
		}
		logStr( 2, "INFO: AutoView mapping display corners:\n tl(%.4f,%.4f,%.4f)\n tr(%.4f,%.4f,%.4f)\n bl(%.4f,%.4f,%.4f)\n br(%.4f,%.4f,%.4f).\n",
				ptl->x, ptl->y, ptl->z,
				ptr->x, ptr->y, ptr->z,
				pbl->x, pbl->y, pbl->z,
				pbr->x, pbr->y, pbr->z );

		// calculate the display corners in host coordinates
		dtl = VWB_VEC3d( VWB_VEC3f::ptr( (VWB_float*)ptl ) );
		dtr = VWB_VEC3d( VWB_VEC3f::ptr( (VWB_float*)ptr ) );
		dbl = VWB_VEC3d( VWB_VEC3f::ptr( (VWB_float*)pbl ) );
		dbr = VWB_VEC3d( VWB_VEC3f::ptr( (VWB_float*)pbr ) );
	} else {
		if( ( VWB_WARPBLENDMESHEX_HAS_POS | VWB_WARPBLENDMESHEX_HAS_UV ) != ( wb.pMesh->has & ( VWB_WARPBLENDMESHEX_HAS_POS | VWB_WARPBLENDMESHEX_HAS_UV ) ) ) {
			logStr( 1, "WARINIG: AutoView mesh missing position or uv data.\n" );
			return VWB_ERROR_GENERIC;
		}
		// find vertices with extreme u and v values
		VWB_WarpBlendVertexEx* topLeft = nullptr, * topRight = nullptr, * bottomLeft = nullptr, * bottomRight = nullptr;
		for( auto vtx = wb.pMesh->vtx, vtxE = vtx + wb.pMesh->nVtx; vtx != vtxE; vtx++ ) {
			if( isnan( vtx->pos[0] ) )
				continue;
			if( !topLeft || ( vtx->uv[0] + vtx->uv[1] < topLeft->uv[0] + topLeft->uv[1] ) )
				topLeft = vtx;
			if( !topRight || ( vtx->uv[0] - vtx->uv[1] > topRight->uv[0] - topRight->uv[1] ) )
				topRight = vtx;
			if( !bottomLeft || ( vtx->uv[0] - vtx->uv[1] < bottomLeft->uv[0] - bottomLeft->uv[1] ) )
				bottomLeft = vtx;
			if( !bottomRight || ( vtx->uv[0] + vtx->uv[1] > bottomRight->uv[0] + bottomRight->uv[1] ) )
				bottomRight = vtx;
		}
		if( !topLeft || !topRight || !bottomLeft || !bottomRight ) {
			logStr( 1, "WARINIG: AutoView cannot find corners of screen (mesh vertices).\n" );
			return VWB_ERROR_GENERIC;
		} 
		logStr( 2, "INFO: AutoView mapping display corners (from mesh):\n tl(%.4f,%.4f,%.4f)\n tr(%.4f,%.4f,%.4f)\n bl(%.4f,%.4f,%.4f)\n br(%.4f,%.4f,%.4f).\n",
				topLeft->pos[0], topLeft->pos[1], topLeft->pos[2],
				topRight->pos[0], topRight->pos[1], topRight->pos[2],
				bottomLeft->pos[0], bottomLeft->pos[1], bottomLeft->pos[2],
				bottomRight->pos[0], bottomRight->pos[1], bottomRight->pos[2] );

		// calculate the display corners in host coordinates
		dtl = VWB_VEC3d( VWB_VEC3f::ptr( topLeft->pos ) );
		dtr = VWB_VEC3d( VWB_VEC3f::ptr( topRight->pos) );
		dbl = VWB_VEC3d( VWB_VEC3f::ptr( bottomLeft->pos ) );
		dbr = VWB_VEC3d( VWB_VEC3f::ptr( bottomRight->pos ) );
	}

	// calculate corners in IG coordinates
	dtl = Bi * dtl;
	dtr = Bi * dtr;
	dbr = Bi * dbr;
	dbl = Bi * dbl;

	// use corners to calculate screen axes
	dx = dtr + dbr - dtl - dbl;
	dy = dtl + dtr - dbl - dbr;

	// calculate display local base matrix to IG coordinates, this is the rotation from (0,0,0) looking to +/-z (depending on handedness) with y up, to look at the display, as it would be in the virtual world 
	VWB_MAT33d M = VWB_MAT33d::Base( dx, dy ); // this will always create a right-handed base with normalized vectors
	VWB_MAT44d T = VWB_MAT44d( M ) * Bi; // T contains now a transformation from VIOSO coordinates to a display local coordinate system
	{
		VWB_VEC3d c = ( dtl + dtr + dbl + dbr ) / 4; // the centre of the view plane in IG coordinates
		VWB_VEC3d cL = M * c; // centre in display local coordinates
		screenDist = VWB_float( cL.z ); // IG (0,0,0) to plane distance
		if( m_bRH ) // turn screenDist positive, in case we are right-handed, as z points backwards
			screenDist *= -1;
	}

	logStr( 3,
			"View:\n"
			"[%.6f, %.6f, %.6f, %.6f]\n"
			"[%.6f, %.6f, %.6f, %.6f]\n"
			"[%.6f, %.6f, %.6f, %.6f]\n"
			"[%.6f, %.6f, %.6f, %.6f]\n",
			M._11, M._12, M._13, 0.0,
			M._21, M._22, M._23, 0.0,
			M._31, M._32, M._33, 0.0,
			0.0, 0.0, 0.0, 1.0 );

	VWB_VEC3f::ptr( dir ) = VWB_VEC3f( ( m_bRH ? M.GetR() : -M.GetR() ) * ( 180.0 / M_PI ) );

	// caclulate FoVs
	double minDx = std::numeric_limits<float>::max(), minDy = std::numeric_limits<float>::max();  // minimal horizontal and vertical projected distance on render plane, for quality purposes
	double maxL = std::numeric_limits<float>::max(), maxT = -std::numeric_limits<float>::max(), maxR = -std::numeric_limits<float>::max(), maxB = std::numeric_limits<float>::max();  // maximum horizontal and vertical view size, left top right bottom
	double maxEL = std::numeric_limits<float>::max(), maxET = -std::numeric_limits<float>::max(), maxER = -std::numeric_limits<float>::max(), maxEB = std::numeric_limits<float>::max();  // maximum horizontal and vertical view size, left top right bottom throughout moving space

	// now we get the corners of a box in that coordinate system
	VWB_BOXd b( VWB_VEC3d( std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max() ), VWB_VEC3d( -std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(), -std::numeric_limits<double>::max() ) );

	size_t sz = ptrdiff_t( wb.header.width ) * wb.header.height;
	// double l = autoViewC * screenDist / 4; optimized (we devide everything by screenDist and multiply the final result):
	double l = autoViewC / 4;

	if( wb.pWarp ) {
		VWB_WarpRecord* pW = wb.pWarp;
		VWB_BlendRecord2* pB = wb.pBlend2;

		// the distance maps and iterators
		vector<double> distanceMapX( wb.header.width * wb.header.height, 0 );
		auto distX = distanceMapX.begin();
		vector<double> distanceMapY( wb.header.width * wb.header.height, 0 );
		auto distY = distanceMapY.begin();

		// find the clip planes
		vector<VWB_VEC3d> prevL( ptrdiff_t( wb.header.width ) ); // for optimization, we keep the transformed values of the previous line of the mapping
		for( auto& v : prevL )
			v = VWB_VEC3d::O(); // set to all zero
		// start iterating through mapping
		for( VWB_WarpRecord const* pWE = pW + sz; pW != pWE; ) {
			VWB_VEC3d prev = VWB_VEC3d::O(); // for optimization, we also keep previous transformed value
			auto prevT = prevL.begin(); // reset to start
			// start iterating through line
			for( VWB_WarpRecord const* pWLE = pW + wb.header.width; pW != pWLE; pW++, pB++, prevT++, distX++, distY++ ) {
				if( 1 == pW->w && // map contains valid value
					0 != pB->a && // not masked
					0 != ( pB->r + pB->g + pB->b ) && // not entirely blended black
					( 0 != pW->x || 0 != pW->y || 0 != pW->z ) ) // we skip (0,0,0)
				{
					VWB_VEC3d v( pW->x, pW->y, pW->z ); // this is the 3D VIOSO coordinate
					VWB_VEC3d vTT = T * v; // transform to display local coordinate

					// in case we have right-handed coordinate system
					// it will mirror forward direction thus toggle the sign on z to turn it positive
					if( m_bRH )
						vTT.z *= -1;

					// calculate deviation value depending on distance to screen plane
					// if a point sticks out from the screen plane, it can potentionally
					// move out of the minimal frustum, so we need to widen it.
					// using 1 as autoViewC allows for a moving volume of half the
					// screen (plane) distance
					//double dd = abs( l * screenDist / vTT.z - l );
					double dd = abs( l / vTT.z - l / screenDist );

					// project v to the local display's screen plane
					// this yields the a coordinate on a plane at z = screenDist, x right, y up
					//double vx = vTT.x * screenDist / vTT.z;
					//double vy = vTT.y * screenDist / vTT.z; optimized
					double vx = vTT.x / vTT.z;
					double vy = vTT.y / vTT.z;
					b += VWB_VEC3d( vx, vy, 1 ); // add to AABB

					if( maxL > vx ) // left, minimal x (x points right)
						maxL = vx;
					if( maxT < vy ) // top, maximal y (y points up)
						maxT = vy;
					if( maxR < vx ) // right, maximal x
						maxR = vx;
					if( maxB > vy ) // bottom, minimal y
						maxB = vy;

					if( maxEL > vx - dd ) // left, minimal x (x points right)
						maxEL = vx - dd;
					if( maxET < vy + dd ) // top, maximal y (y points up)
						maxET = vy + dd;
					if( maxER < vx + dd ) // right, maximal x
						maxER = vx + dd;
					if( maxEB > vy - dd ) // bottom, minimal y
						maxEB = vy - dd;

					// sum up for x, if prev is valid
					if( 1 == prev.z ) { // previous value valid, go for horizontal
						*distX = abs( vx - prev.x );
					}
					if( 1 == prevT->z ) { // upper value is valid
						*distY = abs( vy - prevT->y );
					}
					*prevT = prev = VWB_VEC3d( vx, vy, 1 );
				} else {
					prev.z = prevT->z = 0; // mark invalid
				}
			}
		}

		if( std::numeric_limits<float>::max() == maxEL || std::numeric_limits<float>::max() == maxET || -std::numeric_limits<float>::max() == maxER || -std::numeric_limits<float>::max() == maxEB ) {
			logStr( 1, "WARNING: AutoView cannot calculate FoVs.\n" );
			return VWB_ERROR_GENERIC;
		}

		{ // calculate optimal resolution

			double sum = 0; // sum of all valid projected mapping distances
			size_t num = 0; // counter

			for( auto x : distanceMapX ) {
				if( 0 < x ) {
					sum += x;
					num++;
				}
			}
			double avgX = sum / num;
			sum = 0; num = 0;
			for( auto y : distanceMapY ) {
				if( 0 < y ) {
					sum += y;
					num++;
				}
			}
			double avgY = sum / num;

			optimalRes.cx = VWB_int( ( maxR - maxL ) / avgX );
			optimalRes.cy = VWB_int( ( maxT - maxB ) / avgY );
			logStr( 1, "INFO: AutoView average resolution is %ix%i.", optimalRes.cx, optimalRes.cy );

			// now we search for lowest, but ignoring values, that would result in more than double resolution
			// 2 * width = ( maxR - maxL ) / capX
			// <=> capX = 0.5 * ( maxR - maxL ) / width
			double minX = std::numeric_limits<double>::max();
			double cap = 0.5 * ( maxR - maxL ) / wb.header.width;
			size_t droppedX = 0;
			for( auto x : distanceMapX ) {
				if( cap <= x ) {
					if( x < minX ) {
						minX = x;
					}
				} else
					droppedX++;
			}

			double minY = std::numeric_limits<double>::max();
			cap = 0.5 * ( maxT - maxB ) / wb.header.height;
			size_t droppedY = 0;
			for( auto y : distanceMapY ) {
				if( cap <= y ) {
					if( y < minY ) {
						minY = y;
					}
				} else
					droppedY++;
			}

			optimalRes.cx = VWB_int( ceil( ( maxR - maxL ) / minX ) );
			optimalRes.cy = VWB_int( ceil( ( maxT - maxB ) / minY ) );
			logStr( 1, "INFO: AutoView optimal resolution coverage is %i%%x%i%%.", 100 - droppedX * 100 / wb.header.height / wb.header.width, 100 - droppedY * 100 / wb.header.height / wb.header.width );
		}
	} else {
		// NOTE: it would be good to refine the mesh along the full-black blend areas before calculating the frustum
		for( auto vtx = wb.pMesh->vtx, vtxE = vtx + wb.pMesh->nVtx; vtx != vtxE; vtx++ ) {
			if( isnan( vtx->pos[0] ) )
				continue;
			auto vTT = T * VWB_VEC3d( vtx->pos[0], vtx->pos[1], vtx->pos[2] ); // transform VIOSO screen coordinate to display local coordinate

			// in case we have right-handed coordinate system
			// it will mirror forward direction thus toggle the sign on z to turn it positive
			if( m_bRH )
				vTT.z *= -1;

			// calculate deviation value depending on distance to screen plane
			// if a point sticks out from the screen plane, it can potentionally
			// move out of the minimal frustum, so we need to widen it.
			// using 1 as autoViewC allows for a moving volume of half the
			// screen (plane) distance
			double dd = abs( l / vTT.z - l / screenDist );

			// project v to the local display's screen plane
			// this yields the a coordinate on a plane at z = screenDist, x right, y up
			//double vx = vTT.x * screenDist / vTT.z;
			//double vy = vTT.y * screenDist / vTT.z; optimized
			double vx = vTT.x / vTT.z;
			double vy = vTT.y / vTT.z;
			b += VWB_VEC3d( vx, vy, 1 ); // add to AABB

			if( maxL > vx ) // left, minimal x (x points right)
				maxL = vx;
			if( maxT < vy ) // top, maximal y (y points up)
				maxT = vy;
			if( maxR < vx ) // right, maximal x
				maxR = vx;
			if( maxB > vy ) // bottom, minimal y
				maxB = vy;

			if( maxEL > vx - dd ) // left, minimal x (x points right)
				maxEL = vx - dd;
			if( maxET < vy + dd ) // top, maximal y (y points up)
				maxET = vy + dd;
			if( maxER < vx + dd ) // right, maximal x
				maxER = vx + dd;
			if( maxEB > vy - dd ) // bottom, minimal y
				maxEB = vy - dd;

		}
		if( std::numeric_limits<float>::max() == maxEL || std::numeric_limits<float>::max() == maxET || -std::numeric_limits<float>::max() == maxER || -std::numeric_limits<float>::max() == maxEB ) {
			logStr( 1, "WARNING: AutoView cannot calculate FoVs.\n" );
			return VWB_ERROR_GENERIC;
		}

		// calculate optimal resolution
		// we do this by comparing the edge lengths in NDC space to the UV lengths of the mesh
		{
			double clip[] = { -maxEL, maxET, maxER, -maxEB, 0.5, 1024 };
			// calculate view/projection matrix
			auto P = m_bRH ? VWB_MAT44d::DXFrustumRH( clip ) : VWB_MAT44d::DXFrustumLH( clip );
			auto PT = P * T;

			float maxUMag = .0f;
			float maxVMag = .0f;

			// we got triangles, so we go over 3 indices each 
			for( auto idx = wb.pMesh->idx, idxE = idx + wb.pMesh->nIdx; idx != idxE; idx += 3 ) {
				// points of the triangle
				VWB_WarpBlendVertexEx* original[]{
					wb.pMesh->vtx + idx[0],
					wb.pMesh->vtx + idx[1],
					wb.pMesh->vtx + idx[2]
				};
				// check for valid positions
				if( isnan( original[0]->pos[0] ) ||	isnan( original[1]->pos[0] ) ||	isnan( original[2]->pos[0] ) )
					continue;

				// project to NDC, operator*() does the matrix multiplication and the homogeneous divide
				VWB_VEC3f ndc[] = {
					VWB_VEC3f( PT * VWB_VEC3d( VWB_VEC3f::ptr( original[0]->pos ) ) ),
					VWB_VEC3f( PT * VWB_VEC3d( VWB_VEC3f::ptr( original[1]->pos ) ) ),
					VWB_VEC3f( PT * VWB_VEC3d( VWB_VEC3f::ptr( original[2]->pos ) ) )
				};

				// calculate pseudo UV
				VWB_float pseudoUV[][2] = {
					{ .5f - ndc[0].x * .5f, .5f + ndc[0].y * .5f },
					{ .5f - ndc[1].x * .5f, .5f + ndc[1].y * .5f },
					{ .5f - ndc[2].x * .5f, .5f + ndc[2].y * .5f },
				};

				constexpr std::array<std::pair<int,int>,3> edges = { std::make_pair( 0, 1 ), std::make_pair( 1, 2 ), std::make_pair( 2, 0 ) };
				for( auto edge : edges ) {
					auto dUOriginal = original[edge.second]->uv[0] - original[edge.first]->uv[0];
					auto dUNDC = pseudoUV[edge.second][0] - pseudoUV[edge.first][0];
					if( !close( dUOriginal, .0f ) ) {
						// the edge is along u direction
						// now we calculate the magnification, this is the ratio of NDC length to UV length
						// a value > 1 means we need more pixels in that direction
						auto mag = abs( dUNDC / dUOriginal );
						if( mag > maxUMag )
							maxUMag = mag;
					}
					auto dVOriginal = original[edge.second]->uv[1] - original[edge.first]->uv[1];
					auto dVNDC = pseudoUV[edge.second][1] - pseudoUV[edge.first][1];
					if( !close( dVOriginal, .0f ) ) {
						// the edge is along v direction ...
						auto mag = abs( dVNDC / dVOriginal );
						if( mag > maxVMag )
							maxVMag = mag;
					}
				}
			}
			optimalRes.cx = VWB_int( ceil( maxUMag * wb.header.width ) );
			optimalRes.cy = VWB_int( ceil( maxVMag * wb.header.height ) );
		}
	}

	// calculate FoVs
	double left = b.vMin.x;
	double top = b.vMax.y;
	double right = b.vMax.x;
	double bottom = b.vMin.y;
	screenDist = abs( screenDist );

	fov[0] = VWB_float( RAD2DEG( atan( -maxEL ) ) );
	fov[1] = VWB_float( RAD2DEG( atan( maxET ) ) );
	fov[2] = VWB_float( RAD2DEG( atan( maxER ) ) );
	fov[3] = VWB_float( RAD2DEG( atan( -maxEB ) ) );

	// the border fit gives a hint about the quality of the screen plane; it shows how much the movement affects the FoVs.
	VWB_VEC4d borderFit( ( left - maxEL ) / ( maxER - maxEL ), ( maxET - top ) / ( maxET - maxEB ), ( maxER - right ) / ( maxER - maxEL ), ( bottom - maxEB ) / ( maxET - maxEB ) );

	optimalRect.left = 0;
	optimalRect.top = 0;
	optimalRect.right = optimalRes.cx;
	optimalRect.bottom = optimalRes.cy;
	logStr( 1, "INFO: AutoView for channel [%s] yields:\n"
			"dir=[%.6f, %.6f, %.6f]\n"
			"fov=[%.6f, %.6f, %.6f, %.6f]\n"
			"screen=%.6f\n"
			"optimalRes=[%d, %d]\n"
			"optimalRect=[%d, %d, %d, %d]\n"
			"handedness=%s\n"
			"borderFitError=[%ipx, %ipx, %ipx, %ipx]\n",
			channel,
			dir[0], dir[1], dir[2],
			fov[0], fov[1], fov[2], fov[3],
			screenDist,
			optimalRes.cx, optimalRes.cy,
			optimalRect.left, optimalRect.top, optimalRect.right, optimalRect.bottom,
			m_bRH ? "right" : "left",
			VWB_int( borderFit.x * wb.header.width ), VWB_int( borderFit.y * wb.header.height ), VWB_int( borderFit.z * wb.header.width ), VWB_int( borderFit.w * wb.header.height ) );
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::FixWraparound( VWB_WarpBlend& wb ) {
	if( !wb.pWarp )
		return VWB_ERROR_PARAMETER;

	VWB_WarpRecord* pW = wb.pWarp;

	// check for wrap-arounds
	typedef struct { long x, y; float xF, yF; float vLTRB[4]; } SSPCoord;

	size_t qCand;
	vector<int32_t> rgn;
	long i, k, kS, kE, l, lS, lE;
	VWB_WarpRecord const* pWR, * pWL;
	float xF, yF, * pMain, * pTest;
	int32_t* pR, * pRS, * pRR, * pRL;
	std::vector<SSPCoord> lCand;
	const long _Width = wb.header.width;
	const long _Height = wb.header.height;
	const long qPx = _Width * _Height;
	const long lastX = _Width - 1;
	const long lastY = _Height - 1;
	VWB_WarpRecord* pWS = pW;
	VWB_WarpRecord const* pWE = pWS + qPx;
	long qRgn = 0;

	try {
		rgn.resize( qPx );
		lCand.resize( (size_t)qPx );
	} catch( ... ) {
		return VWB_ERROR_GENERIC;
	}

	pR = rgn.data();
	pRS = pR;

	do {
		for( ; ( pW < pWE ) && ( ( pW->z <= 0.5f ) || ( *pR > 0 ) ); pW++, pR++ );
		if( pW >= pWE )
			break;

		SSPCoord& c = lCand[0];

		i = (long)( pW - pWS );
		c.y = i / _Width;
		c.x = i % _Width;
		c.xF = pW->x;
		c.yF = pW->y;
		qCand = 1;
		*pR = ++qRgn;

		do {
			SSPCoord& c = lCand[--qCand];

			kS = MAX( c.y - 1, 0 );
			kE = MIN( c.y + 1, lastY );
			lS = MAX( c.x - 1, 0 );
			lE = MIN( c.x + 1, lastX );
			xF = c.xF;
			yF = c.yF;
			i = kS * _Width + lS;

			for( pRL = pRS + i, pWL = pWS + i, k = kS; k <= kE; k++, pWL += _Width, pRL += _Width )
				for( pRR = pRL, pWR = pWL, l = lS; l <= lE; l++, pWR++, pRR++ )
					if( ( pWR->z > 0.5f ) && ( *pRR == 0 ) &&
						( ::fabsf( xF - pWR->x ) <= 0.5f ) && ( ::fabsf( yF - pWR->y ) <= 0.5f )
						) {
						SSPCoord& c = lCand[qCand++];

						c.y = k;
						c.x = l;
						c.xF = pWR->x;
						c.yF = pWR->y;
						*pRR = qRgn;
					}
		} while( qCand );

	} while( 1 );

	if( qRgn == 0 ) {
		logStr( 1, "WARINIG: FixWraparound cannot find any region. Is this an empty map?\n" );
		return VWB_ERROR_GENERIC;
	}

	if( qRgn == 1 ) {
		logStr( 2, "NOTE: FixWraparound found one region. No seam detected - nothing to do.\n" );
		return VWB_ERROR_NONE;
	}

	logStr( 2, "NOTE: FixWraparound found %d regions. Wrapping UVs.\n", qRgn );
	for( i = 1; i <= qRgn; i++ )
		lCand[i].x = 0;

	for( pR = pRS, pW = pWS; pW < pWE; pW++, pR++ )
		if( *pR > 0 ) {
			SSPCoord& c = lCand[*pR];

			if( c.x == 0 ) {
				c.vLTRB[0] = c.vLTRB[2] = pW->x;
				c.vLTRB[1] = c.vLTRB[3] = pW->y;
			} else {
				xF = pW->x;
				if( xF < c.vLTRB[0] )
					c.vLTRB[0] = xF;
				else if( xF > c.vLTRB[2] )
					c.vLTRB[2] = xF;
				yF = pW->y;
				if( yF < c.vLTRB[1] )
					c.vLTRB[1] = yF;
				else if( yF > c.vLTRB[3] )
					c.vLTRB[3] = yF;
			}
			c.x++;
		}

	k = 1;
	kS = lCand[1].x;
	for( i = 2; i <= qRgn; i++ )
		if( lCand[i].x > kS ) {
			k = i;
			kS = lCand[i].x;
		}

	pMain = lCand[k].vLTRB;
	for( i = 1; i <= qRgn; i++ )
		if( i != k ) {
			pTest = lCand[i].vLTRB;

			if( ( pMain[0] - ( pTest[2] - 1.0f ) ) < ( pTest[0] - pMain[2] ) )
				lCand[i].xF = -1.0f;
			else if( ( ( pTest[0] + 1.0f ) - pMain[2] ) < ( pMain[0] - pTest[2] ) )
				lCand[i].xF = 1.0f;
			else
				lCand[i].xF = 0.0f;

			if( ( pMain[1] - ( pTest[3] - 1.0f ) ) < ( pTest[1] - pMain[3] ) )
				lCand[i].yF = -1.0f;
			else if( ( ( pTest[1] + 1.0f ) - pMain[3] ) < ( pMain[1] - pTest[3] ) )
				lCand[i].yF = 1.0f;
			else
				lCand[i].yF = 0.0f;
		} else {
			lCand[i].xF = 0.0f;
			lCand[i].yF = 0.0f;
		}

	for( pR = pRS, pW = pWS; pW < pWE; pW++, pR++ )
		if( *pR > 0 ) {
			SSPCoord& c = lCand[*pR];

			pW->x += c.xF;
			pW->y += c.yF;
		}

	{
		// we need to recalculate...
		float minU = std::numeric_limits<float>::max(), minV = std::numeric_limits<float>::max();
		float maxU = -std::numeric_limits<float>::max(), maxV = -std::numeric_limits<float>::max();
		for( auto const* p = wb.pWarp, *pE = p + wb.header.width * wb.header.height; p != pE; p++ ) {
			if( 0.5f < p->z ) {
				if( minU > p->x )
					minU = p->x;
				if( maxU < p->x )
					maxU = p->x;
				if( minV > p->y )
					minV = p->y;
				if( maxV < p->y )
					maxV = p->y;
			}
		}
		if( minU != std::numeric_limits<float>::max() && minV != std::numeric_limits<float>::max() &&
			maxU != -std::numeric_limits<float>::max() && maxV != -std::numeric_limits<float>::max() ) {
			if( wb.header.vCntDispPx[4] || wb.header.vCntDispPx[4] ) { // if we got a content size, we keep it. Just recalculating the partial rect
				auto fX = 1.0f / ( wb.header.vCntDispPx[4] * wb.header.width * ( maxU - minU ) );
				auto fY = 1.0f / ( wb.header.vCntDispPx[5] * wb.header.height * ( maxV - minV ) );
				wb.header.vPartialCnt[0] = minU * fX;
				wb.header.vPartialCnt[1] = minV * fY;
				wb.header.vPartialCnt[2] = maxU * fX;
				wb.header.vPartialCnt[3] = maxV * fY;
			} else {
				wb.header.vCntDispPx[4] = ( maxU - minU ) / wb.header.width;
				wb.header.vCntDispPx[5] = ( maxV - minV ) / wb.header.height;
				wb.header.vPartialCnt[0] = minU;
				wb.header.vPartialCnt[1] = minV;
				wb.header.vPartialCnt[2] = maxU;
				wb.header.vPartialCnt[3] = maxV;
			}
			logStr( 2, "FixWraparound calculated new content size in wrap mode:\n"
					"  optimalRes: [%d,%d]\n",
					int( 1.0f / wb.header.vCntDispPx[4] ), int( 1.0f / wb.header.vCntDispPx[4] ) );
		} else {
			logStr( 1, "WARINIG: FixWraparound cannot find min-max bound of screen.\n" );
			return VWB_ERROR_GENERIC;
		}
	}

	logStr( 2, "FixWraparound calculated new bounds in wrap mode:\n"
			" optimalRect: [%d,%d,%d,%d]\n",
			int( wb.header.vPartialCnt[0] / wb.header.vCntDispPx[4] ),
			int( wb.header.vPartialCnt[1] / wb.header.vCntDispPx[5] ),
			int( wb.header.vPartialCnt[2] / wb.header.vCntDispPx[4] ),
			int( wb.header.vPartialCnt[3] / wb.header.vCntDispPx[5] )
	);

	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::Render( VWB_param inputTexture, VWB_uint stateMask ) {
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::Render2( VWB_param inputTexture, VWB_uint stateMask ) {
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::getWarpBlend( VWB_WarpBlend const*& wb ) {
	return VWB_ERROR_NOT_IMPLEMENTED;
}

VWB_ERROR VWB_Warper_base::getShaderVPMatrix( VWB_float* pMVP ) {
	if( nullptr == pMVP )
		return VWB_ERROR_PARAMETER;
	memcpy( pMVP, &m_mVP._11, sizeof( m_mVP ) );
	return VWB_ERROR_NONE;
}

VWB_ERROR VWB_Warper_base::getWarpMesh( VWB_int cols, VWB_int rows, VWB_WarpBlendMesh& mesh ) {
	return VWB_ERROR_NOT_IMPLEMENTED;
}
