// Implement mprotect() for Win32
// Copyright (C) 2000, Brian King
// GNU Public License

#include <float.h>

#include "sysconfig.h"
#include "sysdeps.h"
#ifdef WINDOWS
#include "mman_windows.h"
#endif
//#include "sys/mman.h"
#include "memory.h"
#include "options.h"
#include "autoconf.h"
//#include "win32.h"


//#define NATMEM_OFFSET natmem_offset;
//#undef NATMEM_OFFSET
// just some value - FS
//#define MAX_SHMID 128


#if defined(NATMEM_OFFSET)

/* JIT can access few bytes outside of memory block of it executes code at the very end of memory block */
#define BARRIER 32

#define MAXZ3MEM 0x7F000000
#define MAXZ3MEM64 0xF0000000

static struct shmid_ds shmids[MAX_SHMID];
uae_u8 *natmem_offset, *natmem_offset_end;
static uae_u8 *p96mem_offset;
static int p96mem_size;
//static SYSTEM_INFO si;
int maxmem;

/*
static void *VirtualAlloc(void *address, int size, int type, int protect) {
    STUB("address=%p size=%d", address, size);
    if (address == NULL) {
        address = malloc(size);
    }
    return address;
}

static int VirtualFree(void *address, int size, int type) {
        STUB("address=%p size=%d", address, size);
    return 1;
}

static int GetLastError() {
    return 0;
}
*/

static uae_u8 *virtualallocwithlock (LPVOID addr, SIZE_T size, DWORD allocationtype, DWORD protect)
{
	uae_u8 *p = (uae_u8*)VirtualAlloc (addr, size, allocationtype, protect);
	return p;
}
static void virtualfreewithlock (LPVOID addr, SIZE_T size, DWORD freetype)
{
	VirtualFree(addr, size, freetype);
}

void cache_free (uae_u8 *cache)
{
	virtualfreewithlock (cache, 0, MEM_RELEASE);
}

