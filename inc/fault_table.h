/* ============================================================================
 * File: NvM_Interface.h (formerly fault_table.h)
 * Description: Non-Volatile Memory (NvM) Interface definitions.
 * Exposes the hardware read/write APIs to the DEM and DCM layers.
 * ============================================================================ */

#ifndef FAULT_TABLE_H /* Change to NVM_INTERFACE_H if you rename the file */
#define FAULT_TABLE_H

#include <stdint.h>

/**
 * @brief Writes a diagnostic record to the physical EEPROM.
 * @param did  The 16-bit Data Identifier.
 * @param data Pointer to the payload buffer.
 * @param size Expected size of the payload (8 or 10 bytes).
 */
void NvM_WriteDemRecord(uint16_t did, uint8_t* data, uint16_t size);

/**
 * @brief Reads a diagnostic record from the physical EEPROM.
 * @param did  The 16-bit Data Identifier.
 * @param data Pointer to the buffer where data will be stored.
 * @param size Expected size of the payload (8 or 10 bytes).
 */
void NvM_ReadDemRecord(uint16_t did, uint8_t* data, uint16_t size);

#endif /* FAULT_TABLE_H */