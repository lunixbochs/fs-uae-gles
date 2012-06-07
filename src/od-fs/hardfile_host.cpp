/**
  * UAE - The Un*x Amiga Emulator
  *
  * Hardfile emulation for *nix systems
  *
  * Copyright 2003-2006 Richard Drummond
  * Copyright 2008-2010 Mustafa TUFAN
  * Based on hardfile_win32.c
  */

#include "sysconfig.h"
#include "sysdeps.h"

#include "filesys.h"
#include "zfile.h"

#define hfd_log write_log

//#define HDF_DEBUG
#ifdef  HDF_DEBUG
#define DEBUG_LOG write_log ( "%s: ", __func__); write_log
#else
#define DEBUG_LOG(...) do ; while(0)
#endif

static int usefloppydrives = 0;

struct hardfilehandle
{
	int zfile;
	struct zfile *zf;
	FILE *h;
};

struct uae_driveinfo {
	char vendor_id[128];
	char product_id[128];
	char product_rev[128];
	char product_serial[128];
	char device_name[2048];
	char device_path[2048];
	uae_u64 size;
	uae_u64 offset;
	int bytespersector;
	int removablemedia;
	int nomedia;
	int dangerous;
	int readonly;
};

#define HDF_HANDLE_WIN32 1
#define HDF_HANDLE_ZFILE 2
#define HDF_HANDLE_LINUX 3
#define INVALID_HANDLE_VALUE NULL

#define CACHE_SIZE 16384
#define CACHE_FLUSH_TIME 5

/* safety check: only accept drives that:
* - contain RDSK in block 0
* - block 0 is zeroed
*/

int harddrive_dangerous, do_rdbdump;
static struct uae_driveinfo uae_drives[MAX_FILESYSTEM_UNITS];

static void rdbdump (FILE *h, uae_u64 offset, uae_u8 *buf, int blocksize)
{
	static int cnt = 1;
	int i, blocks;
	char name[100];
	FILE *f;

	blocks = (buf[132] << 24) | (buf[133] << 16) | (buf[134] << 8) | (buf[135] << 0);
	if (blocks < 0 || blocks > 100000)
		return;
	_stprintf (name, "rdb_dump_%d.rdb", cnt);
	f = fopen (name, "wb");
	if (!f)
		return;
	for (i = 0; i <= blocks; i++) {
		long outlen;
		if (fseek (h, (long)offset, SEEK_SET) != 0)
			break;
	   	outlen = fread (buf, 1, blocksize, h);
		fwrite (buf, 1, blocksize, f);
		offset += blocksize;
	}
	fclose (f);
	cnt++;
}

//static int ismounted (int hd)
//FIXME:
static int ismounted (FILE* f) {
    STUB("");
	int mounted;
	//mounted = 1;
	mounted = 0;
	return mounted;
}

