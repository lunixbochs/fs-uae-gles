#include "menu.h"

#include <SDL.h>

#include "render.h"
#include "libfsemu.h"
#include "texture.h"
#include "video.h"
#include "font.h"

static fs_emu_font *g_font_title = NULL;
static fs_emu_font *g_font_menu = NULL;

static fs_emu_menu* g_menu = NULL;
static fs_emu_menu* g_top_menu = NULL;
static GList* g_menu_stack = NULL;
static int g_top_menu_focus = 0;

// FIXME: set to 0 later
int g_fs_emu_menu_mode = 0;

static int g_update_current_menu = 0;

void fs_emu_update_current_menu() {
	g_update_current_menu = 1;
}

fs_emu_font *fs_emu_font_get_title() {
	return g_font_title;
}

fs_emu_font *fs_emu_font_get_menu() {
	return g_font_menu;
}

int fs_emu_menu_or_dialog_is_active() {
    return g_fs_emu_menu_mode || fs_emu_in_dialog_mode();
}

int fs_emu_menu_is_active() {
    return g_fs_emu_menu_mode;
}

void fs_emu_menu_toggle() {
    fs_emu_log("fs_emu_toggle_menu\n");
    if (g_fs_emu_menu_mode) {
        // leave menu
        g_fs_emu_menu_mode = 0;
        fs_emu_pause(0);
    }
    else {
        // enter menu
        //fs_emu_pause(1);
        g_fs_emu_menu_mode = 1;
        if (g_menu) {
            if (g_menu->update) {
                g_menu->update(g_menu);
            }
        }
    }
}

fs_emu_menu *fs_emu_get_menu() {
    return g_menu;
}

static void select_next() {
    if (g_menu == NULL) {
        return;
    }
    int index = g_menu->index + 1;
    while (index < g_menu->count) {
        if (g_menu->items[index]->type == FS_EMU_MENU_ITEM_TYPE_HEADING) {
            index++;
            continue;
        }
        g_menu->index = index;
        break;
    }
}

static int on_quit(fs_emu_menu_item* item, void **result_data) {
    fs_emu_log("on_quit selected from menu\n");
    fs_emu_quit();
    return FS_EMU_MENU_RESULT_NONE;
}

static int on_volume(fs_emu_menu_item* item, void **result_data) {
    fs_emu_log("on_volume selected from menu\n");
    if (fs_emu_audio_get_volume() > 0.0) {
        fs_emu_audio_set_volume(0.0);
    }
    else {
        fs_emu_audio_set_volume(1.0);
    }
    return FS_EMU_MENU_RESULT_NONE;
}

static int on_aspect(fs_emu_menu_item* item, void **result_data) {
    fs_emu_log("on_aspect selected from menu\n");
    fs_emu_video_set_aspect_correction(!fs_emu_video_get_aspect_correction());
    return FS_EMU_MENU_RESULT_NONE;
}

static int go_back_in_menu_stack() {
    if (g_menu_stack != NULL) {
        fs_unref(g_menu);
        GList* last = g_list_last(g_menu_stack);
        g_menu = last->data;
        g_menu_stack = g_list_delete_link(g_menu_stack, last);
        if (g_menu->update) {
            g_menu->update(g_menu);
        }
        return 1;
    }
    return 0;
}

