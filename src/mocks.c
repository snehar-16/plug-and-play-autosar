#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(x) Sleep(x)
#else
#include <unistd.h>
#define sleep_ms(x) usleep(x * 1000)
#endif

/* Mock 1Mbit EEPROM Storage (128KB) */
static uint8_t eeprom_storage[2048][64]; 

/* Simulates the STM32 RTC Epoch Provider */
uint64_t get_epoch_seconds(void) {
    return (uint64_t)time(NULL);
}

/* Mock Hardware Functions for the PC Demo */
void EEPROM_Write(uint16_t page, uint16_t offset, uint8_t *data, uint16_t size) {
    if (page < 2048) {
        memcpy(&eeprom_storage[page][offset], data, size);
        printf("[HAL-SIM] Write: Page %d | Offset %d | Data: ", page, offset);
        for(int i=0; i<size; i++) printf("%02X ", data[i]);
        printf("\n");
    }
}

void EEPROM_Read(uint16_t page, uint16_t offset, uint8_t *data, uint16_t size) {
    if (page < 2048) {
        memcpy(data, &eeprom_storage[page][offset], size);
    }
}

/* Initialization Mocks */
void NvM_Table_Init(void) { printf("[System] Mock EEPROM Storage Initialized\n"); }
void Dem_Init(void) { printf("[System] DEM Module Initialized\n"); }

/* FreeRTOS Mocks for PC Compilation */
typedef uint32_t TickType_t;
void vTaskDelay(const TickType_t x) { sleep_ms(x); }
#include <stdint.h>
typedef int I2C_HandleTypeDef;
I2C_HandleTypeDef hi2c1;
int HAL_I2C_Mem_Write(void* hi, uint16_t dev, uint16_t addr, uint16_t size, uint8_t* d, uint16_t l, uint32_t t) { return 0; }
int HAL_I2C_Mem_Read(void* hi, uint16_t dev, uint16_t addr, uint16_t size, uint8_t* d, uint16_t l, uint32_t t) { return 0; }
