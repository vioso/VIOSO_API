#include "Platform.h"
#include "PathHelper.h"

#ifndef WIN32
#include <dlfcn.h>
#include <stdlib.h>
#include <regex>
#include <cstdlib>
#endif // ndef WIN32

#include <filesystem>
#include <fstream>
#include <string>

using namespace std;
using namespace std::filesystem;

char* MkPath( char* szPath, size_t nMaxPath, char const* ext )
{
	if( NULL == szPath || 0 == szPath[0] )
		return NULL;

	char* pp = szPath;

	// remove leading white spaces
    while( ' ' == *pp || '\t' == *pp || '\r' == *pp || '\n' == *pp || '\v' == *pp || '\f' == *pp )
		pp++;

	// remove trailing white spaces
	char* lnwsc = pp + strlen( pp );
	while( --lnwsc > pp )
        if( ' ' == *lnwsc || '\t' == *lnwsc || '\r' == *lnwsc || '\n' == *lnwsc || '\v' == *lnwsc || '\f' == *lnwsc )
			*lnwsc = 0;
		else
			break;

	// remove quotes
    lnwsc-= - 1;
	if( lnwsc > pp )
	{
		if( '"' == *pp && '"' == *lnwsc )
		{
			pp++;
			*lnwsc = 0;
		}
		else if( '\'' == *pp && '\'' == *lnwsc )
		{
			pp++;
			*lnwsc = 0;
		}
	}

	if( 0 == pp[0] )
		return NULL;

	path fsPath( pp );

	// expand environment variables
	#ifdef WIN32
	{
		wchar_t* path2 = new wchar_t[nMaxPath];
		DWORD max = nMaxPath > MAXDWORD ? MAXDWORD : (DWORD)nMaxPath;
		if( ::ExpandEnvironmentStringsW( fsPath.c_str(), path2, max ) )
			fsPath = path2;
		delete[] path2;
	}
	#else
	{
        static const regex env_re{ R"--(\$\{([^}]+)\})--" };
		std::smatch match;
		path::string_type s = fsPath;
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
		path::string_type sModPath( MAX_PATH, path::string_type::value_type( 0 ) );
    #ifdef WIN32
		if( ::GetModuleFileNameW( g_hModDll, sModPath.data(), MAX_PATH ) )
		{
			sModPath = path( sModPath.c_str() ).parent_path(); // using c_str() operator also cuts off all zeros
    #else
		Dl_info dl_info;
		if( dladdr( ( void* )MkPath, &dl_info ) && dl_info.dli_fname[0] )
		{
			sModPath = path( dl_info.dli_fname ).parent_path();
    #endif //def WIN32
		}
		else
		{
			sModPath = current_path();
		}
		fsPath = path( sModPath ) / fsPath;
	}

	// update extension
	if( ext && ext[0] && !fsPath.has_extension() )
	{
		fsPath.replace_extension( ext );
	}

    // make nice
    try{
        fsPath = weakly_canonical(fsPath);
	}
	catch( std::exception& e ) { ( e ); }

    strcpy_s( szPath, nMaxPath , (char const*)fsPath.u8string().c_str() );
	return szPath;
}

std::string MkPath(std::string const pathIn, char const* ext)
{
	std::string pathOut;
	pathOut = pathIn;
	pathOut.resize(MAX_PATH);
	MkPath(pathOut.data(), pathOut.size(), ext );
	pathOut.erase(pathOut.find('\0'));
	return pathOut;
}

bool GetIniString(char const* szSection, char const* szKey, char const* szDefault, char* s, VWB_uint sz, char const* szConfigFile)
{
	bool bRet = false;
	bool bInChannel = false;
	if (szDefault)
		strcpy_s(s, sz, szDefault);
	else
		*s = 0;

	std::ifstream fs( szConfigFile );
	if (!fs.fail())
	{
		std::string line;
		while( std::getline( fs, line ) )
		{
			//// remove comments
			//for( wchar_t* c = line; *c; c++ )
			//	if( L';' == *c )
			//	{
			//		*c = 0;
			//		break;
			//	}
            // remove white spaces at end
            char const* ws = " \t\r\n\f\v";
            line.erase( 0, line.find_first_not_of(ws));
            line.erase( line.find_last_not_of(ws) + 1);
            char* b = line.data();

			// match section
			if (!bInChannel)
			{
				// match section
				if ('[' == *b)
				{
					b++;
                    char* e = strchr(b, ']');
					if (e)
					{
						*e = 0;
						if (0 == _stricmp(szSection, b))
							bInChannel = true;
					}
				}
			}
			else // match key
			{
				if ('[' == *b)
				{
					bInChannel = false;
				}
				else
				{
					char* bv = strchr(b, '=');
					if (bv)
					{
						*bv++ = 0;
						if (0 == _stricmp(b, szKey))
						{
							if (NO_ERROR == strcpy_s(s, sz, bv))
							{
								bRet = true;
								break;
							}
						}
					}
				}
			}
		}
	}
	return bRet;
}

VWB_int GetIniInt(char const* szSection, char const* szKey, VWB_int iDefault, char const* szConfigFile)
{
	char s[512] = { 0 };
	if (GetIniString(szSection, szKey, "", s, 512, szConfigFile))
		return atoi(s);
	return iDefault;
}

VWB_float GetIniFloat(char const* szSection, char const* szKey, VWB_float fDefault, char const* szConfigFile)
{
	char  s[512]= { 0 };
	if (GetIniString(szSection, szKey, "", s, 512, szConfigFile))
		return (VWB_float)atof(s);
	return fDefault;
}

VWB_float* GetIniMat(char const* szSection, char const* szKey, int dimX, int dimY, VWB_float const* fDefault, VWB_float* f, char const* szConfigFile, bool bTranspose)
{
	if (NULL == f)
		return NULL;
	if (0 == dimX || 0 == dimY)
		return NULL;
	char s[512] = { 0 };
	GetIniString(szSection, szKey, "", s, 512, szConfigFile);
	char const* pS = s;
	if( *pS == '[' )
	{
		pS++;
		int o = 0;
		int oE = dimX * dimY;
		if( bTranspose )
		{
			for( int i = 0; i != dimX; i++ )
			{
				for( o = 0; o != oE && 1 == sscanf_s( pS, "%f", &f[o + i] ); o += dimX )
				{
					char const* pFS = strchr( pS, ',' );
					char const* pFSn = strchr( pS, ';' );
					if( NULL != pFSn && pFSn < pFS )
						pFS = pFSn;
					if( pFS || ( pFS = strchr( pS, ']' ) ) )
						pS = pFS + 1;
					else
					{
						i = dimX - 1;
						break;
					}
				}
			}
		}
		else
		{
			for( ; 1 == sscanf_s( pS, "%f", &f[o] ) && o != oE; o++ )
			{
				char const* pFS = strchr( pS, ',' );
				char const* pFSn = strchr( pS, ';' );
				if( NULL != pFSn && pFSn < pFS )
					pFS = pFSn;
				if( pFS || ( pFS = strchr( pS, ']' ) ) )
					pS = pFS + 1;
				else
					break;
			}
		}
		if( oE == o )
			return f;
	}
	if( NULL != fDefault )
	{
		memcpy( f, fDefault, sizeof( VWB_float ) * dimX * dimY );
		return f;
	}

	for (int y = 0; y != dimY; y++)
		for (int x = 0; x != dimX; x++)
			f[dimX * y + x] = 0.0f;
	return NULL;
}