void fs_emu_menu_function(int action, int state) {
	fs_emu_acquire_gui_lock();
	fs_emu_dialog *dialog = fs_emu_get_current_dialog();
	if (dialog) {
		fs_emu_handle_dialog_action(action, state);
		fs_emu_release_gui_lock();
		return;
	}

    if (state == 0 || g_menu == NULL) {
    	fs_emu_release_gui_lock();
        return;
    }

    //printf("fs_emu_menu_function %d %d\n", action, state);
    // we now lock the video render mutex so we do not change rendering
    // state while actually rendering the menu

    // FIXME: Render mutex lock is deprecated
    fs_emu_video_render_mutex_lock();
    if (action == ACTION_MENU_UP) {
        if (g_top_menu_focus) {

        }
        else {
            int last_index = g_menu->index;
            int index = g_menu->index - 1;
            while (index >= 0) {
                if (g_menu->items[index]->type == \
                        FS_EMU_MENU_ITEM_TYPE_HEADING) {
                    index--;
                    continue;
                }
                g_menu->index = index;
                break;
            }
            if (g_menu->index == last_index) {
                // we were already at the top-most selectable item, so
                // we move focus to the top menu
                g_top_menu_focus = 1;
            }
        }
    }
    else if (action == ACTION_MENU_DOWN) {
        if (g_top_menu_focus) {
            g_top_menu_focus = 0;
        }
        else {
            select_next();
        }
    }
    else if (action == ACTION_MENU_LEFT) {
        if (g_top_menu_focus) {
            if (g_top_menu->index == 0) {
                // special case: skip clock
                g_top_menu->index = 2;
            }
            else if (g_top_menu->index < g_top_menu->count - 1) {
                g_top_menu->index++;
            }
        }
    }
    else if (action == ACTION_MENU_RIGHT) {
        if (g_top_menu_focus) {
            if (g_top_menu->index == 2) {
                // special case: skip clock
                g_top_menu->index = 0;
            }
            else if (g_top_menu->index > 0) {
                g_top_menu->index--;
            }
        }
    }
    else if (action == ACTION_MENU_BACK) {
        go_back_in_menu_stack();
    }
    else if (action == ACTION_MENU_ESCAPE) {
        if (!go_back_in_menu_stack()) {
            // no more menus to back out of, go out of menu mode instead
            fs_emu_menu_toggle();
        }
    }
    else if (action == ACTION_MENU_PRIMARY) {
        if (g_top_menu_focus) {
            if (g_top_menu->items[g_top_menu->index]->activate) {
                void *result_data = NULL;
                g_top_menu->items[g_top_menu->index]->activate(
                        g_top_menu->items[g_top_menu->index], &result_data);
            }
        }
        else {
            fs_emu_menu_item *item = g_menu->items[g_menu->index];
            if (item->activate && !item->disabled) {
                void *result_data;
                int result = item->activate(item, &result_data);
                if (result & FS_EMU_MENU_RESULT_CLOSE) {
                    // FIXME: closing the menu here may mean that the state
                    // for the enter key (if it was used to activate an
                    // item) will not be properly reset, disabling for now
                    fs_emu_menu_toggle();
                    fs_emu_clear_menu_input_states();
                    //if (g_menu->update) {
                    //    g_menu->update(g_menu);
                    //}
                }
                if (result & FS_EMU_MENU_RESULT_MENU) {
                    g_menu_stack = g_list_append(g_menu_stack, g_menu);
                    g_menu = result_data;
                    if (g_menu->update) {
                        g_menu->update(g_menu);
                    }
                    while(g_menu->index < g_menu->count - 1 && \
                            g_menu->items[g_menu->index]->type == \
                            FS_EMU_MENU_ITEM_TYPE_HEADING) {
                        select_next();
                    }
                }
                if (result & FS_EMU_MENU_RESULT_UPDATE) {
                    if (g_menu->update) {
                        g_menu->update(g_menu);
                    }
                }
                if (result & FS_EMU_MENU_RESULT_BACK) {
                    go_back_in_menu_stack();
                }
                if (result & FS_EMU_MENU_RESULT_ROOT) {
                    while (g_menu_stack) {
                        go_back_in_menu_stack();
                    }
                }
            }
        }
    }
    fs_emu_video_render_mutex_unlock();
    fs_emu_release_gui_lock();
}

void fs_emu_set_menu(fs_emu_menu *menu) {
    fs_emu_log("fs_emu_set_menu %p\n", menu);
    g_menu = menu;
    if (g_menu->items[g_menu->index]->type == FS_EMU_MENU_ITEM_TYPE_HEADING) {
        select_next();
    }
}

void fs_emu_menu_destroy(void* ptr) {
    fs_log("fs_emu_menu_destroy\n");
    fs_emu_menu* menu = ptr;
    for (int i = 0; i < menu->count; i++) {
        fs_unref(menu->items[i]);
    }
    g_free(menu);
}

fs_emu_menu *fs_emu_menu_new() {
    fs_emu_menu *menu = g_malloc(sizeof(fs_emu_menu));
    memset(menu, 0, sizeof(fs_emu_menu));
    fs_ref_initialize(menu, fs_emu_menu_destroy);
    return menu;
}

