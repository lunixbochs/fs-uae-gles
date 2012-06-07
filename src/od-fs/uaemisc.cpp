#include "sysconfig.h"
#include "sysdeps.h"

#include "include/options.h"
#include "sleep.h"

#ifndef PICASSO96
// just to make ncr_scsi compile. it will not work, of course,
// so do not try to use functions in ncr_scsi
uaecptr p96ram_start;
#endif

int pause_emulation = 0;
int uaelib_debug = 0;

int sleep_resolution = 1000 / 1;
int pissoff_value = 25000;

extern int uaeser_getdatalength (void);
int uaeser_getdatalenght (void) {
    return uaeser_getdatalength();
}

void target_default_options (struct uae_prefs *p, int type) {
    //write_log("STUB: target_default_options p=%p type=%d\n", p, type);
    write_log("target_default_options p=%p type=%d\n", p, type);
    // FIXME: move out of here - into a (lib)amiga_ function
    write_log("target_default_options: enabling floppy sounds\n");
    p->floppyslots[0].dfxclick = 1;
    p->floppyslots[1].dfxclick = 1;
    p->floppyslots[2].dfxclick = 1;
    p->floppyslots[3].dfxclick = 1;
    p->dfxclickvolume = 80;

	if (type == 2 || type == 0) {
		// if this isn't set to -1, will caused problems for parallel
		// port joysticks
		p->win32_samplersoundcard = -1;
	}

	p->picasso96_modeflags = 0x212;

    return;
}

/**
 * sleep_millis_main was introduced to custom.cpp in WinUAE 2.4.0b5.
 * FIXME: what does _main signify here?
 */
void sleep_millis_main (int ms) {
    // FIXME: HOW EXACT MUST THE SLEEP BE?
    //write_log("sleep %d\n", ms);
    usleep(ms * 1000);
    //uae_msleep(ms);
}

void sleep_millis (int ms) {
    // FIXME: HOW EXACT MUST THE SLEEP BE?
	//write_log("sleep_millis %d\n", ms);
    // FIXME: check usage of this for CD32
    usleep(ms * 1000);
    //uae_msleep(ms);
}

int same_aname (const char *an1, const char *an2) {
    // FIXME: latin 1 chars?
    // FIXME: compare with latin1 table in charset/filesys_host/fsdb_host
    return strcasecmp (an1, an2) == 0;
}

void console_out_f (const TCHAR *, ...) {
    // FIXME:
    STUB("");
}

void console_out (const TCHAR *) {
    // FIXME:
    STUB("");
}

int console_get_gui (TCHAR *out, int maxlen) {
    STUB("");
    return 0;
}

int console_get (TCHAR *out, int maxlen) {
    STUB("");
    return 0;
}

void console_flush (void) {
    STUB("");
}

TCHAR console_getch (void) {
    STUB("");
    return 0;
}

void close_console (void) {
    STUB("");
}

/*
struct uae_filter usedfilter_storage
struct uae_filter *usedfilter = &usedfilter_storage;
*/

//struct uae_prefs currprefs;
/*
uae_u8 *mapped_malloc (size_t s, TCHAR *file)
{
	return xmalloc (uae_u8, s);
}

void mapped_free (uae_u8 *p)
{
	xfree (p);
}
*/

//#include "fsdb.h"
// FIXME: to fsdb_unix.cpp


#include "include/driveclick.h"

extern unsigned char drive_click_data[];
extern unsigned char drive_spin_data[];
extern unsigned char drive_spinnd_data[];
extern unsigned char drive_startup_data[];
extern unsigned char drive_snatch_data[];
extern int drive_click_data_size;
extern int drive_spin_data_size;
extern int drive_spinnd_data_size;
extern int drive_startup_data_size;
extern int drive_snatch_data_size;

