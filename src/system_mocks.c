#include <stdint.h>
#include <time.h>
uint32_t Get_RTC_Timestamp(void) {
    return (uint32_t)time(NULL);
}