void fs_emu_menu_item_destroy(void* ptr) {
    //fs_log("fs_emu_menu_item_destroy\n");
    fs_emu_menu_item* item = ptr;
    /*
    if (image->data) {
        free(image->data);
    }
    */
    if (item->title) {
        g_free(item->title);
    }
    g_free(item);
}

fs_emu_menu_item *fs_emu_menu_item_new() {
    fs_emu_menu_item *item = g_malloc(sizeof(fs_emu_menu_item));
    memset(item, 0, sizeof(fs_emu_menu_item));
    fs_ref_initialize(item, fs_emu_menu_item_destroy);
    return item;
}

void fs_emu_menu_append_item(fs_emu_menu *menu,
        fs_emu_menu_item *item) {
    menu->items[menu->count] = item;
    menu->count += 1;
}

fs_emu_menu_item *fs_emu_menu_item_at(fs_emu_menu *menu, int index) {
    //return (fs_emu_menu_item *) g_list_nth_data(menu->items, index);
    return menu->items[index];
}

int fs_emu_menu_item_count(fs_emu_menu *menu) {
    //return (fs_emu_menu_item *) g_list_length(menu->items);
    return menu->count;
}

void fs_emu_menu_item_set_type(fs_emu_menu_item *item, int type) {
    item->type = type;
}

void fs_emu_menu_item_set_enabled(fs_emu_menu_item *item, int enabled) {
    item->disabled = !enabled;
}

void fs_emu_menu_item_set_title(fs_emu_menu_item *item, const gchar* title) {
    if (item->title) {
        g_free(item->title);
    }
    item->title = g_strdup(title);
}

int fs_emu_menu_item_get_idata(fs_emu_menu_item *item) {
    return item->idata;
}

void *fs_emu_menu_item_get_pdata(fs_emu_menu_item *item) {
    return item->pdata;
}

void fs_emu_menu_item_set_idata(fs_emu_menu_item *item, int idata) {
    item->idata = idata;
}

void fs_emu_menu_item_set_pdata(fs_emu_menu_item *item, void *pdata) {
    item->pdata = pdata;
}

void fs_emu_menu_set_update_function(fs_emu_menu *menu,
        void (*function)(fs_emu_menu* menu)) {
    menu->update = function;
}

void fs_emu_menu_item_set_activate_function(fs_emu_menu_item *item,
        int (*function)(fs_emu_menu_item* menu_item, void **result)) {
    item->activate = function;
}

static void initialize() {
    static int initialized = 0;
    if (initialized == 1) {
        return;
    }
    initialized = 1;
    fs_emu_log("initializing menu\n");

    g_font_title = fs_emu_font_new_from_file("title_font");
    g_font_menu = fs_emu_font_new_from_file("menu_font");

    g_top_menu = fs_emu_menu_new();
    fs_emu_menu_item *item;
    // close item
    item = fs_emu_menu_item_new();
    fs_emu_menu_item_set_activate_function(item, on_quit);
    fs_emu_menu_append_item(g_top_menu, item);
    // clock item
    item = fs_emu_menu_item_new();
    //g_top_menu = g_list_append(g_top_menu, item);
    fs_emu_menu_append_item(g_top_menu, item);
    // volume item
    item = fs_emu_menu_item_new();
    fs_emu_menu_item_set_activate_function(item, on_volume);
    //g_top_menu = g_list_append(g_top_menu, item);
    fs_emu_menu_append_item(g_top_menu, item);
    // aspect item
    item = fs_emu_menu_item_new();
    fs_emu_menu_item_set_activate_function(item, on_aspect);
    //g_top_menu = g_list_append(g_top_menu, item);
    fs_emu_menu_append_item(g_top_menu, item);
}

void fs_emu_menu_init_opengl() {
    initialize();
}

#define GROUP_SPACING 26

#define HEADING_TEXT_LEFT (1920 - 560 + 40 + 20 - 28)
#define ITEM_TEXT_LEFT    (1920 - 560 + 40 + 20)
#define SIDEBAR_START_Y   (1080 - 90 - 50)

