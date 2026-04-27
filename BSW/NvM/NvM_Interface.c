#include "FreeRTOS.h"
#include "task.h"
#include "Dem_Cfg.h"
#include "main.h"
#include <string.h>

/* MISRA Note: Externs should ideally be in a header, but allowed here if not globally shared */
extern I2C_HandleTypeDef hi2c1;
#define EEPROM_DEV_ADDR 0xA2U

/**
 * @brief Standard CRC-32 Calculation (IEEE 802.3)
 * Provides superior error detection for safety-critical railway records.
 */
static uint32_t NvM_CalculateCRC32(const uint8_t *data, uint16_t len) {
    uint32_t crc = 0xFFFFFFFFU;
    
    if (data != NULL) {
        for (uint16_t i = 0U; i < len; i++) {
            crc ^= (uint32_t)data[i];
            for (uint8_t j = 0U; j < 8U; j++) {
                /* MISRA Compliant alternative to: -(crc & 1) */
                if ((crc & 1U) != 0U) {
                    crc = (crc >> 1U) ^ 0xEDB88320U;
                } else {
                    crc = (crc >> 1U);
                }
            }
        }
    }
    return (crc ^ 0xFFFFFFFFU);
}

/**
 * @brief Maps SMOP_SWRS v1.1 DIDs to separate EEPROM partitions.
 * Partition 1 (0xF1xx): Fault Logs | Partition 2 (0xF2xx): Operational Records
 */
static void Get_Addr(uint16_t did, uint16_t *pg, uint16_t *off) {
    if ((pg != NULL) && (off != NULL)) {
        /* Session 1: System Faults (DIDs 0xF100 - 0xF10E) */
        if ((did >= 0xF100U) && (did <= 0xF10EU)) {
            *pg  = DEM_S1_START_PAGE + ((did - 0xF100U) / 4U); /* 4 records per page */
            *off = ((did - 0xF100U) % 4U) * 16U;               /* 16-byte alignment */
        } 
        /* Session 2: Operational Events (DIDs 0xF200 - 0xF205) */
        else if ((did >= 0xF200U) && (did <= 0xF205U)) {
            /* SMOP_SWRS_30: Separate partition to avoid wear-leveling conflicts */
            *pg  = DEM_S2_START_PAGE + ((did - 0xF200U) / 4U);
            *off = ((did - 0xF200U) % 4U) * 16U;
        } else {
            *pg = 0U;
            *off = 0U;
        }
    }
}

/**
 * @brief Writes a 16-byte record with CRC-32 and Mandatory Read-Back Verification.
 * Ensures SMOP_SWRS compliance for non-volatile storage integrity.
 */
void NvM_Write_Verified(uint16_t did, const uint8_t* data, uint16_t size) {
    uint16_t pg = 0U;
    uint16_t off = 0U;
    uint8_t writeBuf[16] = {0U};
    uint8_t verifyBuf[16] = {0U};
    
    /* MISRA: Check bounds and pointer validity */
    if ((data != NULL) && (size <= 12U)) {
        Get_Addr(did, &pg, &off);
        
        if (pg > 0U) {
            /* 1. Prepare Payload: Data + CRC-32 for integrity */
            (void)memcpy(writeBuf, data, (size_t)size);
            uint32_t crc = NvM_CalculateCRC32(writeBuf, 12U); /* Calc on first 12 bytes */
            
            /* MISRA: Byte-wise assignment prevents strict-aliasing violations */
            writeBuf[12] = (uint8_t)((crc >> 24U) & 0xFFU);
            writeBuf[13] = (uint8_t)((crc >> 16U) & 0xFFU);
            writeBuf[14] = (uint8_t)((crc >> 8U)  & 0xFFU);
            writeBuf[15] = (uint8_t)(crc & 0xFFU);

            /* 2. Physical Write: Addressing 1Mbit space via I2C */
            uint16_t memAddress = (pg * EEPROM_PAGE_SIZE) + off;
            (void)HAL_I2C_Mem_Write(&hi2c1, EEPROM_DEV_ADDR, memAddress, 2U, writeBuf, 16U, 100U);
            
            /* SMOP_SWRS_29/30: Mandatory 5ms EEPROM internal write cycle */
            vTaskDelay(pdMS_TO_TICKS(5U));

            /* 3. Read-Back Verification to ensure "Vital Message" integrity */
            (void)HAL_I2C_Mem_Read(&hi2c1, EEPROM_DEV_ADDR, memAddress, 2U, verifyBuf, 16U, 100U);
            
            if (memcmp(writeBuf, verifyBuf, 16U) != 0) {
                /* Trigger Critical Failure: EEPROM Write Failure (0xF10B) */
                Dem_SetEventStatus(0xF10BU, 1U);
            }
        }
    }
}

/**
 * @brief Reads a record and validates integrity via stored CRC-32.
 */
void NvM_ReadDemRecord(uint16_t did, uint8_t* data, uint16_t size) {
    uint16_t pg = 0U;
    uint16_t off = 0U;
    uint8_t recordBuffer[16] = {0U};
    
    if ((data != NULL) && (size <= 12U)) {
        Get_Addr(did, &pg, &off);
        
        if (pg > 0U) {
            uint16_t memAddress = (pg * EEPROM_PAGE_SIZE) + off;
            (void)HAL_I2C_Mem_Read(&hi2c1, EEPROM_DEV_ADDR, memAddress, 2U, recordBuffer, 16U, 100U);

            /* MISRA: Reconstruct CRC via bitwise shifts */
            uint32_t storedCrc = ((uint32_t)recordBuffer[12] << 24U) |
                                 ((uint32_t)recordBuffer[13] << 16U) |
                                 ((uint32_t)recordBuffer[14] << 8U)  |
                                 ((uint32_t)recordBuffer[15]);

            uint32_t calcCrc = NvM_CalculateCRC32(recordBuffer, 12U);

            if (storedCrc == calcCrc) {
                /* Copy requested payload size back to DCM */
                (void)memcpy(data, recordBuffer, (size_t)size);
            } else {
                /* Return zeroed buffer on CRC mismatch to prevent unsafe data read-back */
                (void)memset(data, 0x00, (size_t)size);
            }
        } else {
            /* Return zeroed buffer if DID address is invalid */
            (void)memset(data, 0x00, (size_t)size);
        }
    }
}