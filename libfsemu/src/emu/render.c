#include <stdio.h>
#include <string.h>
#include "fs/ml.h"
#include "render.h"
#include "libfsemu.h"
#include "video.h"
#include "texture.h"
#include "util.h"
#include "menu.h"
#include "audio.h"
#include "font.h"

#ifdef HAVE_GLES
#define glScaled glScalef
#define glTranslated glTranslatef
#define glRotated glRotatef
#define double float
#endif

#define VIDEO_DEBUG_SCALE_TIMES 2.5

// menu transition
static double g_menu_transition = 0.0;
static double g_menu_transition_target = 0.0;

//static fs_emu_texture g_frame_texture = {};
static GLuint g_frame_texture = 0;
static int g_frame_texture_width = 0;
static int g_frame_texture_height = 0;

// crop coordinates of emulator video frame
static fs_emu_rect g_crop = {};

#define FS_EMU_VIEWPORT_MODE_CROP 0
#define FS_EMU_VIEWPORT_MODE_CENTER 1
static int g_viewport_mode = FS_EMU_VIEWPORT_MODE_CROP;
static int g_effective_viewport_mode = FS_EMU_VIEWPORT_MODE_CROP;

// size of emulator video frame
static int g_frame_width = 0;
static int g_frame_height = 0;

static double g_frame_aspect = 0;

static uint8_t *g_scanline_buffer = NULL;
static int g_scanline_buffer_width = -1;
static int g_scanline_buffer_height = -1;

static void setup_opengl() {
    fs_log("setup_opengl\n");
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    CHECK_GL_ERROR();
}

static void context_notification_handler(int notification, void *data) {
    if (notification == FS_GL_CONTEXT_DESTROY) {
        if (g_frame_texture != 0) {
            glDeleteTextures(1, &g_frame_texture);
            CHECK_GL_ERROR();
            g_frame_texture = 0;
        }
    }
    else if (notification == FS_GL_CONTEXT_CREATE) {
        setup_opengl();
    }
}

void fs_emu_initialize_opengl() {
    setup_opengl();
    fs_emu_initialize_textures();
    fs_gl_add_context_notification(context_notification_handler, NULL);
}

