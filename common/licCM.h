#pragma once

#include "lic.h"
#include "CodeMeter.h"
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
		, VWBLic( checkInterval ) {}

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
		// failed
		return false;
	}

	virtual std::shared_ptr<LicenseInfo> _acquireChannel( LicenseInfo const& lic ) {
		return std::shared_ptr<LicenseInfo>( new LicenseInfoInt( lic ) );
	};

	virtual void _releaseChannel( std::shared_ptr<LicenseInfo>& lcs ) {
		// cast to internal type, as long as lcs is not rested, the object is valid
		auto& pIntern = static_cast<LicenseInfoInt&>( *lcs );

		// then release the handle and return the license
		if( pIntern.handle ) {
			CmRelease( pIntern.handle );
			pIntern.handle = 0;
		}
	};

};