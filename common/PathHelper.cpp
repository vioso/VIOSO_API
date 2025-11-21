// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "PathHelper.h"
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>
#else
#include <dlfcn.h>
#include <stdlib.h>
#include <regex>
#include <cstdlib>
#endif // def WIN32

#include <filesystem>
#include <fstream>
#include <string>
#include <optional>
#include <string_view>
#include <charconv>

#include "StringConversions.h"

using namespace std;
namespace fs = std::filesystem;

fs::path MkPath( fs::path const& path, char const* ext, fs::path const& basePath )
{
	if ( path.empty() )
		return path;

	fs::path fsPath( path );

	// expand environment variables
	#ifdef WIN32
	{
		fs::path::string_type s(MAX_PATH, 0);
		if( ::ExpandEnvironmentStringsW( fsPath.c_str(), s.data(), MAX_PATH) )
			fsPath = s.c_str();
	}
	#else
	{
        static const regex env_re{ R"--(\$\{([^}]+)\})--" };
		std::smatch match;
		std::string s = fsPath.string();
		while( std::regex_search( s, match, env_re ) ) {
			auto const from = match[0];
			auto const var_name = match[1].str().c_str();
			s.replace( from.first, from.second, std::getenv( var_name ) );
		}
		fsPath = s;
	}
	#endif //def WIN32

	// add module path, as we want to be relative to the dll/so or executable
	if( fsPath.is_relative() )
	{
		if( !basePath.empty() && basePath.is_absolute() ) {
			fsPath = basePath / fsPath;
		} else {
			fs::path::string_type sModPath( MAX_PATH, 0 );
		#ifdef WIN32
			MEMORY_BASIC_INFORMATION mbi{};
			if( VirtualQuery(reinterpret_cast<void*>(&MkPath), &mbi, sizeof(mbi)) && ::GetModuleFileNameW( static_cast<HMODULE>(mbi.AllocationBase), sModPath.data(), MAX_PATH ) ) {
				sModPath = fs::path( sModPath.c_str() ).parent_path(); // using c_str() operator also cuts off all zeros
		#else
			Dl_info dl_info{};
			if( dladdr( (void*)&MkPath, &dl_info ) && dl_info.dli_fname[0] ) {
				sModPath = fs::path( dl_info.dli_fname ).parent_path();
		#endif //def WIN32
			} else {
				sModPath = fs::current_path();
			}
			fsPath = fs::path( sModPath ) / fsPath;
		}
	}

	// update extension
	if( ext && ext[0] )	{
		if( !fsPath.has_extension() )
			fsPath.replace_extension( ext );
	}

    // make nice
    try{
        fsPath = weakly_canonical(fsPath);
	}
	catch( std::exception& e ) { ( e ); }

	return fsPath;
}

// Return string value from INI, or std::nullopt if not found
std::optional<std::string> GetIniString(std::string_view section,
                                         std::string_view key,
                                         std::filesystem::path const& configFile)
{
    using namespace VWBUtil;
    std::ifstream fs(configFile);
    if (!fs.is_open())
        return std::nullopt;

    // strip BOM
    if( fs.peek() == 0xEF ) {
        char bom[3]{};
        fs.read(bom, 3);
        if( !( (unsigned char)( bom[1] ) == 0xBB &&
               (unsigned char)( bom[2] ) == 0xBF ) ) {
            // No BOM, rewind
            fs.seekg( 0 );
        }
    }

    bool inSection = false;
    std::string line;

    while (std::getline(fs, line)) {
        // trim
        line = trim( line );

        if (line.empty() || line.front() == ';' || line.front() == '#')
            continue;

        if (!inSection) {
            if (line.front() == '[') {
                auto posE = line.find(']');
                if (posE != std::string::npos) {
                    auto secName = trim(std::string_view(line).substr(1, posE - 1));
                    if (secName == section)
                        inSection = true;
                }
            }
        } else {
            if (line.front() == '[') {
                return std::nullopt; // next section, so no find
            } else {
                auto posEq = line.find('=');
                if (posEq != std::string::npos) {
                    auto keyName = trim(std::string_view(line).substr(0, posEq));
                    if (keyName == key) {
                        auto value = trim(std::string_view(line).substr(posEq + 1));
                        return std::string(value);
                    }
                }
            }
        }
    }
    return std::nullopt;
}

// --- typed accessors ---

VWB_int GetIniInt(std::string_view section,
                   std::string_view key,
                   VWB_int iDefault,
                   std::filesystem::path const& configFile)
{
    if (auto val = GetIniString(section, key, configFile)) {
        VWB_int result{};
        auto [ptr, ec] = std::from_chars(val->data(), val->data() + val->size(), result);
        if (ec == std::errc{}) return result;
    }
    return iDefault;
}

VWB_float GetIniFloat(std::string_view section,
                       std::string_view key,
                       VWB_float fDefault,
                       std::filesystem::path const& configFile)
{
    if (auto val = GetIniString(section, key, configFile)) {
        VWB_float result{};
        auto [ptr, ec] = std::from_chars(val->data(), val->data() + val->size(), result);
        if (ec == std::errc{}) return result;
    }
    return fDefault;
}

std::optional<std::vector<VWB_float>> GetMat( std::string const& val, int dimX, int dimY, bool bTranspose ) {
    if( val.empty() )
        throw std::invalid_argument( "GetMat: input must not be empty" );
    if( dimX <= 0 || dimY <= 0)
		throw std::invalid_argument( "GetMat: dimX and dimY must be > 0" );

    std::vector<VWB_float> result(dimX * dimY, 0.0f);

    std::string_view sv( val );
    if (!sv.empty() && sv.front() == '[') {
        sv.remove_prefix(1);

        std::vector<VWB_float> values;
        values.reserve(dimX * dimY);

        while (!sv.empty() && sv.front() != ']') {
            auto nextDelim = sv.find_first_of(",;]");
            auto token = VWBUtil::trim(sv.substr(0, nextDelim));

            if (!token.empty()) {
                float v{};
                auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), v);
                if (ec == std::errc{}) {
                    values.push_back(v);
                } else {
                    break;
                }
            }

            if (nextDelim == std::string_view::npos || sv[nextDelim] == ']')
                break;
            sv.remove_prefix(nextDelim + 1);
        }

        if (values.size() == static_cast<size_t>(dimX * dimY)) {
            if (bTranspose) {
                for (int y = 0; y < dimY; ++y)
                    for (int x = 0; x < dimX; ++x)
                        result[y + x * dimY] = values[y * dimX + x];
            } else {
                std::copy(values.begin(), values.end(), result.begin());
            }
            return result;
        }
    }

    return std::nullopt;
}

std::optional<std::vector<VWB_float>> GetIniMat( std::string_view section,
                                                 std::string_view key,
                                                 int dimX,
                                                 int dimY,
                                                 std::filesystem::path const& configFile,
                                                 bool bTranspose ) {
    if( auto val = GetIniString( section, key, configFile ) ) {
		return GetMat( val.value(), dimX, dimY, bTranspose );
    }
	return std::nullopt;
}