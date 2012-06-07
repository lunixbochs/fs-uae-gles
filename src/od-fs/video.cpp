#include "sysconfig.h"
#include "sysdeps.h"

#include "include/options.h"
#include "include/xwin.h"
#include "include/uae.h"
#include "include/custom.h"
#include "include/drawing.h"
#include "include/gfxfilter.h"
#include <limits.h>
#include <stdlib.h>

#ifdef PICASSO96
#include "picasso96_host.h"
#endif

volatile bool vblank_found_chipset;
volatile bool vblank_found_rtg;

int flashscreen = 0;
#define MAXBLOCKLINES_MAX INT_MAX;

#define AMIGA_WIDTH (AMIGA_WIDTH_MAX * 2)
//#define AMIGA_HEIGHT (AMIGA_HEIGHT_MAX * 2)
//#define AMIGA_HEIGHT 574
#define AMIGA_HEIGHT 572

uae_s32 tyhrgb[65536];
uae_s32 tylrgb[65536];
uae_s32 tcbrgb[65536];
uae_s32 tcrrgb[65536];

//#define USE_BUFMEM
//#define USE_LINEMEM

struct MultiDisplay Displays[MAX_DISPLAYS] = {};

static int g_picasso_enabled = 0;
static int g_picasso_width = 0;
static int g_picasso_height = 0;
static int g_picasso_depth = 0;
static int g_picasso_format = 0;

static int g_has_flushed_line = 0;
static int g_has_flushed_block = 0;
static int g_has_flushed_screen = 0;

static int g_largest_width = 0;
static int g_largest_height = 0;

struct uae_filter uaefilters[] = {
    { UAE_FILTER_NULL, 0, 1, _T("Null filter"), _T("null"),
            UAE_FILTER_MODE_16_16 | UAE_FILTER_MODE_32_32 },
    { 0 }
};

static bool render_ok;
volatile bool thread_vblank_found;
// --- win32gfx.c
int screen_is_picasso = 0;
struct uae_filter *usedfilter;
uae_u32 redc[3 * 256], grec[3 * 256], bluc[3 * 256];

static double remembered_vblank;
static int vblankbasewait, vblankbasefull;
RenderData libamiga_rd;
static int g_screen_updated = 0;

static uae_u8 g_linemem[4096 * 4];
static unsigned char* g_bufmem = NULL;

static int g_red_bits;
static int g_green_bits;
static int g_blue_bits;
static int g_alpha_bits;
static int g_red_shift;
static int g_green_shift;
static int g_blue_shift;
static int g_alpha_shift;

int g_amiga_rtg_modes[] = {
        640, 360, // 16:9

        800, 450, // 16:9
        800, 500, // 16:10
        800, 600,

        1024, 576, // 16:9
        1024, 600, // 16:10
        1024, 768,

        1280, 720, // 16:9
        //add_mode (md1, 1280, 800, 32, 50, 1); // 16:10
        //add_mode (md1, 1280, 720, 32, 50, 1);

        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        -1, -1,
};

#if 0
void uae_line_update(int line, int update) {
    printf("%d %d\n", line, update);
    if (!update) {
        return;
    }
    // mark this line as not needing copy from the previous render buffer
    libamiga_rd.line[line] = 0;
    g_screen_updated = 1;
}
#endif

void flush_screen (struct vidbuffer *buffer, int first_line, int last_line) {
    //write_log("flush_screen\n");
    g_has_flushed_screen = 1;
}

