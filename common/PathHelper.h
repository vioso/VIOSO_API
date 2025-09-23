// VIOSO API
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#ifndef VWB_PATH_HELPER_HPP
#define VWB_PATH_HELPER_HPP

#include "../Include/VWBTypes.h"
#include <string>
#include <string_view>
#include <filesystem>
#include <optional>
#include <algorithm>
#include <cmath>
/// @brief creates a full canonical path from a filename
/// @description if it does not start with a / or a drive letter (no root path)
/// it is set relative to the module currently set in g_hModule
/// @param [IN|OUT] path the input path or file name
/// @param [IN_OPT] ext an extension to add, set to 
/// @param [IN_OPT] basePath the relative path to look 
/// @return the value of path, containing the formatted and canonical path,
/// NULL in case of an error
/// @remarks
/// removes leading and trailing whitespaces
/// removes leading single and double quotes and their counterparts
/// expands environment strings
/// removes double directory separators
/// removes dir/../
/// adds .ext in case \b ext isn't null and the extension isn't already present
std::filesystem::path MkPath(std::filesystem::path const& path, char const* ext = NULL, std::filesystem::path const& basePath = "" );


/// @brief reads a string from a Windows ini style text file
/// @param [IN] szSection the section to search for a value,
/// a section start mark is a single line with a name in 
/// square brackets: [name]
/// @param [IN] key a key name
/// @param [IN] szDefault the default value
/// @param [IN] configFile the path to the ini file
/// @return true in case the key was found in given section
/// and a value was found
/// @remarks
/// if a section marker is present more than once in a file,
/// only the first section is browsed for keys
/// all \b szSection and \szKey must only contain upper and lower
/// case letters, names are case sensitive
/// the equal sign must follow directly in case of a 
/// key=value entry
/// in case a key is not found in the section, a optional null is returned
std::optional<std::string> GetIniString( std::string_view section, std::string_view key, std::filesystem::path const& configFile );

/// @brief reads an integer from a Windows ini style text file
/// @param [IN] szSection the section to search for a value,
/// a section start mark is a single line with a name in 
/// square brackets: [name]
/// @param [IN] key a key name
/// @param [IN] iDefault the default value
/// @param [IN] configFile the path to the ini file
/// @return the read value or the value given in iDefault
VWB_int
GetIniInt(std::string_view section, std::string_view key, VWB_int iDefault, std::filesystem::path const& configFile);

/// @brief reads a float from a Windows ini style text file
/// @param [IN] szSection the section to search for a value,
/// a section start mark is a single line with a name in 
/// square brackets: [name]
/// @param [IN] key a key name
/// @param [IN] fDefault the default value
/// @param [IN] configFile the path to the ini file
/// @return the read value or the value given in fDefault
VWB_float
GetIniFloat(std::string_view section, std::string_view key, VWB_float fDefault, std::filesystem::path const& configFile);

/// @brief reads a float vector or matrix from a Windows ini style text file
/// @param [IN] szSection the section to search for a value,
/// a section start mark is a single line with a name in 
/// square brackets: [name]
/// @param [IN] key a key name
/// @param [IN] iDefault the default value
/// @param [IN] configFile the path to the ini file
/// @param [IN|OPT] transpose matrix; this will transpose while reading from file, default must also be transposed in case
/// @return f in case of success; this includes , NULL otherwise
/// if fDefault is NULL, an empty matrix (all 0.0f) is returned
std::optional<std::vector<VWB_float>> GetIniMat( std::string_view section, std::string_view key, int dimX, int dimY, std::filesystem::path const& configFile, bool bTranspose = false );
std::optional<std::vector<VWB_float>> GetMat( std::string const& val, int dimX, int dimY, bool bTranspose = false );

/// @brief copy a vector to a POD array, it copies sz or less elements
/// @param dst the destination array
/// @param sz the maximum size of the array
/// @param src the source vector
/// @return the destination array
template< typename T > T* copyMat( T* dst, size_t sz, std::vector<T> const& src ) {
    if( src.size() < sz )
        sz = src.size();
    std::copy_n( src.begin(), sz, dst );
    return dst;
}
template< typename T, size_t _S > T* copyMat( T( &dst )[_S], std::vector<T> const& src ) { return copyMat<T>( dst, _S, src ); }

template< typename T1, typename T2 > T1* copyMatAs( T1* dst, size_t sz, std::vector<T2> const& src ) {
    if( src.size() < sz )
        sz = src.size();
    std::transform( src.begin(), src.begin() + sz, dst, [](T2 const& val){ return static_cast<T1>( val ); } );
    return dst;
}
template< typename T1, typename T2, size_t _S > T1* copyMatAs( T1( &dst )[_S], std::vector<T2> const& src ) { return copyMatAs<T1,T2>( dst, _S, src ); }

template< typename T1, typename T2 > std::vector<T1> createMat( T2 const* src, size_t sz ) {
    return std::vector<T1>( src, src + sz );
}
template< typename T1, typename T2, size_t _S > std::vector<T1> createMat( T2( &src )[_S] ) {
    return createMat<T1, T2>( src, _S );
}
template< typename T1, size_t _S > std::vector<T1> createMat( T1( &src )[_S] ) {
    return createMat<T1, T1>( src, _S );
}

constexpr size_t intSqrt(size_t n) {
    size_t r = 0;
    while ((r + 1) * (r + 1) <= n) {
        ++r;
    }
    return r;
}

template< typename T2, typename T1 = T2 > std::vector<T1> createMatTrans( T2 const* src, size_t dimX, size_t dimY ) {
	std::vector<T1> ret( dimX * dimY );
	auto it = ret.begin();
	ptrdiff_t step = -ptrdiff_t( dimX ) * dimY + 1;
    // fill transposed
	for( auto el = src, elE = src + dimX; el != elE; el += step ) {
        for( auto elLE = el + dimX * dimY; el != elLE; el+= dimX ) {
			*(it++) = *el;
		}
	}
    return ret;
}
// the second parameter is optional, if not given, a square matrix is assumed
template< typename T2, typename T1 = T2, size_t _S > std::vector<T1> createMatTrans( T2( &src )[_S] ) {
    // If caller did not override dimX, check that _S is a perfect square
	constexpr size_t dim = intSqrt( _S );
    static_assert( dim * dim == _S, "Default dimX assumes a square matrix (_S must be a perfect square).");
    return createMatTrans<T2, T1>( src, dim, dim );
}
template< typename T2, typename T1 = T2, size_t _S > std::vector<T1> createMatTrans( T2( &src )[_S], size_t dimX ) {
    return createMatTrans<T2, T1>( src, dimX, _S / dimX );
}


#endif //ndef VWB_PATH_HELPER_HPP


