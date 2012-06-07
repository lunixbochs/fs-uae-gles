#include "sysconfig.h"
#include "sysdeps.h"

#include "uae.h"
#include "autoconf.h"
#include <string.h>
#include <glib.h>
#include <string.h>
#include <glib/gstdio.h>

#include "memory.h"
#include "options.h"
#include "keyboard.h"
#include "inputdevice.h"
#include "disk.h"
#include "events.h"

int uae_get_memory_checksum();

void keyboard_settrans (void);
libamiga_callbacks g_libamiga_callbacks = {};
log_function g_amiga_gui_message_function = NULL;

int g_amiga_netplay_mode = 0;
int g_amiga_paused = 0;
char *g_libamiga_save_image_path = NULL;

#ifdef DEBUG_SYNC
FILE* g_fs_uae_sync_debug_file = NULL;
#endif

int g_amiga_video_format = AMIGA_VIDEO_FORMAT_RGBA;

extern "C" {

int amiga_init() {
    printf("libamiga (based on %s) initialized\n",
            get_libamiga_base_version());
    write_log("libamiga (based on %s) initialized\n",
            get_libamiga_base_version());

    /*
#ifdef DEBUG_SYNC
    g_sync_debug_file = fopen("sync.log", "wb");
#endif
    */

    // clock sync base is 1000000 (microseconds)
    syncbase = 1000000;

    filesys_host_init();
    return 1;
}

void amiga_set_video_format(int format) {
    g_amiga_video_format = format;
}

void amiga_add_rtg_resolution(int width, int height) {
    write_log("adding rtg resolution %dx%d\n", width, height);
    int *m = g_amiga_rtg_modes;
    while (1) {
        if (*m == -1) {
            write_log("too many resolutions\n");
            return;
        }
        if (*m == width && *(m + 1) == height) {
            write_log("resolution already exists\n");
            return;
        }
        if (*m == 0 && *(m + 1) ==0) {
            *m = width;
            *(m + 1) = height;
            return;
        }
        m += 2;
    }
}

void amiga_map_cd_drives(int enable) {
    write_log("setting automount_cddrives to %d\n", enable != 0);
    currprefs.win32_automount_cddrives = (enable != 0);
    changed_prefs.win32_automount_cddrives = (enable != 0);
}

void amiga_enable_netplay_mode() {
    write_log("libamiga enabling net play mode\n");
    g_amiga_netplay_mode = 1;
}

void amiga_write_uae_config(const char *path) {
	write_log("writing uae config to %s\n", path);
	cfgfile_save(&currprefs, path, 0);
}

void amiga_set_paths(const char **rom_paths, const char **floppy_paths,
		const char **cd_paths, const char **hd_paths) {
	for (int i = 0; i < MAX_PATHS; i++) {
		if (floppy_paths[i] == NULL || floppy_paths[i][0] == '\0') {
			break;
		}
		strncpy(&(currprefs.path_floppy.path[i][0]),
				floppy_paths[i], MAX_PATH - 1);
		strncpy(&(changed_prefs.path_floppy.path[i][0]),
				floppy_paths[i], MAX_PATH - 1);
	}
	for (int i = 0; i < MAX_PATHS; i++) {
		if (cd_paths[i] == NULL || cd_paths[i][0] == '\0') {
			break;
		}
		strncpy(&(currprefs.path_cd.path[i][0]),
				cd_paths[i], MAX_PATH - 1);
		strncpy(&(changed_prefs.path_cd.path[i][0]),
				cd_paths[i], MAX_PATH - 1);
	}
	for (int i = 0; i < MAX_PATHS; i++) {
		if (hd_paths[i] == NULL || hd_paths[i][0] == '\0') {
			break;
		}
		strncpy(&(currprefs.path_hardfile.path[i][0]),
				hd_paths[i], MAX_PATH - 1);
		strncpy(&(changed_prefs.path_hardfile.path[i][0]),
				hd_paths[i], MAX_PATH - 1);
	}
	for (int i = 0; i < MAX_PATHS; i++) {
		if (rom_paths[i] == NULL || rom_paths[i][0] == '\0') {
			break;
		}
		strncpy(&(currprefs.path_rom.path[i][0]),
				rom_paths[i], MAX_PATH - 1);
		strncpy(&(changed_prefs.path_rom.path[i][0]),
				rom_paths[i], MAX_PATH - 1);
	}
}

int amiga_set_synchronization_log_file(const char *path) {
#ifdef DEBUG_SYNC
	FILE *f = g_fopen(path, "wb");
	if (f) {
		write_log("sync debug log to %s\n", path);
		g_fs_uae_sync_debug_file = f;
		return 1;
	}
	else {
		write_log("error opening synchronization log file\n");
		return 0;
	}
#endif
}

int amiga_quickstart(int quickstart_model, int quickstart_config,
        int accuracy) {
    int quickstart_compa = 1 - accuracy;
    int quickstart_romcheck = 0;
    write_log("amiga_quickstart model=%d config=%d compa=%d (accuracy %d)\n",
            quickstart_model, quickstart_config, quickstart_compa, accuracy);
    return built_in_prefs(&currprefs, quickstart_model, quickstart_config,
            quickstart_compa, quickstart_romcheck);
}

void amiga_set_save_image_dir(const char *path) {
    write_log("amiga_set_save_image_dir %s\n", path);
    g_libamiga_save_image_path = g_strdup(path);
}

int amiga_get_rand_checksum() {
    return uaerand() & 0x00ffffff;
}

int amiga_get_state_checksum() {
    int checksum = uae_get_memory_checksum();
#ifdef DEBUG_SYNC
	write_sync_log("memcheck: %08x\n", checksum);
#endif
    return checksum & 0x00ffffff;
}

//int amiga_main(int argc, char** argv) {
void amiga_main() {
    write_log("amiga_main\n");
    keyboard_settrans();
    int argc = 1;
    char *argv[4] = {
            "fs-uae",
            NULL,
    };
	real_main(argc, argv);
#ifdef FILESYS
	write_log("real_main returned\n");
	write_log("calling filesys_flush_cache\n");
    filesys_flush_cache ();
#endif
}

void amiga_write_config(const char *path) {
    cfgfile_save(&currprefs, path, 0);
}

int amiga_enable_serial_port() {
    write_log("amiga_enable_serial_port\n");
    changed_prefs.use_serial = 1;
    currprefs.use_serial = 1;
    //strcpy(changed_prefs.sername, "dummy");
    //strcpy(currprefs.sername, "dummy");
    //config_changed = 1;
    return 1;
}

void amiga_set_cpu_idle(int idle) {
    write_log("setting cpu_idle = %d\n", idle);
    changed_prefs.cpu_idle = idle;
    currprefs.cpu_idle = idle;
}

int amiga_pause(int pause) {
    //pause_emulation = pause;
    if (pause) {
        write_log("calling pausemode (1)\n");
        //pausemode(-1);
        g_amiga_paused = 1;
        pausemode(9);
    }
    else {
        write_log("calling pausemode (0)\n");
        g_amiga_paused = 0;
        //pausemode(-1);
        pausemode(0);
    }
    return 0;
}

int amiga_reset(int hard) {
    uae_reset(hard);
    return hard;
}

int amiga_state_save(int slot) {
    if (slot < 0) {
        return 0;
    }
    if (slot >= 9) {
        return 0;
    }
    write_log("amiga_state_save %d\n", slot);
    int code = AKS_STATESAVEQUICK1 + slot * 2;
    inputdevice_add_inputcode(code, 1);
    return 1;
}

int amiga_state_load(int slot) {
    if (slot < 0) {
        return 0;
    }
    if (slot >= 9) {
        return 0;
    }
    write_log("amiga_state_load %d\n", slot);
    int code = AKS_STATERESTOREQUICK1 + slot * 2;
    inputdevice_add_inputcode(code, 1);
    return 1;
}

const char *amiga_floppy_get_file(int index) {
    return currprefs.floppyslots[index].df;
}

const char *amiga_floppy_get_list_entry(int index) {
    return currprefs.dfxlist[index];
}

int amiga_floppy_get_drive_type(int index) {
    return currprefs.floppyslots[index].dfxtype + 1;
}

int amiga_get_num_cdrom_drives() {
    for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
        if (currprefs.cdslots[i].inuse == 0) {
            return i;
        }
    }
    return MAX_TOTAL_SCSI_DEVICES;
}