static void create_texture_if_needed(int width, int height) {
    //g_frame_texture.video_version = g_fs_emu_video_version;
    if (g_frame_texture && g_frame_texture_width >= width &&
            g_frame_texture_height >= height) {
        return;
    }
    fs_gl_bind_texture(0);

    if (g_frame_texture) {
        glDeleteTextures(1, &g_frame_texture);
        CHECK_GL_ERROR();
    }
    g_frame_texture_width = 1;
    while (g_frame_texture_width < width) {
        g_frame_texture_width *= 2;
    }
    g_frame_texture_height = 1;
    while (g_frame_texture_height < height) {
        g_frame_texture_height *= 2;
    }
    glGenTextures(1, &g_frame_texture);
    CHECK_GL_ERROR();
    //g_frame_texture.opengl_context_stamp = g_fs_ml_opengl_context_stamp;
    //fs_emu_set_texture(&g_frame_texture);
    fs_gl_bind_texture(g_frame_texture);
    // with high quality borders, there should be no reason to initialize
    // the texture to black
    void *data = NULL;
    //void *data = g_malloc0(g_frame_texture_width * g_frame_texture_height * 4);
#ifndef HAVE_GLES
    fs_gl_unpack_row_length(0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, fs_emu_get_texture_format(),
            g_frame_texture_width, g_frame_texture_height, 0,
            fs_emu_get_video_format(), GL_UNSIGNED_BYTE, data);
    CHECK_GL_ERROR();
    if (data) {
        g_free(data);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR();
}

static void fix_border(fs_emu_video_buffer *buffer, int *upload_x,
		int *upload_y, int *upload_w, int *upload_h) {
    // upload fixed border as well
    int ux = g_crop.x;
    int uy = g_crop.y;
	int uw = g_crop.w;
    int uh = g_crop.h;

    // currently disabled border fixing ... -because this "destroys" the
    // buffer so it cannot be copied properly from when reusing lines
    // for the next frame
#if 0
    int32_t *idata = (int32_t*) buffer->data;
    int32_t *src;
    int32_t *dst;

	if (uy > 1) {
		src = idata + uy * buffer->width + ux;
		uy--;
		dst = idata + uy * buffer->width + ux;
		for (int x = 0; x < uw; x++) {
			*dst++ = *src++;
		}
		uh++;
	}
	//printf("%d %d %d\n", uy, uh, buffer->buffer_height);
    if (uy + uh < buffer->buffer_height) {
		src = idata + (uy + uh - 1) * buffer->width + ux;
    	uh++;
		dst = idata + (uy + uh - 1) * buffer->width + ux;
		for (int x = 0; x < uw; x++) {
			*dst++ = *src++;
		}
    }
    if (ux > 1) {
    	src = idata + uy * buffer->width + ux;
    	ux--;
		dst = idata + uy * buffer->width + ux;
		for (int y = 0; y < uh; y++) {
			*dst = *src;
			src += buffer->width;
			dst += buffer->width;
		}
    	uw++;
    }
    if (ux + uw < buffer->buffer_width) {
    	src = idata + uy * buffer->width + (ux + uw - 1);
    	uw++;
		dst = idata + uy * buffer->width + (ux + uw - 1);
		for (int y = 0; y < uh; y++) {
			*dst = *src;
			src += buffer->width;
			dst += buffer->width;
		}
    }
#endif
    *upload_x = ux;
    *upload_y = uy;
    *upload_w = uw;
    *upload_h = uh;
}

static void update_texture() {
    fs_emu_video_buffer *buffer = fs_emu_lock_video_buffer();
    // unlocked in fs_emu_video_after_update

    uint8_t *frame = buffer->data;
    if (frame == NULL) {
    	return;
    }
    int is_new_frame = 1;
    static int last_seq_no = -1;
    if (buffer->seq == last_seq_no + 1) {
        // normal
    }
    else if (buffer->seq == last_seq_no) {
        //fs_log("WARNING: repeated frame %d\n", info->seq_no);
        g_fs_emu_repeated_frames++;
        if (g_fs_emu_repeated_frames > 9999) {
            g_fs_emu_repeated_frames = 9999;
        }
        is_new_frame = 0;
    }
    else {
        int lost_frame_count = buffer->seq - last_seq_no - 1;
        g_fs_emu_lost_frames += lost_frame_count;
        //fs_log("lost %d frame(s)\n", lost_frame_count);
    }
    last_seq_no = buffer->seq;

    int width = buffer->width;
    int height = buffer->height;
    int bpp = buffer->bpp;

    if (g_fs_emu_screenshot) {
        static int count = 1;
        g_fs_emu_screenshot = 0;
        gchar *name, *path;
        time_t t = time(NULL);
        struct tm *tm_struct = localtime(&t);
        char strbuf[20];
        strftime(strbuf, 20, "%Y-%m-%d_%H-%M-%S", tm_struct);
        name = g_strdup_printf("%s_%d.png", strbuf, count);
        count += 1;
        path = g_build_filename(
                g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP),
                name, NULL);
        fs_log("writing screenshot to %s\n", path);
        int len = width * height * bpp;
        uint8_t *out_data = malloc(width * height * 3);
        uint8_t *op = out_data;

        if (fs_emu_get_video_format() == FS_EMU_VIDEO_FORMAT_BGRA) {
            for (int x = 0; x < len; x += 4) {
                *op++ = frame[x + 2];
                *op++ = frame[x + 1];
                *op++ = frame[x + 0];
            }
        }
        else {
            for (int x = 0; x < len; x += 4) {
                *op++ = frame[x + 0];
                *op++ = frame[x + 1];
                *op++ = frame[x + 2];
            }
        }
        int result = fs_image_save_data(path, out_data, width, height, 3);
        if (result) {
            fs_log("saved screenshot\n");
        }
        else {
            fs_log("error saving screenshot\n");
        }
        g_free(name);
        g_free(path);
    }

    int format = 0;
    if (bpp == 3) {
        format = GL_RGB;
    }
    else if (bpp == 4) {
#if 0
        if (fs_emu_get_video_format() == GL_BGRA) {
            format = GL_BGRA;
        }
        else {
            format = GL_RGBA;
        }
#endif
        format = fs_emu_get_video_format();
    }
    else {
        //fs_log("na..\n");
        return;
        //fs_emu_fatal("bpp is neither 3 nor 4\n");
    }
    if (g_fs_emu_video_crop_mode) {
        g_crop = buffer->crop;
        if (g_crop.w == 0) {
            g_crop.w = width;
        }
        if (g_crop.h == 0) {
            g_crop.h = height;
        }
    }
    else {
        g_crop.x = 0;
        g_crop.y = 0;
        g_crop.w = buffer->width;
        g_crop.h = buffer->height;
    }

    int upload_x, upload_y, upload_w, upload_h;
    g_effective_viewport_mode = g_viewport_mode;
    if (buffer->flags & FS_EMU_FORCE_VIEWPORT_CROP_FLAG) {
        g_effective_viewport_mode = FS_EMU_VIEWPORT_MODE_CROP;
    }

    if (g_effective_viewport_mode == FS_EMU_VIEWPORT_MODE_CROP) {
        fix_border(buffer, &upload_x, &upload_y, &upload_w, &upload_h);
    }
    else {
        upload_x = 0;
        upload_y = 0;
        upload_w = buffer->width;
        upload_h = buffer->height;
    }

    if (g_fs_emu_scanlines &&
            (buffer->flags & FS_EMU_NO_SCANLINES_FLAG) == 0) {
        //printf("new frame? %d\n", is_new_frame);
        if (is_new_frame) {
            if (g_scanline_buffer_width != buffer->width ||
                    g_scanline_buffer_height != buffer->height) {
                if (g_scanline_buffer) {
                    free(g_scanline_buffer);
                }
                g_scanline_buffer = malloc(buffer->width * buffer->height * 4);
                g_scanline_buffer_width = buffer->width;
                g_scanline_buffer_height = buffer->height;
            }
            fs_emu_render_scanlines(g_scanline_buffer, buffer,
                    upload_x, upload_y, upload_w, upload_h,
                    g_fs_emu_scanlines_dark, g_fs_emu_scanlines_light);
        }
        if (g_scanline_buffer) {
            frame = g_scanline_buffer;
        }
    }

    //fs_log("%d %d %d %d %d %d %d\n", width, height, bpp,
    //        g_crop.x, g_crop.y, g_crop.w, g_crop.h);
    g_frame_width = width;
    g_frame_height = height;
    g_frame_aspect = buffer->aspect;

    create_texture_if_needed(width, height);
    fs_gl_bind_texture(g_frame_texture);

    uint8_t *data_start = frame + ((upload_y * width) + upload_x) * 4;
#ifndef HAVE_GLES
    fs_gl_unpack_row_length(width);
    glTexSubImage2D(GL_TEXTURE_2D, 0, upload_x, upload_y,
    		upload_w, upload_h, format,
            GL_UNSIGNED_BYTE, data_start);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, upload_w, upload_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data_start);

    for (int y = 0; y < upload_h; y++) {
        char *row = data_start + ((y + upload_y)*width + upload_x) * 4;
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, upload_y, upload_w, 1, GL_RGBA, GL_UNSIGNED_BYTE, row);
    }
