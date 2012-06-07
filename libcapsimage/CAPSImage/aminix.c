#include "config.h"
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <proto/utility.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* unbuffered file functions */
int open(const char *path, int flags, ...)
{
	BPTR file = Open(path, flags & O_CREAT ? MODE_NEWFILE : flags & O_RDWR ? MODE_READWRITE : MODE_OLDFILE);

	/* no real file descriptors yet */
	return file ? file : -1;
}

off_t lseek(int fildes, off_t offset, int whence)
{
	/* no support for seeking beyond the end of a file yet */
	return Seek(fildes, offset, whence == SEEK_SET ? OFFSET_BEGINNING : whence == SEEK_END ? OFFSET_END : OFFSET_CURRENT) != -1 ? Seek(fildes, 0, OFFSET_CURRENT) : -1;
}

ssize_t read(int d, void *buf, size_t nbytes)
{
	return Read(d, buf, nbytes);
}

ssize_t write(int d, const void *buf, size_t nbytes)
{
	return Write(d, (APTR)buf, nbytes);
}

int close(int d)
{
	return Close(d) ? 0 : -1;
}

/* string functions */
int memcmp(const void *b1, const void *b2, size_t len)
{
	const unsigned char *p1 = b1, *p2 = b2;
	unsigned long r, c;

	if ((r = len))
	{
		do
		{
			r = *p1++;
			c = *p2++;
		}
		while (!(r -= c) && --len);
	}
	return r;
}

void bcopy(const void *src, void *dst, size_t len)
{
	/* no support for overlapping copies yet */
	CopyMem((APTR)src, dst, len);
}

void bzero(void *b, size_t len)
{
	char *p = b;

	if (len)
	{
		do
			*p++ = 0;
		while (--len);
	}
}

/* time functions */
time_t time(time_t *tloc)
{
	static struct DateStamp date;
	time_t t;

	DateStamp(&date);
	/* 2922 days between 1.1.1970 and 1.1.1978 */
	t = ((date.ds_Days+2922)*1440 + date.ds_Minute)*60 + date.ds_Tick/TICKS_PER_SECOND;
	
	if (tloc)
		*tloc = t;

	return t;
}

struct tm *localtime(const time_t *clock)
{
	static struct ClockData cdata;
	static struct tm tm;

	Amiga2Date(*clock - 2922*1440*60, &cdata);
	/* no support for tm_yday, tm_isdst, tm_zone and tm_gmtoff yet */
	tm.tm_sec = cdata.sec;
	tm.tm_min = cdata.min;
	tm.tm_hour = cdata.hour;
	tm.tm_mday = cdata.mday;
	tm.tm_mon = cdata.month - 1;
	tm.tm_year = cdata.year - 1900;
	tm.tm_wday = cdata.wday;

	return &tm;
}

