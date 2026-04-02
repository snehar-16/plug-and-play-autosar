#ifndef FAULT_TABLE_H
#define FAULT_TABLE_H

#include <stdint.h>

/* EEPROM Driver Prototypes used by NvM */
void EEPROM_Write(uint16_t page, uint16_t offset, uint8_t *data, uint16_t size);
void EEPROM_Read(uint16_t page, uint16_t offset, uint8_t *data, uint16_t size);

#endif


