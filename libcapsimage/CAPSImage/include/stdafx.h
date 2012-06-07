#ifndef STDAFX_H
#define STDAFX_H

#ifdef _WIN32
#define DEFFILEMODE (0)
#endif

#ifndef __APPLE__
#include "config.h"
#endif /* __APPLE__ */
#include "afxgen.h"

#ifdef AMIGA
typedef UBYTE     uint8_t;
typedef LONG      SDWORD;
typedef ULONG     UDWORD;
typedef ULONG     uint32_t;
typedef unsigned long long UQUAD;
#else
typedef uint8_t   UBYTE;
typedef uint16_t  UWORD;
typedef int32_t   SDWORD;
typedef uint32_t  UDWORD;
typedef uint64_t  UQUAD;
#endif /* AMIGA */

typedef void     *PVOID;
typedef char     *PCHAR;
typedef UBYTE    *PUBYTE;
typedef UDWORD   *PUDWORD;
typedef SDWORD   *PSDWORD;

typedef struct CapsDateTimeExt *PCAPSDATETIMEEXT;
typedef struct CapsImageInfo   *PCAPSIMAGEINFO;
typedef struct CapsTrackInfo   *PCAPSTRACKINFO;
typedef struct CapsTrackInfoT1 *PCAPSTRACKINFOT1;
typedef struct CapsTrackInfoT2 *PCAPSTRACKINFOT2;
typedef struct CapsVersionInfo *PCAPSVERSIONINFO;
typedef struct CapsSectorInfo  *PCAPSSECTORINFO;
typedef struct CapsDataInfo    *PCAPSDATAINFO;
typedef struct CapsDrive       *PCAPSDRIVE;
typedef struct CapsFdc         *PCAPSFDC;
typedef struct CapsFormatBlock *PCAPSFORMATBLOCK;
typedef struct CapsFormatTrack *PCAPSFORMATTRACK;

#define DF_0	(1L<<0)
#define DF_1	(1L<<1)
#define DF_2	(1L<<2)
#define DF_3	(1L<<3)
#define DF_4	(1L<<4)
#define DF_5	(1L<<5)
#define DF_6	(1L<<6)
#define DF_7	(1L<<7)
#define DF_8	(1L<<8)
#define DF_9	(1L<<9)
#define DF_10	(1L<<10)
#define DF_11	(1L<<11)
#define DF_12	(1L<<12)
#define DF_13	(1L<<13)
#define DF_14	(1L<<14)
#define DF_15	(1L<<15)
#define DF_16	(1L<<16)
#define DF_17	(1L<<17)
#define DF_18	(1L<<18)
#define DF_19	(1L<<19)
#define DF_20	(1L<<20)
#define DF_21	(1L<<21)
#define DF_22	(1L<<22)
#define DF_23	(1L<<23)
#define DF_24	(1L<<24)
#define DF_25	(1L<<25)
#define DF_26	(1L<<26)
#define DF_27	(1L<<27)
#define DF_28	(1L<<28)
#define DF_29	(1L<<29)
#define DF_30	(1L<<30)
#define DF_31	(1L<<31)

#if !defined(WORDS_BIGENDIAN) && !defined(__BIG_ENDIAN__)
#define INTEL
#endif /* !defined(WORDS_BIGENDIAN) && !defined(__BIG_ENDIAN__) */

#ifdef _DEBUG
#define NODEFAULT assert(0)
#else
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define NODEFAULT __builtin_unreachable()
#else
#define NODEFAULT __builtin_trap()
#endif /* __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5) */
#endif /* _DEBUG */

#include "crc.h"
#include "caps/capsimage.h"
#include "Capsdef.h"
#include "CapsFile.h"
#include "DiskEnc.h"
#include "DiskImg.h"
#include "CapsLdr.h"
#include "CapsImgS.h"
#include "caps/fdc.h"
#include "CapsEFDC.h"
#include "caps/form.h"
#include "CapsEMFM.h"
#include "CapsCore.h"

#endif /* STDAFX_H */
