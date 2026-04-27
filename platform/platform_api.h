#ifndef PLATFORM_API_H
#define PLATFORM_API_H

#include <stdint.h>

/* Status Codes */
#define PLATFORM_OK      0U
#define PLATFORM_NOT_OK  1U

/* Function Prototypes */
void Platform_Init(void);
void Platform_RTC_Init(void);
void Platform_WdgTrigger(void);
void Platform_EnterCritical(void);
void Platform_ExitCritical(void);
uint32_t Platform_GetTick_ms(void);

/* Use strict uint8_t pointers for NvM access (MISRA compliant) */
uint8_t Platform_NvmWrite_Final(uint16_t blockId, const uint8_t *data, uint16_t len);
uint8_t Platform_NvmRead(uint16_t blockId, uint8_t *data, uint16_t len);

#endif /* PLATFORM_API_H *//* Change void* to const uint8_t* for Write */
uint8_t Platform_NvmWrite_Final(uint16_t blockId, const uint8_t *data, uint16_t len);

/* Change void* to uint8_t* for Read */
uint8_t Platform_NvmRead(uint16_t blockId, uint8_t *data, uint16_t len);/**
 * @file    platform_api.h
 * @brief   Board-agnostic platform abstraction layer for the embedded diagnostic stack.
 */

#ifndef PLATFORM_API_H
#define PLATFORM_API_H

#include <stdint.h>

#define PLATFORM_OK      ((uint8_t)0x00U)
#define PLATFORM_NOT_OK  ((uint8_t)0x01U)

/* --- Initialization --- */
void     Platform_Init(void);
void     Platform_RTC_Init(void);

/* --- Critical Sections --- */
void     Platform_EnterCritical(void);
void     Platform_ExitCritical(void);

/* --- Timing & Watchdog --- */
uint32_t Platform_GetTick_ms(void);
void     Platform_WdgTrigger(void);

/* --- Non-Volatile Memory --- */
uint8_t  Platform_NvmWrite_Final(uint16_t blockId, const void *data, uint16_t len);
uint8_t  Platform_NvmRead(uint16_t blockId, void *data, uint16_t len);

#endif /* PLATFORM_API_H */