#endif
    CHECK_GL_ERROR();

#if 1
    int hq_border = 2;
    if (hq_border >= 1) {
        if (upload_y > 0) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, upload_y - 1, width, 1,
                    format, GL_UNSIGNED_BYTE,
                    frame + (upload_y) * width * bpp);
            CHECK_GL_ERROR();
        }

        if (upload_y + upload_h < g_frame_texture_height) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, upload_y + upload_h,
                    width, 1, format, GL_UNSIGNED_BYTE,
                    frame + (upload_y + upload_h - 1) * width * bpp);
            CHECK_GL_ERROR();
        }

        if (upload_x > 0) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, upload_x - 1, upload_y,
                    1, upload_h, format, GL_UNSIGNED_BYTE, frame +
                    ((upload_y) * width + upload_x) * bpp);
            CHECK_GL_ERROR();
        }

        if (upload_x + upload_w < g_frame_texture_width) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, upload_x + upload_w,
                    upload_y, 1, upload_h, format,
                    GL_UNSIGNED_BYTE, frame +
                    ((upload_y) * width + upload_x + upload_w - 1) * bpp);
            CHECK_GL_ERROR();
        }
    }
#if 0
    if (hq_border >= 2) {
        if (upload_y + upload_h < g_frame_texture.height - 1) {
            //printf("1\n");
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, upload_y + upload_h + 1,
                    width, 1, format, GL_UNSIGNED_BYTE,
                    frame + (upload_y + upload_h - 1) * width * bpp);
                    //frame + (upload_y + upload_h - 2) * width * bpp);
        }

        if (upload_x + upload_w < g_frame_texture.width - 1) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, upload_x + upload_w + 1,
                    upload_y, 1, upload_h, format,
                    GL_UNSIGNED_BYTE, frame +
                    ((upload_y) * width + upload_x + upload_w - 1) * bpp);
        }
    }
#endif
#endif
}

static void render_gloss(double alpha) {
    //fs_emu_set_texture(g_tex_screen_gloss);
    fs_gl_blending(1);
    //fs_emu_blending(0);
    //fs_emu_texturing(0);
    fs_gl_color4f(alpha, alpha, alpha, alpha);
    //fs_ml_color4f(alpha, 0.0, 0.0, alpha);
    //glBegin(GL_QUADS);
    //glTexCoord2f(0.0, 1.0); glVertex2f(-1.0, -1.0);
    //glTexCoord2f(1.0, 1.0); glVertex2f( 1.0, -1.0);
    //glTexCoord2f(1.0, 0.0); glVertex2f( 1.0,  1.0);
    //glTexCoord2f(0.0, 0.0); glVertex2f(-1.0,  1.0);
    fs_emu_draw_texture_with_size(TEXTURE_GLOSS, -1.0, -1.0, 2.0, 2.0);
    //glEnd();
}

