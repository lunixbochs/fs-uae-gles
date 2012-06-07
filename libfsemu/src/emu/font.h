#ifndef LIBFSEMU_FONT_H_
#define LIBFSEMU_FONT_H_

#include "fs/emu.h"
#include "fs/image.h"
#include "texture.h"

typedef struct _fs_emu_font {
    //fs_emu_texture *texture;
    fs_image *image;
    //uint8_t *data;
    int width;
    int height;
    int x[256];
    int y[256];
    int w[256];
    int h;
} fs_emu_font;

fs_emu_font *fs_emu_font_new_from_file(const char *name);
int fs_emu_font_render(fs_emu_font *font, const char *text, float x, float y,
        float r, float g, float b, float alpha);
int fs_emu_font_render_with_outline(fs_emu_font *font, const char *text,
        float x, float y, float r, float g, float b, float alpha,
        float o_r, float o_g, float o_b, float o_a, float o_w);
void fs_emu_font_measure(fs_emu_font *font, const char *text, int* width,
        int *height);

fs_emu_font *fs_emu_font_get_title();
fs_emu_font *fs_emu_font_get_menu();

#endif // LIBFSEMU_FONT_H_