bool render_screen (void) {
    //write_log("render_screen line: %d block %d screen %d\n",
    //        g_has_flushed_line, g_has_flushed_block, g_has_flushed_screen);
    int flushed = g_has_flushed_line || g_has_flushed_block ||
            g_has_flushed_screen;

    static int cx, cy, cw, ch;
    //printf("g_picasso_enabled %d\n", g_picasso_enabled);
    if (g_picasso_enabled) {
        libamiga_rd.width = g_picasso_width;
        libamiga_rd.height = g_picasso_height;
        libamiga_rd.limit_x = 0;
        libamiga_rd.limit_y = 0;
        libamiga_rd.limit_w = g_picasso_width;
        libamiga_rd.limit_h = g_picasso_height;
        //libamiga_rd.updated = g_screen_updated;
        libamiga_rd.flags = AMIGA_RTG_BUFFER_FLAG;

#ifdef USE_BUFMEM
        //memcpy(libamiga_rd.pixels, g_bufmem, g_picasso_width * g_picasso_height * 4);
#endif
        // FIXME
        memset(libamiga_rd.line, 0, AMIGA_MAX_LINES);
    }
    else {
        //printf(".\n");
        if (gfxvidinfo.outbuffer) {
            // if gfxvidinfo.outbuffer is not set, get_custom_limits will
            // crash
            if (flushed) {
                get_custom_limits(&cw, &ch, &cx, &cy);
            }
            else {
                // reuse last custom limits
            }
        }
        //printf("..\n");
        if (cx < 0) {
            write_log("WARNING: custom limit x (%d) is < 0 - clamping\n", cx);
            cx = 0;
        }
        if (cy < 0) {
            write_log("WARNING: custom limit y (%d) is < 0 - clamping\n", cy);
            cy = 0;
        }
        if (cx + cw > AMIGA_WIDTH) {
            write_log("WARNING: custom limit x (%d) + w (%d) is > "
                    "AMIGA_WIDTH (%d) - clamping\n", cx, cw, AMIGA_WIDTH);
            cw = AMIGA_WIDTH - cx;
        }
        if (cy + ch > AMIGA_HEIGHT) {
            write_log("WARNING: custom limit y (%d) + h (%d) is > "
                    "AMIGA_HEIGHT (%d) - clamping\n", cy, ch, AMIGA_HEIGHT);
            ch = AMIGA_HEIGHT - cy;
        }
        //printf("...\n");
        libamiga_rd.width = AMIGA_WIDTH;
        libamiga_rd.height = AMIGA_HEIGHT;
        libamiga_rd.limit_x = cx;
        libamiga_rd.limit_y = cy;
        libamiga_rd.limit_w = cw;
        libamiga_rd.limit_h = ch;
        //libamiga_rd.updated = g_screen_updated;
        libamiga_rd.flags = 0;
#ifdef USE_BUFMEM
        //printf("libamiga_rd.pixels %p %p", libamiga_rd.pixels, g_bufmem);
        memcpy(libamiga_rd.pixels, g_bufmem, AMIGA_WIDTH * AMIGA_HEIGHT * 4);
#endif
    }
    //libamiga_rd.line[first_line] = 0;
    //libamiga_rd.line[first_line + 1] = 0;
    //for (int y = first_line; y <= last_line; y++) {
    //    libamiga_rd.line[y] = 0;
    //}
    g_screen_updated = 0;
    //printf("flush_screen (%d -> %d) %d %d %d %d\n", first_line, last_line,
    //        cx, cy, cw, ch);

    if (g_libamiga_callbacks.render) {
        g_libamiga_callbacks.render(&libamiga_rd);
    }

    g_has_flushed_line = 0;
    g_has_flushed_block = 0;
    g_has_flushed_screen = 0;
    return 1;
}

void show_screen (void) {
    //write_log("show_screen\n\n");
    if (g_libamiga_callbacks.display) {
        g_libamiga_callbacks.display();
    }
}

bool show_screen_maybe (bool show) {
    //printf("show_screen_maybe %d\n", show);
    //show_screen ();
    //return false;

    struct apmode *ap = picasso_on ? &currprefs.gfx_apmode[1] : &currprefs.gfx_apmode[0];
    if (!ap->gfx_vflip || ap->gfx_vsyncmode == 0 || !ap->gfx_vsync) {
        if (show)
            show_screen ();
        return false;
    }
    return false;
    /*
    if (ap->gfx_vflip < 0) {
        doflipevent ();
        return true;
    }
    return false;
    */
}

