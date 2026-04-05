#include "FreeRTOS.h"
#include "task.h"
#include "Dem_Cfg.h"
#include "main.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;
#define EEPROM_DEV_ADDR 0xA2

/**
 * @brief Standard CRC-32 Calculation (IEEE 802.3)
 * Provides superior error detection for safety-critical railway records.
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
 * @brief Maps SMOP_SWRS v1.1 DIDs to separate EEPROM partitions.
 * Partition 1 (0xF1xx): Fault Logs | Partition 2 (0xF2xx): Operational Records
 */
static void Get_Addr(uint16_t did, uint16_t *pg, uint16_t *off) {
    /* Session 1: System Faults (DIDs 0xF100 - 0xF10E) */
    if (did >= 0xF100 && did <= 0xF10E) {
        *pg  = DEM_S1_START_PAGE + ((did - 0xF100) / 4); // 4 records per page
        *off = ((did - 0xF100) % 4) * 16;                // 16-byte alignment
    } 
    /* Session 2: Operational Events (DIDs 0xF200 - 0xF205) */
    else if (did >= 0xF200 && did <= 0xF205) {
        /* SMOP_SWRS_30: Separate partition to avoid wear-leveling conflicts */
        *pg  = DEM_S2_START_PAGE + ((did - 0xF200) / 4);
        *off = ((did - 0xF200) % 4) * 16;
    }
}

/**
 * @brief Writes a 16-byte record with CRC-32 and Mandatory Read-Back Verification.
 * Ensures SMOP_SWRS compliance for non-volatile storage integrity.
 */
void NvM_Write_Verified(uint16_t did, uint8_t* data, uint16_t size) {
    uint16_t pg = 0, off = 0;
    uint8_t writeBuf[16] = {0};
    uint8_t verifyBuf[16] = {0};
    
    Get_Addr(did, &pg, &off);
    
    if (pg > 0) {
        /* 1. Prepare Payload: Data + CRC-32 for integrity */
        memcpy(writeBuf, data, size);
        uint32_t crc = NvM_CalculateCRC32(writeBuf, 12); // Calc on first 12 bytes
        memcpy(&writeBuf[12], &crc, 4);

        /* 2. Physical Write: Addressing 1Mbit space via I2C */
        uint16_t memAddress = (pg * EEPROM_PAGE_SIZE) + off;
        HAL_I2C_Mem_Write(&hi2c1, EEPROM_DEV_ADDR, memAddress, 2, writeBuf, 16, 100);
        
        /* SMOP_SWRS_29/30: Mandatory 5ms EEPROM internal write cycle */
        vTaskDelay(pdMS_TO_TICKS(5));

        /* 3. Read-Back Verification to ensure "Vital Message" integrity */
        HAL_I2C_Mem_Read(&hi2c1, EEPROM_DEV_ADDR, memAddress, 2, verifyBuf, 16, 100);
        
        if (memcmp(writeBuf, verifyBuf, 16) != 0) {
            /* Trigger Critical Failure: EEPROM Write Failure (0xF10B) */
            Dem_SetEventStatus(0xF10B, 1);
        }
    }
}

/**
 * @brief Reads a record and validates integrity via stored CRC-32.
 */
void NvM_ReadDemRecord(uint16_t did, uint8_t* data, uint16_t size) {
    uint16_t pg = 0, off = 0;
    uint8_t recordBuffer[16] = {0};
    
    Get_Addr(did, &pg, &off);
    
    if (pg > 0) {
        uint16_t memAddress = (pg * EEPROM_PAGE_SIZE) + off;
        HAL_I2C_Mem_Read(&hi2c1, EEPROM_DEV_ADDR, memAddress, 2, recordBuffer, 16, 100);

        /* Validate Integrity */
        uint32_t storedCrc;
        memcpy(&storedCrc, &recordBuffer[12], 4);
        uint32_t calcCrc = NvM_CalculateCRC32(recordBuffer, 12);

        if (storedCrc == calcCrc) {
            /* Copy requested payload size back to DCM */
            memcpy(data, recordBuffer, size);
        } else {
            /* Return zeroed buffer on CRC mismatch to prevent unsafe data read-back */
            memset(data, 0x00, size);
        }
    }
}
