#include <stdint.h>
extern uint64_t get_epoch_seconds(void);
#include "FreeRTOS.h"
#include "Dem_Cfg.h"
#include "fault_table.h"

/* Explicit declaration to fix the warning */
extern void NvM_WriteDemRecord(uint16_t did, uint8_t* data, uint16_t size);

void Dem_ReportEvent(uint16_t did) {
    uint8_t record[16] = {0};
    uint64_t ts = get_epoch_seconds(); 

    if ((did & 0xFF00) == 0xF100) {
        /* Session 1: Fault Logic */
        record[0] = (uint8_t)(did >> 8); 
        record[1] = (uint8_t)(did & 0xFF);
        memcpy(&record[2], &ts, 8);
        NvM_WriteDemRecord(did, record, 10);
    } 
    else if ((did & 0xFF00) == 0xF200) {
        /* Session 2: Last-Occurrence Overwrite */
        memcpy(&record[0], &ts, 8);
        NvM_WriteDemRecord(did, record, 8);
    }
}
