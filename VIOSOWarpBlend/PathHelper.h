#ifndef VWB_PATH_HELPER_HPP
#define VWB_PATH_HELPER_HPP

#include "../Include/VWBTypes.h"
#include <string>
#include <filesystem>
/// @brief creates a full canonical path from a filename
/// @description if it does not start with a / or a drive letter (no root path)
/// it is set relative to the module currently set in g_hModule
/// @param [IN|OUT] path the input path or file name
/// @param [IN_OPT] ext an extension to add, set to 
/// @return the value of path, containing the formatted and canonical path,
/// NULL in case of an error
/// @remarks
/// removes leading and trailing whitespaces
/// removes leading single and double quotes and their counterparts
/// expands environment strings
/// removes double directory separators
/// removes dir/../
/// adds .ext in case \b ext isn't null and the extension isn't already present
std::filesystem::path MkPath(std::filesystem::path const& path, char const* ext = NULL);


/// @brief reads a string from a Windows ini style text file
/// @param [IN] szSection the section to search for a value,
/// a section start mark is a single line with a name in 
/// square brackets: [name]
/// @param [IN] key a key name
/// @param [IN] szDefault the default value
/// @param [OUT] s the returned string
/// @param [IN] sz the number of characters, the string can hold
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
/// in case a key is not found in the section, the default value is copied,
/// if szDefault is NULL, an empty string is returned
bool 
GetIniString(char const* szSection, char const* szKey, char const* szDefault, char* s, VWB_uint sz, std::filesystem::path const& configFile );
template<size_t sz>
bool
GetIniString(char const* szSection, char const* szKey, char const* szDefault, char(&s)[sz], std::filesystem::path const& configFile )
{ return GetIniString(szSection, szKey, szDefault, s, sz, configFile); }

/// @brief reads an integer from a Windows ini style text file
/// @param [IN] szSection the section to search for a value,
/// a section start mark is a single line with a name in 
/// square brackets: [name]
/// @param [IN] key a key name
/// @param [IN] iDefault the default value
/// @param [IN] configFile the path to the ini file
/// @return the read value or the value given in iDefault
VWB_int
GetIniInt(char const* szSection, char const* szKey, VWB_int iDefault, std::filesystem::path const& configFile);

/// @brief reads a float from a Windows ini style text file
/// @param [IN] szSection the section to search for a value,
/// a section start mark is a single line with a name in 
/// square brackets: [name]
/// @param [IN] key a key name
/// @param [IN] fDefault the default value
/// @param [IN] configFile the path to the ini file
/// @return the read value or the value given in fDefault
VWB_float
GetIniFloat(char const* szSection, char const* szKey, VWB_float fDefault, std::filesystem::path const& configFile);

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
VWB_float*
GetIniMat(char const* szSection, char const* szKey, int dimX, int dimY, VWB_float const* fDefault, VWB_float* f, std::filesystem::path const& configFile, bool bTranspose = false );
template< int sz>
VWB_float*
GetIniMat(char const* szSection, char const* szKey, VWB_float const* fDefault, VWB_float(&f)[sz], std::filesystem::path const& configFile, bool bTranspose = false )
{ return GetIniMat(szSection, szKey, sz, 1, fDefault, f, configFile, bTranspose); }

#endif //ndef VWB_PATH_HELPER_HPP


