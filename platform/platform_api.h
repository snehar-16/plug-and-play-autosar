/**
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
uint8_t  Platform_NvmWrite(uint16_t blockId, const void *data, uint16_t len);
uint8_t  Platform_NvmRead(uint16_t blockId, void *data, uint16_t len);

#endif /* PLATFORM_API_H */