int driveclick_loadresource (struct drvsample *sp, int drivetype) {
    int i, ok;
    ok = 1;
    for (i = 0; i < 5; i++) {
        int type = -1;
        int len = -1;
        unsigned char* data = NULL;
        switch(i) {
            case 0:
                type = DS_CLICK;
                data = drive_click_data;
                len = drive_click_data_size;
                break;
            case 1:
                type = DS_SPIN;
                data = drive_spin_data;
                len = drive_spin_data_size;
                break;
            case 2:
                type = DS_SPINND;
                data = drive_spinnd_data;
                len = drive_spinnd_data_size;
                break;
            case 3:
                type = DS_START;
                data = drive_startup_data;
                len = drive_startup_data_size;
                break;
            case 4:
                type = DS_SNATCH;
                data = drive_snatch_data;
                len = drive_snatch_data_size;
                break;
        }
        struct drvsample* s = sp + type;
        //write_log("decode drive click sample %d from %p len %d\n", type,
        //        data, len);
        s->p = decodewav((uae_u8*) data, &len);
        s->len = len;
    }
	return ok;
}

void driveclick_fdrawcmd_close(int drive) {

}

int driveclick_fdrawcmd_open(int drive) {
    return 0;
}

void driveclick_fdrawcmd_detect(void) {

}

void driveclick_fdrawcmd_seek(int drive, int cyl) {

}
void driveclick_fdrawcmd_motor (int drive, int running) {

}

void driveclick_fdrawcmd_vsync(void) {

}

int my_setcurrentdir (const TCHAR *curdir, TCHAR *oldcur) {
    STUB("curdir=\"%s\" oldcur=\"%s\"", curdir, oldcur);
    return 0;
}

bool my_isfilehidden (const TCHAR *path) {
    STUB("path=\"%s\"", path);
    return 0;
}

void my_setfilehidden (const TCHAR *path, bool hidden) {
    STUB("path=\"%s\" hidden=%d", path, hidden);
}

int amiga_clipboard_want_data (void) {
    STUB("");
    return 0;
}

void fpux_save (int *v) {
    STUB("");
}

void fpux_restore (int *v) {
    STUB("");
}

int target_get_volume_name (struct uaedev_mount_info *mtinf,
        const TCHAR *volumepath, TCHAR *volumename, int size, bool inserted,
        bool fullcheck) {
    STUB("");
    return 0;
}

static char *console_buffer;
static int console_buffer_size;

char *setconsolemode (char *buffer, int maxlen) {
    char *ret = NULL;
    if (buffer) {
        console_buffer = buffer;
        console_buffer_size = maxlen;
    }
    else {
        ret = console_buffer;
        console_buffer = NULL;
    }
    return ret;
}

// writelog
TCHAR* buf_out (TCHAR *buffer, int *bufsize, const TCHAR *format, ...) {
    va_list parms;
    va_start (parms, format);
    if (buffer == NULL) {
        return 0;
    }
    vsnprintf (buffer, (*bufsize) - 1, format, parms);
    va_end (parms);
    *bufsize -= _tcslen (buffer);
    return buffer + _tcslen (buffer);
}

void fixtrailing (TCHAR *p) {
    if (strlen(p) == 0) {
        return;
    }
    if (p[strlen(p) - 1] == '/' || p[strlen(p) - 1] == '\\') {
        return;
    }
    strcat(p, FSDB_DIR_SEPARATOR_S);
}

void getpathpart(TCHAR *outpath, int size, const TCHAR *inpath) {
    strcpy(outpath, inpath);
    TCHAR *p = strrchr(outpath, '/');
#ifdef WINDOWS
	if (!p) {
		p = strrchr(outpath, '\\');
	}
#endif
    if (p) {
        p[0] = 0;
    }
    fixtrailing(outpath);
}

void getfilepart(TCHAR *out, int size, const TCHAR *path) {
    out[0] = 0;
    const TCHAR *p = strrchr(path, '/');
#ifdef WINDOWS
	if (!p) {
		p = strrchr(path, '\\');
	}
#endif
    if (p) {
        strcpy(out, p + 1);
    }
    else {
    	strcpy(out, path);
    }
}