#define CA "Commodore\0Amiga\0"
static int safetycheck (FILE *h, const char *name, uae_u64 offset, uae_u8 *buf, int blocksize)
{
	int i, j, blocks = 63, empty = 1;
	long outlen;

	for (j = 0; j < blocks; j++) {
		if (fseek (h, (long)offset, SEEK_SET) != 0) {
			write_log ("hd ignored, SetFilePointer failed, error %d\n", errno);
			return 1;
		}
		memset (buf, 0xaa, blocksize);
	   	outlen = fread (buf, 1, blocksize, h);
		if (outlen != blocksize) {
			write_log ("hd ignored, read error %d!\n", errno);
			return 2;
		}
		if (j == 0 && offset > 0)
			return -5;
		if (j == 0 && buf[0] == 0x39 && buf[1] == 0x10 && buf[2] == 0xd3 && buf[3] == 0x12) {
			// ADIDE "CPRM" hidden block..
			if (do_rdbdump)
				rdbdump (h, offset, buf, blocksize);
			write_log ("hd accepted (adide rdb detected at block %d)\n", j);
			return -3;
		}
		if (!memcmp (buf, "RDSK", 4) || !memcmp (buf, "DRKS", 4)) {
			if (do_rdbdump)
				rdbdump (h, offset, buf, blocksize);
			write_log ("hd accepted (rdb detected at block %d)\n", j);
			return -1;
		}

		if (!memcmp (buf + 2, "CIS@", 4) && !memcmp (buf + 16, CA, strlen (CA))) {
			write_log ("hd accepted (PCMCIA RAM)\n");
			return -2;
		}
		if (j == 0) {
			for (i = 0; i < blocksize; i++) {
				if (buf[i])
					empty = 0;
			}
		}
		offset += blocksize;
	}
	if (!empty) {
		int mounted;
		mounted = ismounted (h);
		if (!mounted) {
			write_log ("hd accepted, not empty and not mounted in Windows\n");
			return -8;
		}
		if (mounted < 0) {
			write_log ("hd ignored, NTFS partitions\n");
			return 0;
		}
		if (harddrive_dangerous == 0x1234dead)
			return -6;
		write_log ("hd ignored, not empty and no RDB detected or Windows mounted\n");
		return 0;
	}
	write_log ("hd accepted (empty)\n");
	return -9;
}

/*
static void trim (TCHAR *s)
{
	while(_tcslen(s) > 0 && s[_tcslen(s) - 1] == ' ') {
		s[_tcslen(s) - 1] = 0;
    }
}
*/

int isharddrive (const TCHAR *name)
{
	int i;

	for (i = 0; i < hdf_getnumharddrives (); i++) {
		if (!_tcscmp (uae_drives[i].device_name, name))
			return i;
	}
	return -1;
}

static TCHAR *hdz[] = { "hdz", "zip", "rar", "7z", NULL };

