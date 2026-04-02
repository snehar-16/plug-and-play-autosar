#include "OsAl.h"

#if defined(AUTOSAR_OS_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"

static uint32_t OsAl_CriticalNesting = 0U;

void OsAl_Init(void)
{
    OsAl_CriticalNesting = 0U;
}

uint32_t OsAl_GetTickMs(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void OsAl_EnterCritical(void)
{
    taskENTER_CRITICAL();
    OsAl_CriticalNesting++;
}

void OsAl_ExitCritical(void)
{
    if (OsAl_CriticalNesting > 0U) {
        OsAl_CriticalNesting--;
        taskEXIT_CRITICAL();
    }
}

void OsAl_Delay(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}
#endif
