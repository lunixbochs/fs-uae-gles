#include "texture.h"

#include <fs/glee.h>
#include <SDL.h>
#ifdef HAVE_GLES
#include <GLES/gl.h>
#include <GLES/glues.h>
#else
#include <SDL_opengl.h>
#endif

#include <fs/ml.h>

#include "fs/image.h"
#include "video.h"
#include "render.h"

static fs_emu_texture *g_atlas = NULL;

typedef struct texture_entry {
    int x;
    int y;
    int w;
    int h;
} texture_entry;

static texture_entry g_entries[] = {
    {   0,   0,   0,    0},
    {  96, 512, 512,  512}, // TEXTURE_GLOSS
    { 640, 896, 128,  128}, // TEXTURE_GLOW_LEFT
    { 640, 736, 128,  128}, // TEXTURE_GLOW_TOP_LEFT
    { 800, 736, 128,  128}, // TEXTURE_GLOW_TOP
    {  50,   0,  10, 1020}, // TEXTURE_SIDEBAR
    {   0,   0,  60, 1020}, // TEXTURE_SIDEBAR_EDGE
    { 224,   0,  64,   64}, // TEXTURE_CLOSE
    { 288,   0,  64,   64}, // TEXTURE_VOLUME
    { 352,   0,  64,   64}, // TEXTURE_VOLUME_MUTED
    {  96,   0,  64,   64}, // TEXTURE_ASPECT
    { 160,   0,  64,   64}, // TEXTURE_STRETCH
    {  96, 384, 540,   96}, // TEXTURE_ITEM_BACKGROUND
    { 658, 402,  60,   60}, // TEXTURE_TOP_ITEM_BG
    { 672, 480,  32,   32}, // TEXTURE_HEADING_BG
};

void fs_emu_draw_from_atlas(float dx, float dy, float dw, float dh,
        int sx, int sy, int sw, int sh) {
    float tx = sx / 1024.0;
    float ty = sy / 1024.0;
    float tw = sw / 1024.0;
    float th = sh / 1024.0;
    //fs_gl_texturing(1);
    //fs_gl_bind_texture(g_atlas->texture);
    //printf("%f %f %f %f - %f %f %f %f\n", dx, dy, dw, dh, tx, ty, tw, th);

    fs_emu_set_texture(g_atlas);
    GLfloat tex[] = {
        tx, ty + th,
        tx + tw, ty + th,
        tx + tw, ty,
        tx, ty
    };
    GLfloat vert[] = {
        dx, dy,
        dx + dw, dy,
        dx + dw, dy + dh,
        dx, dy + dh
    };
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0, vert);
    glTexCoordPointer(2, GL_FLOAT, 0, tex);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    CHECK_GL_ERROR();
}

void fs_emu_prepare_texture(int entry, float *tx1, float *ty1,
        float *tx2, float *ty2) {
    *tx1 = g_entries[entry].x / 1024.0;
    *ty1 = g_entries[entry].y / 1024.0;
    *tx2 = *tx1 + g_entries[entry].w / 1024.0;
    *ty2 = *ty1 + g_entries[entry].h / 1024.0;
    fs_emu_set_texture(g_atlas);
}

void fs_emu_draw_texture_with_size(int entry, float x, float y, float w,
        float h) {
    fs_emu_draw_from_atlas(x, y, w, h, g_entries[entry].x,
            g_entries[entry].y, g_entries[entry].w, g_entries[entry].h);
}

void fs_emu_draw_texture(int entry, float x, float y) {
    //printf("%d - %f %f\n", entry, x, y);
    fs_emu_draw_texture_with_size(entry, x, y, g_entries[entry].w,
            g_entries[entry].h);
}

void fs_emu_initialize_textures() {
    g_atlas = fs_emu_texture_new_from_file("atlas");
}

void load_texture(fs_emu_texture *texture) {
    fs_image *image = texture->image;
    //printf("loading texture from image %p\n", image);
    if (!image) {
        return;
    }
    unsigned int opengl_texture;
    glGenTextures(1, &opengl_texture);
    //texture->opengl_context_stamp = g_fs_ml_opengl_context_stamp;
    fs_gl_bind_texture(opengl_texture);
#ifndef HAVE_GLES
    fs_gl_unpack_row_length(0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->width, image->height,
            0, fs_emu_get_video_format(), GL_UNSIGNED_BYTE, image->data);
    CHECK_GL_ERROR();
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    //        GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
    //        GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR();
    //glGenerateMipmapEXT(GL_TEXTURE_2D);
    texture->texture = opengl_texture;
}

