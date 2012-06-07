#ifndef EXTRA_DEFINES_H
#define EXTRA_DEFINES_H

/*
#ifdef WINDOWS
#include <Windows.h>
#undef _WIN32
#undef WIN32
#undef WCHAR
#endif
*/

#include "tchar.h"

// rename init_audio to make init_audio available

#define init_audio uae_init_audio

#include "uae_host.h"

#define STUB(format, ...) write_log(" -- stub -- %s " format "\n", \
        __func__, ##__VA_ARGS__)

#define VERBOSE_STUB(format, ...)

// we are using our own main function, not the one from UAE...

#define NO_MAIN_IN_MAIN_C

#ifndef WINDOWS
#define CAN_PRINTF_LONG_LONG
#endif

// UAE code must use this instead of RAND_MAX, and use uaerand for all
// random number generation
#define UAE_RAND_MAX 2147483647

/*
unsigned int uaerand_impl (void);
static inline unsigned int uaerand (void) {
    printf("   >>>>    uaerand    <<<< %s:%d:%s\n",  __BASE_FILE__, __LINE__,
            __func__);
    return uaerand_impl();
}
*/

#define DEBUG_SYNC
//#define DEBUG_SYNC_MEMORY
#ifdef DEBUG_SYNC
#include <stdio.h>
extern FILE *g_fs_uae_sync_debug_file;
#define write_sync_log(format, ...) \
    if (g_fs_uae_sync_debug_file) { \
        fprintf(g_fs_uae_sync_debug_file, format, __VA_ARGS__); \
    }
#endif

// needed to include functions in include/debug.h, which in turn
// is needed by code

#define DEBUGGER 1

#define A_DMS
#define A_ZIP
//#define A2065
#define A2091
#define ACTION_REPLAY
#define AGA
//#define AHI
#define ARCADIA
#define AUTOCONFIG

#if !defined(WINDOWS) && !defined(ANDROID)
#define BSDSOCKET
#endif

#define CD32
#define CDTV
#define ECS_DENISE
#define CAPS
#define CPUEMU_0
#define CPUEMU_11
#define CPUEMU_12
#define CPUEMU_20
#define CPUEMU_21
#define CPUEMU_31
//#define DEBUGGER
#define DRIVESOUND
//#define ENFORCER
#define FDI2RAW
#define FILESYS
#define FPUEMU /* FPU emulation */
#define FPU_UAE
#define GFXFILTER
#define MMU
#define MMUEMU /* Aranym 68040 MMU */
//#define MULTIDISPLAY 1
//#define NATMEM_OFFSET natmem_offset
#define FULLMMU /* Aranym 68040 MMU */
#define PARALLEL_PORT
#define PICASSO96
#define PICASSO96_SUPPORTED
#define SAVESTATE
#define SCSIEMU
#define SERIAL_PORT
#define SUPPORT_THREADS
#define UNALIGNED_PROFITABLE
#define UAESERIAL
//#define UAE_FILESYS_THREADS
#define UAE_FILESYS_ASYNCHRONOUS
//#define USE_SDL

#define XARCADE
#define GNU_SOURCE 1

// needed several places in the code
#define MAX_DPATH 1024
#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
//#ifdef WINDOWS
//#define MAX_PATH 512
//#else
//define MAX_PATH 1024
//#endif
#endif

// needed by serial.cpp
#ifdef WINDOWS
// FIXME: DUMMY
#define O_NDELAY 0
#endif

// These should be valid on all supported systems, also Windows
#define FSDB_DIR_SEPARATOR '/'
#define FSDB_DIR_SEPARATOR_S "/"

// FIXME: OK?
#define _stat64 stat

#define INVALID_SOCKET -1
#ifdef WINDOWS
typedef unsigned int SOCKET;
#else
typedef int SOCKET;
#endif

typedef unsigned short USHORT;


#ifndef DO_NOT_INCLUDE_WINUAE_COMPAT_H
// we do not always want to include these, especially not in uae_host.cpp
// were we actually need access to some of the underlying function names
#include "winuae_compat.h"
#endif

#include "../include/sysdeps.h"

// make use of enums compatible with C++: in C++ you cannot assign an
// enum value to an int

#undef ENUMNAME
#undef ENUMDECL
#define ENUMDECL enum
#define ENUMNAME(name) ; typedef int name ;

#include "machdep/machdep.h"

// some code needs uae_sem_t but does not include thread.h properly
// (include/bsdsocket.h)

#include "threaddep/thread.h"

// FIXME: IMPLEMENT
//void _stprintf(wchar_t* buffer, wchar_t* format, ...);

extern TCHAR start_path_data[];
extern void picasso_reset (void);
extern void picasso_handle_hsync (void);
extern void init_hz_p96 (void);
int GetDriveType(TCHAR* vol);

void sleep_millis(int ms);
//void install_driver(int flags);

extern int uae_start_thread_fast (void *(*f)(void *), void *arg,
        uae_thread_id *thread);

#ifdef WINDOWS

#else
#endif

#define FILEFLAG_WRITE S_IWUSR
#define FILEFLAG_READ S_IRUSR
#define FILEFLAG_EXECUTE S_IXUSR
#define FILEFLAG_DIR S_IFDIR


#define Sleep sleep_millis
typedef unsigned int WPARAM;
typedef long LPARAM;
typedef int BOOL;
#define TRUE 1
#define FALSE 0

// needed to compile gencpu.cpp as C code (which is needed because
// of int to enum conversions (illegal in C++)
#ifndef __cplusplus
// FIXME: 
//typedef int bool
#define bool int
#define true 1
#define false 0
#endif


// -------------------------- windows temp --------------------------
// FIXME: THESE ARE HERE IN ORDER TO COMPILE blkdev.cpp
#define UAESCSI_CDEMU 0
#define UAESCSI_SPTI 1
#define UAESCSI_SPTISCAN 2
#define UAESCSI_ASPI_FIRST 3
#define UAESCSI_ADAPTECASPI 3
#define UAESCSI_NEROASPI 4
#define UAESCSI_FROGASPI 5

//#ifndef WINDOWS
// just setting DRIVE_CDROM to something here..
#define DRIVE_CDROM 0
//#endif

#include "uae_util.h"
#include <stddef.h>

#endif // EXTRA_DEFINES_H