int amiga_get_num_floppy_drives() {
    for (int i = 0; i < 4; i++) {
        if (currprefs.floppyslots[i].dfxtype < 0) {
            return i;
        }
    }
    return 4;
}

int amiga_floppy_set_from_list(int drive, int index) {
    write_log("insert floppy (%d) into drive (%d)\n", index, drive);
	if (drive < 0 || drive > 3) {
		// assert
		return 0;
	}
	if (index < 0 || index >= AMIGA_FLOPPY_LIST_SIZE) {
		// assert
		return 0;
	}
	write_log("perform disk_swap drive %d floppy entry %d\n", drive, index);
	strcpy(changed_prefs.floppyslots[drive].df, currprefs.dfxlist[index]);
	//strcpy(currprefs.floppyslots[drive].df, currprefs.dfxlist[entry]);
	disk_insert(drive, currprefs.floppyslots[drive].df);
    /*
    int i;
    // eject disk from other drive (if inserted)
    for (i = 0; i < 4; i++) {
        if (strcmp (currprefs.floppyslots[i].df, currprefs.dfxlist[index]) == 0)
            changed_prefs.floppyslots[i].df[0] = 0;
    }
    // insert disk
    // FIXME: IMPORTANT: CHECK length of file (prevent buffer overrun)
    strcpy(changed_prefs.floppyslots[drive].df, currprefs.dfxlist[index]);
    config_changed = 1;
    return 1;
    */
	return 1;
}