int hdf_open_target (struct hardfiledata *hfd, const char *pname)
{
	FILE *h = INVALID_HANDLE_VALUE;
	int i;
	struct uae_driveinfo *udi;
	char *name = strdup (pname);

	hfd->flags = 0;
	hfd->drive_empty = 0;
	hdf_close (hfd);
	hfd->cache = (uae_u8*)xmalloc (uae_u8, CACHE_SIZE);
	hfd->cache_valid = 0;
	hfd->virtual_size = 0;
	hfd->virtual_rdb = NULL;
	if (!hfd->cache) {
		write_log ("VirtualAlloc(%d) failed, error %d\n", CACHE_SIZE, errno);
		goto end;
	}
	hfd->handle = xcalloc (struct hardfilehandle, 1);
	hfd->handle->h = INVALID_HANDLE_VALUE;
	hfd_log ("hfd open: '%s'\n", name);
	if (_tcslen (name) > 4 && !_tcsncmp (name,"HD_", 3)) {
		hdf_init_target ();
		i = isharddrive (name);
		if (i >= 0) {
			long r;
			udi = &uae_drives[i];
			hfd->flags = HFD_FLAGS_REALDRIVE;
			if (udi->nomedia)
				hfd->drive_empty = -1;
			if (udi->readonly)
				hfd->readonly = 1;
			h = fopen (udi->device_path, hfd->readonly ? "rb" : "r+b");
			hfd->handle->h = h;
			if (h == INVALID_HANDLE_VALUE)
				goto end;
			_tcsncpy (hfd->vendor_id, udi->vendor_id, 8);
			_tcsncpy (hfd->product_id, udi->product_id, 16);
			_tcsncpy (hfd->product_rev, udi->product_rev, 4);
			hfd->offset = udi->offset;
			hfd->physsize = hfd->virtsize = udi->size;
			hfd->blocksize = udi->bytespersector;
			if (hfd->offset == 0 && !hfd->drive_empty) {
				int sf = safetycheck (hfd->handle->h, udi->device_path, 0, hfd->cache, hfd->blocksize);
				if (sf > 0)
					goto end;
				if (sf == 0 && !hfd->readonly && harddrive_dangerous != 0x1234dead) {
					write_log ("'%s' forced read-only, safetycheck enabled\n", udi->device_path);
					hfd->dangerous = 1;
					// clear GENERIC_WRITE
					fclose (h);
					h = fopen (udi->device_path, "r+b");
					hfd->handle->h = h;
					if (h == INVALID_HANDLE_VALUE)
						goto end;
				}
			}
			hfd->handle_valid = HDF_HANDLE_LINUX;
			hfd->emptyname = strdup (name);
		} else {
			hfd->flags = HFD_FLAGS_REALDRIVE;
			hfd->drive_empty = -1;
			hfd->emptyname = strdup (name);
		}
	} else {
		int zmode = 0;
		char *ext = _tcsrchr (name, '.');
		if (ext != NULL) {
			ext++;
			for (i = 0; hdz[i]; i++) {
				if (!_tcsicmp (ext, hdz[i]))
					zmode = 1;
			}
		}
		h = fopen (name, hfd->readonly ? "rb" : "r+b");
		if (h == INVALID_HANDLE_VALUE)
			goto end;
		hfd->handle->h = h;
		i = _tcslen (name) - 1;
		while (i >= 0) {
			if ((i > 0 && (name[i - 1] == '/' || name[i - 1] == '\\')) || i == 0) {
				_tcscpy (hfd->vendor_id, "UAE");
				_tcsncpy (hfd->product_id, name + i, 15);
				_tcscpy (hfd->product_rev, "0.3");
				break;
			}
			i--;
		}
		if (h != INVALID_HANDLE_VALUE) {
			int ret, low;
			ret = fseek (h, 0, SEEK_END);
			if (ret)
				goto end;
			low = ftell (h);
			if (low == -1)
				goto end;
			low &= ~(hfd->blocksize - 1);
			hfd->physsize = hfd->virtsize = low;
			hfd->handle_valid = HDF_HANDLE_LINUX;
			if (hfd->physsize < 64 * 1024 * 1024 && zmode) {
				write_log ("HDF '%s' re-opened in zfile-mode\n", name);
				fclose (h);
				hfd->handle->h = INVALID_HANDLE_VALUE;
				hfd->handle->zf = zfile_fopen(name, hfd->readonly ? "rb" : "r+b", ZFD_NORMAL);
				hfd->handle->zfile = 1;
				if (!h)
					goto end;
				zfile_fseek (hfd->handle->zf, 0, SEEK_END);
				hfd->physsize = hfd->virtsize = zfile_ftell (hfd->handle->zf);
				zfile_fseek (hfd->handle->zf, 0, SEEK_SET);
				hfd->handle_valid = HDF_HANDLE_ZFILE;
			}
		} else {
			write_log ("HDF '%s' failed to open. error = %d\n", name, errno);
		}
	}
	if (hfd->handle_valid || hfd->drive_empty) {
		hfd_log ("HDF '%s' opened, size=%dK mode=%d empty=%d\n",
			name, (int) (hfd->physsize / 1024), hfd->handle_valid, hfd->drive_empty);
		return 1;
	}
end:
	hdf_close (hfd);
	xfree (name);
	return 0;
}

/*
static void freehandle (struct hardfilehandle *h)
{
	if (!h)
		return;
	if (!h->zfile && h->h != INVALID_HANDLE_VALUE)
		fclose (h->h);
	if (h->zfile && h->zf)
		zfile_fclose (h->zf);
	h->zf = NULL;
	h->h = INVALID_HANDLE_VALUE;
	h->zfile = 0;
}
*/

void hdf_close_target (struct hardfiledata *hfd) {
 	//freehandle (hfd->handle);
	xfree (hfd->handle);
	xfree (hfd->emptyname);
	hfd->emptyname = NULL;
	hfd->handle = NULL;
	hfd->handle_valid = 0;
	if (hfd->cache)
		xfree (hfd->cache);
	xfree(hfd->virtual_rdb);
	hfd->virtual_rdb = 0;
	hfd->virtual_size = 0;
	hfd->cache = 0;
	hfd->cache_valid = 0;
	hfd->drive_empty = 0;
	hfd->dangerous = 0;
}

