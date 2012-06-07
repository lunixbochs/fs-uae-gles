#include "config.h"
#include "capsimage_protos.h"
#include <exec/initializers.h>
#include <exec/resident.h>
#include <exec/errors.h>
#include <proto/exec.h>

/* local prototypes for setting a4 */
VOID SetupGlobals(APTR __asm("a4"));
VOID CleanupGlobals(APTR __asm("a4"));

LONG CAPSInit(APTR __asm("a4"));
LONG CAPSExit(APTR __asm("a4"));
LONG CAPSAddImage(APTR __asm("a4"));
LONG CAPSRemImage(LONG id, APTR __asm("a4"));
LONG CAPSLockImage(LONG id, STRPTR name, APTR __asm("a4"));
LONG CAPSLockImageMemory(LONG id, UBYTE *buffer, ULONG length, ULONG flag, APTR __asm("a4"));
LONG CAPSUnlockImage(LONG id, APTR __asm("a4"));
LONG CAPSLoadImage(LONG id, ULONG flag, APTR __asm("a4"));
LONG CAPSGetImageInfo(struct CapsImageInfo *pi, LONG id, APTR __asm("a4"));
LONG CAPSLockTrack(APTR ptrackinfo, LONG id, ULONG cylinder, ULONG head, ULONG flag, APTR __asm("a4"));
LONG CAPSUnlockTrack(LONG id, ULONG cylinder, ULONG head, APTR __asm("a4"));
LONG CAPSUnlockAllTracks(LONG id, APTR __asm("a4"));
STRPTR CAPSGetPlatformName(ULONG pid, APTR __asm("a4"));
LONG CAPSGetVersionInfo(APTR pversioninfo, ULONG flag, APTR __asm("a4"));
ULONG CAPSFdcGetInfo(LONG iid, struct CapsFdc *pc, LONG ext, APTR __asm("a4"));
LONG CAPSFdcInit(struct CapsFdc *pc, APTR __asm("a4"));
VOID CAPSFdcReset(struct CapsFdc *pc, APTR __asm("a4"));
VOID CAPSFdcEmulate(struct CapsFdc *pc, ULONG cyclecnt, APTR __asm("a4"));
ULONG CAPSFdcRead(struct CapsFdc *pc, ULONG address, APTR __asm("a4"));
VOID CAPSFdcWrite(struct CapsFdc *pc, ULONG address, ULONG data, APTR __asm("a4"));
LONG CAPSFdcInvalidateTrack(struct CapsFdc *pc, LONG drive, APTR __asm("a4"));
LONG CAPSFormatDataToMFM(APTR pformattrack, ULONG flag, APTR __asm("a4"));
LONG CAPSGetInfo(APTR pinfo, LONG id, ULONG cylinder, ULONG head, ULONG inftype, ULONG infid, APTR __asm("a4"));

/* linker symbols for reentrancy support */
extern LONG __a4_init;
extern LONG __bss_size;
extern LONG __data_size;
extern LONG __datadata_relocs[];

static const char DevName[] = CAPS_NAME;
static const char DevIdString[] = VSTRING;

static const APTR DevFuncTable[] = {
	DevOpen,
	DevClose,
	DevExpunge,
	DevReserved,
	DevBeginIO,
	DevAbortIO,
	DevCAPSInit,
	DevCAPSExit,
	DevCAPSAddImage,
	DevCAPSRemImage,
	DevCAPSLockImage,
	DevCAPSLockImageMemory,
	DevCAPSUnlockImage,
	DevCAPSLoadImage,
	DevCAPSGetImageInfo,
	DevCAPSLockTrack,
	DevCAPSUnlockTrack,
	DevCAPSUnlockAllTracks,
	DevCAPSGetPlatformName,
	DevCAPSGetVersionInfo,
	DevCAPSFdcInit,
	DevCAPSFdcReset,
	DevCAPSFdcEmulate,
	DevCAPSFdcRead,
	DevCAPSFdcWrite,
	DevCAPSFdcInvalidateTrack,
	DevCAPSFormatDataToMFM,
	DevCAPSGetInfo,
	(APTR)-1
};

