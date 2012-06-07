#include "sysconfig.h"
#include "sysdeps.h"

#include <time.h>
#include <fs/base.h>

static struct tm *g_local_time;

struct tm *uae_get_amiga_time() {
    //printf("uae_get_amiga_time\n");

    if (uae_synchronous_mode()) {
        // FIXME: get synchronized clock here
        time_t t = 0;
#ifdef DEBUG_SYNC
        write_sync_log("uae_get_amiga_time: (fixed to %d)\n", 0);
#endif
        return gmtime(&t);
    }
    else {
        time_t t = time(NULL);
        //t += currprefs.cs_rtc_adjust;
        g_local_time = localtime (&t);
        return g_local_time;
    }
}

#if 0
frame_time_t read_processor_time() {
    return fs_get_monotonic_time();
}
#endif

#if 0

#include "machdep/rpt.h"
extern void sleep_millis (int ms);
extern frame_time_t syncbase;

#ifdef WINDOWS
#include <Windows.h>
#elif defined(MACOSX)
#include <mach/mach_time.h>
#else
#define CLOCK_FREQ (10 * 1000 * 1000)
#endif

frame_time_t read_processor_time (void) {
#ifdef WINDOWS
    // FIXME: USING timeGetTime because of problems with
    // QueryPerformanceFrequency lying

    /*
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result.QuadPart;
    */
    return ((frame_time_t) timeGetTime()) * (10 * 1000);

#elif defined(MACOSX)
    return mach_absolute_time();
#else
    int clock_gettime(clockid_t clk_id, struct timespec *tp);
    struct timespec tp;
    static time_t base_secs = 0;
    if (base_secs == 0) {
        clock_gettime(CLOCK_REALTIME, &tp);
        base_secs = tp.tv_sec;
    }
    clock_gettime(CLOCK_REALTIME, &tp);
    tp.tv_sec -= base_secs;
    frame_time_t t = tp.tv_sec * CLOCK_FREQ + tp.tv_nsec / 100;
    return t;
#endif
}

//extern int rpt_available;
static void figure_processor_speed (void)
{
#ifdef WINDOWS
    /*
    LARGE_INTEGER result;
    QueryPerformanceFrequency(&result);
    // FIXME: OK?
    unsigned long freq = result.QuadPart;
    unsigned long qpfrate = freq;
    syncbase = (unsigned long)qpfrate;
    */
    syncbase = (10 * 1000 * 1000);
    //rpt_available = 1;
#elif defined(MACOSX)
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    unsigned long freq = 1000000000 * info.denom / info.numer;
    unsigned long qpfrate = freq;
    syncbase = (unsigned long)qpfrate;
    //rpt_available = 1;
#else
    /*
    struct timespec res;
    clock_getres(CLOCK_REALTIME, &res);
    //write_log("Res: %d %d\n", res.tv_sec, res.tv_nsec);
    long long nanores = res.tv_sec * 1000000000 + res.tv_nsec;
    //double fracres = res.tv_sec * (double) res.tv_nsec / 1000000000.0;
    //double freq = 1.0 / fracres; //(nanores / 1000000000);
    double freq = 1000000000.0 / nanores;
    */

    unsigned long freq = CLOCK_FREQ;
    unsigned long qpfrate = freq;
    long qpcdivisor = 0;
    write_log("CLOCKFREQ: CLOCK_REALTIME %.2fMHz (%.2fMHz, DIV=%d)\n", freq / 1000000.0,
            qpfrate / 1000000.0, 1 << qpcdivisor);
    syncbase = (unsigned long)qpfrate;

    // set rpt_available so custom.cpp knowns that we can limit the amiga
    // speed
    //rpt_available = 1;
#endif
}
#endif