int hdf_dup_target (struct hardfiledata *dhfd, const struct hardfiledata *shfd)
{
	if (!shfd->handle_valid)
		return 0;

    return 0;
}

static int hdf_seek (struct hardfiledata *hfd, uae_u64 offset)
{
    size_t ret;

	if (hfd->handle_valid == 0) {
		gui_message ("hd: hdf handle is not valid. bug.");
		abort();
	}
	if (offset >= hfd->physsize - hfd->virtual_size) {
		gui_message ("hd: tried to seek out of bounds! (0x%llx >= 0x%llx)\n", offset, hfd->physsize);
		abort ();
    }
    offset += hfd->offset;
    if (offset & (hfd->blocksize - 1)) {
		gui_message ("hd: poscheck failed, offset=0x%llx not aligned to blocksize=%d! (0x%llx & 0x%04.4x = 0x%04.4x)\n",
			offset, hfd->blocksize, offset, hfd->blocksize, offset & (hfd->blocksize - 1));
		abort ();
    }
	if (hfd->handle_valid == HDF_HANDLE_LINUX) {
		ret = fseek (hfd->handle->h, (long)offset, SEEK_SET);
		if (ret)
			return -1;
	} else if (hfd->handle_valid == HDF_HANDLE_ZFILE) {
		zfile_fseek (hfd->handle->zf, (long)offset, SEEK_SET);
	}
    return 0;
}

static void poscheck (struct hardfiledata *hfd, int len)
{
	int ret;
	uae_u64 pos = 0;

	if (hfd->handle_valid == HDF_HANDLE_LINUX) {
		ret = fseek (hfd->handle->h, 0, SEEK_CUR);
		if (ret) {
			gui_message ("hd: poscheck failed. seek failure, ret %d", ret);
			abort ();
		}
		pos = ftell (hfd->handle->h);
	} else if (hfd->handle_valid == HDF_HANDLE_ZFILE) {
		pos = zfile_ftell (hfd->handle->zf);
	}
	if (len < 0) {
		gui_message ("hd: poscheck failed, negative length! (%d)", len);
		abort ();
    }
	if (pos < hfd->offset) {
		gui_message ("hd: poscheck failed, offset out of bounds! (0x%llx < 0x%llx)", pos, hfd->offset);
		abort ();
    }
	if (pos >= hfd->offset + hfd->physsize - hfd->virtual_size || pos >= hfd->offset + hfd->physsize + len - hfd->virtual_size) {
		gui_message ("hd: poscheck failed, offset out of bounds! (0x%llx >= 0x%llx, LEN=%d)", pos, hfd->offset + hfd->physsize, len);
		abort ();
    }
    if (pos & (hfd->blocksize - 1)) {
		gui_message ("hd: poscheck failed, offset not aligned to blocksize! (0x%llx & 0x%04.4x = 0x%04.4x\n", pos, hfd->blocksize, pos & hfd->blocksize);
		abort ();
    }
}

static int isincache (struct hardfiledata *hfd, uae_u64 offset, int len)
{
	if (!hfd->cache_valid)
		return -1;
	if (offset >= hfd->cache_offset && offset + len <= hfd->cache_offset + CACHE_SIZE)
		return (int)(offset - hfd->cache_offset);
	return -1;
}

#if 0
void hfd_flush_cache (struct hardfiledata *hfd, int now)
{
	DWORD outlen = 0;
	if (!hfd->cache_needs_flush || !hfd->cache_valid)
		return;
	if (now || time (NULL) > hfd->cache_needs_flush + CACHE_FLUSH_TIME) {
		hdf_log ("flushed %d %d %d\n", now, time(NULL), hfd->cache_needs_flush);
		hdf_seek (hfd, hfd->cache_offset);
		poscheck (hfd, CACHE_SIZE);
		WriteFile (hfd->handle, hfd->cache, CACHE_SIZE, &outlen, NULL);
		hfd->cache_needs_flush = 0;
	}
}
#endif