static const struct {
	struct { UBYTE command; UBYTE offset; UBYTE source; };
	struct { UBYTE command; UBYTE offset; ULONG source; };
	struct { UBYTE command; UBYTE offset; UBYTE source; };
	struct { UBYTE command; UBYTE offset; UWORD source; };
	struct { UBYTE command; UBYTE offset; UWORD source; };
	struct { UBYTE command; UBYTE offset; ULONG source; };
	struct { UBYTE command; };
} __attribute__((aligned(2))) DevInitStruct = {
	{ 0xa0, (UBYTE)OFFSET(Library, lib_Node.ln_Type), NT_DEVICE },
	{ 0x80, (UBYTE)OFFSET(Library, lib_Node.ln_Name), (ULONG)DevName },
	{ 0xa0, (UBYTE)OFFSET(Library, lib_Flags), LIBF_SUMUSED|LIBF_CHANGED },
	{ 0x90, (UBYTE)OFFSET(Library, lib_Version), VERSION },
	{ 0x90, (UBYTE)OFFSET(Library, lib_Revision), REVISION },
	{ 0x80, (UBYTE)OFFSET(Library, lib_IdString), (ULONG)DevIdString },
	{ 0x00 }
};

static const ULONG DevInitTable[] = {
	sizeof(struct CapsImageBase),
	(ULONG)DevFuncTable,
	(ULONG)&DevInitStruct,
	(ULONG)DevInitFunction
};

static const struct Resident __attribute__((used)) ROMTag = {
        RTC_MATCHWORD,
	(APTR)&ROMTag,
	(APTR)&ROMTag+1,
        RTF_AUTOINIT,
        VERSION,
	NT_DEVICE,
        0,
	(char *)DevName,
	(char *)DevIdString,
	(APTR)DevInitTable
};

struct CapsImageBase *DevInitFunction(struct CapsImageBase *CapsImageBase __asm("d0"), BPTR seglist __asm("a0"), struct ExecBase *SysBase __asm("a6"))
{
	CapsImageBase->cib_Parent = CapsImageBase;
	CapsImageBase->cib_SegList = seglist;
	CapsImageBase->cib_SysBase = SysBase;
	CapsImageBase->cib_A4 = &__a4_init;

	InitSemaphore(&CapsImageBase->cib_Lock);

	if (OpenLibs(SysBase))
	{
		SetupGlobals(CapsImageBase->cib_A4);
		DevCAPSInit(CapsImageBase);

		return CapsImageBase;
	}

	CloseLibs();
		
	FreeMem((UBYTE *)CapsImageBase - CapsImageBase->cib_Dev.dd_Library.lib_NegSize, CapsImageBase->cib_Dev.dd_Library.lib_NegSize + CapsImageBase->cib_Dev.dd_Library.lib_PosSize);

	return NULL;
}