double vblank_calibrate (double approx_vblank, bool waitonly) {
    STUB("");
    return -1;
}

// FIXME: What is this?
int extraframewait = 0;
static int frame_missed, frame_counted, frame_errors;
static int frame_usage, frame_usage_avg, frame_usage_total;
//extern int log_vsync;
static bool dooddevenskip;
static volatile frame_time_t vblank_prev_time, thread_vblank_time;
//static bool vblankbaselace;
static int vblankbaselace_chipset;
//static bool vblankthread_oddeven;

void vsync_busywait_start(void) {
    STUB("");
    //changevblankthreadmode_fast (VBLANKTH_ACTIVE_START);
    vblank_prev_time = thread_vblank_time;
}

static bool isthreadedvsync (void) {
    return isvsync_chipset () <= -2 || isvsync_rtg () < 0;
}

bool vsync_busywait_do (int *freetime, bool lace, bool oddeven) {
    STUB("");
    return false;
#if 0
    bool v;
    static bool framelost;
    int ti;
    frame_time_t t;
    frame_time_t prevtime = vblank_prev_time;

    dooddevenskip = false;

    if (lace)
        vblankbaselace_chipset = oddeven;
    else
        vblankbaselace_chipset = -1;

    t = read_processor_time ();
    ti = t - prevtime;
    //if (ti > 2 * vblankbasefull || ti < -2 * vblankbasefull) {
    if (ti > 1 * vblankbasefull || ti < -1 * vblankbasefull) {
#if 0
        waitvblankstate (false, NULL);
#endif
        t = read_processor_time ();
        vblank_prev_time = t;
        thread_vblank_time = t;
        frame_missed++;
        return true;
    }

    //if (log_vsync) {
    //    console_out_f(_T("F:%8d M:%8d E:%8d %3d%% (%3d%%) %10d\r"), frame_counted, frame_missed, frame_errors, frame_usage, frame_usage_avg, (t - vblank_prev_time) - vblankbasefull);
    //}

    if (freetime)
        *freetime = 0;
    if (currprefs.turbo_emulation) {
        frame_missed++;
        return true;
    }
#if 0
    frame_usage = (t - prevtime) * 100 / vblankbasefull;
    if (frame_usage > 99)
        frame_usage = 99;
    else if (frame_usage < 0)
        frame_usage = 0;
    frame_usage_total += frame_usage;
    if (freetime)
        *freetime = frame_usage;
    if (frame_counted)
        frame_usage_avg = frame_usage_total / frame_counted;
#endif
    v = false;

    if (isthreadedvsync ()) {

        framelost = false;
        v = true;

    } else {
#if 0
        bool doskip = false;

        if (!framelost && t - prevtime > vblankbasefull) {
            framelost = true;
            frame_missed++;
            return true;
        }

        if (vblanklaceskip ()) {
            doskip = true;
            dooddevenskip = true;
        }

        if (!doskip) {
            while (!framelost && read_processor_time () - prevtime < vblankbasewait1) {
                vsync_sleep (false);
            }
            v = vblank_wait ();
        } else {
            v = true;
        }
        framelost = false;
#endif
    }

    if (v) {
        vblank_prev_time = read_processor_time ();
        frame_counted++;
        return true;
    }
    frame_errors++;
    return false;
#endif
}

static void vsync_sleep (bool preferbusy) {
#if 0
    struct apmode *ap = picasso_on ? &currprefs.gfx_apmode[1] : &currprefs.gfx_apmode[0];
    bool dowait;
    if (vsync_busy_wait_mode == 0) {
        dowait = ap->gfx_vflip || !preferbusy;
    } else if (vsync_busy_wait_mode < 0) {
        dowait = true;
    } else {
        dowait = false;
    }
    dowait = true;
    if (dowait && currprefs.m68k_speed >= 0)
        sleep_millis_main (1);
#endif
}