// convert path to absolute or relative
void fullpath (TCHAR *path, int size) {
    // FIXME: forward/backslash fix needed
    if (path[0] == 0 || (path[0] == '\\' && path[1] == '\\') ||
            path[0] == ':') {
        return;
    }
    /* <drive letter>: is supposed to mean same as <drive letter>:\ */
}

TCHAR start_path_data[MAX_DPATH];

void fetch_path (TCHAR *name, TCHAR *out, int size) {
        int size2 = size;
    //printf("fetch_path %s\n", name);
    //_tcscpy (start_path_data, "./");
    _tcscpy (start_path_data, "");
    _tcscpy (out, start_path_data);
    /*
    if (!name) {
        return;
    }
    if (!_tcscmp (name, "FloppyPath")) {
        _tcscat (out, "./");
    }
    else if (!_tcscmp (name, "CDPath")) {
        _tcscat (out, "./");
    }
    else if (!_tcscmp (name, "hdfPath")) {
        _tcscat (out, "./");
    }
    else if (!_tcscmp (name, "KickstartPath")) {
        _tcscat (out, "./");
    }
    else if (!_tcscmp (name, "ConfigurationPath")) {
        _tcscat (out, "./");
    }
    */
}

void fetch_saveimagepath (TCHAR *out, int size, int dir) {
    fetch_path("SaveimagePath", out, size);
    out[0] = '\0';
    if (g_libamiga_save_image_path) {
        strcpy(out, g_libamiga_save_image_path);
    }
}

void fetch_configurationpath (TCHAR *out, int size) {
    fetch_path("ConfigurationPath", out, size);
}

void fetch_screenshotpath (TCHAR *out, int size) {
    fetch_path("ScreenshotPath", out, size);
}
void fetch_ripperpath (TCHAR *out, int size) {
    fetch_path("RipperPath", out, size);
}
void fetch_statefilepath (TCHAR *out, int size) {
    fetch_path("StatefilePath", out, size);
}

void fetch_inputfilepath (TCHAR *out, int size) {
    fetch_path("InputPath", out, size);
}

void fetch_datapath (TCHAR *out, int size) {
        fetch_path (NULL, out, size);
}

void to_lower (TCHAR *s, int len) {
    for (int i = 0; i < len; i++) {
        s[i] = tolower(s[i]);
    }
}

void to_upper (TCHAR *s, int len) {
    for (int i = 0; i < len; i++) {
        s[i] = toupper(s[i]);
    }
}

TCHAR *target_expand_environment (const TCHAR *path) {
    // FIXME:
    return strdup(path);
}

// sana2.cpp
volatile int uaenet_int_requested;
volatile int uaenet_vsync_requested;

#include <signal.h>
#include "debug.h"
#ifdef __cplusplus_disabled
static RETSIGTYPE sigbrkhandler(...)
#else
static RETSIGTYPE sigbrkhandler (int foo)
#endif
{
#ifdef DEBUGGER
    activate_debugger ();
#endif

#if !defined(__unix) || defined(__NeXT__)
    signal (SIGINT, sigbrkhandler);
#endif
}


void setup_brkhandler (void)
{
    /*
#if defined(__unix) && !defined(__NeXT__)
    struct sigaction sa;
    sa.sa_handler = sigbrkhandler;
    sa.sa_flags = 0;
#ifdef SA_RESTART
    sa.sa_flags = SA_RESTART;
#endif
    sigemptyset (&sa.sa_mask);
    sigaction (SIGINT, &sa, NULL);
#else
    signal (SIGINT, sigbrkhandler);
#endif
    */
}

#include "include/zfile.h"
// --- win32gui.cpp ---
static int qs_override;

int target_cfgfile_load (struct uae_prefs *p, const TCHAR *filename, int type, int isdefault)
{
    STUB("");
    return 1;
}

