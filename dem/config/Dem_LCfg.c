#include "Dem.h"

/* Kavach DMI — DEM Link-time Configuration */

/* Event class for all Kavach events — minimal config */
static const Dem_EventClassType Kavach_EventClass = {
    .EventDestination    = DEM_DTC_ORIGIN_PRIMARY_MEMORY,
    .EventPriority       = 1U,
    .FFPrestorageSupported = FALSE,
    .AgingAllowed        = TRUE,
    .OperationCycleRef   = DEM_OPERATION_CYCLE_ID_POWER,
    .AgingCycleRef       = DEM_OPERATION_CYCLE_ID_POWER,
    .ConsiderPtoStatus   = FALSE,
};

/* Event parameter table — one entry per Kavach DMI event */
const Dem_EventParameterType EventParameter[] = {
    /* RAIL_EVT_DISPLAY_FAIL */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_DISPLAY_FAIL,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_DISPLAY_PARTIAL */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_DISPLAY_PARTIAL,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_COMM_LOSS_LTCAS */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_COMM_LOSS_LTCAS,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_COMM_TIMEOUT */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_COMM_TIMEOUT,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_LTCAS_DATA_INVALID */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_LTCAS_DATA_INVALID,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_LTCAS_DATA_STALE */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_LTCAS_DATA_STALE,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_SPEED_SENSOR_FAIL */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_SPEED_SENSOR_FAIL,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_SPEED_DISPLAY_ERROR */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_SPEED_DISPLAY_ERROR,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_MA_INVALID */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_MA_INVALID,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_MA_LOSS */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_MA_LOSS,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_MODE_TRANSITION_FAIL */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_MODE_TRANSITION_FAIL,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_MODE_UNKNOWN */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_MODE_UNKNOWN,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_POWER_SUPPLY_FAULT */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_POWER_SUPPLY_FAULT,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* RAIL_EVT_WATCHDOG_TIMEOUT */
    { .EventClass = &Kavach_EventClass, .EventID = RAIL_EVT_WATCHDOG_TIMEOUT,
      .EventKind = DEM_EVENT_KIND_SWC, .Arc_EOL = FALSE },

    /* End of table marker — must be last */
    { .Arc_EOL = TRUE }
};

const Dem_ConfigSetType DEM_ConfigSet = {
    .EventParameter = EventParameter,
};


