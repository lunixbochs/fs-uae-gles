#ifdef WINDOWS
#define _WIN32_WINNT 0x0501
#include <Windows.h>
#endif
#include <stdio.h>
#include <glib.h>
#include <fs/base.h>
#include <fs/log.h>
#include "fs/ml.h"

#ifdef LINUX
# if 0
#include <sched.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif
#endif

// FIXME: REMOVE
#include "../emu/video.h"

#include "ml_internal.h"

static GQueue *g_input_queue = NULL;
static GMutex *g_input_mutex = NULL;
static fs_ml_input_function g_input_function = NULL;

int g_fs_ml_video_width = 0;
int g_fs_ml_video_height = 0;
int g_fs_ml_video_sync = 0;
int g_fs_ml_vblank_sync = 0;

int g_fs_ml_target_refresh_rate = 0;
int g_fs_ml_target_frame_time = 0;

// when OpenGL is reinitialized, we update this version (because we may need
// to reload textures, etc)
int g_fs_ml_opengl_context_stamp = 0;

fs_ml_simple_function g_fs_ml_video_update_function = NULL;
fs_ml_simple_function g_fs_ml_video_render_function = NULL;
fs_ml_simple_function g_fs_ml_video_post_render_function = NULL;

//fs_ml_input_device *g_fs_ml_input_devices = NULL;
fs_ml_input_device *g_fs_ml_input_devices = NULL;
int g_fs_ml_input_device_count = 0;

static int g_quit;
static fs_ml_simple_function g_quit_function = NULL;

void fs_ml_set_quit_function(fs_ml_simple_function function) {
    g_quit_function = function;
}

void fs_ml_quit() {
    if (g_quit) {
        fs_log("fs_ml_quit already called\n");
        return;
    }
    fs_log("fs_ml_quit called\n");
    if (g_quit_function) {
        g_quit_function();
    }
    g_quit = 1;
}

int fs_ml_is_quitting() {
    return g_quit;
}

int fs_ml_input_device_count() {
    //return g_fs_ml_input_device_count;
    return FS_ML_INPUT_DEVICES_MAX;
}

fs_ml_input_device *fs_ml_get_input_devices(int* count) {
	if (count) {
		*count = g_fs_ml_input_device_count;
	}
    return g_fs_ml_input_devices;
}

int fs_ml_input_device_get(int index, fs_ml_input_device *device) {
    if (index < 0) {
        return 0;
    }
    if (index >= FS_ML_INPUT_DEVICES_MAX) {
    //if (index >= g_fs_ml_input_device_count) {
        return 0;
    }
    if (device == NULL) {
        return 0;
    }
    if (g_fs_ml_input_devices[index].name == NULL) {
        return 0;
    }
    *device = g_fs_ml_input_devices[index];
    return 1;
}

int fs_ml_get_vblank_sync() {
    return g_fs_ml_vblank_sync;
}

int fs_ml_get_video_sync() {
    return g_fs_ml_video_sync;
}
void fs_ml_video_sync_enable() {
    g_fs_ml_video_sync = 1;
}

void fs_ml_vblank_sync_enable() {
    g_fs_ml_vblank_sync = 1;
}

int fs_ml_video_width() {
    return g_fs_ml_video_width;
}

int fs_ml_video_height() {
    return g_fs_ml_video_height;
}

void fs_ml_video_set_update_function(fs_ml_simple_function function) {
    g_fs_ml_video_update_function = function;
}

void fs_ml_video_set_render_function(fs_ml_simple_function function) {
    g_fs_ml_video_render_function = function;
}

void fs_ml_video_set_post_render_function(fs_ml_simple_function function) {
    g_fs_ml_video_post_render_function = function;
}

fs_ml_event* fs_ml_alloc_event() {
    return g_malloc(sizeof(fs_ml_event));
}

void fs_ml_input_event_free(fs_ml_event *event) {
    g_free(event);
}

int fs_ml_get_event(fs_ml_event *event) {
    /*
    g_mutex_lock(g_input_mutex);
    fs_ml_event *e = g_queue_pop_head(g_input_queue);
    g_mutex_unlock(g_input_mutex);
    if (e == NULL) {
        // empty queue, we do not modify the event at all and return 0
        // to signify that there is no more events
        return 0;
    }
    *event = *e;
    fs_ml_input_event_free(e);
    return 1;
    */
    return 0;
}

void fs_ml_set_input_function(fs_ml_input_function function) {
    g_input_function = function;
}

int fs_ml_post_event(fs_ml_event* event) {
	if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
		if (fs_ml_handle_keyboard_shortcut(event)) {
			return 1;
		}
	}
    if (g_input_function) {
        g_input_function(event);;
    }
    /*
    g_mutex_lock(g_input_mutex);
    g_queue_push_tail(g_input_queue, event);
    g_mutex_unlock(g_input_mutex);
    */
    return 1;
}

