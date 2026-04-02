#include "OsAl.h"

#if defined(AUTOSAR_ADAPTIVE)
#include <time.h>
#include <unistd.h>
#include <pthread.h>

static pthread_mutex_t OsAl_Mutex   = PTHREAD_MUTEX_INITIALIZER;
static uint32_t        OsAl_Nesting = 0U;
static struct timespec OsAl_StartTime;

void OsAl_Init(void)
{
    clock_gettime(CLOCK_MONOTONIC, &OsAl_StartTime);
    OsAl_Nesting = 0U;
}

uint32_t OsAl_GetTickMs(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    uint32_t ms  = (uint32_t)(now.tv_sec - OsAl_StartTime.tv_sec) * 1000U;
    ms += (uint32_t)((now.tv_nsec - OsAl_StartTime.tv_nsec) / 1000000L);
    return ms;
}

void OsAl_EnterCritical(void)
{
    if (OsAl_Nesting == 0U) {
        pthread_mutex_lock(&OsAl_Mutex);
    }
    OsAl_Nesting++;
}

void OsAl_ExitCritical(void)
{
    if (OsAl_Nesting > 0U) {
        OsAl_Nesting--;
        if (OsAl_Nesting == 0U) {
            pthread_mutex_unlock(&OsAl_Mutex);
        }
    }
}

void OsAl_Delay(uint32_t ms)
{
    usleep((useconds_t)(ms * 1000U));
}
#endif
