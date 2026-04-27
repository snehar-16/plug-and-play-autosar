#include <stdio.h>    /* MISRA DEVIATION: Required for PC HAL Simulation */
#include <stdint.h>
#include <string.h>
#include "../platform/platform_api.h"

/* MISRA: Define magic numbers as explicit macros */
#define MOCK_EEPROM_PAGES       2048U
#define MOCK_EEPROM_PAGE_SIZE   64U

/* Mocks for SM-OCIP Platform HAL */

void Platform_Init(void) { 
    /* MISRA: Empty blocks must be documented */ 
}

void Platform_RTC_Init(void) { 
    /* Mock empty function */ 
}

void Platform_WdgTrigger(void) { 
    /* Mock empty function */ 
}

void Platform_EnterCritical(void) { 
    /* Mock empty function */ 
}

void Platform_ExitCritical(void) { 
    /* Mock empty function */ 
}

uint32_t Platform_GetTick_ms(void) {
    static uint32_t tick = 0U;
    
    /* MISRA Rule 13.4: Do not use the result of an assignment operator */
    tick += 5U; /* Increment by 5ms to instantly pass timing guards in tests */
    
    return tick; 
}

/* Mock EEPROM Storage */
static uint8_t mock_eeprom[MOCK_EEPROM_PAGES][MOCK_EEPROM_PAGE_SIZE]; 
static uint8_t stuck_blocks[MOCK_EEPROM_PAGES] = {0U}; /* Track physically damaged blocks */

/* MISRA Rule 11.5: Replaced 'void *' with 'uint8_t *' to maintain strict typing */
uint8_t Platform_NvmWrite(uint16_t blockId, const uint8_t *data, uint16_t len) {
    uint8_t status = PLATFORM_NOT_OK;
    uint16_t safeLen;

    /* MISRA: Defensive NULL pointer check */
    if ((blockId < MOCK_EEPROM_PAGES) && (data != NULL)) {
        safeLen = (len > MOCK_EEPROM_PAGE_SIZE) ? MOCK_EEPROM_PAGE_SIZE : len;
        
        (void)memcpy(mock_eeprom[blockId], data, (size_t)safeLen);
        
        /* If hardware is damaged, the write fails to change the stuck bit! */
        if (stuck_blocks[blockId] != 0U) {
            mock_eeprom[blockId][0] = 0xFFU; 
        }
        
        status = PLATFORM_OK;
    }
    
    return status;
}

uint8_t Platform_NvmRead(uint16_t blockId, uint8_t *data, uint16_t len) {
    uint8_t status = PLATFORM_NOT_OK;
    uint16_t safeLen;

    if ((blockId < MOCK_EEPROM_PAGES) && (data != NULL)) {
        safeLen = (len > MOCK_EEPROM_PAGE_SIZE) ? MOCK_EEPROM_PAGE_SIZE : len;
        
        (void)memcpy(data, mock_eeprom[blockId], (size_t)safeLen);
        status = PLATFORM_OK;
    }
    
    return status;
}

/* Test function to simulate permanent silicon damage */
void Mock_Force_EEPROM_Stuck_Bit(uint16_t blockId) {
    if (blockId < MOCK_EEPROM_PAGES) {
        stuck_blocks[blockId] = 1U;
        mock_eeprom[blockId][0] = 0xFFU;
        
        (void)printf("[Mock] Simulated permanent hardware corruption on block %u\n", blockId);
    }
}