static void init_input() {
    fs_log("init_input\n");
    g_input_queue = g_queue_new();
    g_input_mutex = g_mutex_new();

    fs_log("calling fs_ml_video_init\n");
    fs_ml_video_init();

    int size = sizeof(fs_ml_input_device) * FS_ML_INPUT_DEVICES_MAX;
    // allocate zeroed memory
    g_fs_ml_input_devices = g_malloc0(size);
    fs_log("calling fs_ml_input_init\n");
    fs_ml_input_init();
}

int fs_ml_handle_keyboard_shortcut(fs_ml_event *event) {
	int state = event->key.state;
	int key = event->key.keysym.sym;
	int mod = event->key.keysym.mod;
	//printf("%d %d -- %d %d\n", key, mod, SDLK_RETURN, KMOD_ALT);

#ifdef MACOSX
	int alt_mod = mod & (KMOD_ALT | KMOD_META);
#else
	int alt_mod = mod & KMOD_ALT;
#endif

	if (key == SDLK_RETURN && alt_mod) {
		if (state) {
			fs_log("ALT+Return key press detected\n");
			fs_ml_toggle_fullscreen();
		}
		return 1;
	}
	else if (key == SDLK_F4 && alt_mod) {
        if (state) {
            fs_log("ALT+F4 key press detected\n");
            fs_ml_quit();
        }
	}
	else if (key == SDLK_TAB && alt_mod) {
		if (state) {
			fs_log("ALT+Tab key press detected\n");
#ifdef WINDOWS
			// input grab will be released be deactivation
                        // event in this case
#else
			if (fs_ml_has_input_grab()) {
				fs_log("- releasing input grab");
				fs_ml_grab_input(0, 1);
				g_fs_ml_had_input_grab = 1;
			}
			if (g_fs_emu_video_fullscreen == 1 &&
					g_fs_emu_video_fullscreen_window == 0) {
				fs_log("- switching to window mode\n");
				g_fs_ml_was_fullscreen = 1;
				fs_ml_toggle_fullscreen();
			}
#endif
		}
		return 1;
	}

	return 0;
}

void fs_ml_init() {
#ifdef WINDOWS
    fs_log("WINDOWS\n");
#endif
#ifdef MACOSX
    fs_log("MACOSX\n");
#endif
#ifdef LINUX
    fs_log("LINUX\n");
#endif
    g_fs_ml_video_render_function = NULL;
    g_fs_ml_video_post_render_function = NULL;

#ifdef WINDOWS
#ifndef TIMERR_NOERROR
#define TIMERR_NOERROR 0
#endif
    if (timeBeginPeriod(1) == TIMERR_NOERROR) {
        fs_log("successfully set timeBeginPeriod(1)\n");
    }
    else {
        fs_log("error setting timeBeginPeriod(1)\n");
    }
    if (SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS)) {
        fs_log("set process priority class to ABOVE_NORMAL_PRIORITY_CLASS\n");
    }
    else {
        int dwError = GetLastError();
        fs_log("Failed to set process priority class (%d)\n", dwError);
    }
#endif

#ifdef LINUX
#if 0
    struct rlimit rlim;
    getrlimit(RLIMIT_RTPRIO, &rlim);
    printf("%d %d\n", rlim.rlim_cur, rlim.rlim_max);
    //rlim.rlim_cur = 10;
    //rlim.rlim_max = 10;
    //setrlimit(RLIMIT_RTPRIO, &rlim);
    //getrlimit(RLIMIT_RTPRIO, &rlim);
    //printf("%d %d\n", rlim.rlim_cur, rlim.rlim_max);

    struct sched_param params;
    //params.sched_priority = sched_get_priority_min(SCHED_FIFO);
    params.sched_priority = 50;
    fs_log("trying to set priority to %d\n", params.sched_priority);
    int result = sched_setscheduler(0, SCHED_FIFO, &params);
    if (result == 0) {
    	fs_log("has set real time priority\n");
    }
    else {
    	fs_log("could not set real time priority, errno = %d\n", errno);
    }
#endif
#endif
#if 0
    fs_ml_calibrate_clock();
#endif
}

void fs_ml_init_2() {
    fs_ml_video_mode mode;
    memset(&mode, 0, sizeof(fs_ml_video_mode));
    if (fs_ml_video_mode_get_current(&mode) == 0) {
        g_fs_ml_target_refresh_rate = mode.fps;
    }
    else {
        fs_log("WARNING: could not read refresh rate from current mode\n");
        g_fs_ml_target_refresh_rate = 0;
    }
    if (g_fs_ml_target_refresh_rate) {
        g_fs_ml_target_frame_time = 1000000 / g_fs_ml_target_refresh_rate;
    }
    fs_log("assuming refresh rate: %d (%d usec per frame)\n",
            g_fs_ml_target_refresh_rate, g_fs_ml_target_frame_time);

    init_input();
}

double fs_ml_get_refresh_rate() {
    return g_fs_ml_target_refresh_rate;
}

#ifdef WINDOWS
// parameters from WinMain
int g_fs_ml_ncmdshow;
HINSTANCE g_fs_ml_hinstance;
#endif
