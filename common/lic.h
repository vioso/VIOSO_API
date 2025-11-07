#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>

class VWBLic {
public:
	struct LicenseInfo {
		int pluginId;
		std::chrono::seconds keepValidFor;
		std::chrono::steady_clock::time_point lastChecked;
		bool isValid;
		bool canWarp;
		bool canBlend;
		bool canBlacklevel;
		bool canDirectionalShading;
	};

private:
	std::mutex m_listMutex;
	std::mutex m_checkingMutex;
	std::vector <std::shared_ptr<LicenseInfo>> m_acquiredLicenses;
	std::chrono::seconds m_checkInterval = std::chrono::seconds( 5 );
	bool m_isRunning = false;
	std::thread m_checkThread; // must be last for proper construction
	
	static std::unique_ptr<VWBLic>& instancePtr() {
		static std::unique_ptr<VWBLic> pInstance;
		return pInstance;
	}

protected:
	static VWBLic& instance() {
		return *instancePtr();
	}

	VWBLic( std::chrono::seconds checkInterval ) 
		: m_checkThread( std::thread( [](VWBLic* that) {
			using namespace std;
			using namespace std::chrono_literals;
			that->m_isRunning = true;
			while( that->m_isRunning ) {
				vector<shared_ptr<LicenseInfo>> shadow;
				{
					auto lock = lock_guard( that->m_listMutex );
					shadow = that->m_acquiredLicenses;
				}
				{
					auto lock = lock_guard( that->m_checkingMutex );
					for( auto& lic : shadow ) {
						if( lic ) {
							if( that->_checkLicense( *lic ) )
								lic->lastChecked = std::chrono::steady_clock::now();
							else {
								if( std::chrono::steady_clock::now() - lic->lastChecked >= lic->keepValidFor ) {
									lic->isValid = false;
								}
							}
						}
					}
				}
				this_thread::sleep_for( that->m_checkInterval );
			}
		}, this ) )
	{}
	VWBLic( VWBLic const& ) = delete;
	VWBLic& operator=( VWBLic const& ) = delete;

	void addLicenseInfo( std::shared_ptr<LicenseInfo>& lic ) {
		auto lock = std::lock_guard<std::mutex>( m_listMutex );
		m_acquiredLicenses.push_back( lic );
	}

    void removeLicenseInfo( std::shared_ptr<LicenseInfo> const& lic ) {
		std::scoped_lock lock( m_listMutex, m_checkingMutex ); // Lock both mutexes at once
		auto it = std::find( m_acquiredLicenses.begin(), m_acquiredLicenses.end(), lic );
		if( it != m_acquiredLicenses.end() ) {
			m_acquiredLicenses.erase( it );
		}
    }

	// protected virtuals
	virtual bool _checkLicense( LicenseInfo& info ) = 0;
	virtual std::shared_ptr<LicenseInfo> _acquireChannel( LicenseInfo const& lic ) = 0;
	virtual void _releaseChannel( std::shared_ptr<LicenseInfo>& lcs ) = 0;
public:
	template<typename T, typename ...ARGS>
	static bool init( ARGS&& ...args ) {
		if( !instancePtr() ) {
			try {
				instancePtr().reset( new T( std::forward<ARGS>( args )... ) );
			} catch( ... ) {
				return false;
			}
		}
		return true;
	}

	static std::shared_ptr< const LicenseInfo > acquireChannel( int pluginId, std::chrono::seconds keepValidFor = std::chrono::seconds( 10 ) ) {
		auto ret = instance()._acquireChannel( LicenseInfo{ pluginId, keepValidFor, std::chrono::steady_clock::now(), true, true, true, true, true } ); // insert a valid license by default
		instance()._checkLicense( *ret ); // do an immediate check
		instance().addLicenseInfo( ret );
		return ret;
	}

	static void releaseChannel( std::shared_ptr<LicenseInfo>& lic ) {
		if( !lic )
			return;

		// remove from list, removeLicenseInfo locks both mutexes, so lic will never be checked again
		instance().removeLicenseInfo( lic );

		// call cleanup
		instance()._releaseChannel( lic );

		// finally reset the shared ptr to release the object
		lic.reset();
	}

	VWBLic( VWBLic&& ) = default;
	VWBLic& operator=( VWBLic&& ) = default;
	virtual ~VWBLic() {
		m_isRunning = false;
		if( m_checkThread.joinable() ) {
			m_checkThread.join();
		}
	}

};
