#ifndef RTE_DEM_TYPE_H
#define RTE_DEM_TYPE_H

/*
 * Rte_Dem_Type.h
 * RTE type stub — minimal, no conflicts with Dem_Types.h
 */

#include "Platform_Types.h"

typedef uint8_t  Dem_EventStatusType;
#define DEM_EVENT_STATUS_PASSED     0x00U
#define DEM_EVENT_STATUS_FAILED     0x01U
#define DEM_EVENT_STATUS_PREPASSED  0x02U
#define DEM_EVENT_STATUS_PREFAILED  0x03U

typedef uint8_t  Dem_InitMonitorReasonType;
#define DEM_INIT_MONITOR_CLEAR      0x01U
#define DEM_INIT_MONITOR_RESTART    0x02U

typedef uint8_t  Dem_OperationCycleStateType;
#define DEM_CYCLE_STATE_START       0x01U
#define DEM_CYCLE_STATE_END         0x00U

typedef uint8_t  Dem_DTCFormatType;
#define DEM_DTC_FORMAT_OBD          0x00U
#define DEM_DTC_FORMAT_UDS          0x01U
#define DEM_DTC_FORMAT_J1939        0x02U

typedef uint8_t  Dem_DTCOriginType;
#define DEM_DTC_ORIGIN_PRIMARY_MEMORY   0x01U
#define DEM_DTC_ORIGIN_MIRROR_MEMORY    0x02U
#define DEM_DTC_ORIGIN_PERMANENT_MEMORY 0x03U
#define DEM_DTC_ORIGIN_SECONDARY_MEMORY 0x04U

typedef uint8_t  Dem_DTCKindType;
#define DEM_DTC_KIND_ALL_DTCS           0x01U
#define DEM_DTC_KIND_EMISSION_REL_DTCS  0x02U

typedef uint8_t  Dem_FilterWithSeverityType;
typedef uint8_t  Dem_FilterForFDCType;
typedef uint8_t  Dem_DTCSeverityType;
typedef uint8_t  Dem_DTCStatusMaskType;
typedef uint8_t  Dem_IndicatorStatusType;
#define DEM_INDICATOR_OFF               0x00U
#define DEM_INDICATOR_CONTINUOUS        0x01U
#define DEM_INDICATOR_BLINKING          0x02U

typedef uint8_t  Dem_DebounceResetStatusType;
#define DEM_DEBOUNCE_STATUS_FREEZE      0x00U
#define DEM_DEBOUNCE_STATUS_RESET       0x01U

typedef uint8_t  Dem_ReturnSetFilterType;
typedef uint8_t  Dem_ReturnGetStatusOfDTCType;
typedef uint8_t  Dem_ReturnGetNextFilteredDTCType;
typedef uint8_t  Dem_ReturnGetNumberOfFilteredDTCType;
typedef uint8_t  Dem_ReturnControlDTCStorageType;
typedef uint8_t  Dem_ReturnControlEventUpdateType;
typedef uint8_t  Dem_ReturnGetExtendedDataRecordByDTCType;
typedef uint8_t  Dem_ReturnGetDTCByOccurenceTimeType;
typedef uint8_t  Dem_ReturnGetFreezeFrameDataByDTCType;
typedef uint8_t  Dem_ReturnGetSizeOfExtendedDataRecordByDTCType;
typedef uint8_t  Dem_ReturnGetSizeOfFreezeFrameType;
typedef uint8_t  Dem_ReturnGetSeverityOfDTCType;
typedef uint8_t  Dem_ReturnDisableDTCRecordUpdateType;
typedef uint8_t  Dem_ReturnGetSizeOfFreezeFrameByDTCType;
typedef uint8_t  Dem_MonitorStatusType;
typedef uint8_t  Dem_EventOBDReadinessGroup;
typedef uint8_t  Dem_IUMPRGroup;
typedef uint8_t  Dem_RatioKindType;
typedef uint8_t  Dem_IumprDenomCondIdType;
typedef uint8_t  Dem_IumprDenomCondStatusType;
typedef uint8_t  Dem_EventStatusExtendedType;



#endif /* RTE_DEM_TYPE_H */
