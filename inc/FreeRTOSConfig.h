#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

/* Required by latest FreeRTOS headers */
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS
#define configUSE_16_BIT_TICKS                  0

/* Basic OS Settings */
#define configUSE_PREEMPTION                    1
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               3
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            1024
#define configMINIMAL_STACK_SIZE                128
#define configMAX_PRIORITIES                    5
#define configTICK_RATE_HZ                      1000
#define portTICK_PERIOD_MS                      (1000 / configTICK_RATE_HZ)

/* Type Definitions for the simulation */
typedef uint32_t TickType_t;
typedef uint32_t BaseType_t;
typedef uint32_t UBaseType_t;

/* Simulation Mocks */
#define vTaskStartScheduler()                   while(1) { vTaskTester(NULL); sleep(2); break; }
#define xTaskCreate(a,b,c,d,e,f)                printf("[OS] Task Created: %s\n", b)
#define vTaskDelay(x)                           usleep(x * 1000)
#define pdMS_TO_TICKS(x)                        (x)
#define xTaskGetHandle(x)                       ((void*)1)
#define vTaskDelete(x)                          printf("[OS] Task Deleted\n")
#define xTimerCreate(a,b,c,d,e)                 ((void*)2)
#define xTimerStart(a,b)                        printf("[OS] Timer Started: %s\n", a)

#endif
