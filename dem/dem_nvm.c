/**
 * @file dem_nvm.c
 * @brief Integrated NvM Layer with IEC 61508 Boot Fallback & Stuck-Bit Detection
 */

#include "../platform/platform_api.h"
#include "dem_cfg.h"
#include <string.h>

/* Primary and Mirror block definitions from the partition map [cite: 17, 32] */
#define NVM_PRIMARY_BLOCK    0x01
#define NVM_MIRROR_BLOCK     0x02
#define MAX_NVM_BLOCK_SIZE   4096 

/* Fault DID for EEPROM/NvM hardware failure [cite: 17, 26] */
#define DID_EEPROM_FAILURE   0xF10B 

/* Extern for immediate fault logging into the DEM core */
extern void Dem_SetEventStatus(uint16_t did, uint8_t isFailed);

/**
 * @brief CRC-32 Calculator (Standard IEEE 802.3 Polynomial) 
 */
static uint32_t NvM_CalculateCRC32(const uint8_t *data, uint16_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

/**
 * @brief SIL-4 High-Integrity Write with Stuck-Bit Detection [cite: 17]
 * Logic: Write -> 5ms HW Settling Delay -> Read-Back Verify -> Fault Trigger
 */
uint8_t NvM_Write_Verified(uint16_t blockId, uint8_t* data, uint16_t size) {
    /* Static buffer prevents stack overflow for SIL-4 deterministic execution */
    static uint8_t verifyBuf[MAX_NVM_BLOCK_SIZE]; 
    
    if (size > MAX_NVM_BLOCK_SIZE) return PLATFORM_NOT_OK;

    Platform_EnterCritical(); /* Disable interrupts for I2C bus stability */
    
    if (Platform_NvmWrite(blockId, data, size) != PLATFORM_OK) {
        Platform_ExitCritical();
        return PLATFORM_NOT_OK;
    }
    
    /* 5ms mandatory delay for physical EEPROM settling while feeding watchdog  */
    uint32_t startTick = Platform_GetTick_ms();
    while((Platform_GetTick_ms() - startTick) < 5) {
        Platform_WdgTrigger();
    }

    if (Platform_NvmRead(blockId, verifyBuf, size) != PLATFORM_OK) {
        Platform_ExitCritical();
        return PLATFORM_NOT_OK;
    }
    
    Platform_ExitCritical(); 

    /* Stuck-Bit Detection: mismatch triggers Fault 0xF10B immediately [cite: 17, 26] */
    if (memcmp(data, verifyBuf, size) != 0) {
        Dem_SetEventStatus(DID_EEPROM_FAILURE, 1); 
        return PLATFORM_NOT_OK; 
    }
    return PLATFORM_OK;
}

/**
 * @brief Dual-Block Mirror Save [cite: 17]
 */
void Dem_Nvm_Save(uint8_t *blockData, uint16_t length) {
    if (length < 4) return; 

    /* Append CRC-32 to the last 4 bytes of the data block */
    uint32_t crc = NvM_CalculateCRC32(blockData, length - 4);
    memcpy(&blockData[length - 4], &crc, 4);

    /* Write to both Primary and Mirror blocks [cite: 17] */
    NvM_Write_Verified(NVM_PRIMARY_BLOCK, blockData, length);
    NvM_Write_Verified(NVM_MIRROR_BLOCK, blockData, length);
}

/**
 * @brief IEC 61508 Boot Fallback Chain [cite: 17]
 * Logic: Step 1 (Primary) -> Step 2 (Mirror + Auto-Repair) -> Step 3 (Clean Start)
 */
uint8_t Dem_Nvm_Load(uint8_t *outData, uint16_t length) {
    static uint8_t readBuf[MAX_NVM_BLOCK_SIZE];
    uint32_t storedCrc, calcCrc;

    if (length > MAX_NVM_BLOCK_SIZE || length < 4) return PLATFORM_NOT_OK;

    /* Step 1: Try Primary Partition [cite: 17] */
    if (Platform_NvmRead(NVM_PRIMARY_BLOCK, readBuf, length) == PLATFORM_OK) {
        memcpy(&storedCrc, &readBuf[length - 4], 4);
        calcCrc = NvM_CalculateCRC32(readBuf, length - 4);
        if (storedCrc == calcCrc) {
            memcpy(outData, readBuf, length);
            return PLATFORM_OK;
        }
    }

    /* Step 2: Fallback to Mirror Partition [cite: 17] */
    memset(readBuf, 0x00, length); 
    if (Platform_NvmRead(NVM_MIRROR_BLOCK, readBuf, length) == PLATFORM_OK) {
        memcpy(&storedCrc, &readBuf[length - 4], 4);
        calcCrc = NvM_CalculateCRC32(readBuf, length - 4);
        if (storedCrc == calcCrc) {
            memcpy(outData, readBuf, length);
            
            /* SIL-4 Auto-Repair: Restore Primary from healthy Mirror [cite: 17] */
            NvM_Write_Verified(NVM_PRIMARY_BLOCK, readBuf, length);
            return PLATFORM_OK;
        }
    }

    /* Step 3: Clean Start (Final Fallback) [cite: 17] */
    memset(outData, 0x00, length);
    return PLATFORM_NOT_OK; 
}