// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "logging.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <stdarg.h>
VWB_int g_logLevel = 2;
std::filesystem::path g_logFilePath;

#ifndef WIN32
#define NOERROR 0
#endif

int logStr( VWB_int level, char const* format, ... )
{
	if( g_logLevel >= level )
	{
		std::ostream* pOs = nullptr;
		std::ofstream ofs;
		if ( g_logFilePath.empty()
			|| 
			's' == g_logFilePath.c_str()[0] &&
			't' == g_logFilePath.c_str()[1] &&
			'd' == g_logFilePath.c_str()[2] &&
			'o' == g_logFilePath.c_str()[3] &&
			'u' == g_logFilePath.c_str()[4] &&
			't' == g_logFilePath.c_str()[5] &&
			'\0' == g_logFilePath.c_str()[6]
			)
		{
			pOs = &std::cout;
		}
		else if (
			's' == g_logFilePath.c_str()[0] &&
			't' == g_logFilePath.c_str()[1] &&
			'd' == g_logFilePath.c_str()[2] &&
			'e' == g_logFilePath.c_str()[3] &&
			'r' == g_logFilePath.c_str()[4] &&
			'r' == g_logFilePath.c_str()[5] &&
			'\0' == g_logFilePath.c_str()[6]
			)
		{
			pOs = &std::cerr;
		}
		else if (
			's' == g_logFilePath.c_str()[0] &&
			't' == g_logFilePath.c_str()[1] &&
			'd' == g_logFilePath.c_str()[2] &&
			'l' == g_logFilePath.c_str()[3] &&
			'o' == g_logFilePath.c_str()[4] &&
			'g' == g_logFilePath.c_str()[5] &&
			'\0' == g_logFilePath.c_str()[6]
			)
		{
			pOs = &std::clog;
		}
		else
		{
			int c = 0;
			while( 10 != c++ )
			{
				ofs.open( g_logFilePath, std::ios_base::app );

				if( !ofs.is_open() )
					std::this_thread::sleep_for( std::chrono::milliseconds(1) );
				else
				{
					pOs = &ofs;
					break;
				}
			} 
		}
		if( pOs )
		{
			auto in_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			*pOs << std::put_time(std::localtime(&in_time_t), "%X ");

			va_list params;
			va_list params_copy;
            va_start( params, format );
			va_copy(params_copy, params);
            auto size = std::vsnprintf(nullptr, 0, format, params_copy );
			va_end(params_copy);
			std::string out(size + 1, 0);  // C99 says "size is excluding the terminating NULL", but it seems to be including on G++
			size = std::vsnprintf(out.data(), size + 1, format, params);
			while( !out.empty() && !out.back() ) out.pop_back(); // remove all zeros, this is also UTF-8 safe, as valid codepoints do not end with a 0-byte
			va_end(params);
			*pOs << out << std::endl;
		}
	}
	return 0;
}

void logClear()
{
	FILE* f = NULL;
	std::filesystem::path bakFile(g_logFilePath);
	bakFile += ".bak";
	if( std::filesystem::exists( bakFile ) )
		std::filesystem::remove( bakFile );
	if( std::filesystem::exists( g_logFilePath ) )
		std::filesystem::rename( g_logFilePath, bakFile );
}
