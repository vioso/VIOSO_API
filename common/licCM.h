#include "lic.h"
#define _CODEMETER_NODEFLIB
#define _CODEMETER_API_STATIC_LIB
#include "CodeMeter.h"

// it gets a bit messy here, as we don't want to let a missing library fail the load of ours
// this is a template to implement late binding of functions defined in a dll/so
// we define the functions as weak symbols, so that if they are not present, we can handle it gracefully
// a not present function will just return an the usual error code
// make sure to define LIC_CREATE_STATIC_FUNCTIONS in one and only one compilation unit to create the symbols
// use this in all of these kind of VWBLic extensions

#ifdef LIC_CREATE_STATIC_FUNCTIONS
#include <atomic>
// create the symbols for used functions
void* _hCMModule = nullptr;
std::atomic_int _cmStaticInstanceCounter = 0;
typedef HCMSysEntry( *fnCmAccess2_t )( CMULONG flCtrl, CMACCESS2* pcmAcc );
typedef int( *fnCmRelease_t )( HCMSysEntry hcmse );
typedef int( *fnCmGetSecureData_t )( HCMSysEntry hcmse, CMSECUREDATA* pcmSecureData, CMENTRYDATA* pcmEntryData );
typedef int( *fnCmDecryptPioData_t )( CMBYTE* pabDest, CMUINT cbDest, CMBYTE* pabPiodkDest, CMUINT cbPiodkDest );
typedef int( *fnCmGetInfo_t )( HCMSysEntry hcmse, CMULONG flCtrl, void* pvDest, CMUINT cbDest );
fnCmAccess2_t pfnCmAccess2 = nullptr;
fnCmRelease_t pfnCmRelease = nullptr;
fnCmGetSecureData_t pfnCmGetSecureData = nullptr;
fnCmDecryptPioData_t pfnCmDecryptPioData = nullptr;
fnCmGetInfo_t pfnCmGetInfo = nullptr;

HCMSysEntry CmAccess2( CMULONG flCtrl, CMACCESS2* pcmAcc ) {
	HCMSysEntry ret{};
	if( pfnCmAccess2 )
		ret = pfnCmAccess2( flCtrl, pcmAcc );
	return ret;
}

int CmRelease( HCMSysEntry hcmse ) {
	int ret = 0;
	if( pfnCmRelease )
		ret = pfnCmRelease( hcmse );
	return ret;
}

int CmGetSecureData( HCMSysEntry hcmse, CMSECUREDATA* pcmSecureData, CMENTRYDATA* pcmEntryData ) {
	int ret = 0;
	if( pfnCmGetSecureData )
		ret = pfnCmGetSecureData( hcmse, pcmSecureData, pcmEntryData );
	return ret;
}

int CmDecryptPioData( CMBYTE* pabDest, CMUINT cbDest, CMBYTE* pabPiodkDest, CMUINT cbPiodkDest ) {
	int ret = 0;
	if( pfnCmDecryptPioData )
		ret = pfnCmDecryptPioData( pabDest, cbDest, pabPiodkDest, cbPiodkDest );
	return ret;
}

int CmGetInfo( HCMSysEntry hcmse, CMULONG flCtrl, void* pvDest, CMUINT cbDest ) {
	int ret = 0;
	if( pfnCmGetInfo )
		ret = pfnCmGetInfo( hcmse, flCtrl, pvDest, cbDest );
	return ret;
}
#else
extern void* _hCMModule;
extern std::atomic_int _cmStaticInstanceCounter;
typedef HCMSysEntry( *fnCmAccess2_t )( CMULONG flCtrl, CMACCESS2* pcmAcc );
typedef int( *fnCmRelease_t )( HCMSysEntry hcmse );
typedef int( *fnCmGetSecureData_t )( HCMSysEntry hcmse, CMSECUREDATA* pcmSecureData, CMENTRYDATA* pcmEntryData );
typedef int( *fnCmDecryptPioData_t )( CMBYTE* pabDest, CMUINT cbDest, CMBYTE* pabPiodkDest, CMUINT cbPiodkDest );
typedef int( *fnCmGetInfo_t )( HCMSysEntry hcmse, CMULONG flCtrl, void* pvDest, CMUINT cbDest );
extern fnCmAccess2_t pfnCmAccess2;
extern fnCmRelease_t pfnCmRelease;
extern fnCmGetSecureData_t pfnCmGetSecureData;
extern fnCmDecryptPioData_t pfnCmDecryptPioData;
extern fnCmGetInfo_t pfnCmGetInfo;
#endif // def CM_CREATE_STATIC_FUNCTIONS

