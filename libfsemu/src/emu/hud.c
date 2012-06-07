/* libfsemu - a library with emulator support functions
 * Copyright (C) 2011 Frode Solheim <frode-code@fengestad.no>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <fs/emu.h>
#include "libfsemu.h"
#include "render.h"
#include "menu.h"
#include "font.h"

#define MAX_VISIBLE_TIME (10 * 1000 * 1000)

char g_fs_emu_chat_string[FS_EMU_MAX_CHAT_STRING_SIZE + 1] = {};
static int g_fs_emu_chat_string_pos = 0;
static int g_fs_emu_chat_mode = 0;
static int64_t g_last_line_time = 0;
int g_fs_emu_hud_mode = 0;

typedef struct console_line {
    int64_t time;
    char *text;
} console_line;

static GQueue *g_console_lines = NULL;

void fs_emu_initialize_hud_module() {
    g_console_lines = g_queue_new();
}

int fs_emu_in_chat_mode() {
    return g_fs_emu_chat_mode;
}

void fs_emu_enable_chat_mode() {
    g_fs_emu_chat_mode = 1;
    g_fs_emu_hud_mode = 1;
}

void fs_emu_add_console_line(const char *text, int flags) {
    console_line *line = g_malloc(sizeof(console_line));
    line->text = g_strdup(text);
    line->time = fs_emu_monotonic_time();
    fs_emu_acquire_gui_lock();
    g_queue_push_head(g_console_lines, line);
    g_last_line_time = line->time;
    fs_emu_release_gui_lock();
}

void fs_emu_add_chat_message(const char *text, const char *player) {
    char *line;
    if (text[0] == 1) {
        line = g_strdup_printf("* %s taunts: %s", player, text);
    }
    else {
        line = g_strdup_printf("<%s> %s", player, text);
    }
    fs_emu_add_console_line(line, 0);
    g_free(line);
}

void fs_emu_say(const char *text) {
    fs_emu_send_netplay_message(text);
    //char *line = g_strdup_printf("<%s> %s",
     //       fs_emu_get_netplay_tag(-1), text);
    //fs_emu_add_console_line(line, 0);
    //g_free(line);
    fs_emu_add_chat_message(text, fs_emu_get_netplay_tag(-1));
}

static void process_command(const char* text) {
    if (text[0] == '\0') {
        return;
    }
    fs_log("process_command: %s\n", g_fs_emu_chat_string);
    if (text[0] != '/') {
        fs_emu_say(text);
    }
}

int fs_emu_handle_chat_input(fs_emu_event *event) {
    fs_emu_acquire_gui_lock();
    int key_code = event->key.keysym.sym;
    if (key_code == FS_ML_KEY_RETURN) {
        fs_emu_release_gui_lock();
        process_command(g_fs_emu_chat_string);
        fs_emu_acquire_gui_lock();
        g_fs_emu_chat_string_pos = 0;
        g_fs_emu_chat_string[g_fs_emu_chat_string_pos] = 0;
    }
    else if (key_code == FS_ML_KEY_ESCAPE) {
        g_fs_emu_chat_mode = 0;
        g_fs_emu_hud_mode = 0;
        g_fs_emu_chat_string_pos = 0;
        g_fs_emu_chat_string[g_fs_emu_chat_string_pos] = 0;
    }
    else if (key_code == FS_ML_KEY_TAB) {
        g_fs_emu_chat_mode = 0;
        g_fs_emu_hud_mode = 0;
        // hide all messages when dismissing the hud
        //g_last_line_time = 0;
    }
    else if (key_code == FS_ML_KEY_BACKSPACE) {
        if (g_fs_emu_chat_string_pos > 0) {
            g_fs_emu_chat_string_pos--;
            g_fs_emu_chat_string[g_fs_emu_chat_string_pos] = 0;
        }
    }
    else {
        //printf("%d\n", event->key.keysym.unicode);
        if (g_fs_emu_chat_string_pos < FS_EMU_MAX_CHAT_STRING_SIZE - 1 &&
                event->key.keysym.unicode >= 32 &&
                event->key.keysym.unicode < 128) {
            g_fs_emu_chat_string[g_fs_emu_chat_string_pos] =
                    (char) event->key.keysym.unicode;
            g_fs_emu_chat_string_pos++;
            g_fs_emu_chat_string[g_fs_emu_chat_string_pos] = '\0';
        }
    }
    //fs_log("chat: %s\n", g_fs_emu_chat_string);
    fs_emu_release_gui_lock();
    return 1;
}

#define MAX_VISIBLE_LINES 12

void fs_emu_render_chat() {
    GList *link;
    int k;

    fs_emu_assert_gui_lock();
    int64_t now = fs_emu_monotonic_time();

    int64_t time_diff = now - g_last_line_time;
    if (time_diff > MAX_VISIBLE_TIME && !g_fs_emu_chat_mode) {
        return;
    }

    // FIXME: IMPLEMENT A MAX NUMBER OF LINES
    // FIXME: IMPLEMENT SCROLLING / PAGE_UP / PAGE DOWN

    /*
    int total_height = 0;
    GList *link = g_queue_peek_head_link(g_console_lines);
    int k = 0;
    while (link) {
        console_line *line = (console_line *) link->data;
        total_height += 40;

        GList* link2 = link;
        link = link->next;
        k++;
        //g_queue_delete_link(g_console_lines, link2);
        if (k == MAX_VISIBLE_LINES) {
            break;
        }
    }
    */

    /*
    if (total_height == 0 && !g_fs_emu_chat_mode) {
        return;
    }
    */

    fs_gl_ortho_hd();
