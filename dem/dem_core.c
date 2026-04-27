/**
 * @file dem_core.c
 * @brief Integrated DEM Core with Dual-Debounce Engine and 8-bit UDS Status
 * Compliance: SIL-4 / BHEL Kavach SMOP_SWRS v1.1, MISRA C:2012
 */

#include "dem_cfg.h"
#include "../dcm/Dcm_Cfg.h"
#include "../platform/platform_api.h"
#include "dem_event_logger.h"
#include <string.h>

/* Functional Data Structures  */
static uint8_t  EventStatus[TOTAL_DTC_SLOTS];
static uint16_t AgingCounter[TOTAL_DTC_SLOTS];
static uint8_t  PassedCycleCount[TOTAL_DTC_SLOTS];
/* MISRA: Timers must be unsigned */
static uint16_t CounterDebounceTimer[TOTAL_DTC_SLOTS];
static uint16_t TimeDebounceTimer[TOTAL_DTC_SLOTS];

/* CRC Fault Identifier (Used for instant bypass) */
#define DID_CRC_VITAL      DEM_EVT_CRC_ERROR_VITAL 

/**
 * @brief Required by main.c to ensure RAM is clean on boot
 */
void Dem_Init(void) {
    /* MISRA: Explicit cast to void for unused return values */
    (void)memset(EventStatus, 0, sizeof(EventStatus));
    (void)memset(AgingCounter, 0, sizeof(AgingCounter));
    (void)memset(PassedCycleCount, 0, sizeof(PassedCycleCount));
    (void)memset(CounterDebounceTimer, 0, sizeof(CounterDebounceTimer));
    (void)memset(TimeDebounceTimer, 0, sizeof(TimeDebounceTimer));
    
    /* [Hardware UART Log]: DEM RAM Structures Initialized. */
}

/**
 * @brief Set 8-bit UDS Status and send to Black Box Logger 
 */
void Dem_SetEventStatus(uint16_t did, uint8_t isFailed) {
    uint8_t index = (uint8_t)(did % TOTAL_DTC_SLOTS);
    uint8_t isFault;
    
    /* Fault range: 0xF100 to 0xF1FF (System Faults) */
    if ((did >= 0xF100U) && (did <= 0xF1FFU)) {
        isFault = 1U;
    } else {
        isFault = 0U;
    }

    /* MISRA: Explicit boolean evaluation */
    if (isFailed != 0U) {
        /* Set UDS Bits: TF, TFTOC, PDTC, CDTC, TFSLC  */
        EventStatus[index] |= (DEM_UDS_TF | DEM_UDS_TFTOC | DEM_UDS_PDTC | DEM_UDS_CDTC | DEM_UDS_TFSLC);
        /* MISRA: Explicit downcast required after bitwise NOT */
        EventStatus[index] &= (uint8_t)~(DEM_UDS_TNCSLC | DEM_UDS_TNCTOC);
        
        if (isFault != 0U) {
            EventStatus[index] |= DEM_UDS_WIR; /* Warning Indicator Requested (Buzzer) */
            AgingCounter[index] = 0U;
            PassedCycleCount[index] = 0U;
        }
        
        /* Log Critical Fault to Circular Event Logger  */
        DEM_EventLogger_Write(did, 3U /*CRIT*/, 0U, 0U /*FAIL*/, did, EventStatus[index]);
    } else {
        /* Clear Test Failed bit */
        EventStatus[index] &= (uint8_t)~DEM_UDS_TF;
        PassedCycleCount[index]++;
        
        /* Log Recovery to Circular Event Logger  */
        DEM_EventLogger_Write(did, 0U /*LOW*/, 0U, 1U /*PASS*/, did, EventStatus[index]);
    }
}

/**
 * @brief Enhanced Dual-Debounce Engine 
 * Implements 50ms Counter-based and 100ms Time-based persistence.
 */
void Dem_ReportError(uint16_t did, uint8_t rawSignal) {
    uint8_t index = (uint8_t)(did % TOTAL_DTC_SLOTS);
    
    /* 1. Explicit CRC Fault Bypass: Log within <10ms per safety requirement  */
    if ((did == DID_CRC_VITAL) && (rawSignal == 1U)) {
        Dem_SetEventStatus(did, 1U);
    } else {
        /* 2. Counter-Based Debounce (Uses DEM_DEBOUNCE_FAIL_LIMIT from dem_cfg.h) */
        if (rawSignal != 0U) {
            if (CounterDebounceTimer[index] < DEM_DEBOUNCE_FAIL_LIMIT) {
                CounterDebounceTimer[index]++;
            }
            if (CounterDebounceTimer[index] >= DEM_DEBOUNCE_FAIL_LIMIT) { 
                /* 3. Time-Based Debounce (Uses DEBOUNCE_TIME_LIMIT from dem_cfg.h) */
                if (TimeDebounceTimer[index] < DEBOUNCE_TIME_LIMIT) {
                    TimeDebounceTimer[index]++;
                } else {
                    Dem_SetEventStatus(did, 1U);
                }
            }
        } else {
            /* Reset timers on recovery to prevent transient noise spikes  */
            CounterDebounceTimer[index] = 0U;
            TimeDebounceTimer[index] = 0U;
            Dem_SetEventStatus(did, 0U);
        }
    }
}

/**
 * @brief Main Loop (Called every 10ms from FreeRTOS task)
 */
void Dem_MainFunction(void) {
    Platform_WdgTrigger(); /* SIL-4 Watchdog Feed */

    /* MISRA: Loop counter matches array indexing types (unsigned) */
    for (uint8_t i = 0U; i < TOTAL_DTC_SLOTS; i++) {
        /* Aging: Auto-clear after 40 operation cycles (DEM_AGING_THRESHOLD) */
        if (((EventStatus[i] & DEM_UDS_TF) == 0U) && ((EventStatus[i] & DEM_UDS_PDTC) != 0U)) {
            AgingCounter[i]++;
            if (AgingCounter[i] >= DEM_AGING_THRESHOLD) {
                EventStatus[i] = 0U; 
                AgingCounter[i] = 0U;
            }
        }
        /* Healing: 3 consecutive PASSED cycles (DEM_HEALING_THRESHOLD) clears WIR  */
        if (PassedCycleCount[i] >= DEM_HEALING_THRESHOLD) {
            EventStatus[i] &= (uint8_t)~DEM_UDS_WIR; 
        }
    }
}