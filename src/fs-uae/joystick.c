#include <uae/uae.h>
#include <fs/emu.h>
#include <glib.h>
#include "fs-uae.h"

fs_uae_input_port g_fs_uae_input_ports[4] = {};

static fs_emu_input_mapping g_joystick_mappings[][15] = {
    {
        { "left", INPUTEVENT_JOY1_LEFT },
        { "right", INPUTEVENT_JOY1_RIGHT },
        { "up", INPUTEVENT_JOY1_UP },
        { "down", INPUTEVENT_JOY1_DOWN },
        { "1", INPUTEVENT_JOY1_FIRE_BUTTON },
        { "2", INPUTEVENT_JOY1_2ND_BUTTON },
        { "3", INPUTEVENT_JOY1_3RD_BUTTON },
        { "red", INPUTEVENT_JOY1_CD32_RED },
        { "yellow", INPUTEVENT_JOY1_CD32_YELLOW },
        { "green", INPUTEVENT_JOY1_CD32_GREEN },
        { "blue", INPUTEVENT_JOY1_CD32_BLUE },
        { "rewind", INPUTEVENT_JOY1_CD32_RWD },
        { "forward", INPUTEVENT_JOY1_CD32_FFW },
        { "play", INPUTEVENT_JOY1_CD32_PLAY },
        { NULL, 0 },
    },
    {
        { "left", INPUTEVENT_JOY2_LEFT },
        { "right", INPUTEVENT_JOY2_RIGHT },
        { "up", INPUTEVENT_JOY2_UP },
        { "down", INPUTEVENT_JOY2_DOWN },
        { "1", INPUTEVENT_JOY2_FIRE_BUTTON },
        { "2", INPUTEVENT_JOY2_2ND_BUTTON },
        { "3", INPUTEVENT_JOY2_3RD_BUTTON },
        { "red", INPUTEVENT_JOY2_CD32_RED },
        { "yellow", INPUTEVENT_JOY2_CD32_YELLOW },
        { "green", INPUTEVENT_JOY2_CD32_GREEN },
        { "blue", INPUTEVENT_JOY2_CD32_BLUE },
        { "rewind", INPUTEVENT_JOY2_CD32_RWD },
        { "forward", INPUTEVENT_JOY2_CD32_FFW },
        { "play", INPUTEVENT_JOY2_CD32_PLAY },
        { NULL, 0 },
    },
    {
        { "left", INPUTEVENT_PAR_JOY1_LEFT },
        { "right", INPUTEVENT_PAR_JOY1_RIGHT },
        { "up", INPUTEVENT_PAR_JOY1_UP },
        { "down", INPUTEVENT_PAR_JOY1_DOWN },
        { "1", INPUTEVENT_PAR_JOY1_FIRE_BUTTON },
        { "2", INPUTEVENT_PAR_JOY1_2ND_BUTTON },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
    },
    {
        { "left", INPUTEVENT_PAR_JOY2_LEFT },
        { "right", INPUTEVENT_PAR_JOY2_RIGHT },
        { "up", INPUTEVENT_PAR_JOY2_UP },
        { "down", INPUTEVENT_PAR_JOY2_DOWN },
        { "1", INPUTEVENT_PAR_JOY2_FIRE_BUTTON },
        { "2", INPUTEVENT_PAR_JOY2_2ND_BUTTON },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
    },
};

static void map_mouse(int port) {
	fs_log("mapping mouse to port %d\n", port);
    if (port == 0) {
        fs_emu_configure_mouse(INPUTEVENT_MOUSE1_HORIZ,
                INPUTEVENT_MOUSE1_VERT, INPUTEVENT_JOY1_FIRE_BUTTON,
                0, INPUTEVENT_JOY1_2ND_BUTTON);
    }
    else if (port == 1) {
        fs_emu_configure_mouse(INPUTEVENT_MOUSE2_HORIZ,
                INPUTEVENT_MOUSE2_VERT, INPUTEVENT_JOY2_FIRE_BUTTON,
                0, INPUTEVENT_JOY2_2ND_BUTTON);
    }
    else {
    	fs_log("WARNING: cannot map mouse to this port\n");
    }
}

