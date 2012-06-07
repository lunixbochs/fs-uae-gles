#ifndef CLIB_CAPSIMAGE_PROTOS_H
#define CLIB_CAPSIMAGE_PROTOS_H

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif /* DOS_DOS_H */
#ifndef EXEC_EXECBASE_H
#include <exec/execbase.h>
#endif /* EXEC_EXECBASE_H */
#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif /* EXEC_SEMAPHORES_H */
#ifndef DEVICES_TRACKDISK_H
#include <devices/trackdisk.h>
#endif /* DEVICES_TRACKDISK_H */
#ifndef CAPS_CAPSIMAGE_H
#include <caps/capsimage.h>
#endif /* CAPS_CAPSIMAGE_H */
#ifndef CAPS_FDC_H
#include <caps/fdc.h>
#endif /* CAPS_FDC_H */
#ifndef CAPS_FORM_H
#include <caps/form.h>
#endif /* CAPS_FORM_H */

#define CAPS_PRI	5

#define CAPS_PUDDLESIZE		32768
#define CAPS_THRESHOLDSIZE	16384

/* startup message for unit process */
struct StartupMessage
{
	struct Message		sm_Message;
	struct CapsImageUnit   *sm_Unit;
};

/* caps unit structure */
struct CapsImageUnit
{
	struct TDU_PublicUnit	ciu_Unit;
	struct MsgPort		ciu_RexxMsgPort;
	UBYTE			ciu_UnitNum;
	BYTE			ciu_Reserved0;
	LONG			ciu_ContainerID;
	struct Process	       *ciu_Process;
	struct StartupMessage	ciu_StartupMessage;
};

/* caps device base */
struct CapsImageBase
{
	struct Device		cib_Dev;
	WORD			cib_Reserved0;
	struct CapsImageBase   *cib_Parent;
	BPTR			cib_SegList;
	struct ExecBase	       *cib_SysBase;
	APTR			cib_A4;
	struct SignalSemaphore	cib_Lock;
	struct CapsImageUnit   *cib_Units[CAPS_UNITS];
};

struct CapsImageBase *DevInitFunction(struct CapsImageBase *CapsImageBase __asm("d0"), BPTR seglist __asm("a0"), struct ExecBase *SysBase __asm("a6"));
VOID DevOpen(ULONG unitNumber __asm("d0"), struct IOExtTD *ioExtTD __asm("a1"), ULONG flags __asm("d1"), struct CapsImageBase *CapsImageBase __asm("a6"));
BPTR DevClose(struct IOExtTD *ioExtTD __asm("a1"), struct CapsImageBase *CapsImageBase __asm("a6"));
BPTR DevExpunge(struct CapsImageBase *CapsImageBase __asm("a6"));
ULONG DevReserved(VOID);
VOID DevBeginIO(struct IOExtTD *ioExtTD __asm("a1"), struct CapsImageBase *CapsImageBase __asm("a6"));
VOID DevAbortIO(struct IOExtTD *ioExtTD __asm("a1"), struct CapsImageBase *CapsImageBase __asm("a6"));

LONG DevCAPSInit(struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSExit(struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSAddImage(struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSRemImage(LONG id __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSLockImage(LONG id __asm("d0"), STRPTR name __asm("a0"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSLockImageMemory(LONG id __asm("d0"), UBYTE *buffer __asm("a0"), ULONG length __asm("d1"), ULONG flag __asm("d2"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSUnlockImage(LONG id __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSLoadImage(LONG id __asm("d0"), ULONG flag __asm("d1"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSGetImageInfo(struct CapsImageInfo *pi __asm("a0"), LONG id __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSLockTrack(APTR ptrackinfo __asm("a0"), LONG id __asm("d0"), ULONG cylinder __asm("d1"), ULONG head __asm("d2"), ULONG flag __asm("d3"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSUnlockTrack(LONG id __asm("d0"), ULONG cylinder __asm("d1"), ULONG head __asm("d2"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSUnlockAllTracks(LONG id __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"));
STRPTR DevCAPSGetPlatformName(ULONG pid __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSGetVersionInfo(APTR pversioninfo __asm("a0"), ULONG flag __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"));
ULONG DevCAPSFdcGetInfo(LONG iid __asm("d0"), struct CapsFdc *pc __asm("a0"), LONG ext __asm("d1"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSFdcInit(struct CapsFdc *pc __asm("a0"), struct CapsImageBase *CapsImageBase __asm("a6"));
VOID DevCAPSFdcReset(struct CapsFdc *pc __asm("a0"), struct CapsImageBase *CapsImageBase __asm("a6"));
VOID DevCAPSFdcEmulate(struct CapsFdc *pc __asm("a0"), ULONG cyclecnt __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"));
ULONG DevCAPSFdcRead(struct CapsFdc *pc __asm("a0"), ULONG address __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"));
VOID DevCAPSFdcWrite(struct CapsFdc *pc __asm("a0"), ULONG address __asm("d0"), ULONG data __asm("d1"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSFdcInvalidateTrack(struct CapsFdc *pc __asm("a0"), LONG drive __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSFormatDataToMFM(APTR pformattrack __asm("a0"), ULONG flag __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"));
LONG DevCAPSGetInfo(APTR pinfo __asm("a0"), LONG id __asm("d0"), ULONG cylinder __asm("d1"), ULONG head __asm("d2"), ULONG inftype __asm("d3"), ULONG infid __asm("d4"), struct CapsImageBase *CapsImageBase __asm("a6"));

BOOL __saveds OpenLibs(struct ExecBase *SysBase __asm("a6"));
VOID __saveds CloseLibs();
VOID __saveds InitUnit(ULONG unitNumber, struct CapsImageBase *CapsImageBase);
VOID __saveds ExpungeUnit(struct CapsImageUnit *CapsImageUnit, struct CapsImageBase *CapsImageBase);
VOID __saveds UnitProcess(VOID);

#endif /* CLIB_CAPSIMAGE_PROTOS_H */
