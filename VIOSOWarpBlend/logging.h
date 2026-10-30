// VIOSO API
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#ifndef VWB_LOGGING_HPP
#define VWB_LOGGING_HPP

#include "../Include/VWBTypes.h"
extern VWB_int g_logLevel;
extern char g_logFilePath[MAX_PATH];

#if defined(__GNUC__)
#define VWB_PRINTF_FORMAT( fmtIdx, argIdx ) __attribute__(( format( printf, fmtIdx, argIdx ) ))
#else
#define VWB_PRINTF_FORMAT( fmtIdx, argIdx )
#endif

int logStr( VWB_int level, char const* format, ... ) VWB_PRINTF_FORMAT( 2, 3 ); // log someting, ALWAYS returns 0!
void logClear(); // backup and purge logfile

#endif //ndef VWB_LOGGING_HPP
