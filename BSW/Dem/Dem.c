#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "Dem_Cfg.h"
#include "NvM_Interface.h"

/* Status Tracking Arrays */
static uint8_t  EventStatus[TOTAL_DTC_SLOTS];
static uint16_t AgingCounter[TOTAL_DTC_SLOTS];
static uint8_t  PassedCycleCount[TOTAL_DTC_SLOTS];

/* Debounce Counters for SMOP_SWRS_31 (50ms HW Debounce) [cite: 11] */
static int16_t DebounceTimer[TOTAL_DTC_SLOTS];

extern uint64_t get_epoch_seconds(void);

/**
 * @brief Logs data to 1Mbit EEPROM based on DID Category.
 * Session 1 (Faults): [2B DID][8B Timestamp][2B Counter] = 12B Payload [cite: 12]
 * Session 2 (Events): [2B DID][8B Timestamp] = 10B Payload [cite: 17]
 */
void Dem_Internal_Log(uint16_t did, uint8_t isFault) {
    uint8_t record[16] = {0};
    uint16_t count = 0;
    uint64_t ts = get_epoch_seconds(); 
    uint8_t payloadSize = isFault ? 12 : 10; [cite: 12, 17]

    if (isFault) {
        /* Retrieve existing occurrence count for Fault DIDs [cite: 12] */
        NvM_ReadDemRecord(did, record, 16);
        memcpy(&count, &record[10], 2);
        count++;
    }

    uint8_t newPayload[12];
    memcpy(&newPayload[0], &did, 2);      /* Byte 0-1: DID  */
    memcpy(&newPayload[2], &ts, 8);       /* Byte 2-9: Timestamp  */
    
    if (isFault) {
        memcpy(&newPayload[10], &count, 2); /* Byte 10-11: Occurrence [cite: 12] */
    }

    /* High-integrity write with Read-Back Verification  */
    NvM_Write_Verified(did, newPayload, payloadSize);
}

/**
 * @brief Updates status and triggers logging based on SWRS v1.1 classification[cite: 7, 12].
 */
void Dem_SetEventStatus(uint16_t did, uint8_t isFailed) {
    uint8_t index = did % TOTAL_DTC_SLOTS;
    uint8_t isSession1 = (did >= 0xF100 && did <= 0xF10E); [cite: 12]

    if (isFailed) {
        /* Standard UDS Status Byte update [cite: 12] */
        EventStatus[index] |= (DEM_UDS_STATUS_TF | DEM_UDS_STATUS_PDTC | DEM_UDS_STATUS_CDTC);
        
        if (isSession1) {
            EventStatus[index] |= DEM_UDS_STATUS_WIR; /* Trigger Buzzer for Faults  */
            AgingCounter[index] = 0;
            PassedCycleCount[index] = 0;
            Dem_Internal_Log(did, 1); /* Log as Session 1 Fault [cite: 12] */
        } else {
            /* Session 2: Trigger snapshot only (No Fault bits) [cite: 15] */
            Dem_Internal_Log(did, 0); /* Log as Session 2 Event [cite: 17] */
        }
    } else {
        EventStatus[index] &= ~DEM_UDS_STATUS_TF;
        PassedCycleCount[index]++;
    }
    
    EventStatus[index] &= ~DEM_UDS_STATUS_TNCSLC;
}

/**
 * @brief Implements DEM_DEBOUNCE_COUNTER_BASED for SMOP_SWRS_31.
 * HW debounce is 50ms; thresholds are kept low for rapid detection.
 */
void Dem_ReportError(uint16_t did, uint8_t rawSignal) {
    uint8_t index = did % TOTAL_DTC_SLOTS;
    
    /* Vital Message CRC Faults (0xF107) bypass debounce for immediate logging  */
    if (did == 0xF107 && rawSignal == 1) {
        Dem_SetEventStatus(did, 1);
        return;
    }

    if (rawSignal) {
        DebounceTimer[index]++;
        if (DebounceTimer[index] >= 5) { /* ~250ms Confirmation */
            Dem_SetEventStatus(did, 1);
            DebounceTimer[index] = 5;
        }
    } else {
        DebounceTimer[index]--;
        if (DebounceTimer[index] <= -5) { /* ~250ms Healing */
            Dem_SetEventStatus(did, 0);
            DebounceTimer[index] = -5;
        }
    }
}

void Dem_MainFunction(void) {
    for (int i = 0; i < TOTAL_DTC_SLOTS; i++) {
        /* DTC Aging: Auto-clear Faults after threshold  */
        if (!(EventStatus[i] & DEM_UDS_STATUS_TF)) {
            AgingCounter[i]++;
            if (AgingCounter[i] >= DEM_AGING_THRESHOLD) {
                EventStatus[i] = 0;
                AgingCounter[i] = 0;
            }
        }

        /* DTC Healing: Silence Buzzer after 3 passed cycles  */
        if (PassedCycleCount[i] >= DEM_HEALING_THRESHOLD) {
            EventStatus[i] &= ~DEM_UDS_STATUS_WIR; [cite: 19]
        }
    }
}