static void render_frame(double alpha, int perspective) {
    //printf("--- render frame ---\n");
    //float t = g_snes_height / 512.0;
    //fs_log("%d %d %d %d\n", g_crop.x, g_crop.y, g_crop.w, g_crop.h);
    double s1;
    double s2;
    double t1;
    double t2;

    double emu_aspect;
    if (g_fs_emu_video_crop_mode) {
        //if (g_viewport_mode == FS_EMU_VIEWPORT_MODE_CROP) {
        s1 = (double) g_crop.x / g_frame_texture_width;
        s2 = (double) (g_crop.x + g_crop.w) / g_frame_texture_width;
        t1 = (double) g_crop.y / g_frame_texture_height;
        t2 = (double) (g_crop.y + g_crop.h) / g_frame_texture_height;
        emu_aspect = (double) g_crop.w / (double) g_crop.h;
    }
    else {
        s1 = 0.0;
        s2 = (double) g_frame_width / g_frame_texture_width;
        t1 = 0.0;
        t2 = (double)  g_frame_height / g_frame_texture_height;
        emu_aspect = (double) g_frame_width / (double) g_frame_height;
    }

    emu_aspect *= g_frame_aspect;

    double x1 = -1.0;
    double x2 =  1.0;
    double y1 = -1.0;
    double y2 =  1.0;

    double repeat_right_border = 0;
    //int repeat_bottom_border = 0;

    if (fs_emu_video_get_aspect_correction()) {
        double screen_aspect = (double) fs_ml_video_width() /
                (double) fs_ml_video_height();
        if (emu_aspect > screen_aspect) {
            // emu video is wider than screen
            double h = screen_aspect / emu_aspect;
            //double padding = (2.0 - 2.0 * h) / 2.0;
            double padding = 1.0 - h;
            if (g_effective_viewport_mode == FS_EMU_VIEWPORT_MODE_CROP) {
                y1 += padding;
                y2 -= padding;
            }
            else { // FS_EMU_VIEWPORT_MODE_CENTER
                t1 -= padding / 2.0;
                t2 += padding / 2.0;
                //y2 -= padding;
            }
        }
        else {
            double w = emu_aspect / screen_aspect;
            //double padding = (2.0 - 2.0 * w) / 2.0;
            double padding = 1.0 - w;
            if (g_effective_viewport_mode == FS_EMU_VIEWPORT_MODE_CROP) {
                x1 += padding;
                x2 -= padding;
            }
            else { // FS_EMU_VIEWPORT_MODE_CENTER
                //s1 -= padding / 4.0;

            	// FIXME: THIS IS WRONG
            	s1 -= padding / 2.0;
                //s2 += padding / 2.0;
                x2 -= padding;
                //repeat_right_border = x2;
                repeat_right_border = 1.0 - padding;
            }
        }
    }

    // if video is not stretched, we render black rectangles to cover
    // the rest of the screen
    if (x1 > -1.0 || x2 < 1.0 || y1 > -1.0 || y2 < 1.0) {
        fs_gl_texturing(0);
        if (alpha < 1.0) {
            fs_gl_blending(1);
            fs_gl_color4f(0.0, 0.0, 0.0, alpha);
        }
        else {
            fs_gl_blending(0);
            fs_gl_color4f(0.0, 0.0, 0.0, 1.0);
        }

        glEnableClientState(GL_VERTEX_ARRAY);
        if (x1 > -1.0) {
            GLfloat vert[] = {
                -1.0, y1,
                x1, y1,
                x1, y2,
                -1.0, y2
            };

            glVertexPointer(2, GL_FLOAT, 0, vert);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            CHECK_GL_ERROR();
        }
        if (x2 < 1.0) {
            GLfloat vert[] = {
                x2, y1,
                1.0, y1,
                1.0, y2,
                x2, y2
            };

            glVertexPointer(2, GL_FLOAT, 0, vert);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            CHECK_GL_ERROR();
        }
        if (y1 > -1.0) {
            GLfloat vert1[] = {
                x1, -1.0,
                x2, -1.0,
                x2, y1,
                x1, y1
            };
            GLfloat vert2[] = {
                -1.0, -1.0, -0.1,
                -1.0, -1.0, 0.0,
                -1.0, y1, 0.0,
                -1.0, y1, -0.1
            };

            glVertexPointer(2, GL_FLOAT, 0, vert1);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glVertexPointer(3, GL_FLOAT, 0, vert2);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            CHECK_GL_ERROR();
        }
        if (y2 < 1.0) {
            GLfloat vert1[] = {
                x1, y2,
                x2, y2,
                x2, 1.0,
                x1, 1.0
            };
            GLfloat vert2[] = {
                -1.0, y2, -0.1,
                -1.0, y2, 0.0,
                -1.0, 1.0, 0.0,
                -1.0, 1.0, -0.1
            };

            glVertexPointer(2, GL_FLOAT, 0, vert1);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            glVertexPointer(3, GL_FLOAT, 0, vert2);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            CHECK_GL_ERROR();
        }
        glDisableClientState(GL_VERTEX_ARRAY);
    }
    //printf("--- render frame done ---\n");

    if (perspective) {
        // render left side in 3d mode
        fs_gl_blending(0);
        if (x1 > -1.0) {
            // emu screen does not reach screen edge - side will be black
            fs_gl_texturing(0);
            fs_gl_color4f(0.0, 0.0, 0.0, alpha);
        }
        else {
            fs_gl_texturing(1);
            fs_gl_bind_texture(g_frame_texture);
            fs_gl_color4f(0.33 * alpha, 0.33 * alpha, 0.33 * alpha, alpha);
        }
        GLfloat tex[] = {
            s1, t2,
            s1, t2,
            s1, t1, // TODO: is s1 on this line a bug?
            s1, t1
        };
        GLfloat vert[] = {
            -1.0, y1, -0.1,
            -1.0, y1, 0.0,
            -1.0, y2, 0.0,
            -1.0, y2, -0.1
        };

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glVertexPointer(3, GL_FLOAT, 0, vert);
        glTexCoordPointer(2, GL_FLOAT, 0, tex);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        CHECK_GL_ERROR();
    }

    float color = 1.0;
    if (g_frame_texture == 0) {
        // texture has not been created yet
        color = 0.0;
        fs_gl_texturing(0);
    }
    else {
        fs_gl_texturing(1);
        fs_gl_bind_texture(g_frame_texture);
    }
    if (alpha < 1.0) {
        fs_gl_blending(1);
        fs_gl_color4f(color * alpha, color * alpha, color * alpha, alpha);
    }
    else {
        fs_gl_blending(0);
        fs_gl_color4f(color, color, color, 1.0);
    }

    GLfloat tex[] = {
        s1, t2,
        s2, t2,
        s2, t1,
        s1, t1
    };
    GLfloat vert[] = {
        x1, y1,
        x2, y1,
        x2, y2,
        x1, y2
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0, vert);
    glTexCoordPointer(2, GL_FLOAT, 0, tex);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    CHECK_GL_ERROR();

    //repeat_right_border = 0;
    if (repeat_right_border > 0.0) {
        s1 = s2 = (double) (g_frame_width - 1) / g_frame_texture_width;

        GLfloat tex[] = {
            s1, t2,
            s2, t2,
            s2, t1,
            s1, t1
        };
        GLfloat vert[] = {
            repeat_right_border, y1,
            1.0, y1,
            1.0, y2,
            repeat_right_border, y2
        };

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glVertexPointer(2, GL_FLOAT, 0, vert);
        glTexCoordPointer(2, GL_FLOAT, 0, tex);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        CHECK_GL_ERROR();

        // so the following code does not render black rectangle over
        // the right border
        x2 = 1.0;
    }
}

