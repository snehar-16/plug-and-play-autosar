/* ============================================================================
 * File: dem_cfg.h
 * Description: Configuration header for the Diagnostic Event Manager.
 * Ref: SM-OCIP Final Integration Plan | Contract SBD052 (April 2026)
 * Compliance: MISRA C:2012
 * ============================================================================ */

#ifndef DEM_CFG_H
#define DEM_CFG_H

#include <stdint.h>

/* --------------------------------------------------------------------------
 * UDS STATUS BYTE FLAGS (ISO 14229-1) 
 * Mandatory 8-bit implementation for BHEL SIL-4 compliance.
 * -------------------------------------------------------------------------- */
#define DEM_UDS_TF      0x01U /* Bit 0: Test Failed */
#define DEM_UDS_TFTOC   0x02U /* Bit 1: Test Failed This Operation Cycle */
#define DEM_UDS_PDTC    0x04U /* Bit 2: Pending DTC */
#define DEM_UDS_CDTC    0x08U /* Bit 3: Confirmed DTC */
#define DEM_UDS_TNCSLC  0x10U /* Bit 4: Test Not Completed Since Last Clear */
#define DEM_UDS_TFSLC   0x20U /* Bit 5: Test Failed Since Last Clear */
#define DEM_UDS_TNCTOC  0x40U /* Bit 6: Test Not Completed This Operation Cycle */
#define DEM_UDS_WIR     0x80U /* Bit 7: Warning Indicator Requested */

/* --------------------------------------------------------------------------
 * DEM THRESHOLDS & DEBOUNCING 
 * -------------------------------------------------------------------------- */
#define TOTAL_DTC_SLOTS           20U /* Total allowable concurrent DTCs */
#define DEM_AGING_THRESHOLD       40U /* Operation cycles to auto-clear */
#define DEM_HEALING_THRESHOLD     3U  /* Passed cycles to clear TNCSLC and WIR */

/* Railway Safety Persistence: 100ms threshold */
#define DEM_TIME_DEBOUNCE_MS      100U 
#define DEM_TASK_CYCLE_MS         10U
#define DEBOUNCE_TIME_LIMIT       (DEM_TIME_DEBOUNCE_MS / DEM_TASK_CYCLE_MS) /* 10 Ticks */

/* Counter-based Debounce: 50ms window */
#define DEM_DEBOUNCE_FAIL_LIMIT   5U /* 5 consecutive 10ms tasks */

/* --------------------------------------------------------------------------
 * EEPROM PARTITION MAP
 * Hard boundaries aligned with Doc A and Doc B merged page map.
 * -------------------------------------------------------------------------- */
#define DEM_RESERVED_PAGE         0U    /* Page 0: Do not use */
#define DEM_HW_FAULT_TABLE_START  1U    /* Pages 1-5: Fault table (SM-OCIP HW Team) */
#define DEM_HW_LOG_START          6U    /* Pages 6-364: HW team circular log */

#define DEM_START_PAGE            365U  /* Start of Event Logger (9 KB) */
#define DEM_END_PAGE              511U  /* End of Event Logger */

#define DEM_S1_START_PAGE         2043U /* Session 1 Fault DID Snapshots (128 B) */
#define DEM_S2_START_PAGE         2045U /* Session 2 Audit DID Records (192 B) */

/* --------------------------------------------------------------------------
 * DATA STRUCTURES
 * -------------------------------------------------------------------------- */
typedef struct {
    uint16_t eventId;
    uint8_t  statusByte;
    uint8_t  failCounter;
    uint8_t  agingCounter;
    uint8_t  healingCounter;
    uint8_t  timeDebounceTimer;
} Dem_DtcSlot_t;

#endif /* DEM_CFG_H */