static void vsync_notvblank (void) {
    return;
#if 0
    for (;;) {
        int vp;
        if (!getvblankpos (&vp))
            return;
        if (vp > 0) {
            //write_log (_T("%d "), vpos);
            break;
        }
        vsync_sleep (true);
    }
#endif
}

// FIXME
extern "C" {
int fs_ml_get_vblank_count();
}

frame_time_t vsync_busywait_end (void) {
#if 0
    printf("vsync_busywait_end\n");
    show_screen ();

    static int last_vblank = 0;
    while (fs_ml_get_vblank_count() == last_vblank) {

    }
    last_vblank++;// = fs_ml_get_vblank_count();

    if (!dooddevenskip) {
#if 0
        vsync_notvblank ();
        while (!vblank_found && vblankthread_mode == VBLANKTH_ACTIVE) {
            vsync_sleep (currprefs.m68k_speed < 0);
        }
#endif
    }
    //changevblankthreadmode_fast (VBLANKTH_ACTIVE_WAIT);
#if 0
    return thread_vblank_time;

    write_log("vsync_busywait_end\n");
#endif
#endif
    return read_processor_time();
}

double getcurrentvblankrate (void) {
    STUB("");
    if (remembered_vblank) {
        return remembered_vblank;
    }
    write_log("STUB: getcurrentvblankrate\n");
    STUB("");
	return 50;
}

static int uae_bits_in_mask (unsigned int mask) {
    int n = 0;
    while (mask) {
        n += mask & 1;
        mask >>= 1;
    } 
    return n;
}

static int uae_mask_shift (unsigned int mask) {
    int n = 0;
    while (!(mask & 1)) {
        n++;
        mask >>= 1;
    }
    return n;
}

static int init_colors (void) {
    write_log("init_colors\n");
    alloc_colors64k(g_red_bits, g_green_bits, g_blue_bits, g_red_shift,
            g_green_shift, g_blue_shift, 0, 0, 0, 0);
    return 1;
}

#ifdef PICASSO96

void gfx_set_picasso_colors (RGBFTYPE rgbfmt) {
    write_log("gfx_set_picasso_colors %d\n", rgbfmt);

    alloc_colors_picasso(g_red_bits, g_green_bits, g_blue_bits, g_red_shift,
            g_green_shift, g_blue_shift, rgbfmt);
}

int picasso_palette (void) {
    int i, changed;

    changed = 0;
    for (i = 0; i < 256; i++) {
        int r = picasso96_state.CLUT[i].Red;
        int g = picasso96_state.CLUT[i].Green;
        int b = picasso96_state.CLUT[i].Blue;
        uae_u32 v = (doMask256 (r, g_red_bits, g_red_shift)
            | doMask256 (g, g_green_bits, g_green_shift)
            | doMask256 (b, g_blue_bits, g_blue_shift))
            | doMask256 (0xff, g_alpha_bits, g_alpha_shift);
        if (v != picasso_vidinfo.clut[i]) {
            //write_log (_T("%d:%08x\n"), i, v);
            picasso_vidinfo.clut[i] = v;
            changed = 1;
        }
    }
    return changed;
}

#endif

void getgfxoffset (int *dxp, int *dyp, int *mxp, int *myp) {
    //FIXME: WHAT DOES THIS DO?
    *dxp = 0;
    *dyp = 0;
    *mxp = 0;
    *myp = 0;
}

void toggle_fullscreen (int mode) {

}

int isfullscreen (void) {
    return 0;
}

