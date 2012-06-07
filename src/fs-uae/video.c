#include <stdio.h>
#include <stdlib.h>
#include <uae/uae.h>
#include <fs/emu.h>
#include "fs-uae.h"

//#define MAX_ZOOM_MODES 5
static int g_zoom_mode = 0;
static int g_zoom_border = 0;

typedef struct zoom_mode {
    char *name;
    int x;
    int y;
    int w;
    int h;
} zoom_mode;

static zoom_mode g_zoom_modes[] = {
    { "Autoscale", 0, 0, 0, 0 },
    { "Full Frame", 0, 0, 752, 572 },
    { "640x512", 74, 36, 640, 512 },
    { "640x480", 74, 36, 640, 480 },
    { "640x400", 74, 36, 640, 400 },
    { NULL, 0, 0, 0, 0 },
};

int g_fs_uae_video_zoom = 1;

struct WindowOverride {
    int sx;
    int sy;
    int sw;
    int sh;
    int dx;
    int dy;
    int dw;
    int dh;
    struct WindowOverride* next;
};

static struct WindowOverride* g_window_override = NULL;
static struct WindowOverride* g_last_window_override = NULL;

static int g_frame_seq_no = 0;
static fs_emu_video_buffer *g_buffer = NULL;
static int g_remember_last_screen = 0;

static int g_use_rtg_scanlines = 0;
static int g_last_seen_mode_rtg = 0;

int read_window_override_int(const char* s, int* pos, int* out) {
    char temp[4];
    int read = 0;
    while(s[*pos] == ' ') ++(*pos);
    while (read < 3) {
        temp[read] = s[*pos];
        ++(*pos);
        ++read;
        char c= s[*pos];
        if (c >= '0' && c <= '9') {
            continue;
        }
        temp[read] = '\0';
        if (read == 1) {
            if (temp[0] == '*') {
                *out = -1;
                return 1;
            }
        }
        *out = atoi(temp);
        return 1;
    }
    // read failed
    return 0;
}

int read_window_override(const char* s, int* pos) {
    while(s[*pos] == ' ') ++(*pos);
    int sx, sy, sw, sh, dx, dy, dw, dh;
    if (!read_window_override_int(s, pos, &sx)) return 0;
    if (!read_window_override_int(s, pos, &sy)) return 0;
    if (!read_window_override_int(s, pos, &sw)) return 0;
    if (!read_window_override_int(s, pos, &sh)) return 0;
    while(s[*pos] == ' ') ++(*pos);
    if (!s[(*pos)++] == '=') return 0;
    if (!s[(*pos)++] == '>') return 0;
    if (!read_window_override_int(s, pos, &dx)) return 0;
    if (!read_window_override_int(s, pos, &dy)) return 0;
    if (!read_window_override_int(s, pos, &dw)) return 0;
    if (!read_window_override_int(s, pos, &dh)) return 0;
    fs_emu_log("viewport transformation: %3d %3d %3d %3d => %3d %3d %3d %3d\n",
            sx, sy, sw, sh, dx, dy, dw, dh);
    struct WindowOverride* wo = (struct WindowOverride*)
            malloc(sizeof(struct WindowOverride));
    wo->sx = sx;
    wo->sy = sy;
    wo->sw = sw;
    wo->sh = sh;
    wo->dx = dx;
    wo->dy = dy;
    wo->dw = dw;
    wo->dh = dh;
    wo->next = NULL;

    if (g_last_window_override == NULL) {
        g_window_override = wo;
    }
    else {
        g_last_window_override->next = wo;
    }
    g_last_window_override = wo;
    return 1;
}

void init_window_overrides() {
    char *s = fs_config_get_string("viewport");
    if (s == NULL) {
        return;
    }
    int pos = 0;
    while (1) {
        int result = read_window_override(s, &pos);
        if (!result) {
            fs_emu_log("error parsing wiewport transformation\n");
        }
        while(s[pos] == ' ') ++(pos);
        int c = s[(pos)++];
        if (c == ';') {
            continue;
        }
        else if (c == ',') {
            continue;
        }
        else if (c == '\0') {
            break;
        }
        else {
            fs_emu_log("unexpected end while reading viewport transformation\n");
            exit(1);
        }
    }
    free(s);
}

static int ucx = 0, ucy = 0, ucw = 0, uch = 0;
static int rd_width, rd_height;

