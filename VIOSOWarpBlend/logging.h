#ifndef VWB_LOGGING_HPP
#define VWB_LOGGING_HPP

#include "../Include/VWBTypes.h"
extern VWB_int g_logLevel;
extern char g_logFilePath[MAX_PATH];

int logStr( VWB_int level, char const* format, ... ); // log someting, ALWAYS returns 0!
void logClear(); // backup and purge logfile

#endif //ndef VWB_LOGGING_HPP