void flush_line (struct vidbuffer *buffer, int line_no) {
    //printf("- flush_line %d\n", line_no);

    //scrlinebuf
#ifdef USE_LINEMEM
    unsigned char *dst = libamiga_rd.pixels + AMIGA_WIDTH * 4 * line_no;
    memcpy(dst, g_linemem, AMIGA_WIDTH * 4);
#endif

#ifndef USE_BUFMEM
    // mark this line as not needing copy from the previous render buffer
    libamiga_rd.line[line_no] = 0;
#endif
    g_screen_updated = 1;
    g_has_flushed_line = 1;
}

void flush_block (struct vidbuffer *buffer, int first_line, int last_line) {
    //printf("- flush_block %d %d\n", first_line, last_line);
    //g_screen_updated = 1;
    g_has_flushed_block = 1;
}

int lockscr(struct vidbuffer *buffer, bool fullupdate) {
    return gfxvidinfo.drawbuffer.lockscr(&gfxvidinfo, buffer);
}

void unlockscr(struct vidbuffer *buffer) {
    gfxvidinfo.drawbuffer.unlockscr(&gfxvidinfo, buffer);
}

int graphics_setup() {
    write_log("graphics_setup\n");
    return 1;
}

static void grow_render_buffer(int width, int height) {
    libamiga_rd.pixels = (unsigned char*) libamiga_rd.grow(width, height);
}

void amiga_set_render_buffer(void *data, int size, int need_redraw,
        void *(*grow)(int width, int height)) {

    libamiga_rd.grow = grow;
    grow_render_buffer(g_largest_width, g_largest_height);

    //printf("\n\n\n\n\n\n\n\n set buffer %p %d\n", data, size);
	//libamiga_rd.pixels = (unsigned char*) data;
	//libamiga_rd.pixels = (unsigned char*) data;

#ifndef USE_BUFMEM
	// reset line information
	memset(libamiga_rd.line, 1, AMIGA_MAX_LINES);
#ifndef USE_LINEMEM
	//printf("setting bufmem\n");
    gfxvidinfo.drawbuffer.bufmem = (unsigned char*) data;
#endif
    //printf("updating row map\n");
    init_row_map();
#endif
}

void gfx_set_picasso_state (int on) {
    write_log("gfx_set_picasso_state %d\n", on);
    g_picasso_enabled = (on != 0);
}

void gfx_set_picasso_modeinfo (uae_u32 w, uae_u32 h, uae_u32 depth,
        RGBFTYPE rgbfmt) {
    write_log("gfx_set_picasso_modeinfo %d %d %d %d\n", w, h, depth, rgbfmt);
    g_picasso_width = w;
    g_picasso_height = h;
    g_picasso_depth = depth;
    g_picasso_format = rgbfmt;

    // register largest width seen, so render buffers can be adjusted if
    // necessary
    if (g_picasso_width > g_largest_width) {
        g_largest_width = g_picasso_width;
    }
    if (g_picasso_height > g_largest_height) {
        g_largest_height = g_picasso_height;
    }
    grow_render_buffer(g_largest_width, g_largest_height);

    gfx_set_picasso_colors (rgbfmt);
}

uint8_t *uae_get_render_buffer() {
    return libamiga_rd.pixels;
}

