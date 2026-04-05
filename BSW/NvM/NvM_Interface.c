#include "FreeRTOS.h"
#include "task.h"
#include "Dem_Cfg.h"
#include "main.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;
#define EEPROM_DEV_ADDR 0xA2

/**
 * @brief Standard CRC-32 Calculation (IEEE 802.3)
 * Provides superior error detection over CRC-16 for safety-critical records. [cite: 47, 57]
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
 * @brief Maps UDS DIDs to specific EEPROM pages and offsets. 
 */
static void Get_Addr(uint16_t did, uint16_t *pg, uint16_t *off) {
    /* Session 1: System Faults (DIDs 0xF1xx) */
    if (did >= 0xF100 && did <= 0xF10E) {
        *pg  = (did <= 0xF105) ? DEM_S1_START_PAGE : (DEM_S1_START_PAGE + 1);
        *off = (did <= 0xF105) ? (did - 0xF100) * 16 : (did - 0xF106) * 16;
    } 
    /* Session 2: Operational Events (DIDs 0xF2xx) */
    else if (did >= 0xF200 && did <= 0xF205) {
        if (did <= 0xF201)      { *pg = DEM_S2_START_PAGE;     *off = (did == 0xF200) ? 0 : 32; }
        else if (did <= 0xF203) { *pg = DEM_S2_START_PAGE + 1; *off = (did == 0xF202) ? 0 : 32; }
        else                    { *pg = DEM_S2_START_PAGE + 2; *off = (did == 0xF204) ? 0 : 32; }
    }
}

/**
 * @brief Writes a 16-byte record with CRC-32 and Read-Back Verification.
 * Superior to Technical Reference: Guarantees data was physically written to silicon. 
 */
void NvM_WriteDemRecord(uint16_t did, uint8_t* data, uint16_t size) {
    uint16_t pg = 0, off = 0;
    uint8_t writeBuf[16] = {0};
    uint8_t verifyBuf[16] = {0};
    
    Get_Addr(did, &pg, &off);
    
    /* Ensure payload leaves room for 4-byte CRC-32 */
    if (pg > 0 && size <= 12) {
        /* 1. Prepare 16-byte record: Data + CRC-32 [cite: 47] */
        memcpy(writeBuf, data, size);
        uint32_t crc = NvM_CalculateCRC32(writeBuf, 12);
        memcpy(&writeBuf[12], &crc, 4);

        /* 2. Physical Write and mandatory 5ms cycle delay [cite: 31, 76] */
        uint16_t memAddress = (pg * EEPROM_PAGE_SIZE) + off;
        HAL_I2C_Mem_Write(&hi2c1, EEPROM_DEV_ADDR, memAddress, I2C_MEMADD_SIZE_16BIT, writeBuf, 16, 100);
        vTaskDelay(pdMS_TO_TICKS(5));

        /* 3. Read-Back Verification [cite: 22] */
        HAL_I2C_Mem_Read(&hi2c1, EEPROM_DEV_ADDR, memAddress, I2C_MEMADD_SIZE_16BIT, verifyBuf, 16, 100);
        
        if (memcmp(writeBuf, verifyBuf, 16) != 0) {
            /* Trigger Critical Hardware Fault on verification failure  */
            Dem_SetEventStatus(DID_EEPROM_WRITE_FAILURE, 1);
        }
    }
}

/**
 * @brief Reads a record and validates integrity via CRC-32. [cite: 22]
 */
void NvM_ReadDemRecord(uint16_t did, uint8_t* data, uint16_t size) {
    uint16_t pg = 0, off = 0;
    uint8_t recordBuffer[16] = {0};
    
    Get_Addr(did, &pg, &off);
    
    if (pg > 0) {
        uint16_t memAddress = (pg * EEPROM_PAGE_SIZE) + off;
        HAL_I2C_Mem_Read(&hi2c1, EEPROM_DEV_ADDR, memAddress, I2C_MEMADD_SIZE_16BIT, recordBuffer, 16, 100);

        /* Validate Integrity using stored CRC-32 */
        uint32_t readCrc;
        memcpy(&readCrc, &recordBuffer[12], 4);
        uint32_t calcCrc = NvM_CalculateCRC32(recordBuffer, 12);

        if (readCrc == calcCrc) {
            memcpy(data, recordBuffer, size);
        } else {
            /* Return zeroed buffer on corruption [cite: 57] */
            memset(data, 0x00, size);
        }
    }
}