static int hdf_read_2 (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	long outlen = 0;
	int coffset;

	if (offset == 0)
		hfd->cache_valid = 0;
	coffset = isincache (hfd, offset, len);
	if (coffset >= 0) {
		memcpy (buffer, hfd->cache + coffset, len);
		return len;
	}
	hfd->cache_offset = offset;
	if (offset + CACHE_SIZE > hfd->offset + (hfd->physsize - hfd->virtual_size))
		hfd->cache_offset = hfd->offset + (hfd->physsize - hfd->virtual_size) - CACHE_SIZE;
	hdf_seek (hfd, hfd->cache_offset);
	poscheck (hfd, CACHE_SIZE);
	if (hfd->handle_valid == HDF_HANDLE_LINUX)
	   	outlen = fread (hfd->cache, 1, CACHE_SIZE, hfd->handle->h);
	else if (hfd->handle_valid == HDF_HANDLE_ZFILE)
		outlen = zfile_fread (hfd->cache, 1, CACHE_SIZE, hfd->handle->zf);
	hfd->cache_valid = 0;
	if (outlen != CACHE_SIZE)
		return 0;
	hfd->cache_valid = 1;
	coffset = isincache (hfd, offset, len);
	if (coffset >= 0) {
		memcpy (buffer, hfd->cache + coffset, len);
		return len;
	}
#ifdef CAN_PRINTF_LONG_LONG
	write_log ("hdf_read: cache bug! offset=0x%llx len=%d\n", offset, len);
#endif
	hfd->cache_valid = 0;
	return 0;
}

int hdf_read_target (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
    int got = 0;
	uae_u8 *p = (uae_u8*)buffer;

	if (hfd->drive_empty)
		return 0;
	if (offset < hfd->virtual_size) {
		uae_u64 len2 = offset + len <= hfd->virtual_size ? len : hfd->virtual_size - offset;
		if (!hfd->virtual_rdb)
			return 0;
		memcpy (buffer, hfd->virtual_rdb + offset, len2);
		return len2;
	}
	offset -= hfd->virtual_size;
	while (len > 0) {
		int maxlen;
		int ret = 0;
		if (hfd->physsize < CACHE_SIZE) {
		    hfd->cache_valid = 0;
		    hdf_seek (hfd, offset);
		    poscheck (hfd, len);
			if (hfd->handle_valid == HDF_HANDLE_LINUX) {
				ret = fread (hfd->cache, 1, len, hfd->handle->h);
				memcpy (buffer, hfd->cache, ret);
			} else if (hfd->handle_valid == HDF_HANDLE_ZFILE) {
				ret = zfile_fread (buffer, 1, len, hfd->handle->zf);
			}
			maxlen = len;
		} else {
			maxlen = len > CACHE_SIZE ? CACHE_SIZE : len;
			ret = hdf_read_2 (hfd, p, offset, maxlen);
		}
		got += ret;
		if (ret != maxlen)
			return got;
		offset += maxlen;
		p += maxlen;
		len -= maxlen;
	}
	return got;
}

