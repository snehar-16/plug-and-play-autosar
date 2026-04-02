#ifndef FREERTOS_H
#define FREERTOS_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/* FreeRTOS Constants */
#define pdFALSE                         ( (long) 0 )
#define pdTRUE                          ( (long) 1 )
#define portTICK_PERIOD_MS              ( (uint32_t) 1 )

/* FreeRTOS Types */
typedef uint32_t     TickType_t;
typedef long         BaseType_t;
typedef unsigned long UBaseType_t;
typedef void* TaskHandle_t;
typedef void* QueueHandle_t;
typedef void* TimerHandle_t;

/* Prototypes */
void vTaskDelay(const TickType_t xTicksToDelay);
void vTaskDelete(TaskHandle_t xTaskToDelete);
TaskHandle_t xTaskGetHandle(const char *pcNameToQuery);
BaseType_t xTaskCreate(void (*pvTaskCode)(void*), const char * const pcName, const uint16_t usStackDepth, void *pvParameters, UBaseType_t uxPriority, TaskHandle_t *pxCreatedTask);
TimerHandle_t xTimerCreate(const char * const pcTimerName, const TickType_t xTimerPeriodInTicks, const UBaseType_t uxAutoReload, void * const pvTimerID, void (*pxCallbackFunction)(TimerHandle_t xTimer));
BaseType_t xTimerStart(TimerHandle_t xTimer, const TickType_t xTicksToWait);
void vTaskStartScheduler(void);

#endif