#if 0
    fs_gl_blending(1);
    fs_gl_texturing(0);

    if (total_height > 0) {
        fs_gl_color4f(0.0, 0.0, 0.0, 0.5);
        glBegin(GL_QUADS);
        glVertex2f(0, 60);
        glVertex2f(1920, 60);
        glVertex2f(1920, 60 + total_height);
        glVertex2f(0, 60 + total_height);
        glEnd();
        glBegin(GL_QUADS);
        glVertex2f(0, 60 + total_height);
        glVertex2f(1920, 60 + total_height);
        fs_gl_color4f(0.0, 0.0, 0.0, 0.0);
        glVertex2f(1920, 60 + total_height + 50);
        glVertex2f(0, 60 + total_height + 50);
        glEnd();
    }

    fs_gl_color4f(0.0, 0.0, 0.0, 0.0);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(1920, 0);
    fs_gl_color4f(0.0, 0.0, 0.0, 0.5);
    glVertex2f(1920, 60);
    glVertex2f(0, 60);
    glEnd();
#endif

    if (g_fs_emu_chat_mode) {
        fs_gl_blending(1);
        fs_gl_texturing(0);
        fs_gl_color4f(0.0, 0.3, 0.5, 0.75);
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(1920, 0);
        glVertex2f(1920, 60);
        glVertex2f(0, 60);
        glEnd();
    }

    fs_emu_font *font = fs_emu_font_get_menu();
    int tx;
    int ty;
    int tw;
    if (g_fs_emu_chat_mode) {
        tx = 65;
        ty = 13;
        tw = fs_emu_font_render(font, "Input:", tx, ty, 1.0, 1.0, 1.0, 1.0);
        tx += tw + 20;
        fs_emu_font_render(font, g_fs_emu_chat_string,
                tx, ty, 1.0, 1.0, 1.0, 1.0);
    }

    tx = 65;
    ty = 65;

    link = g_queue_peek_head_link(g_console_lines);
    k = 0;
    while (link) {
        console_line *line = (console_line *) link->data;
        if (!g_fs_emu_chat_mode && now - line->time > MAX_VISIBLE_TIME) {
            // when not in chat mode, only show lines for a brief period
            // of time
            break;
        }
        fs_emu_font_render_with_outline(font, line->text,
                tx, ty, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.5, 2.0);
        ty += 40;
        link = link->next;
        k++;
        if (k == MAX_VISIBLE_LINES) {
            break;
        }
    }

}