int graphics_init(void) {
    write_log("graphics_init\n");

    g_red_bits    = uae_bits_in_mask(0x000000ff);
    g_green_bits  = uae_bits_in_mask(0x0000ff00);
    g_blue_bits   = uae_bits_in_mask(0x00ff0000);
    g_alpha_bits   = uae_bits_in_mask(0xff000000);
    g_red_shift   = uae_mask_shift(0x000000ff);
    g_green_shift = uae_mask_shift(0x0000ff00);
    g_blue_shift  = uae_mask_shift(0x00ff0000);
    g_alpha_shift  = uae_mask_shift(0xff000000);

    if (g_amiga_video_format == AMIGA_VIDEO_FORMAT_BGRA) {
        g_blue_bits    = uae_bits_in_mask(0x000000ff);
        g_blue_shift   = uae_mask_shift(0x000000ff);
        g_red_bits   = uae_bits_in_mask(0x00ff0000);
        g_red_shift  = uae_mask_shift(0x00ff0000);
    }

    //libamiga_rd.pixels = (unsigned char*) malloc(AMIGA_WIDTH*AMIGA_HEIGHT*4);

    memset(libamiga_rd.line, 0, AMIGA_MAX_LINES);
    gfxvidinfo.maxblocklines = 0;
#ifdef USE_BUFMEM
    g_bufmem = (unsigned char*) malloc(AMIGA_WIDTH * AMIGA_HEIGHT * 4);
    gfxvidinfo.drawbuffer.bufmem = g_bufmem;
    memset(g_bufmem, 0, AMIGA_WIDTH * AMIGA_HEIGHT * 4);
    gfxvidinfo.maxblocklines = MAXBLOCKLINES_MAX;
#endif

#ifdef USE_LINEMEM
    gfxvidinfo.drawbuffer.emergmem = 0;
    gfxvidinfo.drawbuffer.linemem = g_linemem;
#else
    gfxvidinfo.drawbuffer.emergmem = 0; //g_linemem;
    gfxvidinfo.drawbuffer.linemem = 0;
#endif
    gfxvidinfo.drawbuffer.pixbytes = 4;
    gfxvidinfo.drawbuffer.rowbytes = AMIGA_WIDTH * 4;
    gfxvidinfo.drawbuffer.height = AMIGA_HEIGHT;
    gfxvidinfo.drawbuffer.inheight = AMIGA_HEIGHT;
    gfxvidinfo.drawbuffer.outheight = AMIGA_HEIGHT;
    gfxvidinfo.drawbuffer.width = AMIGA_WIDTH;
    gfxvidinfo.drawbuffer.inwidth = AMIGA_WIDTH;
    gfxvidinfo.drawbuffer.outwidth = AMIGA_WIDTH;

    //gfxvidinfo.flush_block = libamiga_flush_block;
    //gfxvidinfo.flush_screen = libamiga_flush_screen;
    //SDL_SetColors (display, arSDLColors, 0, 256);
    write_log("calling reset_drawing\n");
    reset_drawing ();
    init_colors ();

//#ifdef USE_BUFMEM
//    init_row_map();
//#endif

    //write_log("FIXME: NOT USING VSYNC TRICK\n");
    // Trick UAE into sending believing we are in vsync / fullscreen
    // so a flush command is sent for each frame update in do_flush_screen.

    if (currprefs.m68k_speed == -1) {
        write_log("currprefs.m68k_speed is -1, not allowing full sync\n");
    }
    else {
        //currprefs.gfx_apmode[0].gfx_fullscreen = GFX_FULLSCREEN;
        currprefs.gfx_apmode[0].gfx_vsync = 1;
    }

    //currprefs.gfx_apmode[0].gfx_fullscreen = GFX_FULLSCREEN;
    //currprefs.gfx_apmode[0].gfx_vsync = 1;

    //currprefs.gfx_apmode[0].gfx_vsyncmode = 1;
    //currprefs.gfx_apmode[1].gfx_fullscreen = GFX_FULLSCREEN;
    //currprefs.gfx_apmode[1].gfx_vsync = 1;

    //amiga_set_option("gfx_vsync", "true");
    //amiga_set_option("gfx_vsyncmode", "busywait");

    return 1;
}

void graphics_leave (void) {

}

int check_prefs_changed_gfx (void) {
    //write_log("check_prefs_changed_gfx\n");
    return 0;
}

void refreshtitle (void) {
    STUB("");
}

void updatedisplayarea (void) {
    STUB("");
}

void gui_fps(int fps, int idle) {
    //double ffps = (fps + 5) / 10;
    //write_log("fps %0.1f idle %d\n", ffps, idle);

}

int gui_update (void) {
    return 0;
}