static void render_glow(double opacity) {
    //printf("--- render glow ---\n");    
    float tx1, ty1, tx2, ty2;
    //const double dx = 0.1 * 9.0 / 16.0;
    const double dx = 0.15;
    const double dy = 0.15 * 16.0 / 9.0;
    const double z = 0.0;
    const double s = 0.65 * opacity;
    fs_gl_color4f(s, s, s, s);
    fs_gl_blending(1);
    //fs_emu_set_texture(g_tex_glow_top);
    fs_emu_prepare_texture(TEXTURE_GLOW_TOP, &tx1, &ty1, &tx2, &ty2);
    // render top edge
    GLfloat tex[] = {
        tx1, ty2,
        tx2, ty2,
        tx2, ty1,
        tx1, ty1
    };
    GLfloat vert[] = {
        -1.0 + dx, 1.0 - dy, z,
        1.0 - dx, 1.0 - dy, z,
        1.0 - dx, 1.0 + dy, z,
        -1.0 + dx, 1.0 + dy, z
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, 0, vert);
    glTexCoordPointer(2, GL_FLOAT, 0, tex);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    CHECK_GL_ERROR();
    // render corners
    fs_emu_prepare_texture(TEXTURE_GLOW_TOP_LEFT, &tx1, &ty1, &tx2, &ty2);
    GLfloat tex2[] = {
        // top left corner
        tx1, ty2,
        tx2, ty2,
        tx2, ty1,
        tx1, ty1,
        // top right corner
        tx2, ty2,
        tx1, ty2,
        tx1, ty1,
        tx2, ty1
    };
    GLfloat vert2[] = {
        // top left corner
        -1.0 - dx, 1.0 - dy, z,
        -1.0 + dx, 1.0 - dy, z,
        -1.0 + dx, 1.0 + dy, z,
        -1.0 - dx, 1.0 + dy, z,
        // top right corner
        1.0 - dx, 1.0 - dy, z,
        1.0 + dx, 1.0 - dy, z,
        1.0 + dx, 1.0 + dy, z,
        1.0 - dx, 1.0 + dy, z
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0, vert2);
    glTexCoordPointer(2, GL_FLOAT, 0, tex2);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 8);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    CHECK_GL_ERROR();
    // render left and right edge
    fs_emu_prepare_texture(TEXTURE_GLOW_LEFT, &tx1, &ty1, &tx2, &ty2);
    //fs_emu_set_texture(g_tex_glow_left);

    GLfloat color3[] = {
        // left edge
        s, s, s, s,
        s, s, s, s,
        s, s, s, s,
        s, s, s, s,
        // right edge
        s, s, s, s,
        s, s, s, s,
        s, s, s, s,
        s, s, s, s,
        // left edge bottom
        0, 0, 0, 0,
        0, 0, 0, 0,
        s, s, s, s,
        s, s, s, s,
        // right edge bottom
        0, 0, 0, 0,
        0, 0, 0, 0,
        s, s, s, s,
        s, s, s, s,
    };
    GLfloat tex3[] = {
        // left edge
        tx1, ty2,
        tx2, ty2,
        tx2, ty1,
        tx1, ty1,
        // right edge
        tx2, ty2,
        tx1, ty2,
        tx1, ty1,
        tx2, ty1,
        // left edge bottom
        tx1, ty2,
        tx2, ty2,
        tx2, ty1,
        tx1, ty1,
        // right edge bottom
        tx2, ty2,
        tx1, ty2,
        tx1, ty1,
        tx2, ty1
    };
    GLfloat vert3[] = {
        // left edge
        -1.0 - dx, -0.5, z,
        -1.0 + dx, -0.5, z,
        -1.0 + dx, 1.0 - dy, z,
        -1.0 - dx, 1.0 - dy, z,
        // right edge
        1.0 - dx, -0.5, z,
        1.0 + dx, -0.5, z,
        1.0 + dx, 1.0 - dy, z,
        1.0 - dx, 1.0 - dy, z,
        // left edge bottom
        -1.0 - dx, -1.0, z,
        -1.0 + dx, -1.0, z,
        -1.0 + dx, -0.5, z,
        -1.0 - dx, -0.5, z,
        // right edge bottom
        1.0 - dx, -1.0, z,
        1.0 + dx, -1.0, z,
        1.0 + dx, -0.5, z,
        1.0 - dx, -0.5, z
    };

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glColorPointer(4, GL_FLOAT, 0, color3);
    glVertexPointer(3, GL_FLOAT, 0, vert3);
    glTexCoordPointer(2, GL_FLOAT, 0, tex3);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 16);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    CHECK_GL_ERROR();
    //printf("--- render glow done ---\n");        
}