uae_u8 *cache_alloc (int size)
{
	return virtualallocwithlock (NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
}

#if 0
static void setworkingset(void)
{
	typedef BOOL (CALLBACK* SETPROCESSWORKINGSETSIZE)(HANDLE,SIZE_T,SIZE_T);
	SETPROCESSWORKINGSETSIZE pSetProcessWorkingSetSize;
	pSetProcessWorkingSetSize = (SETPROCESSWORKINGSETSIZE)GetProcAddress(GetModuleHandle("kernal32.dll", "GetProcessWorkingSetSize");
	if (!pSetProcessWorkingSetSize)
		return;
	pSetProcessWorkingSetSize(GetCurrentProcess (),
		);
}
#endif

static uae_u32 lowmem (void)
{
	uae_u32 change = 0;
	if (currprefs.z3fastmem_size + currprefs.z3fastmem2_size + currprefs.z3chipmem_size >= 8 * 1024 * 1024) {
		if (currprefs.z3fastmem2_size) {
			change = currprefs.z3fastmem2_size;
			currprefs.z3fastmem2_size = 0;
		} else if (currprefs.z3chipmem_size) {
			if (currprefs.z3chipmem_size <= 16 * 1024 * 1024) {
				change = currprefs.z3chipmem_size;
				currprefs.z3chipmem_size = 0;
			} else {
				change = currprefs.z3chipmem_size / 2;
				currprefs.z3chipmem_size /= 2;
			}
		} else {
			change = currprefs.z3fastmem_size - currprefs.z3fastmem_size / 4;
			currprefs.z3fastmem2_size = changed_prefs.z3fastmem2_size = currprefs.z3fastmem_size / 4;
			currprefs.z3fastmem_size /= 2;
			changed_prefs.z3fastmem_size = currprefs.z3fastmem_size;
		}
	} else if (currprefs.rtgmem_type && currprefs.rtgmem_size >= 1 * 1024 * 1024) {
		change = currprefs.rtgmem_size - currprefs.rtgmem_size / 2;
		currprefs.rtgmem_size /= 2;
		changed_prefs.rtgmem_size = currprefs.rtgmem_size;
	}
	if (currprefs.z3fastmem2_size < 128 * 1024 * 1024)
		currprefs.z3fastmem2_size = changed_prefs.z3fastmem2_size = 0;
	return change;
}

int mman_GetWriteWatch (PVOID lpBaseAddress, SIZE_T dwRegionSize, PVOID *lpAddresses, PULONG_PTR lpdwCount, PULONG lpdwGranularity)
{
    STUB("");
#if 0
    return GetWriteWatch (0, lpBaseAddress, dwRegionSize, lpAddresses, lpdwCount, lpdwGranularity);
#endif
    return 0;
}
void mman_ResetWatch (PVOID lpBaseAddress, SIZE_T dwRegionSize)
{
    STUB("");
#if 0
	if (ResetWriteWatch (lpBaseAddress, dwRegionSize))
		write_log (_T("ResetWriteWatch() failed, %d\n"), GetLastError ());
#endif
}

static uae_u64 size64;
//typedef BOOL (CALLBACK* GLOBALMEMORYSTATUSEX)(LPMEMORYSTATUSEX);

void preinit_shm (void)
{
    printf("\n\n\npreinit_shm\n\n\n\n");
	int i;
	uae_u64 total64;
	uae_u64 totalphys64;
#if 0
	MEMORYSTATUS memstats;
	GLOBALMEMORYSTATUSEX pGlobalMemoryStatusEx;
	MEMORYSTATUSEX memstatsex;
#endif
	uae_u32 max_allowed_mman;

	if (natmem_offset)
		VirtualFree (natmem_offset, 0, MEM_RELEASE);
	natmem_offset = NULL;
	if (p96mem_offset)
		VirtualFree (p96mem_offset, 0, MEM_RELEASE);
	p96mem_offset = NULL;

#if 0
	GetSystemInfo (&si);
#endif
	max_allowed_mman = 1536;
#if 0
	if (os_64bit)
		max_allowed_mman = 2048;

	memstats.dwLength = sizeof(memstats);
	GlobalMemoryStatus(&memstats);
	totalphys64 = memstats.dwTotalPhys;
	total64 = (uae_u64)memstats.dwAvailPageFile + (uae_u64)memstats.dwTotalPhys;
	pGlobalMemoryStatusEx = (GLOBALMEMORYSTATUSEX)GetProcAddress (GetModuleHandle (_T("kernel32.dll")), "GlobalMemoryStatusEx");
	if (pGlobalMemoryStatusEx) {
		memstatsex.dwLength = sizeof (MEMORYSTATUSEX);
		if (pGlobalMemoryStatusEx(&memstatsex)) {
			totalphys64 = memstatsex.ullTotalPhys;
			total64 = memstatsex.ullAvailPageFile + memstatsex.ullTotalPhys;
		}
	}
#endif
	// just using some fake values here
    totalphys64 = 1024 * 1024 * 1024;
    total64 = 1024 * 1024 * 1024;

	size64 = total64;
	if (maxmem < 0)
		size64 = MAXZ3MEM;
	else if (maxmem > 0)
		size64 = maxmem * 1024 * 1024;
#if 0
	if (os_64bit) {
		if (size64 > MAXZ3MEM64)
			size64 = MAXZ3MEM64;
	} else {
#endif
		if (size64 > MAXZ3MEM)
			size64 = MAXZ3MEM;
#if 0
	}
#endif
	if (size64 < 8 * 1024 * 1024)
		size64 = 8 * 1024 * 1024;
	if (max_allowed_mman * 1024 * 1024 > size64)
		max_allowed_mman = size64 / (1024 * 1024);
	max_z3fastmem = max_allowed_mman * 1024 * 1024;
	if (max_z3fastmem < 512 * 1024 * 1024)
		max_z3fastmem = 512 * 1024 * 1024;

	shm_start = 0;
	for (i = 0; i < MAX_SHMID; i++) {
		shmids[i].attached = 0;
		shmids[i].key = -1;
		shmids[i].size = 0;
		shmids[i].addr = NULL;
		shmids[i].name[0] = 0;
	}

	write_log (_T("Max Z3FastRAM %dM. Total physical RAM %uM\n"), max_z3fastmem >> 20, totalphys64 >> 20);
	canbang = 1;
}

static void resetmem (void)
{
    STUB("\n\n\nFIXME\n\n\n");
#if 0
	int i;

	if (!shm_start)
		return;
	for (i = 0; i < MAX_SHMID; i++) {
		struct shmid_ds *s = &shmids[i];
		int size = s->size;
		uae_u8 *shmaddr;
		uae_u8 *result;

		if (!s->attached)
			continue;
		if (!s->natmembase)
			continue;
		shmaddr = natmem_offset + ((uae_u8*)s->attached - (uae_u8*)s->natmembase);
		result = virtualallocwithlock (shmaddr, size, MEM_COMMIT, s->mode);
		if (result != shmaddr)
			write_log (_T("NATMEM: realloc(%p,%d,%d) failed, err=%x\n"), shmaddr, size, s->mode, GetLastError ());
		else
			write_log (_T("NATMEM: rellocated(%p,%d,%s)\n"), shmaddr, size, s->name);
	}
#endif
}

static ULONG getz2rtgaddr (void)
{
	ULONG start;
	start = currprefs.fastmem_size;
	while (start & (currprefs.rtgmem_size - 1) && start < 4 * 1024 * 1024)
		start += 1024 * 1024;
	return start + 2 * 1024 * 1024;
}

static int dwPageSize = 4096;

int init_shm (void)
{
	uae_u32 size, totalsize, z3size, natmemsize;
	uae_u32 rtgbarrier, z3chipbarrier, rtgextra;
	int rounds = 0;
	ULONG z3rtgmem_size = currprefs.rtgmem_type ? currprefs.rtgmem_size : 0;

	printf("\n\n\ninit_shm\n\n\n\n");
	printf("\n\n\nFIXME: retrieve dwPageSize properly?\n\n\n\n");
restart:

	for (;;) {
		int lowround = 0;
		uae_u8 *blah = NULL;
		if (rounds > 0)
			write_log (_T("NATMEM: retrying %d..\n"), rounds);
		rounds++;
		if (natmem_offset)
			VirtualFree (natmem_offset, 0, MEM_RELEASE);
		natmem_offset = NULL;
		natmem_offset_end = NULL;
		canbang = 0;

		z3size = 0;
		size = 0x1000000;
		rtgextra = 0;
		z3chipbarrier = 0;
		rtgbarrier = dwPageSize;
		if (currprefs.cpu_model >= 68020)
			size = 0x10000000;
		if (currprefs.z3fastmem_size || currprefs.z3fastmem2_size || currprefs.z3chipmem_size) {
			z3size = currprefs.z3fastmem_size + currprefs.z3fastmem2_size + currprefs.z3chipmem_size + (currprefs.z3fastmem_start - 0x10000000);
			if (z3rtgmem_size) {
				rtgbarrier = 16 * 1024 * 1024 - ((currprefs.z3fastmem_size + currprefs.z3fastmem2_size) & 0x00ffffff);
			}
			if (currprefs.z3chipmem_size && (currprefs.z3fastmem_size || currprefs.z3fastmem2_size))
				z3chipbarrier = 16 * 1024 * 1024;
		} else {
			rtgbarrier = 0;
		}
		totalsize = size + z3size + z3rtgmem_size;
		while (totalsize > size64) {
			int change = lowmem ();
			if (!change)
				return 0;
			write_log (_T("NATMEM: %d, %dM > %dM = %dM\n"), ++lowround, totalsize >> 20, size64 >> 20, (totalsize - change) >> 20);
			totalsize -= change;
		}
		if ((rounds > 1 && totalsize < 0x10000000) || rounds > 20) {
			write_log (_T("NATMEM: No special area could be allocated (3)!\n"));
			return 0;
		}
		natmemsize = size + z3size;

		if (z3rtgmem_size) {
			rtgextra = dwPageSize;
		} else {
			rtgbarrier = 0;
			rtgextra = 0;
		}
		blah = (uae_u8*)VirtualAlloc (NULL, natmemsize + rtgbarrier + z3chipbarrier + z3rtgmem_size + rtgextra + 16 * dwPageSize, MEM_RESERVE, PAGE_READWRITE);
		if (blah) {
			natmem_offset = blah;
			break;
		}
		write_log (_T("NATMEM: %dM area failed to allocate, err=%d (Z3=%dM,RTG=%dM)\n"),
			natmemsize >> 20, GetLastError (), (currprefs.z3fastmem_size + currprefs.z3fastmem2_size + currprefs.z3chipmem_size) >> 20, z3rtgmem_size >> 20);
		if (!lowmem ()) {
			write_log (_T("NATMEM: No special area could be allocated (2)!\n"));
			return 0;
		}
	}

#if 0
	// FIXME:
	natmemsize = 32 * 1024 * 1024;
	natmem_offset = (uae_u8 *) malloc(natmemsize);
    natmem_offset_end = NULL;
#endif

	p96mem_size = z3rtgmem_size;
	if (currprefs.rtgmem_size && currprefs.rtgmem_type) {
		VirtualFree (natmem_offset, 0, MEM_RELEASE);
		if (!VirtualAlloc (natmem_offset, natmemsize + rtgbarrier + z3chipbarrier, MEM_RESERVE, PAGE_READWRITE)) {
			write_log (_T("VirtualAlloc() part 2 error %d. RTG disabled.\n"), GetLastError ());
			currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
			rtgbarrier = dwPageSize;
			rtgextra = 0;
			goto restart;
		}
		p96mem_offset = (uae_u8*)VirtualAlloc (natmem_offset + natmemsize + rtgbarrier + z3chipbarrier, p96mem_size + rtgextra,
			MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE);
		if (!p96mem_offset) {
			currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
			z3rtgmem_size = 0;
			write_log (_T("NATMEM: failed to allocate special Picasso96 GFX RAM, err=%d\n"), GetLastError ());
		}
	} else if (currprefs.rtgmem_size && !currprefs.rtgmem_type) {
		// This so annoying..
		VirtualFree (natmem_offset, 0, MEM_RELEASE);
		// Chip + Z2Fast
		if (!VirtualAlloc (natmem_offset, 2 * 1024 * 1024 + currprefs.fastmem_size, MEM_RESERVE, PAGE_READWRITE)) {
			write_log (_T("VirtualAlloc() part 2 error %d. RTG disabled.\n"), GetLastError ());
			currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
			rtgbarrier = dwPageSize;
			rtgextra = 0;
			goto restart;
		}
		// After RTG
		if (!VirtualAlloc (natmem_offset + 2 * 1024 * 1024 + 8 * 1024 * 1024,
			natmemsize + rtgbarrier + z3chipbarrier - (2 * 1024 * 1024 + 8 * 1024 * 1024) + dwPageSize, MEM_RESERVE, PAGE_READWRITE)) {
			write_log (_T("VirtualAlloc() part 2 error %d. RTG disabled.\n"), GetLastError ());
			currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
			rtgbarrier = dwPageSize;
			rtgextra = 0;
			goto restart;
		}
		// RTG
		p96mem_offset = (uae_u8*)VirtualAlloc (natmem_offset + getz2rtgaddr (), 10 * 1024 * 1024 - getz2rtgaddr (),
			MEM_RESERVE | MEM_WRITE_WATCH, PAGE_READWRITE);
		if (!p96mem_offset) {
			currprefs.rtgmem_size = changed_prefs.rtgmem_size = 0;
			write_log (_T("NATMEM: failed to allocate special Picasso96 GFX RAM, err=%d\n"), GetLastError ());
		}
	}

	if (!natmem_offset) {
		write_log (_T("NATMEM: No special area could be allocated! (1) err=%d\n"), GetLastError ());
	} else {
		write_log (_T("NATMEM: Our special area: 0x%p-0x%p (%08x %dM)\n"),
			natmem_offset, (uae_u8*)natmem_offset + natmemsize,
			natmemsize, natmemsize >> 20);
		if (currprefs.rtgmem_size)
			write_log (_T("NATMEM: P96 special area: 0x%p-0x%p (%08x %dM)\n"),
			p96mem_offset, (uae_u8*)p96mem_offset + currprefs.rtgmem_size,
			currprefs.rtgmem_size, currprefs.rtgmem_size >> 20);
		canbang = 1;
		if (p96mem_size)
			natmem_offset_end = p96mem_offset + p96mem_size;
		else
			natmem_offset_end = natmem_offset + natmemsize;
	}

	resetmem ();

	return canbang;
}

void mapped_free (uae_u8 *mem)
{
	shmpiece *x = shm_start;

	if (mem == filesysory) {
		while(x) {
			if (mem == x->native_address) {
				int shmid = x->id;
				shmids[shmid].key = -1;
				shmids[shmid].name[0] = '\0';
				shmids[shmid].size = 0;
				shmids[shmid].attached = 0;
				shmids[shmid].mode = 0;
				shmids[shmid].natmembase = 0;
			}
			x = x->next;
		}
		return;
	}

	while(x) {
		if(mem == x->native_address)
			shmdt (x->native_address);
		x = x->next;
	}
	x = shm_start;
	while(x) {
		struct shmid_ds blah;
		if (mem == x->native_address) {
			if (shmctl (x->id, IPC_STAT, &blah) == 0)
				shmctl (x->id, IPC_RMID, &blah);
		}
		x = x->next;
	}
}

static key_t get_next_shmkey (void)
{
	key_t result = -1;
	int i;
	for (i = 0; i < MAX_SHMID; i++) {
		if (shmids[i].key == -1) {
			shmids[i].key = i;
			result = i;
			break;
		}
	}
	return result;
}

STATIC_INLINE key_t find_shmkey (key_t key)
{
	int result = -1;
	if(shmids[key].key == key) {
		result = key;
	}
	return result;
}

int mprotect (void *addr, size_t len, int prot)
{
	int result = 0;
	return result;
}

void *shmat (int shmid, void *shmaddr, int shmflg)
{
	void *result = (void *)-1;
	BOOL got = FALSE;
	int p96special = FALSE;
	DWORD protect = PAGE_READWRITE;

#ifdef NATMEM_OFFSET
	unsigned int size = shmids[shmid].size;

	if (shmids[shmid].attached)
		return shmids[shmid].attached;

	if ((uae_u8*)shmaddr < natmem_offset) {
		if(!_tcscmp (shmids[shmid].name, _T("chip"))) {
			shmaddr=natmem_offset;
			got = TRUE;
			if (getz2endaddr () <= 2 * 1024 * 1024 || currprefs.chipmem_size < 2 * 1024 * 1024)
				size += BARRIER;
		}
		if(!_tcscmp (shmids[shmid].name, _T("kick"))) {
			shmaddr=natmem_offset + 0xf80000;
			got = TRUE;
			size += BARRIER;
		}
		if(!_tcscmp (shmids[shmid].name, _T("rom_a8"))) {
			shmaddr=natmem_offset + 0xa80000;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("rom_e0"))) {
			shmaddr=natmem_offset + 0xe00000;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("rom_f0"))) {
			shmaddr=natmem_offset + 0xf00000;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("rtarea"))) {
			shmaddr=natmem_offset + rtarea_base;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("fast"))) {
			shmaddr=natmem_offset + 0x200000;
			got = TRUE;
			if (!(currprefs.rtgmem_size && !currprefs.rtgmem_type))
				size += BARRIER;
		}
		if(!_tcscmp (shmids[shmid].name, _T("z2_gfx"))) {
			ULONG start = getz2rtgaddr ();
			got = TRUE;
			p96special = TRUE;
			shmaddr = natmem_offset + start;
			p96ram_start = start;
			if (start + currprefs.rtgmem_size < 10 * 1024 * 1024)
				size += BARRIER;
		}
		if(!_tcscmp (shmids[shmid].name, _T("ramsey_low"))) {
			shmaddr=natmem_offset + a3000lmem_start;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("ramsey_high"))) {
			shmaddr=natmem_offset + a3000hmem_start;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("z3"))) {
			shmaddr=natmem_offset + z3fastmem_start;
			if (!currprefs.z3fastmem2_size)
				size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("z3_2"))) {
			shmaddr=natmem_offset + z3fastmem_start + currprefs.z3fastmem_size;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("z3_chip"))) {
			shmaddr=natmem_offset + z3chipmem_start;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("z3_gfx"))) {
			got = TRUE;
			p96special = TRUE;
			p96ram_start = p96mem_offset - natmem_offset;
			shmaddr = natmem_offset + p96ram_start;
			size += BARRIER;
		}
		if(!_tcscmp (shmids[shmid].name, _T("bogo"))) {
			shmaddr=natmem_offset+0x00C00000;
			got = TRUE;
			if (currprefs.bogomem_size <= 0x100000)
				size += BARRIER;
		}
		if(!_tcscmp (shmids[shmid].name, _T("filesys"))) {
			static uae_u8 *filesysptr;
			if (filesysptr == NULL)
				filesysptr = xcalloc (uae_u8, size);
			result = filesysptr;
			shmids[shmid].attached = result;
			return result;
		}
		if(!_tcscmp (shmids[shmid].name, _T("custmem1"))) {
			shmaddr=natmem_offset + currprefs.custom_memory_addrs[0];
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("custmem2"))) {
			shmaddr=natmem_offset + currprefs.custom_memory_addrs[1];
			got = TRUE;
		}

		if(!_tcscmp (shmids[shmid].name, _T("hrtmem"))) {
			shmaddr=natmem_offset + 0x00a10000;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("arhrtmon"))) {
			shmaddr=natmem_offset + 0x00800000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("xpower_e2"))) {
			shmaddr=natmem_offset + 0x00e20000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("xpower_f2"))) {
			shmaddr=natmem_offset + 0x00f20000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("nordic_f0"))) {
			shmaddr=natmem_offset + 0x00f00000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("nordic_f4"))) {
			shmaddr=natmem_offset + 0x00f40000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("nordic_f6"))) {
			shmaddr=natmem_offset + 0x00f60000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp(shmids[shmid].name, _T("superiv_b0"))) {
			shmaddr=natmem_offset + 0x00b00000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("superiv_d0"))) {
			shmaddr=natmem_offset + 0x00d00000;
			size += BARRIER;
			got = TRUE;
		}
		if(!_tcscmp (shmids[shmid].name, _T("superiv_e0"))) {
			shmaddr=natmem_offset + 0x00e00000;
			size += BARRIER;
			got = TRUE;
		}
	}