#ifndef LIC_CM_H_INCLUDED
#define LIC_CM_H_INCLUDED
#include <array>

class VWBLicCM : public VWBLic {
	friend class VWBLic;
protected:
	struct LicenseInfoInt : LicenseInfo {
		void* handle = nullptr;
	};

	CMULONG m_productCode;

	VWBLicCM( std::chrono::seconds checkInterval, CMULONG productCode )
		: m_productCode( productCode )
		, VWBLic( checkInterval ) 
	{
		if( 1 == ++_cmStaticInstanceCounter ) {
		#ifdef WIN32
			_hCMModule = ::LoadLibraryA( "WibuCm64.dll" );
		#else
			_hCMModule = ::dlopen( "libwibucmhip.so", RTLD_LAZY );
		#endif
			if( _hCMModule ) {
			#ifdef WIN32
				pfnCmAccess2 = ( fnCmAccess2_t )::GetProcAddress( (HMODULE)_hCMModule, "CmAccess2" );
				pfnCmRelease = ( fnCmRelease_t )::GetProcAddress( (HMODULE)_hCMModule, "CmRelease" );
				pfnCmGetSecureData = ( fnCmGetSecureData_t )::GetProcAddress( (HMODULE)_hCMModule, "CmGetSecureData" );
				pfnCmDecryptPioData = ( fnCmDecryptPioData_t )::GetProcAddress( (HMODULE)_hCMModule, "CmDecryptPioData" );
				pfnCmGetInfo = ( fnCmGetInfo_t )::GetProcAddress( (HMODULE)_hCMModule, "CmGetInfo" );
			#else
				pfnCmAccess2 = ( fnCmAccess2_t )::dlsym( (HMODULE)_hCMModule, "CmAccess2" );
				pfnCmRelease = ( fnCmRelease_t )::dlsym( (HMODULE)_hCMModule, "CmRelease" );
				pfnCmGetSecureData = ( fnCmGetSecureData_t )::dlsym( (HMODULE)_hCMModule, "CmGetSecureData" );
				pfnCmDecryptPioData = ( fnCmDecryptPioData_t )::dlsym( (HMODULE)_hCMModule, "CmDecryptPioData" );
				pfnCmGetInfo = ( fnCmGetInfo_t )::dlsym( (HMODULE)_hCMModule, "CmGetInfo" );
			#endif
			} else {
				// throwing here will not call my destructor and VWBLic::init() fail
				_cmStaticInstanceCounter--;
				throw std::runtime_error( "Could not load CodeMeter library." );
			}
		}
	}

	virtual ~VWBLicCM() {
		if( 0 == --_cmStaticInstanceCounter ) {
		#ifdef WIN32
			::FreeLibrary( (HMODULE)_hCMModule );
		#else
			::dlclose( _hCMModule );
		#endif
			pfnCmAccess2 = nullptr;
			pfnCmRelease = nullptr;
			pfnCmGetSecureData = nullptr;
			pfnCmDecryptPioData = nullptr;
			pfnCmGetInfo = nullptr;
		}
	}