int amiga_floppy_set_file(int drive, const char *file) {
    write_log("insert floppy (%s) into drive (%d)\n", file, drive);
    int i;
    // eject disk from other drive (if inserted)
    for (i = 0; i < 4; i++) {
        if (strcmp (currprefs.floppyslots[i].df, file) == 0) {
            changed_prefs.floppyslots[i].df[0] = 0;
        }
    }
    // insert disk
    // FIXME: IMPORTANT: CHECK length of file (prevent buffer overrun)
    strcpy(changed_prefs.floppyslots[drive].df, file);
    config_changed = 1;
    return 1;
}

const char *amiga_cdrom_get_file(int index) {
    return currprefs.cdslots[index].name;
}

int amiga_cdrom_set_file(int drive, const char *file) {
    write_log("insert CD (%s) into drive (%d)\n", file, drive);
    int i;
    // eject CD from other drive (if inserted)
    for (i = 0; i < 4; i++) {
        if (strcmp (currprefs.cdslots[i].name, file) == 0) {
            changed_prefs.cdslots[i].name[0] = 0;
        }
    }
    // insert CD
    // FIXME: IMPORTANT: CHECK length of file (prevent buffer overrun)
    strcpy(changed_prefs.cdslots[drive].name, file);
    config_changed = 1;
    return 1;
}

int amiga_floppy_get_speed() {
    int speed = currprefs.floppy_speed;
    write_log("speed is %d\n", speed);
    return speed / 100;
}

int amiga_floppy_set_speed(int speed) {
    write_log("set floppy speed to %d\n", speed);
    changed_prefs.floppy_speed = speed * 100;
    config_changed = 1;
    return 1;
}

int amiga_cpu_get_speed() {
    int speed = currprefs.m68k_speed;
    write_log("cpu speed is %d\n", speed);
    write_log("cpu freq is %d\n", currprefs.cpu_frequency);
    write_log("cpu mult is %d\n", currprefs.cpu_clock_multiplier);
    return speed;
}