#endif

	if (shmids[shmid].key == shmid && shmids[shmid].size) {
		shmids[shmid].mode = protect;
		shmids[shmid].natmembase = natmem_offset;
		if (shmaddr)
			virtualfreewithlock (shmaddr, size, MEM_DECOMMIT);
		result = virtualallocwithlock (shmaddr, size, MEM_COMMIT, protect);
		if (result == NULL)
			virtualfreewithlock (shmaddr, 0, MEM_DECOMMIT);
		result = virtualallocwithlock (shmaddr, size, MEM_COMMIT, protect);
		if (result == NULL) {
			result = (void*)-1;
			write_log (_T("VirtualAlloc %08X - %08X %x (%dk) failed %d\n"),
				(uae_u8*)shmaddr - natmem_offset, (uae_u8*)shmaddr - natmem_offset + size,
				size, size >> 10, GetLastError ());
		} else {
			shmids[shmid].attached = result;
			write_log (_T("VirtualAlloc %08X - %08X %x (%dk) ok (%08X)%s\n"),
				(uae_u8*)shmaddr - natmem_offset, (uae_u8*)shmaddr - natmem_offset + size,
				size, size >> 10, shmaddr, p96special ? _T(" P96") : _T(""));
		}
	}
	return result;
}

int shmdt (const void *shmaddr)
{
	return 0;
}

