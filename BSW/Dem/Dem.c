#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "Dem_Cfg.h"
#include "fault_table.h"

/* Status Tracking Arrays */
static uint8_t  EventStatus[TOTAL_DTC_SLOTS];
static uint16_t AgingCounter[TOTAL_DTC_SLOTS];
static uint8_t  PassedCycleCount[TOTAL_DTC_SLOTS];

extern uint64_t get_epoch_seconds(void);

/**
 * @brief Logs event to EEPROM with an incrementing Occurrence Counter.
 * Format: [2B DID] [8B Timestamp] [2B Counter] [4B CRC32 added by NvM]
 */
void Dem_Report_With_Count(uint16_t did) {
    uint8_t record[16] = {0};
    uint16_t count = 0;
    
    /* 1. Read existing record to retrieve current occurrence count */
    NvM_ReadDemRecord(did, record, 16);
    memcpy(&count, &record[10], 2);
    
    /* 2. Increment counter and prepare updated payload */
    count++;
    uint64_t ts = get_epoch_seconds(); 
    
    uint8_t newPayload[12];
    memcpy(&newPayload[0], &did, 2);      /* DID Bytes 0-1 */
    memcpy(&newPayload[2], &ts, 8);       /* Timestamp Bytes 2-9 */
    memcpy(&newPayload[10], &count, 2);   /* Occurrence Counter Bytes 10-11 */
    
    /* 3. Write with Read-Back Verification and CRC32 */
    NvM_Write_Verified(did, newPayload, 12);
}

/**
 * @brief Updates fault status and triggers high-integrity logging.
 * Implements the 8-bit UDS status byte state machine per ISO 14229-1. [cite: 18]
 */
void Dem_SetEventStatus(uint16_t did, uint8_t isFailed) {
    uint8_t index = did % TOTAL_DTC_SLOTS;

    if (isFailed) {
        /* Set Active, Pending, and Confirmed bits [cite: 18] */
        EventStatus[index] |= (DEM_UDS_STATUS_TF | DEM_UDS_STATUS_PDTC | DEM_UDS_STATUS_CDTC);
        EventStatus[index] |= DEM_UDS_STATUS_WIR; /* Trigger Buzzer [cite: 18, 44] */
        
        AgingCounter[index] = 0;      /* Reset aging cycle  */
        PassedCycleCount[index] = 0;  /* Reset healing cycle  */

        /* Log confirmed fault with incremented occurrence count  */
        Dem_Report_With_Count(did); 
    } else {
        /* Healing: Clear Active bit  */
        EventStatus[index] &= ~DEM_UDS_STATUS_TF;
        PassedCycleCount[index]++;
    }
    
    /* Mark test completed since last clear [cite: 18] */
    EventStatus[index] &= ~DEM_UDS_STATUS_TNCSLC;
}

/**
 * @brief Background Task for DTC Aging and Healing.
 * Mandatory for AUTOSAR DEM compliance. 
 */
void Dem_MainFunction(void) {
    for (int i = 0; i < TOTAL_DTC_SLOTS; i++) {
        /* DTC Aging: Auto-clear after 40 operation cycles  */
        if (!(EventStatus[i] & DEM_UDS_STATUS_TF)) {
            AgingCounter[i]++;
            if (AgingCounter[i] >= DEM_AGING_THRESHOLD) {
                EventStatus[i] = 0; /* Fully clear DTC from RAM */
                AgingCounter[i] = 0;
            }
        }

        /* DTC Healing: Clear Warning Indicator after 3 passed cycles  */
        if (PassedCycleCount[i] >= DEM_HEALING_THRESHOLD) {
            EventStatus[i] &= ~DEM_UDS_STATUS_WIR; /* Silence Station Master Buzzer [cite: 44] */
        }
    }
}