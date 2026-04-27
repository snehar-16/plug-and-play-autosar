#include <stdio.h>    /* MISRA DEVIATION: Required for PC HAL Simulation */
#include <stdint.h>
#include <string.h>
#include <time.h>     /* MISRA DEVIATION: Required for PC RTC Simulation */

#ifdef _WIN32
#include <windows.h>  /* MISRA DEVIATION: Required for Windows Sleep */
/* MISRA: Macro names should generally be uppercase */
#define SLEEP_MS(x) Sleep((DWORD)(x))
#else
#include <unistd.h>   /* MISRA DEVIATION: Required for Linux usleep */
/* MISRA: Cast ignored return value of usleep */
#define SLEEP_MS(x) (void)usleep((x) * 1000U)
#endif

/* Mock 1Mbit EEPROM Storage (128KB) */
static uint8_t eeprom_storage[2048][64]; 

/* Simulates the STM32 RTC Epoch Provider */
uint64_t get_epoch_seconds(void) {
    return (uint64_t)time(NULL);
}

/* Mock Hardware Functions for the PC Demo */
void EEPROM_Write(uint16_t page, uint16_t offset, const uint8_t *data, uint16_t size) {
    /* MISRA: Added NULL checks and bounds validation to prevent PC segfaults */
    if ((page < 2048U) && (data != NULL) && ((offset + size) <= 64U)) {
        (void)memcpy(&eeprom_storage[page][offset], data, (size_t)size);
        
        (void)printf("[HAL-SIM] Write: Page %u | Offset %u | Data: ", page, offset);
        for(uint16_t i = 0U; i < size; i++) {
            (void)printf("%02X ", data[i]);
        }
        (void)printf("\n");
    }
}

void EEPROM_Read(uint16_t page, uint16_t offset, uint8_t *data, uint16_t size) {
    if ((page < 2048U) && (data != NULL) && ((offset + size) <= 64U)) {
        (void)memcpy(data, &eeprom_storage[page][offset], (size_t)size);
    }
}

/* Initialization Mocks */
void NvM_Table_Init(void) { 
    (void)printf("[System] Mock EEPROM Storage Initialized\n"); 
}

void Dem_Init(void) { 
    (void)printf("[System] DEM Module Initialized\n"); 
}

/* FreeRTOS Mocks for PC Compilation */
typedef uint32_t TickType_t;

void vTaskDelay(const TickType_t x) { 
    SLEEP_MS(x); 
}

/* STM32 HAL Mocks for PC Compilation */
typedef uint32_t I2C_HandleTypeDef;
I2C_HandleTypeDef hi2c1 = 0U;

uint8_t HAL_I2C_Mem_Write(I2C_HandleTypeDef* hi, uint16_t dev, uint16_t addr, uint16_t size, const uint8_t* d, uint16_t l, uint32_t t) { 
    /* MISRA: Explicitly acknowledge unused simulation parameters */
    (void)hi; (void)dev; (void)addr; (void)size; (void)d; (void)l; (void)t;
    return 0U; /* 0U = HAL_OK */
}

uint8_t HAL_I2C_Mem_Read(I2C_HandleTypeDef* hi, uint16_t dev, uint16_t addr, uint16_t size, uint8_t* d, uint16_t l, uint32_t t) { 
    (void)hi; (void)dev; (void)addr; (void)size; (void)d; (void)l; (void)t;
    return 0U; /* 0U = HAL_OK */
}