int shmget (key_t key, size_t size, int shmflg, const TCHAR *name)
{
	int result = -1;

	if((key == IPC_PRIVATE) || ((shmflg & IPC_CREAT) && (find_shmkey (key) == -1))) {
		write_log (_T("shmget of size %d (%dk) for %s\n"), size, size >> 10, name);
		if ((result = get_next_shmkey ()) != -1) {
			shmids[result].size = size;
			_tcscpy (shmids[result].name, name);
		} else {
			result = -1;
		}
	}
	return result;
}

int shmctl (int shmid, int cmd, struct shmid_ds *buf)
{
	int result = -1;

	if ((find_shmkey (shmid) != -1) && buf) {
		switch (cmd)
		{
		case IPC_STAT:
			*buf = shmids[shmid];
			result = 0;
			break;
		case IPC_RMID:
			VirtualFree (shmids[shmid].attached, shmids[shmid].size, MEM_DECOMMIT);
			shmids[shmid].key = -1;
			shmids[shmid].name[0] = '\0';
			shmids[shmid].size = 0;
			shmids[shmid].attached = 0;
			shmids[shmid].mode = 0;
			result = 0;
			break;
		}
	}
	return result;
}

#endif

#if 0

int isinf (double x)
{
	const int nClass = _fpclass (x);
	int result;
	if (nClass == _FPCLASS_NINF || nClass == _FPCLASS_PINF)
		result = 1;
	else
		result = 0;
	return result;
}

#endif
