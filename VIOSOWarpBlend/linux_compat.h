//
//  mac_compat.h
//  VIOSOWarpBlend
//
//  Created by Tom Riley on 5/19/17.
//

#ifndef LINUX_COMPAT_H
#define LINUX_COMPAT_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h> // contains various _s functions, if still missing force C++11 or higher
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>

typedef int errno_t;
static const errno_t NO_ERROR = 0;
static const errno_t NOERROR = 0;
typedef void VOID;
typedef struct {
   int cx;
   int cy;
 } SIZE;
#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
   static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))
typedef int LONG;

inline errno_t localtime_s( struct tm* const tmDest, time_t const* const sourceTime) { *tmDest = *localtime( sourceTime );  return tmDest != NULL; }
inline errno_t strcat_s(char* dest, size_t n, const char* src) { strncat( dest, src, n ); dest[n-1] = 0; return 0; }

inline size_t strnlen_s( const char* str, size_t strsz ) { return str ? strlen( str ) : 0; }

inline errno_t strcat_s( char* dest, const char* src ) { strcat( dest, src ); return 0; }
inline errno_t strcpy_s( char* dest, size_t n, const char* src ) { strncpy( dest, src, n ); dest[n-1] = 0; return 0; }
template<size_t size>
inline errno_t strcpy_s( char (& dest)[size], const char* src ) { return strcpy_s( dest, size, src ); }

inline errno_t strncpy_s( char* dest, size_t n, const char* src, size_t count ) { if( n < count ) count = n; strncpy( dest, src, count ); dest[count-1] = 0; return 0; }
template<size_t size>
inline errno_t strncpy_s( char (& dest)[size], const char* src, size_t count ) { return strncpy_s( dest, size, src, count ); }

inline errno_t wcstombs_s(size_t* pReturn, char* mbstr, const wchar_t* src, size_t cnt) { size_t r = wcstombs(mbstr, src, cnt); mbstr[cnt-1]=0; return 0; }

inline int fopen_s(FILE** f, const char* __filename, const char* __mode) { *f = fopen( __filename, __mode );   return ( *f ? 0 : -1 ); }int fopen_s( FILE** f, const char* __restrict __filename, const char* __restrict __mode );

inline errno_t _itoa_s( int value, char* buff, size_t size, int radix ) { snprintf( buff, size, "%d", value ); return 0;}
template< size_t sz> errno_t _itoa_s( int value, char(& buff)[sz], int radix ) { return _itoa_s( value, buff, sz, radix);}
#define vfprintf_s vfprintf
#define fread_s(a,b,c,d,e) fread(a,c,d,e)
#define _stricmp strcasecmp
#define sscanf_s sscanf
#define sprintf_s sprintf
#define sleep(ms) usleep((ms)*1000)
#define InterlockedIncrement(ptr) __sync_fetch_and_add(ptr, 1)
#define InterlockedDecrement(ptr) __sync_fetch_and_sub(ptr, 1)

#endif // LINUX_COMPAT_H
