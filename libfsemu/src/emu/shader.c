#include <glib.h>

#define FILTERING_NOTSET 0
#define FILTERING_NEAREST 1
#define FILTERING_LINEAR 2

#define SCALING_NOTSET 0
#define SCALING_FIXED 1
#define SCALING_INPUT 2
#define SCALING_OUTPUT 3

typedef struct shader_pass {
    int program;
    int filtering;
    int horiscale;
    int vertscale;
} shader_pass;

GList *g_shader_passes = NULL;
GList *g_current_shaders = NULL;

void dummy() {
#if 0

    for (;;) {
        int node = 0;
        int vertex_shader = 0;
        int fragment_shader = 0;
        if (strcmp(node->name, "vertex")) {
            vertex_shader = 1;
        }
        else if (strcmp(node->name, "fragment")) {
            fragment_shader = 1;
        }
        else {
            // If the element’s name is not vertex or fragment, end this iteration of the loop and go immediately to the next iteration.
            continue;
        }
        if (vertex_shader) {
            if (g_current_shaders != NULL) {
                // If "the current list of shaders" is not empty, this shader file is invalid. Abort this algorithm, do not continue trying to load the file. Host applications should alert the user that there was a problem loading the shader.
                fs_emu_warning("Error (1) loading shader");
                return 0;
            }

            // retrieve text
            // compile shader
            int shader = 0;
            // check compilation status
            if (0) {
                fs_emu_warning("Error (2) loading shader");
                return 0;
            }
            g_list_append(g_current_shaders, GINT_TO_POINTER(shader));
            continue;
        }

        // If the element has more than one attribute from the set {size, size_x, scale, scale_x, outscale, outscale_x}, this shader file is invalid. Abort this algorithm, do not continue trying to load the file. Host applications should alert the user that there was a problem loading the file.
        // If the element has more than one attribute from the set {size, size_y, scale, scale_y, outscale, outscale_y}, this shader file is invalid. Abort this algorithm, do not continue trying to load the file. Host applications should alert the user that there was a problem loading the file.
    }
#endif
}