int amiga_cpu_set_speed(int speed) {
    static int initialized = 0;
    static int org_cpu_clock_multiplier;
    static int org_cpu_cycle_exact;
    if (!initialized) {
        org_cpu_clock_multiplier = currprefs.cpu_clock_multiplier;
        org_cpu_cycle_exact = currprefs.cpu_cycle_exact;
        initialized = 1;
    }
    write_log("set cpu speed to %d\n", speed);
    changed_prefs.m68k_speed = speed;
    if (speed == 0) {
        changed_prefs.cpu_clock_multiplier = org_cpu_clock_multiplier;
        changed_prefs.cpu_cycle_exact = org_cpu_cycle_exact;
    }
    else {
        changed_prefs.cpu_clock_multiplier = 32;
        changed_prefs.cpu_cycle_exact = 0;
    }
    config_changed = 1;
    return 1;
}

int amiga_parse_option(const char *option, const char *value, int type) {
    // some strings are modified during parsing
    char *value2 = strdup(value);
    int result = cfgfile_parse_option(&currprefs, (char*) option,
            (char*) value2, type);
    free(value2);
    write_log("set option \"%s\" to \"%s\" (result: %d)\n", option,
            value, result);
    if (result != 1) {
        // FIXME: Log
        amiga_log_warning("failed to set option \"%s\" to \"%s\" "
                "(result: %d)\n", option, value, result);
    }
    return result;
}

int amiga_set_option(const char *option, const char *value) {
	return amiga_parse_option(option, value, 0);
}

int amiga_set_hardware_option(const char *option, const char *value) {
	return amiga_parse_option(option, value, CONFIG_TYPE_HARDWARE);
}

int amiga_set_int_option(const char *option, int value) {
    gchar *str_value = g_strdup_printf("%d", value);
    int result = amiga_set_option(option, str_value);
    g_free(str_value);
    return result;
}

int amiga_quit() {
    uae_quit();
    return 1;
}

void amiga_set_display_function(display_function event_handler) {
    g_libamiga_callbacks.display = event_handler;
}

void amiga_set_event_function(event_function event_handler) {
    g_libamiga_callbacks.event = event_handler;
}

void amiga_set_init_function(init_function function) {
    g_libamiga_callbacks.init = function;
}

void amiga_set_render_function(render_function function) {
    g_libamiga_callbacks.render = function;
}

void amiga_set_log_function(log_function function) {
    g_libamiga_callbacks.log = function;
}

void amiga_set_gui_message_function(log_function function) {
    g_amiga_gui_message_function = function;
}

} // extern "C"

int disk_setwriteprotect (int num, const TCHAR *name, bool writeprotected);
void gui_disk_image_change (int unitnum, const TCHAR *name, bool writeprotected) {
    if (name && strlen(name) > 0) {
        write_log("gui_disk_image_change drive %d name %s write protected %d\n",
                unitnum, name, writeprotected);
        /*
        if (writeprotected) {
            write_log("calling disk_setwriteprotect 0\n");
            disk_setwriteprotect(unitnum, name, 0);
        }
        */
    }
    else {
        write_log("gui_disk_image_change drive %d <no disk>\n", unitnum);
    }

}

bool get_plugin_path (TCHAR *out, int size, const TCHAR *path) {
    static char* plugin_path_none = NULL;
    static char* plugin_path_floppysounds = NULL;

    write_log("\n-----------------> STUB: get_plugin_path, size: %d, path: %s\n", size, path); 
    if (strcmp(path, "floppysounds") == 0) {
        if (!plugin_path_floppysounds) {
            plugin_path_floppysounds = strdup("floppysounds/");
        }
        strncpy(out, plugin_path_floppysounds, size);
        // make sure out is null-terminated in any case
        out[size - 1] = '\0';
    }
    else {
        out[0] = '\0';
    }
    return TRUE;
}

