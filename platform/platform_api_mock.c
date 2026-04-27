#include "platform_api.h"
#include <stdint.h>
uint16_t g_stuckBlock = 0xFFFFU;
void Platform_Init(void) {}
void Platform_RTC_Init(void) {}
void Platform_WdgTrigger(void) {}
void Platform_EnterCritical(void) {}
void Platform_ExitCritical(void) {}
void Mock_Force_EEPROM_Stuck_Bit(uint16_t b){ g_stuckBlock = b; }
uint8_t Platform_NvmWrite_Final(uint16_t b, const uint8_t* d, uint16_t l){ if(b == 0xFF) return 0; return (b == g_stuckBlock) ? 1 : 0; }
uint8_t Platform_NvmRead(uint16_t b, uint8_t* d, uint16_t l){ return 0; }
uint32_t Platform_GetTick_ms(void){ static uint32_t t=0; return t+=10; }
