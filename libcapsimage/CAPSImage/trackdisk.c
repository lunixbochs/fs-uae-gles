#include "config.h"
#include "capsimage_protos.h"
#include <inline/capsimage.h>
#include <utility/utility.h>
#include <dos/dostags.h>
#include <sys/cdefs.h>
#include <proto/rexxsyslib.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* global constructor/destructor lists */
typedef VOID (*FPTR)(VOID);

extern FPTR __CTOR_LIST__[];
extern FPTR __DTOR_LIST__[];

/* alias declarations for const library bases */
__indr_reference(__SysBase, SysBase);
__indr_reference(__DOSBase, DOSBase);
__indr_reference(__UtilityBase, UtilityBase);
__indr_reference(__RexxSysBase, RexxSysBase);

/* these variables must reside in the data section */
struct ExecBase *__SysBase = NULL;
struct DosLibrary *__DOSBase = NULL;
struct UtilityBase *__UtilityBase = NULL;
struct RxsLib *__RexxSysBase = NULL;
APTR MemPool = NULL;

/* call global constructors in data section pointed to by a4 */
VOID SetupGlobals(VOID)
{
	ULONG i;

	MemPool = CreatePool(MEMF_PUBLIC, CAPS_PUDDLESIZE, CAPS_THRESHOLDSIZE);

	for (i = (ULONG)__CTOR_LIST__[0]; i >= 1; i--)
		__CTOR_LIST__[i]();
}

/* call global destructors in data section pointed to by a4 */
VOID CleanupGlobals(VOID)
{
	FPTR *p = __DTOR_LIST__+1;

	while (*p)
		(*(p++))();

	DeletePool(MemPool);
}

/* called by DevInitFunction() when device is loaded into memory */
BOOL __saveds OpenLibs(struct ExecBase *SysBase __asm("a6"))
{
	__SysBase = SysBase;

	if (!(__DOSBase = (struct DosLibrary *)OpenLibrary(DOSNAME, 36)))
		return FALSE;

	if (!(__UtilityBase = (struct UtilityBase *)OpenLibrary(UTILITYNAME, 36)))
		return FALSE;

	if (!(__RexxSysBase = (struct RxsLib *)OpenLibrary(RXSNAME, 0)))
		return FALSE;

	return TRUE;
}

/* called by DevExpunge() when device is about to be flushed */
VOID __saveds CloseLibs(VOID)
{
	CloseLibrary((struct Library *)__RexxSysBase);
	CloseLibrary((struct Library *)__UtilityBase);
	CloseLibrary((struct Library *)__DOSBase);
}

/* called by DevOpen() to initialize a unit */
VOID __saveds InitUnit(ULONG unitNumber, struct CapsImageBase *CapsImageBase)
{
	struct CapsImageUnit *CapsImageUnit;

	if ((CapsImageUnit = AllocVec(sizeof(struct CapsImageUnit), MEMF_CLEAR|MEMF_PUBLIC)))
	{
		/* set port to PA_IGNORE until the new task has a chance to set it up */
		CapsImageUnit->ciu_Unit.tdu_Unit.unit_MsgPort.mp_Node.ln_Type = NT_MSGPORT;
		CapsImageUnit->ciu_Unit.tdu_Unit.unit_MsgPort.mp_Node.ln_Name = CapsImageBase->cib_Dev.dd_Library.lib_Node.ln_Name;
		CapsImageUnit->ciu_Unit.tdu_Unit.unit_MsgPort.mp_Flags = PA_IGNORE;
		NewList(&CapsImageUnit->ciu_Unit.tdu_Unit.unit_MsgPort.mp_MsgList);

		CapsImageUnit->ciu_UnitNum = unitNumber;

		/* start the unit process */
		if ((CapsImageUnit->ciu_Process = CreateNewProcTags(
			NP_Entry, (ULONG)UnitProcess,
			NP_Name, (ULONG)CapsImageBase->cib_Dev.dd_Library.lib_Node.ln_Name,
			NP_Priority, CAPS_PRI,
			TAG_DONE)))
		{
			/* add an image container */
			CapsImageUnit->ciu_ContainerID = CAPSAddImage();
			/* send the unit pointer */
			CapsImageUnit->ciu_StartupMessage.sm_Unit = CapsImageUnit;
			PutMsg(&CapsImageUnit->ciu_Process->pr_MsgPort, (struct Message *)&CapsImageUnit->ciu_StartupMessage);

			CapsImageBase->cib_Units[unitNumber] = CapsImageUnit;
		}
		else
			FreeVec(CapsImageUnit);
	}
}

/* called by DevClose() to remove a unit */
VOID __saveds ExpungeUnit(struct CapsImageUnit *CapsImageUnit, struct CapsImageBase *CapsImageBase)
{
	struct Process *Process = CapsImageUnit->ciu_Process;

	/* signal the unit process to exit */
	CapsImageUnit->ciu_Process = (struct Process *)SysBase->ThisTask;
	SetSignal(0, SIGF_SINGLE);
	Signal((struct Task *)Process, SIGBREAKF_CTRL_F);
	Wait(SIGF_SINGLE);

	/* remove the image container */
	CAPSRemImage(CapsImageUnit->ciu_ContainerID);

	CapsImageBase->cib_Units[CapsImageUnit->ciu_UnitNum] = NULL;

	FreeVec(CapsImageUnit);
}

/* the unit process */
VOID __saveds UnitProcess(VOID)
{
	struct Process *Process = (struct Process *)SysBase->ThisTask;
	struct StartupMessage *StartupMessage;
	struct CapsImageUnit *CapsImageUnit;

	/* this can't fail for a newly created process */
	BYTE unitSignal = AllocSignal(-1);
	BYTE rexxSignal = AllocSignal(-1);

	/* get the unit pointer */
	WaitPort(&Process->pr_MsgPort);
	StartupMessage = (struct StartupMessage *)GetMsg(&Process->pr_MsgPort);
	CapsImageUnit = StartupMessage->sm_Unit;

	/* make the message port live */
	CapsImageUnit->ciu_Unit.tdu_Unit.unit_MsgPort.mp_SigBit = unitSignal;
	CapsImageUnit->ciu_Unit.tdu_Unit.unit_MsgPort.mp_SigTask = Process;
	CapsImageUnit->ciu_Unit.tdu_Unit.unit_MsgPort.mp_Flags = PA_SIGNAL;

	while (TRUE)
	{
		ULONG signals = Wait((1L << unitSignal) | SIGBREAKF_CTRL_F);

		if (signals & SIGBREAKF_CTRL_F)
			break;

		//if (signals & (1L << unitSignal))
	}

	/* cleanup and signal the main process */
	Forbid();
	FreeSignal(rexxSignal);
	FreeSignal(unitSignal);
	Signal((struct Task *)CapsImageUnit->ciu_Process, SIGF_SINGLE);
}