void fs_emu_set_texture(fs_emu_texture *texture) {
    fs_gl_texturing(1);
    /*
    if (texture && texture->opengl_context_stamp && \
            g_fs_ml_opengl_context_stamp != texture->opengl_context_stamp) {
        // OpenGL context has been recreated. load_texture also binds
        load_texture(texture);
    }
    */
    if (!texture) {
        fs_gl_bind_texture(0);
        return;
    }
    if (texture->texture) {
        fs_gl_bind_texture(texture->texture);
    }
    else {
        // texture was not loaded, perhaps due to context recreation
        load_texture(texture);
    }
}

static void context_notification_handler(int notification, void *data) {
    fs_emu_texture *texture = (fs_emu_texture *) data;
    if (notification == FS_GL_CONTEXT_DESTROY) {
        //printf("context_notification_handler DESTROY %d\n", texture->texture);
        if (texture->texture) {
            glDeleteTextures(1, &texture->texture);
            CHECK_GL_ERROR();
            texture->texture = 0;
        }
    }
    else if (notification == FS_GL_CONTEXT_CREATE) {
        // loading the texture right now is not strictly necessary
        load_texture(texture);
    }
}

fs_emu_texture *fs_emu_texture_new_from_file(const char *name) {
    char *full_name = fs_strconcat(name, ".png", NULL);
    char *path = fs_get_program_data_file(full_name);
    if (path == NULL) {
        fs_emu_warning("Could not find texture %s\n", full_name);
        return NULL;
    }

    fs_image *image = fs_image_new_from_file(path);
    fs_emu_log("loading texture \"%s\"\n", path);
    free(path);
    if (image == NULL) {
        fs_emu_warning("Could not load texture from %s\n", full_name);
        free(full_name);
        return NULL;
    }
    free(full_name);

    if (fs_emu_get_video_format() == FS_EMU_VIDEO_FORMAT_BGRA) {
        // convert to premultiplied alpha
        if (image->format == FS_IMAGE_FORMAT_RGBA) {
            int num_pixels = image->width * image->height;
            unsigned char *pixels = image->data;
            for (int i = 0; i < num_pixels; i++) {
                unsigned char alpha = pixels[3];
                unsigned char temp = pixels[2];
                pixels[2] = ((int) pixels[0]) * alpha / 255;
                pixels[1] = ((int) pixels[1]) * alpha / 255;
                pixels[0] = ((int) temp) * alpha / 255;
                pixels += 4;
            }
        }
        else {
            // FIXME: should swap R and B here...
        }
    }
    else {
        // convert to premultiplied alpha
        if (image->format == FS_IMAGE_FORMAT_RGBA) {
            int num_pixels = image->width * image->height;
            unsigned char *pixels = image->data;
            for (int i = 0; i < num_pixels; i++) {
                unsigned char alpha = pixels[3];
                // should really divide by 255, but 256 is faster...
                //pixels[0] = ((int) pixels[0]) * alpha / 256;
                //pixels[1] = ((int) pixels[1]) * alpha / 256;
                //pixels[2] = ((int) pixels[2]) * alpha / 256;
                pixels[0] = ((int) pixels[0]) * alpha / 255;
                pixels[1] = ((int) pixels[1]) * alpha / 255;
                pixels[2] = ((int) pixels[2]) * alpha / 255;
                //pixels[0] = (unsigned char) ((pixels[0] * alpha + 0.5) / 255.0);
                //pixels[1] = (unsigned char) ((pixels[1] * alpha + 0.5) / 255.0);
                //pixels[2] = (unsigned char) ((pixels[2] * alpha + 0.5) / 255.0);
                pixels += 4;
            }
        }
    }

    fs_emu_texture *texture = fs_new(fs_emu_texture, 1);
    texture->width = image->width;
    texture->height = image->height;
    texture->image = image;
    load_texture(texture);
    fs_emu_set_texture(texture);

    fs_gl_add_context_notification(context_notification_handler, texture);

    return texture;
}

void fs_emu_texture_render(fs_emu_texture *texture, int x, int y) {
    fs_emu_render_texture_with_size(texture, x, y, texture->width,
            texture->height);
}

void fs_emu_render_texture_with_size(fs_emu_texture *texture, int x, int y,
        int w, int h) {
    fs_emu_set_texture(texture);
    fs_gl_blending(1);
    //fs_emu_texturing(0);
    //return;
    //fs_log("%d %d %d %d\n", x, y, x + w , y + h);
    GLfloat tex[] = {
        0.0, 1.0,
        1.0, 1.0,
        1.0, 1.0,
        1.0, 0.0
    };
    GLfloat vert[] = {
        x, y,
        x + w, y,
        x + w, y + h,
        x, y + h
    };
    
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glVertexPointer(2, GL_FLOAT, 0, vert);
    glTexCoordPointer(2, GL_FLOAT, 0, tex);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    CHECK_GL_ERROR();
}
