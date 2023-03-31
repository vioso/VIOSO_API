//
//  mac_compat.h
//  VIOSOWarpBlend
//
//  Created by Tom Riley on 5/19/17.
//

#ifndef LINUX_COMPAT_H
#define LINUX_COMPAT_H

#include <stdio.h>
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

 errno_t localtime_s( struct tm* const tmDest, time_t const* const sourceTime );
 size_t strnlen_s( const char* str, size_t strsz ) { return str ? strlen( str ) : nullptr; }
 errno_t strcat_s( char* dest, size_t n, const char* src );
 // Body of a constexpr must be a return stament in c++11
 inline errno_t strcat_s( char* dest, const char* src ) { strcat( dest, src ); return 0; }
 errno_t strcpy_s( char* dest, size_t n, const char* src );
 // Body of a constexpr must be a return stament in c++11
 inline errno_t strcpy_s( char* dest, const char* src ) { strcpy( dest, src ); return 0; }
 errno_t strncpy_s( char* dest, size_t n, const char* src, size_t count );
 errno_t strncpy_s( char* dest, const char* src, size_t count );
 int fopen_s( FILE** f, const char* __restrict __filename, const char* __restrict __mode );
 errno_t wcstombs_s(size_t* pReturn, char*mbstr, const wchar_t* src, size_t cnt );

#define vfprintf_s vfprintf
#define fread_s(a,b,c,d,e) fread(a,c,d,e)
#define _stricmp strcasecmp
#define sscanf_s sscanf
#define sprintf_s sprintf
#define sleep(ms) usleep((ms)*1000)
#define InterlockedIncrement(ptr) __sync_fetch_and_add(ptr, 1)
#define InterlockedDecrement(ptr) __sync_fetch_and_sub(ptr, 1)


#endif // LINUX_COMPAT_H
