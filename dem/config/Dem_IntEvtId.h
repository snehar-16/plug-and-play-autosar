#ifndef DEMINTEVTID_H_
#define DEMINTEVTID_H_

/*
 * Dem_IntEvtId.h
 * Kavach DMI — SW-C event IDs
 * These continue from where Dem_IntErrId.h left off
 */

enum {
    /* Category 1 — Display */
    RAIL_EVT_DISPLAY_FAIL        = DEM_EVENT_ID_LAST_FOR_BSW,
    RAIL_EVT_DISPLAY_PARTIAL,

    /* Category 2 — Communication */
    RAIL_EVT_COMM_LOSS_LTCAS,
    RAIL_EVT_COMM_TIMEOUT,

    /* Category 3 — Data validity */
    RAIL_EVT_LTCAS_DATA_INVALID,
    RAIL_EVT_LTCAS_DATA_STALE,

    /* Category 4 — Speed */
    RAIL_EVT_SPEED_SENSOR_FAIL,
    RAIL_EVT_SPEED_DISPLAY_ERROR,

    /* Category 5 — Movement Authority */
    RAIL_EVT_MA_INVALID,
    RAIL_EVT_MA_LOSS,

    /* Category 6 — Mode transition */
    RAIL_EVT_MODE_TRANSITION_FAIL,
    RAIL_EVT_MODE_UNKNOWN,

    /* Category 7 — System */
    RAIL_EVT_POWER_SUPPLY_FAULT,
    RAIL_EVT_WATCHDOG_TIMEOUT,

    DEM_EVENT_ID_SWC_END
};

#endif /* DEMINTEVTID_H_ */