static void configure_joystick_port(int port, const gchar *value,
        const gchar *port_name, const gchar *joy_dev) {
    fs_emu_log("configuring joystick port %d (%s)\n", port, value);
    fs_uae_input_port *p = g_fs_uae_input_ports + port;

    const char *auto_type = "amiga";
    int auto_mode = AMIGA_JOYPORT_DJOY;
    if (g_fs_uae_amiga_model == MODEL_CD32) {
        auto_type = "amigacd32";
        auto_mode = AMIGA_JOYPORT_CD32JOY;
    }

    if (fs_ascii_strcasecmp(value, "nothing") == 0
            || fs_ascii_strcasecmp(value, "none") == 0) {
        fs_emu_log("nothing connected to port\n");
        strcpy(p->device, "");
        p->new_mode = AMIGA_JOYPORT_NONE;

    }
    else if (fs_ascii_strcasecmp(value, "auto") == 0 && port < 2) {
        if (port == 0) {
        	p->new_mode = AMIGA_JOYPORT_MOUSE;
        	strcpy(p->device, "MOUSE");
        }
        else {
            fs_emu_log("trying to auto-configure joystick 1 in port 1\n");
            p->new_mode = auto_mode;
            int result = fs_emu_configure_joystick("JOYSTICK", auto_type,
                    g_joystick_mappings[port],
                    g_fs_uae_input_ports[port].device, MAX_DEVICE_NAME_LEN);
            if (!result) {
                fs_emu_log("could not auto-configure joystick 1, "
                        "using keyboard emulation\n");
				strcpy(p->device, "KEYBOARD");
            }
        }
    }
    else if (fs_ascii_strcasecmp(value, "dummy mouse") == 0) {
        p->new_mode = AMIGA_JOYPORT_MOUSE;
    }
    else if (fs_ascii_strcasecmp(value, "dummy joystick") == 0) {
        p->new_mode = AMIGA_JOYPORT_DJOY;
    }
    else if (fs_ascii_strcasecmp(value, "mouse") == 0) {
    	strcpy(p->device, "MOUSE");
    	p->new_mode = AMIGA_JOYPORT_MOUSE;
    }
    // deprecated
    else if (fs_ascii_strcasecmp(value, "amiga_mouse") == 0) {
        p->new_mode = AMIGA_JOYPORT_MOUSE;
    }
    // deprecated
    else if (fs_ascii_strcasecmp(value, "amiga_joystick") == 0) {
        p->new_mode = AMIGA_JOYPORT_DJOY;
    }
    else {
        p->new_mode = auto_mode;
        int result = fs_emu_configure_joystick(value, auto_type,
                g_joystick_mappings[port], p->device, MAX_DEVICE_NAME_LEN);
    }

    char *key = fs_strdup_printf("joystick_port_%d_mode", port);
    char *mode_string = fs_config_get_string(key);
    free(key);
    if (mode_string) {
    	char *mode_lower = g_ascii_strdown(mode_string, -1);
    	free(mode_string);
    	if (strcmp(mode_lower, "joystick") == 0) {
    	    p->new_mode = AMIGA_JOYPORT_DJOY;
    	}
    	else if (strcmp(mode_lower, "mouse") == 0) {
    	    p->new_mode = AMIGA_JOYPORT_MOUSE;
    	}
    	else if (strcmp(mode_lower, "cd32 gamepad") == 0) {
    	    p->new_mode = AMIGA_JOYPORT_CD32JOY;
    	}
        else if (strcmp(mode_lower, "nothing") == 0) {
            p->new_mode = AMIGA_JOYPORT_NONE;
        }
        else if (strcmp(mode_lower, "none") == 0) {
            p->new_mode = AMIGA_JOYPORT_NONE;
        }
    	else {
    		fs_log("unknown joystick port mode: %s\n", mode_lower);
    	}
    	g_free(mode_lower);
    }
}

