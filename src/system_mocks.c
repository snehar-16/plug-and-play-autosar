#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../platform/platform_api.h"

/* Mocks for SM-OCIP Platform HAL */

void Platform_Init(void) { }
void Platform_RTC_Init(void) { }
void Platform_WdgTrigger(void) { }
void Platform_EnterCritical(void) { }
void Platform_ExitCritical(void) { }

uint32_t Platform_GetTick_ms(void) {
    static uint32_t tick = 0;
    return tick += 5; /* Increment by 5ms to instantly pass timing guards in tests */
}

/* Mock EEPROM Storage */
static uint8_t mock_eeprom[2048][64]; 
static uint8_t stuck_blocks[2048] = {0}; /* Track physically damaged blocks */

uint8_t Platform_NvmWrite(uint16_t blockId, const void *data, uint16_t len) {
    if (blockId < 2048) {
        memcpy(mock_eeprom[blockId], data, len > 64 ? 64 : len);
        
        /* If hardware is damaged, the write fails to change the stuck bit! */
        if (stuck_blocks[blockId]) {
            mock_eeprom[blockId][0] = 0xFF; 
        }
        return PLATFORM_OK;
    }
    return PLATFORM_NOT_OK;
}

uint8_t Platform_NvmRead(uint16_t blockId, void *data, uint16_t len) {
    if (blockId < 2048) {
        memcpy(data, mock_eeprom[blockId], len > 64 ? 64 : len);
        return PLATFORM_OK;
    }
    return PLATFORM_NOT_OK;
}

/* Test function to simulate permanent silicon damage */
void Mock_Force_EEPROM_Stuck_Bit(uint16_t blockId) {
    if (blockId < 2048) {
        stuck_blocks[blockId] = 1;
        mock_eeprom[blockId][0] = 0xFF;
        printf("[Mock] Simulated permanent hardware corruption on block %d\n", blockId);
    }
}
