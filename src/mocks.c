#include "FreeRTOS.h"
#include "fault_table.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Mock 1Mbit EEPROM Storage */
static uint8_t eeprom_storage[2048][64]; 

/* The missing function the Linker is looking for */
uint64_t get_epoch_seconds(void) {
    return (uint64_t)time(NULL);
}

void EEPROM_Write(uint16_t page, uint16_t offset, uint8_t *data, uint16_t size) {
    if (page < 2048) {
        memcpy(&eeprom_storage[page][offset], data, size);
        printf("[HAL] Write: Page %d | Offset %d | DevAddr: %s\n", 
               page, offset, (page >= 1024 ? "0xA2" : "0xA0"));
    }
}

void EEPROM_Read(uint16_t page, uint16_t offset, uint8_t *data, uint16_t size) {
    if (page < 2048) {
        memcpy(data, &eeprom_storage[page][offset], size);
    }
}

void NvM_Table_Init(void) { printf("[System] NvM SQL Initialized\n"); }
void Dem_Init(void) { printf("[System] DEM Module Initialized\n"); }
void vTaskDelay(const TickType_t x) { usleep(x * 1000); }
void vTaskDelete(TaskHandle_t x) { }
void vTaskStartScheduler(void) { printf("\n[OS] System Running. Ethernet Port 23 (Telnet) Active.\n"); }
BaseType_t xTaskCreate(void (*f)(void*), const char *n, uint16_t s, void *p, UBaseType_t pr, TaskHandle_t *h) { return 1; }