void fs_uae_configure_input() {
    fs_emu_log("configuring joystick ports:\n");
    amiga_set_option("joyport0", "none");
    amiga_set_option("joyport1", "none");

    //fs_emu_set_action_function(joystick_action);
    gchar *value;
    value = fs_config_get_string("joystick_port_0");
    if (value == NULL) {
        value = fs_strdup("mouse");
    }
    configure_joystick_port(0, value, "joyport0", "joy0");
    free(value);

    value = fs_config_get_string("joystick_port_1");
    if (value == NULL) {
        value = fs_strdup("auto");
    }
    configure_joystick_port(1, value, "joyport1", "joy1");
    free(value);

    value = fs_config_get_string("joystick_port_2");
    if (value == NULL) {
        value = fs_strdup("nothing");
    }
    configure_joystick_port(2, value, "joyport2", "joy2");
    free(value);

    value = fs_config_get_string("joystick_port_3");
    if (value == NULL) {
        value = fs_strdup("nothing");
    }
    configure_joystick_port(3, value, "joyport3", "joy3");
    free(value);

    fs_uae_configure_actions();

    /*
    for (int i = 0; i < FS_UAE_NUM_INPUT_PORTS; i++) {
    	g_fs_uae_input_ports[i].mode = g_fs_uae_input_ports[i].new_mode;
    }
    fs_uae_reconfigure_input_ports_host();
    */

    //fs_emu_reset_input_mapping();
    //fs_uae_map_keyboard();
    //fs_uae_reconfigure_input_ports_amiga();
}

void fs_uae_reconfigure_input_ports_amiga() {
	fs_emu_log("fs_uae_reconfigure_input_ports_amiga\n");
	int modes = INPUTEVENT_AMIGA_JOYPORT_MODE_0_LAST -
			INPUTEVENT_AMIGA_JOYPORT_MODE_0_NONE + 1;
	for (int i = 0; i < FS_UAE_NUM_INPUT_PORTS; i++) {
		fs_uae_input_port *port = g_fs_uae_input_ports + i;
		fs_log("sending event to set port %d to mode %d\n", i, port->new_mode);
		int action = INPUTEVENT_AMIGA_JOYPORT_MODE_0_NONE + modes * i + \
				port->new_mode;
		fs_emu_queue_action(action, 1);
	}
}

void fs_uae_reconfigure_input_ports_host() {
	fs_emu_log("fs_uae_reconfigure_input_ports_host\n");
	fs_emu_reset_input_mapping();
	fs_uae_map_keyboard();

	for (int i = 0; i < FS_UAE_NUM_INPUT_PORTS; i++) {
		fs_uae_input_port *port = g_fs_uae_input_ports + i;

		fs_log("configuring joystick port %d\n", i);
		if (port->mode == AMIGA_JOYPORT_NONE) {
			fs_log("* nothing in port\n");
			fs_log("* FIXME\n");
		}
		else if (port->mode == AMIGA_JOYPORT_MOUSE) {
			fs_log("* amiga mouse\n");
		    if (strcmp(port->device, "MOUSE") == 0) {
		    	fs_log("* using host mouse\n");
		        map_mouse(i);
		    }
		    else {
		    	fs_log("* not mapping host device to amiga mouse\n");
		    }
		}
		else if (port->mode == AMIGA_JOYPORT_DJOY) {
			fs_log("* amiga joystick\n");
			if (strcmp(port->device, "MOUSE") == 0) {
				fs_log("* cannot map mouse to joystick\n");
			}
			else {
				fs_log("* using device %s\n", port->device);
				fs_emu_configure_joystick(port->device, "amiga",
						g_joystick_mappings[i], NULL, 0);
			}
		}
		else if (port->mode == AMIGA_JOYPORT_CD32JOY) {
			fs_log("* amiga cd32 gamepad\n");
			if (strcmp(port->device, "MOUSE") == 0) {
				fs_log("* cannot map mouse to cd32 gamepad\n");
			}
			else {
				fs_log("* using device %s\n", port->device);
				fs_emu_configure_joystick(port->device, "amigacd32",
						g_joystick_mappings[i], NULL, 0);
			}
		}
	}

	fs_emu_map_custom_actions();
}