static int hdf_write_2 (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	long outlen = 0;

	if (hfd->readonly)
		return 0;
	if (hfd->dangerous)
		return 0;
	hfd->cache_valid = 0;
	hdf_seek (hfd, offset);
	poscheck (hfd, len);
	memcpy (hfd->cache, buffer, len);
	if (hfd->handle_valid == HDF_HANDLE_LINUX) {
	    outlen = fwrite (hfd->cache, 1, len, hfd->handle->h);
		if (offset == 0) {
			long outlen2;
			uae_u8 *tmp;
			int tmplen = 512;
			tmp = (uae_u8*)xmalloc (uae_u8, tmplen);
			if (tmp) {
				memset (tmp, 0xa1, tmplen);
				hdf_seek (hfd, offset);
				outlen2 = fread (tmp, 1, tmplen, hfd->handle->h);
				if (memcmp (hfd->cache, tmp, tmplen) != 0 || outlen != len)
					gui_message ("Harddrive\n%s\nblock zero write failed!", hfd->device_name);
				xfree (tmp);
			}
		}
	} else if (hfd->handle_valid == HDF_HANDLE_ZFILE) {
		outlen = zfile_fwrite (hfd->cache, 1, len, hfd->handle->zf);
	}
	return outlen;
}
int hdf_write_target (struct hardfiledata *hfd, void *buffer, uae_u64 offset, int len)
{
	int got = 0;
	uae_u8 *p = (uae_u8*)buffer;

	if (hfd->drive_empty)
		return 0;
	if (offset < hfd->virtual_size)
		return len;
	offset -= hfd->virtual_size;
	while (len > 0) {
		int maxlen = len > CACHE_SIZE ? CACHE_SIZE : len;
		int ret = hdf_write_2 (hfd, p, offset, maxlen);
		if (ret < 0)
			return ret;
		got += ret;
		if (ret != maxlen)
			return got;
		offset += maxlen;
		p += maxlen;
		len -= maxlen;
	}
	return got;
}

int hdf_resize_target (struct hardfiledata *hfd, uae_u64 newsize)
{
	int err = 0;

	write_log ("hdf_resize_target: SetEndOfFile() %d\n", err);
	return 0;
}

static int num_drives;

static int hdf_init2 (int force)
{
	int index = 0, index2 = 0, drive;
	uae_u8 *buffer;
	int errormode;
	int dwDriveMask;
	static int done;

	if (done && !force)
		return num_drives;
	done = 1;
	num_drives = 0;
	return num_drives;
}

int hdf_init_target (void)
{
	return hdf_init2 (0);
}

int hdf_getnumharddrives (void)
{
	return num_drives;
}

TCHAR *hdf_getnameharddrive (int index, int flags, int *sectorsize, int *dangerousdrive)
{
	static char name[512];
	char tmp[32];
	uae_u64 size = uae_drives[index].size;
	int nomedia = uae_drives[index].nomedia;
	char *dang = "?";
	char *rw = "RW";

	if (dangerousdrive)
		*dangerousdrive = 0;
	switch (uae_drives[index].dangerous)
	{
	case -5:
		dang = "[PART]";
		break;
	case -6:
		dang = "[MBR]";
		break;
	case -7:
		dang = "[!]";
		break;
	case -8:
		dang = "[UNK]";
		break;
	case -9:
		dang = "[EMPTY]";
		break;
	case -3:
		dang = "(CPRM)";
		break;
	case -2:
		dang = "(SRAM)";
		break;
	case -1:
		dang = "(RDB)";
		break;
	case 0:
		dang = "[OS]";
		if (dangerousdrive)
			*dangerousdrive |= 1;
		break;
	}
	if (nomedia) {
		dang = "[NO MEDIA]";
		if (dangerousdrive)
			*dangerousdrive &= ~1;
	}
	if (uae_drives[index].readonly) {
		rw = "RO";
		if (dangerousdrive && !nomedia)
			*dangerousdrive |= 2;
	}

	if (sectorsize)
		*sectorsize = uae_drives[index].bytespersector;
	if (flags & 1) {
		if (nomedia) {
			_tcscpy (tmp, "N/A");
		} else {
			if (size >= 1024 * 1024 * 1024)
				_stprintf (tmp, "%.1fG", ((double)(uae_u32)(size / (1024 * 1024))) / 1024.0);
			else if (size < 10 * 1024 * 1024)
				_stprintf (tmp, "%dK", (int) (size / 1024));
			else
				_stprintf (tmp, "%.1fM", ((double)(uae_u32)(size / (1024))) / 1024.0);
		}
		_stprintf (name, "%10s [%s,%s] %s", dang, tmp, rw, uae_drives[index].device_name + 3);
		return name;
	}
	if (flags & 2)
		return uae_drives[index].device_path;
	return uae_drives[index].device_name;
}