#include "linux_compat.h"

errno_t localtime_s(tm* const tmDest, const time_t* const sourceTime) { *tmDest = *localtime( sourceTime );  return tmDest != NULL; }

errno_t strcat_s(char* dest, size_t n, const char* src) { strncat( dest, src, n ); dest[n-1] = 0; return 0; }



errno_t strcpy_s(char* dest, size_t n, const char* src) { strncpy( dest, src, n ); dest[n-1] = 0; return 0; }



errno_t strncpy_s(char* dest, size_t n, const char* src, size_t count) { if( n < count ) count = n; strncpy( dest, src, count ); dest[count-1] = 0; return 0; }

errno_t strncpy_s(char* dest, const char* src, size_t count) { strncpy( dest, src, count ); dest[count-1] = 0; return 0; }

errno_t wcstombs_s(size_t* pReturn, char* mbstr, const wchar_t* src, size_t cnt) { size_t r = wcstombs(mbstr, src, cnt); mbstr[cnt-1]=0; return 0; }

int fopen_s(FILE** f, const char* __filename, const char* __mode) { *f = fopen( __filename, __mode );   return ( *f ? 0 : -1 ); }