/****** capsimage.device/OpenDevice *******************************************
*
*   NAME
*        OpenDevice -- open the capsimage device
*
*   SYNOPSIS
*        error = OpenDevice(CAPS_NAME, unit, ioRequest, flags)
*        D0                 A0         D0    A1         D1
*
*        BYTE OpenDevice(STRPTR, ULONG, struct IORequest *, ULONG);
*
*   FUNCTION
*        This is an exec call. Exec will search for the capsimage.device, and
*        if found, will pass this call on to the device.
*
*   INPUTS
*        CAPS_NAME - pointer to the string "capsimage.device"
*        unit      - a unit number less than CAPS_UNITS
*        ioRequest - a pointer to a struct IORequest, initialized by
*                    exec.library/CreateIORequest()
*        flags     - currently ignored
*
*   RESULT
*        io_Error  - 0 if successful
*        io_Device - a pointer to the device base, which can be used to call
*                    the functions the device provides
*        error     - copy of io_Error
*
*   SEE ALSO
*        CloseDevice(), exec.library/OpenDevice()
*
******************************************************************************
*
*/
VOID DevOpen(ULONG unitNumber __asm("d0"), struct IOExtTD *ioExtTD __asm("a1"), ULONG flags __asm("d1"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	struct ExecBase *SysBase = CapsImageBase->cib_SysBase;
	struct CapsImageBase *newCapsImageBase;
	struct CapsImageUnit *CapsImageUnit;
	LONG i;

	/* fake an opener for duration of call */
	CapsImageBase->cib_Dev.dd_Library.lib_OpenCnt++;

	ObtainSemaphore(&CapsImageBase->cib_Lock);

	if ((newCapsImageBase = (struct CapsImageBase *)MakeLibrary((APTR)DevFuncTable, (APTR)&DevInitStruct, NULL, sizeof(struct CapsImageBase) + (ULONG)&__data_size + (ULONG)&__bss_size, NULL)))
	{
		newCapsImageBase->cib_A4 = (UBYTE *)newCapsImageBase + sizeof(struct CapsImageBase) + 0x7ffe;
		newCapsImageBase->cib_Dev.dd_Library.lib_OpenCnt++;
		newCapsImageBase->cib_Parent = CapsImageBase;
		newCapsImageBase->cib_SysBase = SysBase;
		
		/* copy the data section */
		CopyMem((UBYTE *)CapsImageBase->cib_A4 - 0x7ffe, (UBYTE *)newCapsImageBase->cib_A4 - 0x7ffe, (ULONG)&__data_size);

		/* perform relocations */
		for (i = 1; i <= __datadata_relocs[0]; i++)
			*(LONG *)((LONG)newCapsImageBase->cib_A4 - 0x7ffe + __datadata_relocs[i]) -= (LONG)CapsImageBase->cib_A4 - (LONG)newCapsImageBase->cib_A4;

		if (unitNumber < CAPS_UNITS)
		{
			if (!(CapsImageUnit = CapsImageBase->cib_Units[unitNumber]))
				InitUnit(unitNumber, CapsImageBase);

			if ((CapsImageUnit = CapsImageBase->cib_Units[unitNumber]))
			{
				SetupGlobals(newCapsImageBase->cib_A4);

				/* mark iorequest as complete */
				ioExtTD->iotd_Req.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
				ioExtTD->iotd_Req.io_Device = (struct Device *)newCapsImageBase;
				ioExtTD->iotd_Req.io_Unit = (struct Unit *)CapsImageUnit;
				ioExtTD->iotd_Req.io_Error = 0;

				CapsImageBase->cib_Dev.dd_Library.lib_OpenCnt++;
				CapsImageBase->cib_Dev.dd_Library.lib_Flags &= ~LIBF_DELEXP;
				CapsImageUnit->ciu_Unit.tdu_Unit.unit_OpenCnt++;
			}
			else
				ioExtTD->iotd_Req.io_Error = IOERR_OPENFAIL;
		}
		else
			ioExtTD->iotd_Req.io_Error = TDERR_BadUnitNum;
	}
	else
		ioExtTD->iotd_Req.io_Error = TDERR_NoMem;

	if (ioExtTD->iotd_Req.io_Error)
	{
		/* trash io_Device on open failure */
		ioExtTD->iotd_Req.io_Device = (struct Device *)-1;

		if (newCapsImageBase)
			FreeMem((UBYTE *)newCapsImageBase - newCapsImageBase->cib_Dev.dd_Library.lib_NegSize, newCapsImageBase->cib_Dev.dd_Library.lib_NegSize + newCapsImageBase->cib_Dev.dd_Library.lib_PosSize);
	}

	ReleaseSemaphore(&CapsImageBase->cib_Lock);

	/* end of expunge protection */
	--CapsImageBase->cib_Dev.dd_Library.lib_OpenCnt;
}

/****** capsimage.device/CloseDevice ******************************************
*
*   NAME
*        CloseDevice -- close the capsimage device
*
*   SYNOPSIS
*        CloseDevice(ioRequest)
*                    A1
*
*        void CloseDevice(struct IORequest *);
*
*   FUNCTION
*        This is an exec call that closes the device. Every OpenDevice() must
*        be matched with a call to CloseDevice().
*
*   INPUTS
*        ioRequest - a pointer to the same struct IORequest that was used to
*                    open the device
*
*   SEE ALSO
*        OpenDevice(), exec.library/CloseDevice()
*
******************************************************************************
*
*/
BPTR DevClose(struct IOExtTD *ioExtTD __asm("a1"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	struct CapsImageUnit *CapsImageUnit = (struct CapsImageUnit *)ioExtTD->iotd_Req.io_Unit;
	struct ExecBase *SysBase = CapsImageBase->cib_SysBase;
	BPTR seglist = NULL;

	ObtainSemaphore(&CapsImageBase->cib_Parent->cib_Lock);

	CleanupGlobals(CapsImageBase->cib_A4);

	/* make sure the iorequest is not used again */
	ioExtTD->iotd_Req.io_Unit = (struct Unit *)-1;
	ioExtTD->iotd_Req.io_Device = (struct Device *)-1;

	if (!--CapsImageUnit->ciu_Unit.tdu_Unit.unit_OpenCnt)
		ExpungeUnit(CapsImageUnit, CapsImageBase->cib_Parent);

	--CapsImageBase->cib_Parent->cib_Dev.dd_Library.lib_OpenCnt;
	
	ReleaseSemaphore(&CapsImageBase->cib_Parent->cib_Lock);

	if (!CapsImageBase->cib_Parent->cib_Dev.dd_Library.lib_OpenCnt && (CapsImageBase->cib_Dev.dd_Library.lib_Flags & LIBF_DELEXP))
		seglist = DevExpunge(CapsImageBase);

	FreeMem((UBYTE *)CapsImageBase - CapsImageBase->cib_Dev.dd_Library.lib_NegSize, CapsImageBase->cib_Dev.dd_Library.lib_NegSize + CapsImageBase->cib_Dev.dd_Library.lib_PosSize);

        return seglist;
}

BPTR DevExpunge(struct CapsImageBase *CapsImageBase __asm("a6"))
{
	struct ExecBase *SysBase = CapsImageBase->cib_SysBase;
	BPTR seglist = NULL;

	/* get the real base */
	CapsImageBase = CapsImageBase->cib_Parent;

	CapsImageBase->cib_Dev.dd_Library.lib_Flags |= LIBF_DELEXP;

	if (!CapsImageBase->cib_Dev.dd_Library.lib_OpenCnt)
        {
		seglist = CapsImageBase->cib_SegList;

		Remove((struct Node *)CapsImageBase);

		DevCAPSExit(CapsImageBase);
		CleanupGlobals(CapsImageBase->cib_A4);
		CloseLibs();

		FreeMem((UBYTE *)CapsImageBase - CapsImageBase->cib_Dev.dd_Library.lib_NegSize, CapsImageBase->cib_Dev.dd_Library.lib_NegSize + CapsImageBase->cib_Dev.dd_Library.lib_PosSize);
	}

	return seglist;
}

ULONG DevReserved(VOID)
{
	return 0;
}

VOID DevBeginIO(struct IOExtTD *ioExtTD __asm("a1"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
}

VOID DevAbortIO(struct IOExtTD *ioExtTD __asm("a1"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
}

/****** capsimage.device/CAPSInit *********************************************
*
*   NAME
*        CAPSInit -- start caps image support
*
*   SYNOPSIS
*        error = CAPSInit()
*        D0
*
*        LONG CAPSInit();
*
*   FUNCTION
*        The function initialises the library internals. It must be called
*        before any other calls are made.
*
*   RESULT
*        error - imgeOk if successful
*
*   NOTES
*        The program should attempt no further library calls if the function
*        does not succeed, however CAPSExit must be called in order to free
*        resources that might have been allocated during the call.
*
*   SEE ALSO
*        CAPSExit()
*
******************************************************************************
*
*/
LONG DevCAPSInit(struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSInit(CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSExit *********************************************
*
*   NAME
*        CAPSExit -- stop caps image support
*
*   SYNOPSIS
*        error = CAPSExit()
*        D0
*
*        LONG CAPSExit();
*
*   FUNCTION
*        The function closes the library and frees all resources allocated by
*        it.
*
*   RESULT
*        error - imgeOk if successful
*
*   NOTES
*        The program should not attempt any library calls after calling the
*        function if the function succeeds.
*
*   SEE ALSO
*        CAPSInit()
*
******************************************************************************
*
*/
LONG DevCAPSExit(struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSExit(CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSAddImage *****************************************
*
*   NAME
*        CAPSAddImage -- add image container
*
*   SYNOPSIS
*        id = CAPSAddImage()
*        D0
*
*        LONG CAPSAddImage();
*
*   FUNCTION
*        The function allocates an image container to be used by image
*        manipulation functions.
*
*   RESULT
*        id - a container ID greater or equal to 0 if successful
*
*   NOTES
*        A negative return value means error, usually resource related. If an
*        error occurs the result ID should not be used in further calls and
*        CAPSRemImage should not be called with the ID.
*        After freeing a container, its ID will be recycled eventually. The
*        program should not assume the ID values returned by the library.
*
*   SEE ALSO
*        CAPSRemImage()
*
*******************************************************************************
*
*/
LONG DevCAPSAddImage(struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSAddImage(CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSRemImage *****************************************
*
*   NAME
*        CAPSRemImage -- delete image container
*
*   SYNOPSIS
*        oldId = CAPSRemImage(id)
*        D0                   D0
*
*        LONG CAPSRemImage(LONG);
*
*   FUNCTION
*        The function frees an image container used by image manipulation
*        functions.
*
*   INPUTS
*        id - the container ID returned by CAPSAddImage
*
*   RESULT
*        oldId - The supplied container ID if successful, otherwise a negative
*                value
*
*   NOTES
*        A negative return value means error; usually the ID was invalid.
*        After freeing a container its ID will be recycled eventually. The
*        program should not assume the ID values returned by the library.
*
*   SEE ALSO
*        CAPSAddImage()
*
******************************************************************************
*
*/
LONG DevCAPSRemImage(LONG id __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSRemImage(id, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSLockImage ****************************************
*
*   NAME
*        CAPSLockImage -- lock image
*
*   SYNOPSIS
*        error = CAPSLockImage(id, name)
*        D0                    D0  A0
*
*        LONG CAPSLockImage(LONG, STRPTR);
*
*   FUNCTION
*        The function locks an IPF image into a container device.
*
*   INPUTS
*        id   - the container ID returned by CAPSAddImage
*        name - the name of the IPF file to be opened
*
*   RESULT
*        error - imgeOk if successful or related imge error code
*
*   NOTES
*        The image and the file is only locked if the function succeeds,
*        otherwise the container is unlocked and empty ­ CAPSUnlockImage has
*        no effect on it.
*
*   SEE ALSO
*        CAPSAddImage(), CAPSUnlockImage()
*
******************************************************************************
*
*/
LONG DevCAPSLockImage(LONG id __asm("d0"), STRPTR name __asm("a0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSLockImage(id, name, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSLockImageMemory **********************************
*
*   NAME
*        CAPSLockImageMemory -- lock image memory
*
*   SYNOPSIS
*        error = CAPSLockImageMemory(id, buffer, length, flag)
*        D0                          D0  A0      D1      D2
*
*        LONG CAPSLockImageMemory(LONG, UBYTE *, ULONG, ULONG);
*
*   FUNCTION
*        The function locks an IPF image into a container device. The image is
*        supplied in a memory buffer rather than a file reference.
*
*   INPUTS
*        id     - the container ID returned by CAPSAddImage
*        buffer - pointer to the buffer area where the IPF image in memory
*                 starts
*        length - length of the supplied buffer. It must be the same with the
*                 size of the IPF image in file format.
*        flag   - only one flag is supported, DI_LOCK_MEMREF
*
*   RESULT
*        error - imgeOk if successful or related imge error code
*
*   NOTES
*        This function is useful for retrieving images from archive files by
*        first decompressing the IPF file to a memory buffer then calling the
*        lock function.
*        The image is only locked if the function succeeds, otherwise the
*        container is unlocked and empty ­ CAPSUnlockImage has no effect on
*        it.
*
*   SEE ALSO
*        caps/capsimage.h, CAPSAddImage(), CAPSUnlockImage()
*
******************************************************************************
*
*/
LONG DevCAPSLockImageMemory(LONG id __asm("d0"), UBYTE *buffer __asm("a0"), ULONG length __asm("d1"), ULONG flag __asm("d2"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSLockImageMemory(id, buffer, length, flag, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSUnlockImage **************************************
*
*   NAME
*        CAPSUnlockImage -- unlock image
*
*   SYNOPSIS
*        error = CAPSUnlockImage(id)
*        D0                      D0
*
*        LONG CAPSUnlockImage(LONG);
*
*   FUNCTION
*        The function unlocks ­ "ejects" - an IPF image from a container
*        device.
*
*   INPUTS
*        id - the container ID returned by CAPSAddImage
*
*   RESULT
*        error - imgeOk if successful or related imge error code
*
*   NOTES
*        Any resources allocated for the image are freed and the IPF file is
*        unlocked (if a file was locked originally) once the function
*        completes.
*
*   SEE ALSO
*        CAPSAddImage(), CAPSLockImage(), CAPSLockImageMemory()
*
******************************************************************************
*
*/
LONG DevCAPSUnlockImage(LONG id __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSUnlockImage(id, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSLoadImage ****************************************
*
*   NAME
*        CAPSLoadImage -- load and decode complete image
*
*   SYNOPSIS
*        error = CAPSLoadImage(id, flag)
*        D0                    D0  D1
*
*        LONG CAPSLoadImage(LONG, ULONG);
*
*   FUNCTION
*        The function locks all unlocked tracks of an image. Already locked
*        tracks remain unchanged. The function is useful for decoding and
*        pre-caching track data for very fast retrieval.
*
*   INPUTS
*        id   - the container ID returned by CAPSAddImage
*        flag - locking flags
*
*   RESULT
*        error - imgeOk if successful or related imge error code
*
*   NOTES
*        Should only be used when memory usage is not an issue.
*
*   SEE ALSO
*        caps/capsimage.h, CAPSLockTrack(), CAPSUnlockAllTracks()
*
******************************************************************************
*
*/
LONG DevCAPSLoadImage(LONG id __asm("d0"), ULONG flag __asm("d1"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSLoadImage(id, flag, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSGetImageInfo *************************************
*
*   NAME
*        CAPSGetImageInfo -- get image information
*
*   SYNOPSIS
*        error = CAPSGetImageInfo(pi, id)
*        D0                       A0  D0
*
*        LONG CAPSGetImageInfo(struct CapsImageInfo *, LONG);
*
*   FUNCTION
*        The function reads the image information data from a locked IPF file.
*
*   INPUTS
*        pi - pointer to the CapsImageInfo that receives the image data from
*             the library
*        id - the container ID returned by CAPSAddImage
*
*   RESULT
*        error - imgeOk if successful or related imge error code
*
*   SEE ALSO
*        caps/capsimage.h, CAPSAddImage(), CAPSLockImage()
*
******************************************************************************
*
*/
LONG DevCAPSGetImageInfo(struct CapsImageInfo *pi __asm("a0"), LONG id __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSGetImageInfo(pi, id, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSLockTrack ****************************************
*
*   NAME
*        CAPSLockTrack -- load and decode track
*
*   SYNOPSIS
*        error = CAPSLockTrack(ptrackinfo, id, cylinder, head, flag)
*        D0                    A0          D0  D1        D2    D3
*
*        LONG CAPSLockTrack(APTR, LONG, ULONG, ULONG, ULONG)
*
*   FUNCTION
*        The function locks ­ reads and decodes ­ a track from a locked IPF
*        file.
*
*   INPUTS
*        ptrackinfo - pointer to the track info structure that receives the
*                     track data from the library
*        id         - the container ID returned by CAPSAddImage
*        cylinder   - the cylinder number for the track
*        head       - the head number for the track
*        flag       - locking flags
*
*                DI_LOCK_INDEX:    Track data is re-aligned in the buffer as
*                                  if it was index synced recording
*                                  originally, starting at the beginning of
*                                  the buffer. Normally track data decoded is
*                                  properly positioned as was found on the
*                                  original disk, starting at any position or
*                                  distance from the disk index.
*                                  Setting the flag results in a track
*                                  differently positioned therefore data
*                                  differently timed from the original.
*
*                DI_LOCK_ALIGN:    The decoded track data is aligned to be
*                                  word ­ 16 bits - size. If unset buffer
*                                  lengths of odd bytes can be returned for
*                                  locked tracks, as is on the disk
*                                  originally.
*                                  Setting the flag may result in a track of a
*                                  slightly different size than the original.
*
*                DI_LOCK_DENVAR:   Cell density map is generated for a
*                                  variable speed track, like Copylock or
*                                  Speedlock protection tracks.
*                                  Normally only variable density tracks
*                                  should be prompted for a cell density map
*                                  in order to save on memory and enhance
*                                  application performance.
*                                  Other cell densities might be generated and
*                                  processed faster by programming
*                                  workarounds.
*
*                DI_LOCK_DENAUTO:  Cell density map is generated for a
*                                  constant speed track.
*
*                DI_LOCK_DENNOISE: Cell density map is generated for an
*                                  unformatted track.
*
*                DI_LOCK_NOISE:    An unformatted track is algorithmically
*                                  filled with "noise patterns" if set,
*                                  otherwise no buffer is allocated for it.
*
*                DI_LOCK_NOISEREV: An unformatted track is algorithmically
*                                  filled with "noise patterns" if set,
*                                  otherwise no buffer is allocated for it.
*                                  The returned buffer will contain multiple
*                                  revolutions of different data.
*
*                DI_LOCK_MEMREF:   Only used by locking functions that accept
*                                  a memory reference as a parameter, like
*                                  CAPSLockImageMemory.
*                                  If set, the library uses the buffer
*                                  supplied by the caller of the function,
*                                  until the image is unlocked directly or
*                                  indirectly.
*                                  The program should not free the buffer
*                                  supplied with the locking call as long as
*                                  the lock is valid - i.e. not unlocked in
*                                  any way direct or indirect.
*                                  If the flag is clear the library allocates
*                                  a private data buffer and copies the
*                                  content of the supplied buffer to its
*                                  private data area. The program can free the
*                                  buffer given after the called function
*                                  returns; the private data area allocated is
*                                  automatically freed once the lock is
*                                  deleted.
*
*   RESULT
*        error - imgeOk if successful or related imge error code
*
*   NOTES
*        Subsequent calls locking the same track return the same decoded data,
*        regardless of locking flags used, until the track is unlocked
*        directly or indirectly.
*
*   SEE ALSO
*        caps/capsimage.h, CAPSAddImage(), CAPSUnlockTrack()
*
******************************************************************************
*
*/
LONG DevCAPSLockTrack(APTR ptrackinfo __asm("a0"), LONG id __asm("d0"), ULONG cylinder __asm("d1"), ULONG head __asm("d2"), ULONG flag __asm("d3"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSLockTrack(ptrackinfo, id, cylinder, head, flag, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSUnlockTrack **************************************
*
*   NAME
*        CAPSUnlockTrack -- remove track from cache
*
*   SYNOPSIS
*        error = CAPSUnlockTrack(id, cylinder, head)
*        D0                      D0  D1        D2
*
*        LONG CAPSUnlockTrack(LONG, ULONG, ULONG);
*
*   FUNCTION
*        The function unlocks ­ frees all the resources allocated ­ a track
*        from the buffers associated with a locked IPF file.
*
*   INPUTS
*        id       - the container ID returned by CAPSAddImage
*        cylinder - the cylinder number for the track
*        head     - the head number for the track
*
*   RESULT
*        error - imgeOk if successful or related imge error code
*
*   NOTES
*        In order to apply different locking to the same track it must be
*        unlocked first.
*
*   SEE ALSO
*        CAPSAddImage(), CAPSLockTrack(), CAPSUnlockAllTracks()
*
******************************************************************************
*
*/
LONG DevCAPSUnlockTrack(LONG id __asm("d0"), ULONG cylinder __asm("d1"), ULONG head __asm("d2"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSUnlockTrack(id, cylinder, head, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSUnlockAllTracks **********************************
*
*   NAME
*        CAPSUnlockAllTracks -- remove all tracks from cache
*
*   SYNOPSIS
*        error = CAPSUnlockAllTracks(id)
*        D0                          D0
*
*        LONG CAPSUnlockAllTracks(LONG);
*
*   FUNCTION
*        The function unlocks ­ frees all the resources allocated ­ all tracks
*        from the buffers associated with a locked IPF file.
*
*   INPUTS
*        id - the container ID returned by CAPSAddImage
*
*   RESULT
*        error - imgeOk if successful or related imge error code
*
*   SEE ALSO
*        CAPSAddImage(), CAPSLoadImage(), CAPSLockTrack(), CAPSUnlockTrack()
*
******************************************************************************
*
*/
LONG DevCAPSUnlockAllTracks(LONG id __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSUnlockAllTracks(id, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSGetPlatformName **********************************
*
*   NAME
*        CAPSGetPlatformName -- get platform name
*
*   SYNOPSIS
*        name = CAPSGetPlatformName(pid)
*        D0                         D0
*
*        STRPTR CAPSGetPlatformName(ULONG);
*
*   FUNCTION
*        The helper function gets the symbolic name assigned to a platform ID
*        at CapsImageInfo.platform[].
*
*   INPUTS
*        pid - the platform ID available from CapsImageInfo.platform[] array
*              members
*
*   RESULT
*        name - a pointer to the symbolic name of the platform or NULL for an
*               invalid platform ID
*
*   NOTES
*        ciipNA value should be skipped, it is an unused entry in the platform
*        array.
*
*   SEE ALSO
*        caps/capsimage.h, CAPSGetImageInfo()
*
******************************************************************************
*
*/
STRPTR DevCAPSGetPlatformName(ULONG pid __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSGetPlatformName(pid, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSGetVersionInfo ***********************************
*
*   NAME
*        CAPSGetVersionInfo -- get library version (V2)
*
*   SYNOPSIS
*        error = CAPSGetVersionInfo(pversioninfo, flag)
*        D0                         A0            D0
*
*        LONG CAPSGetVersionInfo(APTR, ULONG);
*
*   FUNCTION
*        The helper function gets the version and supported flags of the
*        library.
*
*   INPUTS
*        pversioninfo - pointer to the version info structure that receives
*                       the version information from the library
*        flag         - locking flags
*
*                DI_LOCK_TYPE
*
*   RESULT
*        error - imgeOk if successful or related imge error code
*
*   SEE ALSO
*        caps/capsimage.h
*
******************************************************************************
*
*/
LONG DevCAPSGetVersionInfo(APTR pversioninfo __asm("a0"), ULONG flag __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSGetVersionInfo(pversioninfo, flag, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSFdcGetInfo ***************************************
*
*   NAME
*        CAPSFdcGetInfo -- get fdc emulator information (V4)
*
*   SYNOPSIS
*        res = CAPSFdcGetInfo(iid, pc, ext)
*        D0                   D0   A0  D1
*
*        ULONG CAPSFdcGetInfo(LONG, struct CapsFdc *, LONG);
*
*   SEE ALSO
*        caps/fdc.h
*
******************************************************************************
*
*/
ULONG DevCAPSFdcGetInfo(LONG iid __asm("d0"), struct CapsFdc *pc __asm("a0"), LONG ext __asm("d1"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSFdcGetInfo(iid, pc, ext, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSFdcInit ******************************************
*
*   NAME
*        CAPSFdcInit -- initialise fdc emulator (V4)
*
*   SYNOPSIS
*        error = CAPSFdcInit(pc)
*        D0                  A0
*
*        LONG CAPSFdcInit(struct CapsFdc *);
*
*   SEE ALSO
*        caps/fdc.h
*
******************************************************************************
*
*/
LONG DevCAPSFdcInit(struct CapsFdc *pc __asm("a0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSFdcInit(pc, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSFdcReset *****************************************
*
*   NAME
*        CAPSFdcReset -- fdc hardware reset (V4)
*
*   SYNOPSIS
*        CAPSFdcReset(pc)
*                     A0
*
*        VOID CAPSFdcReset(struct CapsFdc *);
*
*   SEE ALSO
*        caps/fdc.h
*
******************************************************************************
*
*/
VOID DevCAPSFdcReset(struct CapsFdc *pc __asm("a0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	CAPSFdcReset(pc, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSFdcEmulate ***************************************
*
*   NAME
*        CAPSFdcEmulate -- fdc emulator (V4)
*
*   SYNOPSIS
*        CAPSFdcEmulate(pc)
*                       A0
*
*        VOID CAPSFdcEmulate(struct CapsFdc *);
*
*   SEE ALSO
*        caps/fdc.h
*
******************************************************************************
*
*/
VOID DevCAPSFdcEmulate(struct CapsFdc *pc __asm("a0"), ULONG cyclecnt __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	CAPSFdcEmulate(pc, cyclecnt, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSFdcRead ******************************************
*
*   NAME
*        CAPSFdcRead -- read from fdc register (V4)
*
*   SYNOPSIS
*        data = CAPSFdcRead(pc, address)
*        D0                 A0  D0
*
*        ULONG CAPSFdcRead(struct CapsFdc *, ULONG);
*
*   SEE ALSO
*        caps/fdc.h
*
******************************************************************************
*
*/
ULONG DevCAPSFdcRead(struct CapsFdc *pc __asm("a0"), ULONG address __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSFdcRead(pc, address, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSFdcWrite *****************************************
*
*   NAME
*        CAPSFdcWrite -- write to fdc register (V4)
*
*   SYNOPSIS
*        CAPSFdcWrite(pc, address, data)
*                     A0  D0       D1
*
*        VOID CAPSFdcWrite(struct CapsFdc *, ULONG, ULONG);
*
*   SEE ALSO
*        caps/fdc.h
*
******************************************************************************
*
*/
VOID DevCAPSFdcWrite(struct CapsFdc *pc __asm("a0"), ULONG address __asm("d0"), ULONG data __asm("d1"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	CAPSFdcWrite(pc, address, data, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSFdcInvalidateTrack *******************************
*
*   NAME
*        CAPSFdcInvalidateTrack -- invalidate track data, next read cycle will
*                                  request a track change callback as well as
*                                  re-lock the stream (V4)
*
*   SYNOPSIS
*        error = CAPSFdcInvalidateTrack(pc, drive)
*        D0                             A0  D0
*
*        LONG CAPSFdcInvalidateTrack(struct CapsFdc *, LONG);
*
*   SEE ALSO
*        caps/fdc.h
*
******************************************************************************
*
*/
LONG DevCAPSFdcInvalidateTrack(struct CapsFdc *pc __asm("a0"), LONG drive __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSFdcInvalidateTrack(pc, drive, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSFormatDataToMFM **********************************
*
*   NAME
*        CAPSFormatDataToMFM -- convert track data to mfm (V4)
*
*   SYNOPSIS
*        error = CAPSFormatDataToMFM(pformattrack, flag)
*        D0                          A0            D0
*
*        LONG CAPSFormatDataToMFM(APTR, ULONG);
*
*   SEE ALSO
*        caps/fdc.h
*
******************************************************************************
*
*/
LONG DevCAPSFormatDataToMFM(APTR pformattrack __asm("a0"), ULONG flag __asm("d0"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSFormatDataToMFM(pformattrack, flag, CapsImageBase->cib_A4);
}

/****** capsimage.device/CAPSGetInfo ******************************************
*
*   NAME
*        CAPSGetInfo -- get various information after decoding (V4)
*
*   SYNOPSIS
*        error = CAPSGetInfo(pinfo, id, cylinder, head, inftype, infid)
*        D0                  A0     D0  D1        D2    D3       D4
*
*        LONG CAPSGetInfo(APTR, LONG, ULONG, ULONG, ULONG, ULONG);
*
*   SEE ALSO
*        caps/capsimage.h
*
******************************************************************************
*
*/
LONG DevCAPSGetInfo(APTR pinfo __asm("a0"), LONG id __asm("d0"), ULONG cylinder __asm("d1"), ULONG head __asm("d2"), ULONG inftype __asm("d3"), ULONG infid __asm("d4"), struct CapsImageBase *CapsImageBase __asm("a6"))
{
	return CAPSGetInfo(pinfo, id, cylinder, head, inftype, infid, CapsImageBase->cib_A4);
}