static void modify_coordinates(int *cx, int *cy, int *cw, int *ch) {
    int changed = 0;
    int ocx = *cx;
    int ocy = *cy;
    int ocw = *cw;
    int och = *ch;
    if (*cx == 114 && *cy == 96 && *cw == 560 && *ch == 384) {
        fs_log("* amiga 600 kickstart screen?\n");
        *cx = 74; *cy = 92; *cw = 640; *ch = 400; changed = 1;
    }
    else if (*cx == 114 && *cy == 99 && *cw == 560 && *ch == 384) {
        fs_log("* amiga 1200 kickstart screen?\n");
        *cx = 74; *cy = 92; *cw = 640; *ch = 400; changed = 1;
    }
    else if (*cx == 74 && *cy == 30 && *cw == 640 && *ch == 518) {
        fs_log("* workbench 1.3/2.0 screen?\n");
        *cx = 74; *cy = 36; *cw = 640; *ch = 512; changed = 1;
    }
    else if (*cx == 6 && *cy == 36 && *cw == 724 && *ch == 512) {
        fs_log("* workbench screen with too much border?\n");
        *cx = 74; *cy = 36; *cw = 640; *ch = 512; changed = 1;
    }
    else if (*cx == 6 && *cy == 6 && *cw == 724 && *ch == 566) {
        fs_log("* workbench screen with overscan incorrectly placed?\n");
        *cx = 2; *cy = 6; *cw = 724; *ch = 566; changed = 1;
    }
    else if (*cx == 10 && *cy == 7 && *cw == 716 && *ch == 566) {
        fs_log("* amiga cd32 boot screen?\n");
        *cx = 10; *cy = 7; *cw = 704; *ch = 562; changed = 1;
    }
    else if (*cx == 6 && *cy == 96 && *cw == 724 && *ch == 478) {
        fs_log("* amiga cd32 boot screen (booting cd)?\n");
        *cx = 10; *cy = 7; *cw = 704; *ch = 562; changed = 1;
    }
    if (changed) {
        fs_log("* %3d %3d %3d %3d [ %3d %3d %3d %3d ]\n", *cx, *cy, *cw, *ch,
                ocx, ocy, ocw, och);
    }
    return;
}

static void render_screen(RenderData* rd) {
#if 0
    static int64_t last_time = 0;
    int64_t t = fs_emu_monotonic_time();
    int dt = (int) (t - last_time);
    // if we loose a frame in vsync mode, we should have a delay of about
    // 40 ms since last frame
    if (dt > 35 * 1000) {
        printf("fs-uae:render_screen dt %0.2f\n", dt / 1000.0);
    }
    last_time = t;
#endif

#if 0
    if (!rd->updated && !g_remember_last_screen) {
        // video display was not updated, so we want to display the
        // previous one more time
        g_buffer = fs_emu_get_current_video_buffer();
        return;
    }
#endif

    rd_width = rd->width;
    rd_height = rd->height;
    static int lastcx = 0, lastcy = 0, lastcw = 0, lastch = 0;
    // crop rect (autoscale)
    int cx = rd->limit_x, cy = rd->limit_y, cw = rd->limit_w, ch = rd->limit_h;
    // FIXME: custom limits seems to overreport cy with one amiga
    // pixel, at least sometimes.
    /* FIXED?
    if (ch == 510 || ch == 398) {
        cy -= 2;
        ch += 2;
    }
    */
    if (lastcx != cx || lastcy != cy || lastcw != cw || lastch != ch) {
        lastcx = cx;
        lastcy = cy;
        lastcw = cw;
        lastch = ch;
        modify_coordinates(&cx, &cy, &cw, &ch);
        struct WindowOverride* wo = g_window_override;
        while (wo != NULL) {
            if ((wo->sx == -1 || wo->sx == cx) &&
                    (wo->sy == -1 || wo->sy == cy) &&
                    (wo->sw == -1 || wo->sw == cw) &&
                    (wo->sh == -1 || wo->sh == ch)) {
                ucx = wo->dx == -1 ? cx : wo->dx;
                ucy = wo->dy == -1 ? cy : wo->dy;
                ucw = wo->dw == -1 ? cw : wo->dw;
                uch = wo->dh == -1 ? ch : wo->dh;
                fs_emu_log("> %3d %3d %3d %3d [ %3d %3d %3d %3d ]\n",
                        ucx, ucy, ucw, uch, cx, cy, cw, ch);
                break;
            }
            wo = wo->next;
        }
        if (wo == NULL) {
            ucx = cx;
            ucy = cy;
            ucw = cw;
            uch = ch;
            fs_emu_log("> %3d %3d %3d %3d\n", ucx, ucy, ucw, uch);
        }
    }
    float tx0, ty0, tx1, ty1; //source buffer coords
    fs_emu_rect crop;

    crop.x = 0;
    crop.y = 0;
    crop.w = rd_width;
    crop.h = rd_height;

    if (!(rd->flags & AMIGA_RTG_BUFFER_FLAG)) {

        if (g_fs_uae_video_zoom && ucw > 0 && uch > 0) { // autoscale
            if (g_zoom_mode == 0) {
                crop.x = ucx;
                crop.y = ucy;
                crop.w = ucw;
                crop.h = uch;
            }
        }
        if (g_zoom_mode > 0) {
            crop.x = g_zoom_modes[g_zoom_mode].x;
            crop.y = g_zoom_modes[g_zoom_mode].y;
            crop.w = g_zoom_modes[g_zoom_mode].w;
            crop.h = g_zoom_modes[g_zoom_mode].h;
        }
        if (g_zoom_border) {
            crop.x -= 10;
            crop.y -= 10;
            crop.w += 20;
            crop.h += 20;
        }
        if (crop.x < 0) {
            crop.x = 0;
        }
        if (crop.y < 0) {
            crop.y = 0;
        }
        if (crop.x + crop.w > rd_width) {
            crop.w = rd_width - crop.x;
        }
        if (crop.y + crop.h > rd_height) {
            crop.h = rd_height - crop.y;
        }
    }
    //fs_emu_update_video(rd->pixels, AMIGA_WIDTH, AMIGA_HEIGHT, 4, &crop);

    g_buffer->seq = g_frame_seq_no++;
    g_buffer->width = rd_width;
    g_buffer->height = rd_height;
    g_buffer->bpp = 4;
    g_buffer->crop = crop;
    g_buffer->flags = 0;
    if (rd->flags & AMIGA_RTG_BUFFER_FLAG) {
        g_buffer->flags = FS_EMU_FORCE_VIEWPORT_CROP_FLAG;
        if (g_use_rtg_scanlines == 0) {
            g_buffer->flags |= FS_EMU_NO_SCANLINES_FLAG;
        }
    }
    g_last_seen_mode_rtg = rd->flags & AMIGA_RTG_BUFFER_FLAG;
    memcpy(g_buffer->line, rd->line, AMIGA_MAX_LINES);
}