	virtual bool _checkLicense( LicenseInfo& infoExt ) override {
		LicenseInfoInt& info = static_cast<LicenseInfoInt&>( infoExt );
		try {
			// cast to internal type
			// we have an existing handle
			if( info.handle ) {
				// try get current state
				CMBOXINFO bi{};
				auto result = CmGetInfo( info.handle, CM_GEI_BOXINFO, &bi, sizeof( bi ) );
				if( result == 0 ) {
					// this failes, so we release the handle
					CmRelease( info.handle );
					info.handle = 0;
				}
			}
			if( !info.handle ) {
				// no existing handle, we create a new one
				CMACCESS2 access{
					CM_ACCESS_USERLIMIT, // mflCtrl
					102866,              // mulFirmCode
					m_productCode        // mulProductCode
					// rest is zero initialized
				};
				access.mulLicenseQuantity = 1; // we only need one license
				info.handle = CmAccess2( CM_ACCESS_LOCAL_LAN, &access );
			}
			// check again for a valid handle
			if( info.handle ) {

				// set license features
                CMSECUREDATA cmdataFeatures{  
				   { CM_CRYPT_FIRMKEY, 0, 2468 }, // mcmBaseCrypt  ( mflCtrl, mulKeyExtType, mulEncryptionCode )
                   CM_GF_HIDDENDATA,     // musPioType  
                   1,                    // musExtType  
                   { 52, 66, 42, 106, 48, 255, 30, 83, 110, 158, 123, 203, 151, 172, 235, 231 }, // mabPioEncryptionKey  
                };
				// fetch encrypted data
				CMENTRYDATA entrydat{};
				auto data_cnt = CmGetSecureData( info.handle, &cmdataFeatures, &entrydat );

				// decrypt data
				unsigned char dkeyFeatures[]{ 178, 15, 239, 89, 97, 72, 6, 1, 50, 173, 178, 40, 140, 8, 108, 243 };
				CmDecryptPioData( entrydat.mabData, entrydat.mcbData, dkeyFeatures, CM_BLOCK_SIZE );

				// map data to features, NOTE: the map and the loop will be unrolled by the compiler. It is just easier to read this way.
				std::array featureSlotToInfoMap{
					std::pair{ 0, &info.canWarp },
					std::pair{ 1, &info.canBlend },
					std::pair{ 2, &info.canBlacklevel },
					std::pair{ 3, &info.canDirectionalShading }
				};
				for( auto const& m : featureSlotToInfoMap ) {
					if( m.first < data_cnt && entrydat.mabData[m.first] == 0 ) {
						*( m.second ) = true;
					} else {
						*( m.second ) = false;
					}
				}

				// check plugin validity
				CMSECUREDATA cmdataPlugins{
					{ CM_CRYPT_FIRMKEY, 0, 2468 }, // mcmBaseCrypt  ( mflCtrl, mulKeyExtType, mulEncryptionCode )
					CM_GF_HIDDENDATA,     // musPioType  
					2,                    // musExtType  
					{ 11, 85, 198, 60, 106, 91, 139, 163, 82, 145, 89, 38, 69, 134, 86, 225 }, // mabPioEncryptionKey 
				};
				entrydat = {};
				// get encrypted plugin data
				data_cnt = CmGetSecureData( info.handle, &cmdataPlugins, &entrydat );
				// decrypt plugin data
				unsigned char dkeyPlugins[]{ 175, 109, 138, 4, 28, 10, 160, 135, 209, 38, 210, 38, 187, 55, 55, 18 };
				CmDecryptPioData( entrydat.mabData, entrydat.mcbData, dkeyPlugins, CM_BLOCK_SIZE );
				// check validity
				info.isValid = data_cnt > info.pluginId && entrydat.mabData[info.pluginId] == 0;
				return true;
			}
		} catch( ... ) {}
		// failed. Note: this happens only if no license can be found / no codemeter dll present
		return false;
	}

	virtual std::shared_ptr<LicenseInfo> _acquireChannel( LicenseInfo const& lic ) {
		return std::shared_ptr<LicenseInfo>( new LicenseInfoInt( lic ) );
	};

	virtual void _releaseChannel( std::shared_ptr<LicenseInfo>& lcs ) {
		// cast to internal type, as long as lcs is not reseted, the object is valid
		auto pIntern = static_cast<LicenseInfoInt*>( lcs.get() );

		// then release the handle and return the license
		if( pIntern && pIntern->handle ) {
			CmRelease( pIntern->handle );
			pIntern->handle = 0;
		}
	};

};
#endif // ndef LIC_CM_H_INCLUDED