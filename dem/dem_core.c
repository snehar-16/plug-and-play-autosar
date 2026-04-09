/**
 * @file dem_core.c
 * @brief Integrated DEM Core with Dual-Debounce Engine and 8-bit UDS Status
 * Compliance: SIL-4 / BHEL Kavach SMOP_SWRS v1.1
 */

#include "dem_cfg.h"
#include "../dcm/Dcm_Cfg.h"
#include "../platform/platform_api.h"
#include "dem_event_logger.h"
#include <string.h>
#include <stdio.h>

/* Functional Data Structures  */
static uint8_t  EventStatus[TOTAL_DTC_SLOTS];
static uint16_t AgingCounter[TOTAL_DTC_SLOTS];
static uint8_t  PassedCycleCount[TOTAL_DTC_SLOTS];
static int16_t  CounterDebounceTimer[TOTAL_DTC_SLOTS];
static int16_t  TimeDebounceTimer[TOTAL_DTC_SLOTS];

/* CRC Fault Identifier (Used for instant bypass) */
#define DID_CRC_VITAL      DEM_EVT_CRC_ERROR_VITAL 

/**
 * @brief Required by main.c to ensure RAM is clean on boot
 */
void Dem_Init(void) {
    memset(EventStatus, 0, sizeof(EventStatus));
    memset(AgingCounter, 0, sizeof(AgingCounter));
    memset(PassedCycleCount, 0, sizeof(PassedCycleCount));
    memset(CounterDebounceTimer, 0, sizeof(CounterDebounceTimer));
    memset(TimeDebounceTimer, 0, sizeof(TimeDebounceTimer));
    printf("[DEM] RAM Structures Initialized.\n");
}

/**
 * @brief Set 8-bit UDS Status and send to Black Box Logger 
 */
void Dem_SetEventStatus(uint16_t did, uint8_t isFailed) {
    uint8_t index = did % TOTAL_DTC_SLOTS;
    /* Fault range: 0xF100 to 0xF1FF (System Faults) */
    uint8_t isFault = (did >= 0xF100 && did <= 0xF1FF); 

    if (isFailed) {
        /* Set UDS Bits: TF, TFTOC, PDTC, CDTC, TFSLC  */
        EventStatus[index] |= (DEM_UDS_TF | DEM_UDS_TFTOC | DEM_UDS_PDTC | DEM_UDS_CDTC | DEM_UDS_TFSLC);
        /* Clear Not-Completed bits  */
        EventStatus[index] &= ~(DEM_UDS_TNCSLC | DEM_UDS_TNCTOC);
        
        if (isFault) {
            EventStatus[index] |= DEM_UDS_WIR; /* Warning Indicator Requested (Buzzer) */
            AgingCounter[index] = 0;
            PassedCycleCount[index] = 0;
        }
        
        /* Log Critical Fault to Circular Event Logger  */
        DEM_EventLogger_Write(did, 3 /*CRIT*/, 0, 0 /*FAIL*/, did, EventStatus[index]);
    } else {
        /* Clear Test Failed bit */
        EventStatus[index] &= ~DEM_UDS_TF;
        PassedCycleCount[index]++;
        
        /* Log Recovery to Circular Event Logger  */
        DEM_EventLogger_Write(did, 0 /*LOW*/, 0, 1 /*PASS*/, did, EventStatus[index]);
    }
}

/**
 * @brief Enhanced Dual-Debounce Engine 
 * Implements 50ms Counter-based and 100ms Time-based persistence.
 */
void Dem_ReportError(uint16_t did, uint8_t rawSignal) {
    uint8_t index = did % TOTAL_DTC_SLOTS;
    
    /* 1. Explicit CRC Fault Bypass: Log within <10ms per safety requirement  */
    if (did == DID_CRC_VITAL && rawSignal == 1) {
        Dem_SetEventStatus(did, 1);
        return;
    }

    /* 2. Counter-Based Debounce (Uses DEM_DEBOUNCE_FAIL_LIMIT from dem_cfg.h) */
    if (rawSignal) {
        if (CounterDebounceTimer[index] < DEM_DEBOUNCE_FAIL_LIMIT) {
            CounterDebounceTimer[index]++;
        }
        if (CounterDebounceTimer[index] >= DEM_DEBOUNCE_FAIL_LIMIT) { 
            /* 3. Time-Based Debounce (Uses DEBOUNCE_TIME_LIMIT from dem_cfg.h) */
            if (TimeDebounceTimer[index] < DEBOUNCE_TIME_LIMIT) {
                TimeDebounceTimer[index]++;
            } else {
                Dem_SetEventStatus(did, 1);
            }
        }
    } else {
        /* Reset timers on recovery to prevent transient noise spikes  */
        CounterDebounceTimer[index] = 0;
        TimeDebounceTimer[index] = 0;
        Dem_SetEventStatus(did, 0);
    }
}

/**
 * @brief Main Loop (Called every 10ms from FreeRTOS task)
 */
void Dem_MainFunction(void) {
    Platform_WdgTrigger(); /* SIL-4 Watchdog Feed */

    for (int i = 0; i < TOTAL_DTC_SLOTS; i++) {
        /* Aging: Auto-clear after 40 operation cycles (DEM_AGING_THRESHOLD) */
        if (!(EventStatus[i] & DEM_UDS_TF) && (EventStatus[i] & DEM_UDS_PDTC)) {
            AgingCounter[i]++;
            if (AgingCounter[i] >= DEM_AGING_THRESHOLD) {
                EventStatus[i] = 0; 
                AgingCounter[i] = 0;
            }
        }
        /* Healing: 3 consecutive PASSED cycles (DEM_HEALING_THRESHOLD) clears WIR  */
        if (PassedCycleCount[i] >= DEM_HEALING_THRESHOLD) {
            EventStatus[i] &= ~DEM_UDS_WIR; 
        }
    }
}