/**
 * This function is called at the end of the frame rendering function
 */
static void handle_quit_sequence() {
    int fade_time = 750 * 1000;

    int64_t dt = fs_emu_monotonic_time() - g_fs_emu_quit_time;
    if (dt > fade_time && g_fs_emu_emulation_thread_stopped) {
        fs_emu_log("calling fs_ml_stop because emu thread is done\n");
        fs_ml_stop();

    }
    else if (dt > 5 * 1000 * 1000) {
        // 5 seconds has passed after shutdown was requested
        fs_emu_log("calling fs_ml_stop because emu does not stop\n");
        // FIXME: FORCE STOP
        fs_ml_stop();
        fs_emu_log("force-closing the emulator\n");
        exit(1);
    }

    // fade out over 750ms
    float fade = (1.0 * dt) / fade_time;
    if (fade > 1.0) {
        fade = 1.0;
    }

    // draw fading effect
    fs_gl_viewport(0, 0, fs_ml_video_width(), fs_ml_video_height());
    fs_gl_ortho_hd();
    fs_gl_blending(1);
    fs_gl_texturing(0);
    fs_gl_color4f(0.0, 0.0, 0.0, fade);
    GLfloat vert[] = {
        0, 0,
        1920, 0,
        1920, 1080,
        0, 1080
    };
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vert);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
    CHECK_GL_ERROR();
}

void fs_emu_video_update_function() {
    update_texture();
}

void fs_emu_video_render_function() {
    static int initialized_menu = 0;
    if (!initialized_menu) {
        // render menu once (without really showing it, so all menu
        // resources are initialized and loaded, -prevents flickering
        // when really opening the menu later
        fs_emu_render_menu(g_menu_transition);
        initialized_menu = 1;
    }

    if (g_fs_emu_video_debug) {
        int quarter_height = fs_ml_video_height() / 4;
        fs_gl_viewport(0, quarter_height, fs_ml_video_width(),
                fs_ml_video_height() - quarter_height);
    }
    else {
        fs_gl_viewport(0, 0, fs_ml_video_width(), fs_ml_video_height());
    }

    // FIXME: can perhaps remove this soon..
    fs_emu_video_render_mutex_lock();

    int in_menu = fs_emu_menu_is_active();
    if (in_menu && g_menu_transition_target < 1.0) {
        g_menu_transition_target = 1.0;
    }
    if (!in_menu && g_menu_transition_target > 0.0) {
        g_menu_transition_target = 0.0;
    }

    // FIXME: ideally, we would use time-based animation - for now, we use a
    // simple frame-based animation
    if (g_menu_transition < g_menu_transition_target) {
        if (g_menu_transition_target == 1.0) {
            g_menu_transition += 0.10;
        }
    }
    if (g_menu_transition > g_menu_transition_target) {
        if (g_menu_transition_target == 0.0) {
            g_menu_transition -= 0.10;
        }
    }
    if (g_menu_transition > 1.0) {
        g_menu_transition = 1.0;
    }
    else if (g_menu_transition < 0.0) {
        g_menu_transition = 0.0;
    }

    int matrix_pushed = 0;

    double t0_x = 0.0;
    double t0_y = 0.0;
    double t0_z = -2.42;
    double r0_a = 0.0;

    double t1_x = -0.31;
    //double t1_y = -0.04;
    double t1_y = 0.0;
    double t1_z = -3.7;
    double r1_a = 30.0;

    int perspective = 0;
    if (g_menu_transition == 0.0) {
        perspective = 0;
        fs_gl_ortho();
        //glTranslated(1920.0 / 2.0, 1080.0 / 2.0, 0.0);
        //glScaled(1920.0 / 2.0, 1080.0 / 2.0, 1.0);
    }
    else {
        perspective = 1;

        glClearColor(0.1, 0.1, 0.1, 1.0);
        glClear(GL_DEPTH_BUFFER_BIT);
        CHECK_GL_ERROR();
        fs_gl_ortho_hd();

        // transition y-coordinate between floor and wall
        int splt = 361;
        fs_gl_blending(FALSE);
        fs_gl_texturing(FALSE);
        GLfloat color[] = {
            39.0 / 255.0, 44.0 / 255.0, 51.0 / 255.0, 1.0,
            39.0 / 255.0, 44.0 / 255.0, 51.0 / 255.0, 1.0,
            0.0, 0.0, 0.0, 1.0,
            0.0, 0.0, 0.0, 1.0,

            0.0, 0.0, 0.0, 1.0,
            0.0, 0.0, 0.0, 1.0,
            0.0, 0.0, 0.0, 1.0,
            0.0, 0.0, 0.0, 1.0,

            0.0, 0.0, 0.0, 1.0,
            0.0, 0.0, 0.0, 1.0,
            20.0 / 255.0, 22.0 / 255.0, 26.0 / 255.0, 1.0,
            20.0 / 255.0, 22.0 / 255.0, 26.0 / 255.0, 1.0
        };
        GLfloat vert[] = {
               0, splt, -0.9,
            1920, splt, -0.9,
            1920, 1020, -0.9,
               0, 1020, -0.9,

               0, 1020, -0.9,
            1920, 1020, -0.9,
            1920, 1080, -0.9,
               0, 1080, -0.9,

               0,    0, -0.9,
            1920,    0, -0.9,
            1920, splt, -0.9,
               0, splt, -0.9
        };

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        glColorPointer(4, GL_FLOAT, 0, color);
        glVertexPointer(3, GL_FLOAT, 0, vert);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 12);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        CHECK_GL_ERROR();

        fs_gl_perspective();
        double t_x = t0_x + (t1_x - t0_x) * g_menu_transition;
        double t_y = t0_y + (t1_y - t0_y) * g_menu_transition;
        double t_z = t0_z + (t1_z - t0_z) * g_menu_transition;
        double r_a = r0_a + (r1_a - r0_a) * g_menu_transition;

        glPushMatrix();
        matrix_pushed = 1;

        glScaled(16.0 / 9.0, 1.0, 1.0);
        glTranslated(t_x, t_y, t_z);
        glRotated(r_a, 0.0, 1.0, 0.0);
        CHECK_GL_ERROR();
    }

    if (perspective) {
        render_glow(g_menu_transition);
    }
    if (perspective) {
        glPushMatrix();
        glTranslatef(0.0, -2.0, 0.0);
        //glTranslatef(0.0, -1.0, 0.0);
        //glScalef(1.0, -1.0, 1.0);
        glScalef(1.0, -0.5, 1.0);
        glTranslatef(0.0, -1.0, 0.0);
        CHECK_GL_ERROR();
        render_frame(0.33, perspective);
        CHECK_GL_ERROR();
        render_gloss(g_menu_transition * 0.66);
        CHECK_GL_ERROR();
        glPopMatrix();
        CHECK_GL_ERROR();
    }
    render_frame(1.0, perspective);
    if (perspective) {
        render_gloss(g_menu_transition);
    }
    /*
    if (fs_emu_is_paused()) {
        render_pause_fade();
    }
    */

    if (matrix_pushed) {
        glPopMatrix();
        CHECK_GL_ERROR();
        matrix_pushed = 0;
    }

    fs_emu_acquire_gui_lock();
    fs_emu_render_chat();

    //if (fs_emu_menu_is_active()) {
    if (g_menu_transition > 0.0) {
        fs_emu_render_menu(g_menu_transition);
    }

	fs_emu_render_dialog();
    fs_emu_release_gui_lock();

    if (g_fs_emu_hud_mode && fs_emu_netplay_enabled()) {
        fs_gl_ortho_hd();
        fs_gl_texturing(0);
        fs_gl_blending(1);
	fs_gl_color4f(0.0, 0.0, 0.0, 0.5);
        GLfloat vert[] = {
            0, 1030,
            1920, 1030,
            1920, 1080,
            0, 1080
        };
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, vert);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glDisableClientState(GL_VERTEX_ARRAY);
        CHECK_GL_ERROR();
