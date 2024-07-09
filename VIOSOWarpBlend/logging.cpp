// VIOSO API
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "Platform.h"
#include "logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

char g_logFilePath[260] = {0};
VWB_int g_logLevel = 2;
#ifndef WIN32
#define NOERROR 0
#endif

int logStr( VWB_int level, char const* format, ... )
{
	if( g_logLevel >= level )
	{
		FILE* f = NULL;
		errno_t err;
		if ( 
			0 == g_logFilePath[0] 
			|| 
			's' == g_logFilePath[0] &&
			't' == g_logFilePath[1] &&
			'd' == g_logFilePath[2] &&
			'o' == g_logFilePath[3] &&
			'u' == g_logFilePath[4] &&
			't' == g_logFilePath[5] &&
			'\0' == g_logFilePath[6]
			)
		{
			f = stdout;
			err = NOERROR;
		}
		else if (
			's' == g_logFilePath[0] &&
			't' == g_logFilePath[1] &&
			'd' == g_logFilePath[2] &&
			'e' == g_logFilePath[3] &&
			'r' == g_logFilePath[4] &&
			'r' == g_logFilePath[5] &&
			'\0' == g_logFilePath[6]
			)
		{
			f = stderr;
			err = NOERROR;
		}
		else
		{
			int c = 0;
			err = 13;
			while( 10 != c++ )
			{
				err = fopen_s( &f, g_logFilePath, "a+" );
				if( 13 == err )
					sleep( 1 );
				else
					break;
			} 
		}
		if( NOERROR == err )
		{
			va_list params;
			
			time_t t;
			time( &t );
			struct tm tm;
			localtime_s( &tm, &t );
			fprintf( f, "%02d:%02d:%02d ", tm.tm_hour, tm.tm_min, tm.tm_sec );
            va_start( params, format );
            vfprintf(f, format, params);
            va_end(params);
            fputs("\n", f);

			if( f != stdout && f != stderr )
			{
				fclose(f);
			}
		}
	}
	return 0;
}

void logClear()
{
	FILE* f = NULL;
	char szBakFile[MAX_PATH];
	strcpy_s( szBakFile, g_logFilePath );
	strcat_s( szBakFile, ".bak" );
	remove( szBakFile );
	if( 0 == rename( g_logFilePath, szBakFile ) )
		(nullptr); // removes warinig C6031

	if( 0 == fopen_s( &f, g_logFilePath, "w+" ) )
		fclose(f);
}