static void *grow_buffer(int width, int height) {
    fs_emu_grow_render_buffer(g_buffer, width, height);
    return g_buffer->data;
}

static void display_screen() {

    static int64_t last_time = 0;
    int64_t t = fs_emu_monotonic_time();
    if (last_time > 0) {
        int dt = (t - last_time) / 1000;
        //printf("%d\n", dt);
    }

    fs_emu_set_video_buffer(g_buffer);

    g_buffer = fs_emu_get_available_video_buffer(g_remember_last_screen);
    amiga_set_render_buffer(g_buffer->data, g_buffer->size,
            !g_remember_last_screen, grow_buffer);

    fs_emu_video_render(1);

    last_time = fs_emu_monotonic_time();
}

static void toggle_zoom(int flags) {
    if (g_last_seen_mode_rtg) {
        return;
    }

    if (flags == 1) {
        g_zoom_border = !g_zoom_border;
    }
    else {
        //if (g_zoom_mode < MAX_ZOOM_MODES - 1) {
        if (g_zoom_modes[g_zoom_mode + 1].name) {
            g_zoom_mode += 1;
        }
        else {
            g_zoom_mode = 0;
        }
    }
    if (g_zoom_border) {
        fs_emu_warning("Zoom mode: %s (+ border)",
                g_zoom_modes[g_zoom_mode].name);
    }
    else {
        fs_emu_warning("Zoom mode: %s", g_zoom_modes[g_zoom_mode].name);
    }
}

void fs_uae_init_video(void) {
    fs_log("fs_uae_init_video\n");
    init_window_overrides();
    fs_emu_initialize_video_buffers(1024, 1024, 4);

    g_buffer = fs_emu_get_available_video_buffer(g_remember_last_screen);
    amiga_set_render_buffer(g_buffer->data, g_buffer->size,
            !g_remember_last_screen, grow_buffer);
    amiga_set_render_function(render_screen);
    amiga_set_display_function(display_screen);
    if (fs_config_get_boolean("rtg_scanlines") == 1) {
        g_use_rtg_scanlines = 1;
    }

    fs_emu_set_toggle_zoom_function(toggle_zoom);
}
