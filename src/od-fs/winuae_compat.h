#ifndef LIBAMIGA_WINUAE_COMPAT_H_
#define LIBAMIGA_WINUAE_COMPAT_H_

// use custom versions of these functions for platform-specific behaviour

#define _tfopen uae_fopen
#define _ftelli64 uae_ftello64
#define _fseeki64 uae_fseeko64

// convert windows libc names to standard libc function names

#define _stprintf sprintf
#define _wunlink unlink
#define _tcscmp strcmp
#define _tcsncmp strncmp
#define _tcslen strlen
#define _tcscpy strcpy
#define _tcsncpy strncpy
#define _tcsdup strdup
#define _tcscat strcat
#define _tcsncat strncat
#define _tcsspn strspn
#define _tcsicmp strcasecmp
#define _tcsnicmp strncasecmp
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define _tcsrchr strrchr
#define _tcschr strchr
#define _istdigit isdigit
#define _istspace isspace
#define _istupper isupper
#define _tcsstr strstr
#define _tcsftime strftime
#define _tcsftime strftime
#define _tstol atol
#define _tstof atof
#define _tcstod strtod
#define _tcstol strtol
#define _strtoui64 strtoll
#define _totupper toupper
#define _totlower tolower
#define _tcstok strtok
#define _tstoi atoi
#define _vsntprintf vsnprintf
#define _vsnprintf vsnprintf
#define _tprintf printf
#define _timezone timezone
#define _daylight daylight
#define _tzset tzset

#define _istalnum isalnum

// FIXME: OK?

#define REGPARAM
#define REGPARAM2
#define REGPARAM3

// needed by e.g drawing.cpp

#define NOINLINE

#ifdef WINDOWS
// including windef.h now to get RECT and DWORD defined (and not collide with
// later includes of windows.h
#include "windef.h"
#undef _WIN32
#undef WIN32
#endif

#ifndef WINDOWS

#define _ftime ftime
#define _timeb timeb

#define _cdecl

#ifndef ULONG
#define ULONG unsigned long
#endif

//typedef unsigned int UAE_DWORD;
typedef unsigned int DWORD;

typedef struct tagRECT {
    int left;
    int top;
    int right;
    int bottom;
} RECT, *PRECT, *PPRECT;

//#ifndef WINDOWS
//#define DWORD UAE_DWORD
//#define RECT UAE_RECT
//#endif
#endif

// needed by e.g. include/commpipe.h

#define STATIC_INLINE static inline

#endif // LIBAMIGA_WINUAE_COMPAT_H_