#if 0
        glBegin(GL_QUADS);
        glVertex2f(0, 1030);
        glVertex2f(1920, 1030);
        fs_gl_color4f(0.0, 0.0, 0.0, 0.0);
        glVertex2f(1920, 1030 - 50);
        glVertex2f(0, 1030 - 50);
        glEnd();
#endif
        fs_emu_font *menu_font = fs_emu_font_get_menu();
        char *str;

        for (int i = 0; i < MAX_PLAYERS; i++) {
            fs_emu_player *player = g_fs_emu_players + i;
            int x = i * 1920 / 6 + 20;
            int y = 1038;

            int rendered_tag = 0;
            if (player->tag && player->tag[0]) {
                str = g_strdup_printf("%s", player->tag);
                fs_emu_font_render(menu_font, str, x, y,
                        1.0, 1.0, 1.0, 1.0);
                g_free(str);
                rendered_tag = 1;
            }
            if (rendered_tag || player->ping) {
                str = g_strdup_printf("%03d", player->ping);
                fs_emu_font_render(menu_font, str, x + 100, y,
                        1.0, 1.0, 1.0, 1.0);
                g_free(str);
            }
            if (rendered_tag || player->lag) {
                str = g_strdup_printf("%03d", player->lag);
                fs_emu_font_render(menu_font, str, x + 200, y,
                        1.0, 1.0, 1.0, 1.0);
                g_free(str);
            }
        }
    }

    if (g_fs_emu_video_debug) {
        int quarter_height = fs_ml_video_height() / 4;
        fs_gl_viewport(0, 0, fs_ml_video_width(), quarter_height);
        CHECK_GL_ERROR();

        fs_emu_set_texture(NULL);
        CHECK_GL_ERROR();
        static GLuint debug_texture = 0;
        static uint32_t *debug_texture_data = NULL;
        if (debug_texture == 0) {
            debug_texture_data = g_malloc0(256 * 256 * 4);
            glGenTextures(1, &debug_texture);
            CHECK_GL_ERROR();
            fs_gl_bind_texture(debug_texture);
#ifndef HAVE_GLES
            fs_gl_unpack_row_length(0);
#endif
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            CHECK_GL_ERROR();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    GL_CLAMP_TO_EDGE);
            CHECK_GL_ERROR();
        }
        else {
            fs_gl_bind_texture(debug_texture);
            CHECK_GL_ERROR();
        }

        memset(debug_texture_data, 0x00, 256 * 256 * 4);
        CHECK_GL_ERROR();
        fs_emu_video_render_debug_info(debug_texture_data);
        CHECK_GL_ERROR();
        fs_emu_audio_render_debug_info(debug_texture_data);
        CHECK_GL_ERROR();

