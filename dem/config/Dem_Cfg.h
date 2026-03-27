/*-------------------------------- Arctic Core ------------------------------
 * Copyright (C) 2013, ArcCore AB, Sweden, www.arccore.com.
 *-------------------------------- Arctic Core -----------------------------*/
#ifndef DEM_CFG_H_
#define DEM_CFG_H_

/* DEM General */
#define DEM_VERSION_INFO_API                    STD_ON
#define DEM_DEV_ERROR_DETECT                    STD_ON
#define DEM_OBD_SUPPORT                         STD_OFF
#define DEM_PTO_SUPPORT                         STD_OFF
#define DEM_TYPE_OF_DTC_SUPPORTED               0x01
#define DEM_DTC_STATUS_AVAILABILITY_MASK        0xFF
#define DEM_CLEAR_ALL_EVENTS                    STD_OFF

#define DEM_BSW_ERROR_BUFFER_SIZE               20
#define DEM_FF_DID_LENGTH                       2       /* 2 bytes per DID identifier in freeze frame */
#define DEM_DID_IDENTIFIER_SIZE_OF_BYTES        2       /* bytes used to store DID id in freeze frame record */
#define DEM_MAX_NR_OF_DIDS_IN_FREEZEFRAME_DATA  5       /* max DIDs per freeze frame */
#define DEM_NO_DTC                              0x00000000UL  /* sentinel: no DTC assigned */

#define DEM_MAX_NUMBER_EVENT_ENTRY_MIR          0
#define DEM_MAX_NUMBER_EVENT_ENTRY_PER          0
#define DEM_MAX_NUMBER_EVENT_ENTRY_PRI          10
#define DEM_MAX_NUMBER_EVENT_ENTRY_SEC          0
#define DEM_MAX_NUMBER_PRESTORED_FF             0

/* Size limitations */
#define DEM_MAX_NR_OF_RECORDS_IN_EXTENDED_DATA  10
#define DEM_MAX_NR_OF_EVENT_DESTINATION         4
#define DEM_MAX_SIZE_FF_DATA                    10
#define DEM_MAX_SIZE_EXT_DATA                   10
#define DEM_MAX_NUMBER_EVENT                    100
#define DEM_MAX_NUMBER_EVENT_PRE_INIT           20
#define DEM_MAX_NUMBER_FF_DATA_PRE_INIT         20
#define DEM_MAX_NUMBER_EXT_DATA_PRE_INIT        20
#define DEM_MAX_NUMBER_EVENT_PRI_MEM            (DEM_MAX_NUMBER_EVENT_ENTRY_PRI)
#define DEM_MAX_NUMBER_FF_DATA_PRI_MEM          5
#define DEM_MAX_NUMBER_EXT_DATA_PRI_MEM         5
#define DEM_MAX_RECORD_NUMBERS_IN_FF_REC_NUM_CLASS 0

typedef struct {
    uint32  UDSDTC;
    uint32  OBDDTC;
    boolean DTCUsed;
    boolean ImmediateNvStorage;
} Arc_Dem_DTC;

typedef struct {
    boolean JumpUp;
    boolean JumpDown;
    uint16  IncrementStepSize;
    uint16  DecrementStepSize;
    sint16  JumpDownValue;
    sint16  JumpUpValue;
    sint16  FailedThreshold;
    sint16  PassedThreshold;
} Dem_PreDebounceCounterBasedType;

#define E_NO_DTC_AVAILABLE          0x02U
#define DEM_HIGHEST_EXT_DATA_REC_NUM 0xFEU
#define DEM_HIGHEST_FF_REC_NUM      0xFEU

#define DEM_FREEZEFRAME_DEFAULT_VALUE   0xFF

#endif /* DEM_CFG_H_ */

/* Extended data memory configuration */
#define DEM_EXT_DATA_IN_PRE_INIT        0
#define DEM_EXT_DATA_IN_PRI_MEM         0
#define DEM_EXT_DATA_IN_SEC_MEM         0
#define DEM_USE_MEMORY_FUNCTIONS        0

/* Total event entry count */
#define DEM_MAX_NUMBER_EVENT_ENTRY  (DEM_MAX_NUMBER_EVENT_ENTRY_PRI + \
                                     DEM_MAX_NUMBER_EVENT_ENTRY_MIR + \
                                     DEM_MAX_NUMBER_EVENT_ENTRY_SEC + \
                                     DEM_MAX_NUMBER_EVENT_ENTRY_PER)
