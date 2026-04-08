/* ============================================================================
 * File: dem_cfg.h
 * Description: Configuration header for the Diagnostic Event Manager.
 * Ref: SM-OCIP Final Integration Plan | Contract SBD052 (April 2026)
 * ============================================================================ */

#ifndef DEM_CFG_H
#define DEM_CFG_H

#include <stdint.h>

/* --------------------------------------------------------------------------
 * UDS STATUS BYTE FLAGS (ISO 14229-1) 
 * Mandatory 8-bit implementation for BHEL SIL-4 compliance.
 * -------------------------------------------------------------------------- */
#define DEM_UDS_TF      0x01 /* Bit 0: Test Failed */
#define DEM_UDS_TFTOC   0x02 /* Bit 1: Test Failed This Operation Cycle */
#define DEM_UDS_PDTC    0x04 /* Bit 2: Pending DTC */
#define DEM_UDS_CDTC    0x08 /* Bit 3: Confirmed DTC */
#define DEM_UDS_TNCSLC  0x10 /* Bit 4: Test Not Completed Since Last Clear */
#define DEM_UDS_TFSLC   0x20 /* Bit 5: Test Failed Since Last Clear */
#define DEM_UDS_TNCTOC  0x40 /* Bit 6: Test Not Completed This Operation Cycle */
#define DEM_UDS_WIR     0x80 /* Bit 7: Warning Indicator Requested */

/* --------------------------------------------------------------------------
 * DEM THRESHOLDS & DEBOUNCING 
 * -------------------------------------------------------------------------- */
#define TOTAL_DTC_SLOTS           20 /*  */
#define DEM_AGING_THRESHOLD       40 /* Operation cycles to auto-clear  */
#define DEM_HEALING_THRESHOLD     3  /* Passed cycles to clear TNCSLC and WIR [cite: 15, 44] */

/* Railway Safety Persistence: 100ms threshold  */
#define DEM_TIME_DEBOUNCE_MS      100 
#define DEM_TASK_CYCLE_MS         10
#define DEBOUNCE_TIME_LIMIT       (DEM_TIME_DEBOUNCE_MS / DEM_TASK_CYCLE_MS) /* 10 Ticks */

/* Counter-based Debounce: 50ms window  */
#define DEM_DEBOUNCE_FAIL_LIMIT   5 /* 5 consecutive 10ms tasks */

/* --------------------------------------------------------------------------
 * EEPROM PARTITION MAP [cite: 19, 32]
 * Hard boundaries aligned with Doc A and Doc B merged page map.
 * -------------------------------------------------------------------------- */
#define DEM_RESERVED_PAGE         0    /* Page 0: Do not use [cite: 19, 32] */
#define DEM_HW_FAULT_TABLE_START  1    /* Pages 1-5: Fault table (SM-OCIP HW Team) [cite: 19, 32] */
#define DEM_HW_LOG_START          6    /* Pages 6-364: HW team circular log [cite: 19, 32] */

#define DEM_START_PAGE            365  /* Start of Event Logger (9 KB) [cite: 18, 32] */
#define DEM_END_PAGE              511  /* End of Event Logger [cite: 18, 32] */

#define DEM_S1_START_PAGE         2043 /* Session 1 Fault DID Snapshots (128 B) [cite: 17, 32] */
#define DEM_S2_START_PAGE         2045 /* Session 2 Audit DID Records (192 B) [cite: 17, 32] */

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