#ifndef HAVE_GLES
        fs_gl_unpack_row_length(0);
#endif
        CHECK_GL_ERROR();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256,
                GL_RGBA, GL_UNSIGNED_BYTE, debug_texture_data);
        CHECK_GL_ERROR();
        fs_gl_ortho_hd();
        fs_gl_texturing(1);
        fs_gl_blending(0);
        fs_gl_color4f(1.0, 1.0, 1.0, 1.0);
        GLfloat tex[] = {
            0.0, 0.0,
            1.0, 0.0,
            1.0, 1.0,
            0.0, 1.0
        };
        GLfloat vert[] = {
            0, 0,
            1920, 0,
            1920, 1080,
            0, 1080
        };

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glTexCoordPointer(2, GL_FLOAT, 0, tex);
        glVertexPointer(2, GL_FLOAT, 0, vert);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        CHECK_GL_ERROR();

        glPushMatrix();
        glScalef(1.0, 4.0, 1.0);

        fs_emu_font *menu_font = fs_emu_font_get_menu();
        char *str;

        /*
        str = g_strdup_printf("%d", fs_emu_get_audio_frequency());
        fs_emu_font_render(menu_font, str, 1920 / 2 + 20, 3,
                1.0, 1.0, 1.0, 1.0);
        g_free(str);
        */

        str = g_strdup_printf("%0.1f",
                fs_emu_audio_get_measured_avg_buffer_fill(0) / 1000.0);
        fs_emu_font_render(menu_font, str, 1920 / 2 + 220, 3,
                1.0, 1.0, 1.0, 1.0);
        g_free(str);
        str = g_strdup_printf("%d", g_fs_emu_audio_buffer_underruns);
        fs_emu_font_render(menu_font, str, 1920 / 2 + 420, 3,
                1.0, 1.0, 1.0, 1.0);
        g_free(str);

        fs_emu_font_render(menu_font, "EMU", 20, 3, 1.0, 1.0, 1.0, 1.0);
        str = g_strdup_printf("%0.1f", fs_emu_get_average_emu_fps());
        fs_emu_font_render(menu_font, str, 220, 3, 1.0, 1.0, 1.0, 1.0);
        g_free(str);
        str = g_strdup_printf("%d", g_fs_emu_lost_frames);
        fs_emu_font_render(menu_font, str, 420, 3, 1.0, 1.0, 1.0, 1.0);
        g_free(str);
        str = g_strdup_printf("%d", g_fs_emu_repeated_frames);
        fs_emu_font_render(menu_font, str, 620, 3, 1.0, 1.0, 1.0, 1.0);
        g_free(str);

        fs_emu_font_render(menu_font, "SYS", 20, 140, 1.0, 1.0, 1.0, 1.0);
        str = g_strdup_printf("%0.1f", fs_emu_get_average_sys_fps());
        fs_emu_font_render(menu_font, str, 220, 140, 1.0, 1.0, 1.0, 1.0);
        g_free(str);
        str = g_strdup_printf("%d", g_fs_emu_lost_vblanks);
        fs_emu_font_render(menu_font, str, 420, 140, 1.0, 1.0, 1.0, 1.0);
        g_free(str);

        glPopMatrix();
        CHECK_GL_ERROR();
    }

    if (fs_emu_is_quitting()) {
        handle_quit_sequence();
    }

    fs_emu_video_render_mutex_unlock();
}

void fs_emu_video_render_debug_info(uint32_t *texture) {
    //return;
	int x;
	//, y;
    int y1;
	GList *link;
    uint32_t color = 0x80404080;
	fs_gl_ortho_hd();
	fs_gl_blending(TRUE);
	fs_gl_texturing(FALSE);

    // render debug triangles, these are for visually debugging vblank
	// synchronization

    y1 = 0;
    color = 0x80800080;
    static int start_add = 0;
    int add = start_add;
    for (int x = 0; x < 256; x++) {
        int y2 = add % 20;
        for (int y = y1; y < y2; y++) {
            *(texture + (((255 - y) * 256) + x)) = color;
        }
        add += 2;
    }
    start_add += 2;

    x = 127;
    y1 = 128;
    color = 0x80404080;
    link = g_queue_peek_head_link(g_fs_emu_sys_frame_times.queue);
    while (link) {
        int val = GPOINTER_TO_INT(link->data);
        //int x2 = x - 8;
        int y2 = y1 + val * VIDEO_DEBUG_SCALE_TIMES;
        if (y2 > 256) {
            y2 = 256;
        }
        for (int y = y1; y < y2; y++) {
            *(texture + ((y * 256) + x)) = color;
        }
        if (--x < 0) {
            break;
        }
        link = link->next;
    }

    x = 127;
    y1 = 0;
    color = 0x80205080;
    link = g_queue_peek_head_link(g_fs_emu_emu_frame_times.queue);
    while (link) {
        int val = GPOINTER_TO_INT(link->data);
        //int x2 = x - 8;
        int y2 = y1 + val * VIDEO_DEBUG_SCALE_TIMES;
        if (y2 > 256) {
            y2 = 256;
        }
        for (int y = y1; y < y2; y++) {
            *(texture + ((y * 256) + x)) = color;
        }
        if (--x < 0) {
            break;
        }
        link = link->next;
    }
}
