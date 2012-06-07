#ifndef MACHDEP_RPT_H
#define MACHDEP_RPT_H

//typedef unsigned long frame_time_t;
//extern frame_time_t read_processor_time(void);

#include <machdep/machdep.h>
#include <fs/base.h>
static inline frame_time_t read_processor_time() {
    return fs_get_monotonic_time();
}

#endif // MACHDEP_RPT_H