static int render_sidebar_item(int mode, int sel, fs_emu_menu *menu,
        int index, int y) {
    int selected = (menu->index == index) && !g_top_menu_focus;
    fs_emu_menu_item *item = fs_emu_menu_item_at(menu, index);
    int result = 0;
    if (item->type == FS_EMU_MENU_ITEM_TYPE_HEADING) {
        if (index > 0) {
            y -= GROUP_SPACING;
            result = 1;
        }
    }
    if (sel != selected) {
        return result;
    }
    //int x = 1400;
    //int x = 1920 - 560 + 40;
    //int xpadd = 20;
    double r = 1.0;
    double g = 1.0;
    double b = 1.0;
    double alpha = 1.0;

    int text_left = ITEM_TEXT_LEFT;

    if (item->type == FS_EMU_MENU_ITEM_TYPE_HEADING) {
        text_left = HEADING_TEXT_LEFT;
        //alpha = 0.4;
        r = 0.0;
        g = 1.0 * 0x99 / 0xff;
        b = 1.0 * 0xcc / 0xff;

        int x = 0;
        fs_emu_font_measure(g_font_menu, item->title, &x, NULL);
        x += text_left + 12;
        fs_emu_draw_texture_with_size(TEXTURE_HEADING_BG,
                x, y + 14, 1920 - x, 32);
    }
    else {
        if (selected) {
            if (mode == 0) {
                fs_gl_blending(1);
                fs_gl_color4f(1.0, 1.0, 1.0, 1.0);
                fs_emu_draw_texture(TEXTURE_ITEM_BACKGROUND,
                        1920 - 540 + 13, y - 18);
            }
        }
        if (item->disabled) {
            //fs_gl_color4f(1.0, 1.0, 1.0, 0.3);
            alpha = 0.25;
        }
    }
    if (mode == 1) {
        fs_emu_font_render(g_font_menu, item->title, text_left, y + 14,
                r, g, b, alpha);
        /*
        if (selected) {
            fs_emu_texture_render(g_tex_menu_fade_selected_item,
                    1920 - g_tex_menu_fade_selected_item->width, y - 20);
        }
        else {
            fs_emu_texture_render(g_tex_menu_fade_item,
                    1920 - g_tex_menu_fade_item->width, y - 20);
        }
        */
    }

    if (mode == 0) {
        /*
        if (item->type == FS_EMU_MENU_ITEM_TYPE_MENU) {
            if (!item->disabled) {
                //fs_emu_font_render(g_font_title, ">", 1880, y + 12, 0.7);
                fs_emu_texture_render(g_tex_menu_arrow,
                        1920 - g_tex_menu_arrow->width, y - 20);
            }
        }
        */
    }
    return result;
}

static void fs_emu_render_sidebar(int mode) {
    //printf("-- render sidebar mode = %d\n", mode);
    fs_emu_menu *menu = fs_emu_get_menu();
    if (!menu) {
        return;
    }
    if (mode == 0) {
        if (g_update_current_menu) {
        	g_update_current_menu = 0;
        	if (menu->update) {
        		menu->update(g_menu);
        	}
        }
    }
    int count = menu->count;
    //int y = 938;
    int y = SIDEBAR_START_Y;
    for (int i = 0; i < count; i++) {
        int result = render_sidebar_item(mode, 0, menu, i, y);
        if (result) {
            y -= GROUP_SPACING;
        }
        y -= 54;
    }
    y = SIDEBAR_START_Y;
    for (int i = 0; i < count; i++) {
        int result = render_sidebar_item(mode, 1, menu, i, y);
        if (result) {
            y -= GROUP_SPACING;
        }
        y -= 54;
    }
    //printf("-- render sidebar (end)\n");
}

static void render_top_item_background(int selected, int x, int y, int w,
        int h) {
    if (!selected) {
        return;
    }
    fs_gl_blending(1);
    fs_gl_color4f(1.0, 1.0, 1.0, 1.0);
    fs_emu_draw_texture_with_size(TEXTURE_TOP_ITEM_BG, x, y, w, h);
}

