#include "fault_table.h"
#include "fault_table.h"
#include "fault_table.h"
#include "fault_table.h"
#include "fault_table.h"
#include "FreeRTOS.h"
#include "Dem_Cfg.h"
#include "fault_table.h"
#include <string.h>

/* Maps DIDs to specific 1Mbit EEPROM Pages (2043-2047) */
static void Get_Addr(uint16_t did, uint16_t *pg, uint16_t *off) {
    if (did >= 0xF100 && did <= 0xF10E) {
        /* Session 1: Faults */
        *pg = (did <= 0xF105) ? 2043 : 2044;
        *off = (did <= 0xF100 + 5) ? (did - 0xF100) * 10 : (did - 0xF106) * 10;
    } else if (did >= 0xF200 && did <= 0xF205) {
        /* Session 2: Operational Last-Occurrence (32-byte slots) */
        if (did <= 0xF201) { *pg = 2045; *off = (did == 0xF200) ? 0 : 32; }
        else if (did <= 0xF203) { *pg = 2046; *off = (did == 0xF202) ? 0 : 32; }
        else { *pg = 2047; *off = (did == 0xF204) ? 0 : 32; }
    }
}

void NvM_WriteDemRecord(uint16_t did, uint8_t* data, uint16_t size) {
    uint16_t pg = 0, off = 0;
    Get_Addr(did, &pg, &off);
    if (pg > 0) EEPROM_Write(pg, off, data, size);
}

void NvM_ReadDemRecord(uint16_t did, uint8_t* data, uint16_t size) {
    uint16_t pg = 0, off = 0;
    Get_Addr(did, &pg, &off);
    if (pg > 0) EEPROM_Read(pg, off, data, size);
}