static void render_top_item(int mode, int index) {
    int selected = g_top_menu_focus && index == g_top_menu->index;

    int x = 1920;
    int y = 1080 - 60; // textures are 64 px high, but transparent on the
                       // the edges
    x = x - 80;

    if (index == 0) {
        // close
        if (mode == 0) {
            //fs_emu_texture_render(top_item_left, 1920 - 72, 1080 - 90);
            render_top_item_background(selected, x, y, 80, 60);
            //fs_gl_texturing(0);
            //fs_gl_texturing(1);
            fs_gl_blending(1);
            fs_gl_color4f(1.0, 1.0, 1.0, 1.0);
            fs_emu_draw_texture(TEXTURE_CLOSE, x + 8, y - 2);
        }
        return;
    }

    x = x - 148;

    if (index == 1) {
        // clock
        if (mode == 1) {
            time_t t = time(NULL);
            struct tm *tm_struct = localtime(&t);
            char strbuf[6];
            strftime(strbuf, 6, "%H:%M", tm_struct);
            //strftime(strbuf, 6, "00:00", tm_struct);
            int w = 0;
            fs_emu_font_measure(g_font_title, strbuf, &w, NULL);
            //fs_log("%d\n", w);
            //int rx = 1920 - 20 - 72; // - 20;
            //int ry = 1080 - 60 + (60 - g_font_title->h) / 2;
            fs_emu_font_render(g_font_title, strbuf,
                    x + 148 - 20 - w , 1080 - 60 + (60 - g_font_title->h) / 2,
                    1.0, 1.0, 1.0, 1.0);
        }
        return;
    }

    x = x - 80;

    if (index == 2) {
        if (mode == 0) {
            //fs_emu_texture *texture = NULL;
            int texture = TEXTURE_VOLUME;
            if (fs_emu_audio_get_volume() == 0.0) {
                //texture = g_tex_volume_mute;
                texture = TEXTURE_VOLUME_MUTED;
            }
            render_top_item_background(selected, x, y, 80, 60);
            //fs_emu_texture_render(texture, x, y);

            fs_gl_blending(1);
            fs_gl_color4f(1.0, 1.0, 1.0, 1.0);
            fs_emu_draw_texture(texture, x + 8, y - 2);
        }
        return;
    }

    x = x - 80;

    if (index == 3) {
        if (mode == 0) {
            //fs_emu_texture *texture = NULL;
            int texture = TEXTURE_STRETCH;
            if (fs_emu_video_get_aspect_correction()) {
                //texture = g_tex_aspect_correct;
                texture = TEXTURE_ASPECT;
            }
            //else {
            //    texture = g_tex_aspect_stretch;
            //}
            render_top_item_background(selected, x, y, 80, 60);
            //fs_emu_texture_render(texture, x, y);

            fs_gl_blending(1);
            fs_gl_color4f(1.0, 1.0, 1.0, 1.0);
            fs_emu_draw_texture(texture, x + 8, y - 2);
        }
        return;
    }
}

void fs_emu_render_menu(double transition) {
    initialize();
    fs_gl_ortho_hd();

    glPushMatrix();
    glTranslated((1.0 - transition) * 600, 0, 0);
    fs_gl_blending(1);
    fs_gl_color4f(1.0, 1.0, 1.0, 1.0);
    fs_emu_draw_texture(TEXTURE_SIDEBAR_EDGE, 1920 - 600, 0);
    fs_emu_draw_texture_with_size(TEXTURE_SIDEBAR, 1920 - 600 + 50, 0,
            600 - 50, 1020);
    fs_emu_render_sidebar(0);
    fs_emu_render_sidebar(1);
    glPopMatrix();

    glPushMatrix();
    glTranslated(0, (1.0 - transition) * 60.0, 0);
    /*
    glTranslated(0, (1.0 - transition) * g_tex_top_background->height, 0);
    fs_emu_set_texture(g_tex_top_background);
    fs_emu_render_texture_with_size(g_tex_top_background, 0,
            1080 - g_tex_top_background->height, 1920,
            g_tex_top_background->height);
    */

    for (int i = 0; i < g_top_menu->count; i++) {
        render_top_item(0, i);
    }

    const gchar *title;
    if (g_fs_emu_title) {
        title = g_fs_emu_title;
    }
    else {
        title = g_get_application_name();
    }
    int w = fs_emu_font_render(g_font_title, title,
            20, 1080 - 60 + (60 - g_font_title->h) / 2, 1.0, 1.0, 1.0, 1.0);
    if (g_fs_emu_sub_title) {
        fs_emu_font_render(g_font_title, g_fs_emu_sub_title,
                20 + w + 20, 1080 - 60 + (60 - g_font_title->h) / 2,
                1.0, 1.0, 1.0, 0.4);
    }
    for (int i = 0; i < g_top_menu->count; i++) {
        render_top_item(1, i);
    }

    glPopMatrix();
}
