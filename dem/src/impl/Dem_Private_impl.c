
#include <string.h>
#include "Dem.h"

#if defined(USE_NVM)
#include "NvM.h" /** @req DEM176.NvM */
#endif

#include "SchM_Dem.h"
#include "MemMap.h"
#include "Cpu.h"
#include "Dem_Types.h"
#include "Dem_Lcfg.h"
#include "Dem_Internal.h"
#if defined(USE_DEM_EXTENSION)
#include "Dem_Extension.h"
#endif
#define USE_DEBUG_PRINTF
#include "debug.h"

#if defined(USE_RTE)
/*lint -e18 duplicate declarations hidden behinde ifdef  */
#include "Rte_Dem.h"
#endif

#if (DEM_TRIGGER_DLT_REPORTS == STD_ON)
#include "Dlt.h"
#endif
#if defined(USE_FIM)
#include "FiM.h"
#endif
#include "Dem_NvM.h"
/*
 * Local defines
 */
#define DEM_EXT_DATA_IN_PRE_INIT (DEM_MAX_NUMBER_EXT_DATA_PRE_INIT > 0)
#define DEM_EXT_DATA_IN_PRI_MEM ((DEM_MAX_NUMBER_EXT_DATA_PRI_MEM > 0) && ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)))
#define DEM_EXT_DATA_IN_SEC_MEM ((DEM_MAX_NUMBER_EXT_DATA_SEC_MEM > 0) && (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON))
#define DEM_FF_DATA_IN_PRE_INIT (DEM_MAX_NUMBER_FF_DATA_PRE_INIT > 0)
#define DEM_FF_DATA_IN_PRI_MEM ((DEM_MAX_NUMBER_FF_DATA_PRI_MEM > 0) && ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)))
#define DEM_FF_DATA_IN_SEC_MEM ((DEM_MAX_NUMBER_FF_DATA_SEC_MEM > 0) && (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON))
#define DEM_DEFAULT_EVENT_STATUS (DEM_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR | DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE)
#define DEM_PRESTORAGE_FF_DATA_IN_MEM (DEM_MAX_NUMBER_PRESTORED_FF > 0)

#define DEM_PID_IDENTIFIER_SIZE_OF_BYTES        1 // OBD
#define DEM_FAILURE_CNTR_MAX 255
#define DEM_AGING_CNTR_MAX 255
#define DEM_INDICATOR_CNTR_MAX 255
#define DEM_OCCURENCE_COUNTER_MAX 0xFFFF

#define MOST_RECENT_FF_RECORD 0xFF

#define FREE_PERMANENT_DTC_ENTRY 0u

#define ALL_EXTENDED_DATA_RECORDS 0xFF
#define IS_VALID_EXT_DATA_RECORD(_x)    ((0x01 <= (_x)) && (0xEF >= (_x)))

#define IS_VALID_EVENT_ID(_x)   (((_x) > 0) && ((_x) <= DEM_EVENT_ID_LAST_VALID_ID))

#define IS_VALID_INDICATOR_ID(_x)   ((_x) < DEM_NOF_INDICATORS)

#define IS_SUPPORTED_ORIGIN(_x) ((DEM_DTC_ORIGIN_PRIMARY_MEMORY == (_x)) || (DEM_DTC_ORIGIN_SECONDARY_MEMORY == (_x)))

#if (DEM_STORE_UDS_STATUS_BIT_SUBSET_FOR_ALL_EVENTS == STD_ON)
#define NUM_STORED_BITS 2u
#define NOF_EVENTS_PER_BYTE (8u/NUM_STORED_BITS)
#define GET_UDSBIT_BYTE_INDEX(_eventId) (((_eventId)-1u)/NOF_EVENTS_PER_BYTE)
#define GET_UDS_STARTBIT(_eventId) ((((_eventId)-1u)%NOF_EVENTS_PER_BYTE)*2u)
#define UDS_BITMASK 3u
#define UDS_TFSLC_BIT 0u
#define UDS_TNCSLC_BIT 1u
#define UDS_STATUS_BIT_MAGIC UDS_BITMASK
#define UDS_STATUS_BIT_MAGIC_INDEX ((DEM_MAX_NUMBER_EVENT + (NOF_EVENTS_PER_BYTE - 1u))/NOF_EVENTS_PER_BYTE)
#endif

#define DEM_REC_NUM_AND_NUM_DIDS_SIZE 2u

#define VALIDATE_RV(_exp,_api,_err,_rv ) \
        if( !(_exp) ) { \
          DET_REPORTERROR(DEM_MODULE_ID, 0, _api, _err); \
          return _rv; \
        }

#define VALIDATE_NO_RV(_exp,_api,_err ) \
  if( !(_exp) ) { \
          DET_REPORTERROR(DEM_MODULE_ID, 0, _api, _err); \
          return; \
        }


#if (DEM_PTO_SUPPORT == STD_ON)
#error "DEM_PTO_SUPPORT is set to STD_ON, this is not supported by the code."
#endif

#if !((DEM_TYPE_OF_DTC_SUPPORTED == DEM_DTC_TRANSLATION_ISO15031_6) || (DEM_TYPE_OF_DTC_SUPPORTED == DEM_DTC_TRANSLATION_ISO14229_1))
#error "DEM_TYPE_OF_DTC_SUPPORTED is not set to ISO15031-6 or ISO14229-1. Only these are supported by the code."
#endif

#if !defined(USE_DEM_EXTENSION)
#if defined(DEM_FREEZE_FRAME_CAPTURE_EXTENSION)
#error "DEM_FREEZE_FRAME_CAPTURE cannot be DEM_TRIGGER_EXTENSION since Dem extension is not used!"
#endif
#if defined(DEM_EXTENDED_DATA_CAPTURE_EXTENSION)
#error "DEM_EXTENDED_DATA_CAPTURE cannot be DEM_TRIGGER_EXTENSION since Dem extension is not used!"
#endif
#if defined(DEM_DISPLACEMENT_PROCESSING_DEM_EXTENSION)
#error "DEM_DISPLACEMENT_PROCESSING_DEM_EXTENSION cannot be used since Dem extension is not used!"
#endif
#if defined(DEM_FAILURE_PROCESSING_DEM_EXTENSION)
#error "DEM_FAILURE_PROCESSING_DEM_EXTENSION cannot be used since Dem extension is not used!"
#endif
#if defined(DEM_AGING_PROCESSING_DEM_EXTENSION)
#error "DEM_AGING_PROCESSING_DEM_EXTENSION cannot be used since Dem extension is not used!"
#endif
#endif
#if defined(DEM_EXTENDED_DATA_CAPTURE_EVENT_MEMORY_STORAGE)
#error "DEM_EXTENDED_DATA_CAPTURE_EVENT_MEMORY_STORAGE is not supported!"
#endif

#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) || (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) || (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
#define DEM_USE_MEMORY_FUNCTIONS
#endif
#if defined(USE_NVM) && (DEM_USE_NVM == STD_ON)
#define DEM_ASSERT(_exp)        switch (1) {case 0: break; case (_exp): break; }
#endif

#if (DEM_TEST_FAILED_STORAGE == STD_ON)
#define GET_STORED_STATUS_BITS(_x) ((_x) & (DEM_TEST_FAILED_SINCE_LAST_CLEAR | DEM_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR | DEM_PENDING_DTC | DEM_CONFIRMED_DTC | DEM_TEST_FAILED))
#else
#define GET_STORED_STATUS_BITS(_x) ((_x) & (DEM_TEST_FAILED_SINCE_LAST_CLEAR | DEM_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR | DEM_PENDING_DTC | DEM_CONFIRMED_DTC))

#endif

#define IS_VALID_EVENT_STATUS(_x) ((DEM_EVENT_STATUS_PREPASSED == _x) || (DEM_EVENT_STATUS_PASSED == _x) || (DEM_EVENT_STATUS_PREFAILED == _x) || (DEM_EVENT_STATUS_FAILED == _x))

#if (DEM_NOF_EVENT_INDICATORS > 0)
#define DEM_USE_INDICATORS
#endif

#define TO_OBD_FORMAT(_x) ((_x)<<8u)
#define IS_VALID_DTC_FORMAT(_x) ((DEM_DTC_FORMAT_UDS == (_x)) || (DEM_DTC_FORMAT_OBD == (_x)))

#if (DEM_IMMEDIATE_NV_STORAGE == STD_ON)
#define DEM_USE_IMMEDIATE_NV_STORAGE
#endif

#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
#define TO_COMBINED_EVENT_ID(_x) ((_x) | 0x8000u)
#define IS_COMBINED_EVENT_ID(_x) (0u != ((_x) & 0x8000u))
#define TO_COMBINED_EVENT_CFG_IDX(_x) ((_x) & (uint16)(~0x8000u))
#define IS_VALID_COMBINED_ID(_x) ((_x) < DEM_NOF_COMBINED_DTCS)
/* Clear all combined event aging counter when sub event failes */
#define DEM_CLEAR_COMBINED_AGING_COUNTERS_ON_FAIL
#if 0
/* Require all sub events tested and passed to process aging */
#define DEM_COMBINED_DTC_STATUS_AGING
#endif
#endif
/*
 * Local types
 */

// DtcFilterType
typedef struct {
    Dem_EventStatusExtendedType dtcStatusMask;
    Dem_DTCKindType             dtcKind;
    Dem_DTCOriginType           dtcOrigin;
    Dem_FilterWithSeverityType  filterWithSeverity;
    Dem_DTCSeverityType         dtcSeverityMask;
    Dem_FilterForFDCType        filterForFaultDetectionCounter;
    uint16                      DTCIndex;
    Dem_DTCFormatType           dtcFormat;
} DtcFilterType;

// FreezeFrameRecordFilterType
typedef struct {
    uint16                      ffIndex;
    Dem_DTCFormatType           dtcFormat;
} FreezeFrameRecordFilterType;

// DisableDtcStorageType
typedef struct {
    boolean                     settingDisabled;
    Dem_DTCGroupType            dtcGroup;
    Dem_DTCKindType             dtcKind;
} DisableDtcSettingType;


// State variable
typedef enum
{
  DEM_UNINITIALIZED = 0,
  DEM_PREINITIALIZED,
  DEM_INITIALIZED,
  DEM_SHUTDOWN
} Dem_StateType; /** @req DEM169 */

static Dem_StateType demState = DEM_UNINITIALIZED;
#if defined(USE_FIM)
static boolean DemFiMInit = FALSE;
#endif

// Help pointer to configuration set
static const Dem_ConfigSetType *configSet;

/*
 * Allocation of DTC filter parameters
 */
static DtcFilterType dtcFilter;

/*
 * Allocation of freeze frame record filter
 */
static FreezeFrameRecordFilterType ffRecordFilter;


/*
 * Allocation of Disable/Enable DTC setting parameters
 */
static DisableDtcSettingType disableDtcSetting;

/*
 * Allocation of operation cycle state list
 */
/* NOTE: Do not change this without also changing generation of measurement tags */
static Dem_OperationCycleStateType operationCycleStateList[DEM_OPERATION_CYCLE_ID_ENDMARK];

/*
 * Allocation of local event status buffer
 */
/* NOTE: Do not change this without also changing generation of measurement tags */
static EventStatusRecType   eventStatusBuffer[DEM_MAX_NUMBER_EVENT];

/*
 * Allocation of pre-init event memory (used between pre-init and init). Only one
 * memory regardless of event destination.
 */
#if ( DEM_FF_DATA_IN_PRE_INIT )
static FreezeFrameRecType   preInitFreezeFrameBuffer[DEM_MAX_NUMBER_FF_DATA_PRE_INIT];
#endif
#if ( DEM_EXT_DATA_IN_PRE_INIT )
static ExtDataRecType       preInitExtDataBuffer[DEM_MAX_NUMBER_EXT_DATA_PRE_INIT];
#endif

/*
 * Allocation of primary event memory ramlog (after init) in uninitialized memory
 */
#define ADMIN_MAGIC 0xBABE
/** @req DEM162 */
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
#define PRI_MEM_EVENT_BUFFER_ADMIN_INDEX DEM_MAX_NUMBER_EVENT_PRI_MEM
EventRecType                priMemEventBuffer[DEM_MAX_NUMBER_EVENT_PRI_MEM + 1];/* + 1 for admin data */
static boolean              priMemOverflow = FALSE;/* @req DEM397 */
#if ( DEM_FF_DATA_IN_PRI_MEM )
FreezeFrameRecType          priMemFreezeFrameBuffer[DEM_MAX_NUMBER_FF_DATA_PRI_MEM];
#endif
#if (DEM_EXT_DATA_IN_PRI_MEM)
ExtDataRecType              priMemExtDataBuffer[DEM_MAX_NUMBER_EXT_DATA_PRI_MEM];
#endif
#endif
/*
 * Allocation of secondary event memory ramlog (after init) in uninitialized memory
 */
/** @req DEM162 */
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
#define SEC_MEM_EVENT_BUFFER_ADMIN_INDEX DEM_MAX_NUMBER_EVENT_SEC_MEM
EventRecType                secMemEventBuffer[DEM_MAX_NUMBER_EVENT_SEC_MEM + 1];/* + 1 for admin data */
static boolean              secMemOverflow = FALSE;/* @req DEM397 */
#if (DEM_FF_DATA_IN_SEC_MEM)
FreezeFrameRecType          secMemFreezeFrameBuffer[DEM_MAX_NUMBER_FF_DATA_SEC_MEM];
#endif
#if (DEM_EXT_DATA_IN_SEC_MEM)
ExtDataRecType              secMemExtDataBuffer[DEM_MAX_NUMBER_EXT_DATA_SEC_MEM];
#endif
#endif

/*
 * Allocation of permanent event memory ramlog (after init) in uninitialized memory
 */
/** @req DEM162 */
#if (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
#define PERM_MEM_EVENT_BUFFER_ADMIN_INDEX DEM_MAX_NUMBER_EVENT_PERM_MEM
PermanentDTCType            permMemEventBuffer[DEM_MAX_NUMBER_EVENT_PERM_MEM]; /* Free entry == 0u */
#endif

/*
 * Allocation of event memory for pre storage of freeze frames
 */
/** @req DEM191 */
#if ( DEM_PRESTORAGE_FF_DATA_IN_MEM )
FreezeFrameRecType          memPreStoreFreezeFrameBuffer[DEM_MAX_NUMBER_PRESTORED_FF];
#endif

#if defined(DEM_USE_MEMORY_FUNCTIONS) && (DEM_STORE_UDS_STATUS_BIT_SUBSET_FOR_ALL_EVENTS == STD_ON)
/* Buffer for storing subset of UDS status bits for all events */
uint8 statusBitSubsetBuffer[DEM_MEM_STATUSBIT_BUFFER_SIZE];
#endif

#if (DEM_USE_TIMESTAMPS == STD_ON)
/* Timestamp for events */
static uint32 Event_TimeStamp = 0;

/* Timestamp for extended data */
static uint32 ExtData_TimeStamp = 0;

/*
*Allocation of freezeFrame storage timestamp,record the time order
*/
/**private variable for freezeframe */
static uint32 FF_TimeStamp = 0;
#endif

#if (DEM_DTC_SUPPRESSION_SUPPORT == STD_ON)
typedef struct {
    boolean SuppressedByDTC:1;/*lint !e46 *//*structure must remain the same,field type should be _Bool, unsigned int or signed int [MISRA 2004 6.4, 2012 6.1]*/
    boolean SuppressedByEvent:1;/*lint !e46 *//*structure must remain the same,field type should be _Bool, unsigned int or signed int [MISRA 2004 6.4, 2012 6.1]*/
}DemDTCSuppressionType;
static DemDTCSuppressionType DemDTCSuppressed[DEM_NOF_DTCS];
#endif

#if (DEM_ENABLE_CONDITION_SUPPORT == STD_ON)
static boolean DemEnableConditions[DEM_NUM_ENABLECONDITIONS];
#endif

#define NO_DTC_DISABLED 0xFFFFFFFFUL

typedef struct {
    uint32 DTC;
    Dem_DTCOriginType Origin;
}DtcRecordUpdateDisableType;

static DtcRecordUpdateDisableType DTCRecordDisabled;

#if defined(DEM_USE_INDICATORS)
/* @req DEM499 */
#define INDICATOR_FAILED_DURING_FAILURE_CYCLE   1u
#define INDICATOR_PASSED_DURING_FAILURE_CYCLE   (1u<<1u)
#define INDICATOR_FAILED_DURING_HEALING_CYCLE   (1u<<2u)
#define INDICATOR_PASSED_DURING_HEALING_CYCLE   (1u<<3u)

typedef struct {
    Dem_EventIdType EventID;
    uint16 InternalIndicatorId;
    uint8 FailureCounter;
    uint8 HealingCounter;
    uint8 OpCycleStatus;
}IndicatorStatusType;

typedef struct {
    Dem_EventIdType EventID;
    uint8 IndicatorId;
    uint8 FailureCounter;
    uint8 HealingCounter;
}IndicatorNvRecType;

/* Buffer for storing event indicators internally */
static IndicatorStatusType indicatorStatusBuffer[DEM_NOF_EVENT_INDICATORS];

#if defined(DEM_USE_MEMORY_FUNCTIONS)
/* Buffer for storing event indicator status in NvRam */
IndicatorNvRecType indicatorBuffer[DEM_NOF_EVENT_INDICATORS];
#endif
#endif

#if defined(DEM_USE_IUMPR)
// Each index corresponds to RatioID
Dem_RatiosNvm iumprBuffer;
static Dem_RatioStatusType iumprBufferLocal[DEM_IUMPR_REGISTERED_COUNT];

// IUMPR general denominator buffer
Dem_RatioGeneralDenominatorType generalDenominatorBuffer;

// IUMPT ignition cycle count buffer
uint16 ignitionCycleCountBuffer;

// IUMPR additional denominator conditions buffer
#define DEM_IUMPR_ADDITIONAL_DENOMINATORS_COUNT 4
static Dem_IumprDenomCondType iumprAddiDenomCondBuffer[DEM_IUMPR_ADDITIONAL_DENOMINATORS_COUNT];
#endif

/*
 * Local functions
 * */

#ifdef DEM_USE_MEMORY_FUNCTIONS
#if (DEM_STORE_UDS_STATUS_BIT_SUBSET_FOR_ALL_EVENTS == STD_ON)
static void SetDefaultUDSStatusBitSubset(void);
#endif

#if ( DEM_FF_DATA_IN_PRE_INIT || DEM_FF_DATA_IN_PRI_MEM )
static boolean storeOBDFreezeFrameDataMem(const Dem_EventParameterType *eventParam, const FreezeFrameRecType *freezeFrame,
                                          FreezeFrameRecType* freezeFrameBuffer, uint32 freezeFrameBufferSize,
                                          Dem_DTCOriginType origin);
#endif

#if ( DEM_FF_DATA_IN_PRE_INIT || DEM_FF_DATA_IN_PRI_MEM || DEM_FF_DATA_IN_SEC_MEM )
static boolean storeFreezeFrameDataMem(const Dem_EventParameterType *eventParam, const FreezeFrameRecType *freezeFrame,
                                    FreezeFrameRecType* freezeFrameBuffer, uint32 freezeFrameBufferSize,
                                    Dem_DTCOriginType origin);
#endif

static boolean deleteFreezeFrameDataMem(const Dem_EventParameterType *eventParam, Dem_DTCOriginType origin, boolean combinedDTC);
static boolean deleteExtendedDataMem(const Dem_EventParameterType *eventParam, Dem_DTCOriginType origin, boolean combinedDTC);
#endif

static Std_ReturnType getEventFailed(Dem_EventIdType eventId, boolean *eventFailed);

#if (DEM_USE_TIMESTAMPS == STD_ON) && defined(DEM_USE_MEMORY_FUNCTIONS)
#if (DEM_UNIT_TEST == STD_ON)
void rearrangeEventTimeStamp(uint32 *timeStamp);
#else
static void rearrangeEventTimeStamp(uint32 *timeStamp);
#endif
#endif

#if defined(DEM_USE_INDICATORS) && defined(DEM_USE_MEMORY_FUNCTIONS)
static void storeEventIndicators(const Dem_EventParameterType *eventParam);
#endif

static Std_ReturnType getEventStatus(Dem_EventIdType eventId, Dem_EventStatusExtendedType *eventStatusExtended);

static void getFFClassReference(const Dem_EventParameterType *eventParam, Dem_FreezeFrameClassType **ffClassTypeRef);
static Dem_FreezeFrameClassTypeRefIndex getFFIdx(const Dem_EventParameterType *eventParam);

#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL)
uint8 getEventAgingCntr(const Dem_EventParameterType *eventParameter);
#endif

#if (DEM_UNIT_TEST == STD_ON)
/*
 * Procedure:   zeroPriMemBuffers
 * Description: Fill the primary buffers with zeroes
 */
/*lint -efunc(714, demZeroPriMemBuffers) */
void demZeroPriMemBuffers(void)
{
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
    memset(priMemEventBuffer, 0, sizeof(priMemEventBuffer));
    priMemOverflow = FALSE;
#if (DEM_FF_DATA_IN_PRI_MEM)
    memset(priMemFreezeFrameBuffer, 0, sizeof(priMemFreezeFrameBuffer));
#endif
#if (DEM_EXT_DATA_IN_PRI_MEM)
    memset(priMemExtDataBuffer, 0, sizeof(priMemExtDataBuffer));
#endif
#if defined(DEM_USE_INDICATORS) && defined(DEM_USE_MEMORY_FUNCTIONS)
    memset(indicatorBuffer, 0, sizeof(indicatorBuffer));
#endif
#if (DEM_STORE_UDS_STATUS_BIT_SUBSET_FOR_ALL_EVENTS == STD_ON)
    memset(statusBitSubsetBuffer, 0, sizeof(statusBitSubsetBuffer));
#endif
#endif
}

void demZeroSecMemBuffers(void)
{
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
    memset(secMemEventBuffer, 0, sizeof(secMemEventBuffer));
    secMemOverflow = FALSE;
#if ( DEM_FF_DATA_IN_SEC_MEM )
    memset(secMemFreezeFrameBuffer, 0, sizeof(secMemFreezeFrameBuffer));
#endif
#if ( DEM_EXT_DATA_IN_SEC_MEM )
    memset(secMemExtDataBuffer, 0, sizeof(secMemExtDataBuffer));
#endif
#if (DEM_STORE_UDS_STATUS_BIT_SUBSET_FOR_ALL_EVENTS == STD_ON)
    memset(statusBitSubsetBuffer, 0, sizeof(statusBitSubsetBuffer));
#endif
#endif
}

void demZeroPermMemBuffers(void)
{
#if (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
    memset(permMemEventBuffer, FREE_PERMANENT_DTC_ENTRY, sizeof(permMemEventBuffer));
#endif
}

void demZeroPreStoreFFMemBuffer(void)
{
#if (DEM_PRESTORAGE_FF_DATA_IN_MEM)
	memset(memPreStoreFreezeFrameBuffer, 0x00, sizeof(memPreStoreFreezeFrameBuffer));
#endif
}
#if defined(DEM_USE_IUMPR)
void demZeroIumprBuffer(void)
{
	memset(&iumprBuffer, 0, sizeof(iumprBuffer));
}

void demSetIgnitionCounterToMax(void) {
	ignitionCycleCountBuffer = 65535;
}

void demSetDenominatorToMax(Dem_RatioIdType ratioId) {
	iumprBufferLocal[ratioId].denominator.value = 65535;
}

void demSetNumeratorToMax(Dem_RatioIdType ratioId) {
	iumprBufferLocal[ratioId].numerator.value = 65535;
}
#endif
#endif


#ifdef DEM_USE_MEMORY_FUNCTIONS
static boolean eventIsStoredInMem(Dem_EventIdType eventId, const EventRecType* eventBuffer, uint32 eventBufferSize)
{
    boolean eventIdFound = FALSE;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( IS_COMBINED_EVENT_ID(eventId) ) {
        /* Entry is for a combined event */
        const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(eventId)];
        const Dem_DTCClassType *DTCClass = CombDTCCfg->DTCClassRef;
        for (uint32 i = 0u;((i < eventBufferSize) && (eventIdFound == FALSE)); i++) {
            for(uint16 evIdx = 0; (evIdx < DTCClass->NofEvents) && (eventIdFound == FALSE); evIdx++) {
                eventIdFound = (eventBuffer[i].EventData.eventId == DTCClass->Events[evIdx])? TRUE: FALSE;
            }
        }
    }
    else {
        for (uint32 i = 0u;((i < eventBufferSize) && (eventIdFound == FALSE)); i++) {
            eventIdFound = (eventBuffer[i].EventData.eventId == eventId)? TRUE: FALSE;
        }
    }
#else
    for (uint32 i = 0u;((i < eventBufferSize) && (eventIdFound == FALSE)); i++) {
        eventIdFound = (eventBuffer[i].EventData.eventId == eventId)? TRUE: FALSE;
    }
#endif
    return eventIdFound;
}
#endif

/**
 * Determines if a DTC is available or not
 * @param DTCClass
 * @return TRUE: DTC available, FALSE: DTC NOT available
 */
static boolean DTCIsAvailable(const Dem_DTCClassType *DTCClass)
{
    if( (TRUE == DTCClass->DTCRef->DTCUsed)
#if (DEM_DTC_SUPPRESSION_SUPPORT == STD_ON)
            && (FALSE == DemDTCSuppressed[DTCClass->DTCIndex].SuppressedByDTC)
            && (FALSE == DemDTCSuppressed[DTCClass->DTCIndex].SuppressedByEvent)
#endif
    ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * Procedure:   checkDtcKind
 * Description: Return TRUE if "dtcKind" match the events DTCKind or "dtcKind"
 *              is "DEM_DTC_KIND_ALL_DTCS" otherwise FALSE.
 */
static boolean checkDtcKind(Dem_DTCKindType dtcKind, const Dem_EventParameterType *eventParam)
{
    boolean result = FALSE;
    if( (NULL != eventParam->DTCClassRef) && (DTCIsAvailable(eventParam->DTCClassRef) == TRUE) ) {
        result = ( (dtcKind == DEM_DTC_KIND_ALL_DTCS) || (DEM_NON_EMISSION_RELATED != (Dem_Arc_EventDTCKindType) *eventParam->EventDTCKind) )? TRUE: FALSE;
    }

    return result;
}

/**
 * Checks if event status matches current filter.
 * @param filterMask
 * @param eventStatus
 * @return
 */
static boolean checkDtcStatusMask(Dem_EventStatusExtendedType filterMask, Dem_EventStatusExtendedType eventStatus)
{
    boolean result = FALSE;

    if( (DEM_DTC_STATUS_MASK_ALL == filterMask) || (0 != (eventStatus & filterMask)) ) {
        result = TRUE;
    }

    return result;
}
/**
 * Checks if DTC is available on specific format
 * @param eventParam
 * @param dtcFormat
 * @return TRUE: DTC is available on specific format, FALSE: Event is not available on specific format.
 */
static boolean eventHasDTCOnFormat(const Dem_EventParameterType *eventParam, Dem_DTCFormatType dtcFormat)
{
    boolean ret = FALSE;
    if(( NULL != eventParam) && (NULL != eventParam->DTCClassRef) ) {
        ret = ( ((DEM_DTC_FORMAT_UDS == dtcFormat) && (DEM_NO_DTC != eventParam->DTCClassRef->DTCRef->UDSDTC)) ||
                ((DEM_DTC_FORMAT_OBD == dtcFormat) && (DEM_NO_DTC != eventParam->DTCClassRef->DTCRef->OBDDTC)))? TRUE: FALSE;
    }
    return ret;
}

/**
 * Checks if DTC is avalailable on specific format.
 * @param eventParam
 * @param dtcFormat
 * @return
 */
static boolean DTCISAvailableOnFormat(const Dem_DTCClassType *DTCClass, Dem_DTCFormatType dtcFormat)
{
    boolean ret = FALSE;
    if( NULL != DTCClass ) {
        ret = ( ((DEM_DTC_FORMAT_UDS == dtcFormat) && (DEM_NO_DTC != DTCClass->DTCRef->UDSDTC)) ||
                ((DEM_DTC_FORMAT_OBD == dtcFormat) && (DEM_NO_DTC != DTCClass->DTCRef->OBDDTC)))? TRUE: FALSE;
    }
    return ret;
}

/**
 * Checks if dtc is a DTC group
 * @param dtc
 * @param dtcFormat
 * @param groupLower
 * @param groupUpper
 * @return TRUE: dtc is group, FALSE: dtc is NOT a group
 */
static boolean dtcIsGroup(uint32 dtc, Dem_DTCFormatType dtcFormat, uint32 *groupLower, uint32 *groupUpper)
{
    const Dem_GroupOfDtcType * DTCGroups = configSet->GroupOfDtc;
    boolean groupFound = FALSE;
    if( DEM_DTC_FORMAT_UDS == dtcFormat ) {
        while( (FALSE == DTCGroups->Arc_EOL) && (FALSE == groupFound) ) {
            if( dtc == DTCGroups->DemGroupDTCs ) {
                *groupLower = DTCGroups->DemGroupDTCs;
                groupFound = TRUE;
            }
            DTCGroups++;
        }
        *groupUpper = DTCGroups->DemGroupDTCs - 1u;
    }

    return groupFound;
}

/*
 * Procedure:   checkDtcGroup
 * Description: Return TRUE if "dtc" match the events DTC or "dtc" is
 *              "DEM_DTC_GROUP_ALL_DTCS" otherwise FALSE.
 */
/**
 * Checks is event has DTC matching "dtc". "dtc" can be a group or a specific DTC
 * @param dtc
 * @param eventParam
 * @param dtcFormat
 * @param checkFormat
 * @return TRUE: event has DTC matching "dtc", FALSE: DTC does NOT have DTC matching "dtc"
 */
static boolean checkDtcGroup(uint32 dtc, const Dem_EventParameterType *eventParam, Dem_DTCFormatType dtcFormat)
{
    /* NOTE: dtcFormat determines the format of dtc */
    boolean result = FALSE;

    if( (NULL != eventParam->DTCClassRef) && (TRUE == DTCIsAvailable(eventParam->DTCClassRef)) ) {
        if( DEM_DTC_GROUP_ALL_DTCS == dtc ) {
            result = (DEM_DTC_FORMAT_UDS == dtcFormat) ? (DEM_NO_DTC != eventParam->DTCClassRef->DTCRef->UDSDTC) : (DEM_NO_DTC != eventParam->DTCClassRef->DTCRef->OBDDTC);
        }
        else if( DEM_DTC_GROUP_EMISSION_REL_DTCS == dtc ) {
            result = (DEM_DTC_FORMAT_UDS == dtcFormat) ? ((DEM_NO_DTC != eventParam->DTCClassRef->DTCRef->UDSDTC) && (DEM_NO_DTC != eventParam->DTCClassRef->DTCRef->OBDDTC)) : (DEM_NO_DTC != eventParam->DTCClassRef->DTCRef->OBDDTC);
        }
        else {
            /* Not "ALL DTCs" */
            if( TRUE == eventHasDTCOnFormat(eventParam, dtcFormat) ) {
                uint32 DTCGroupLower;
                uint32 DTCGroupUpper;
                if( TRUE == dtcIsGroup(dtc, dtcFormat, &DTCGroupLower, &DTCGroupUpper) ) {
                    if( DEM_DTC_FORMAT_UDS == dtcFormat ) {
                        result = ((eventParam->DTCClassRef->DTCRef->UDSDTC >= DTCGroupLower) && (eventParam->DTCClassRef->DTCRef->UDSDTC < DTCGroupUpper))? TRUE: FALSE;
                    }
                    else {
                        result = ((eventParam->DTCClassRef->DTCRef->OBDDTC >= DTCGroupLower) && (eventParam->DTCClassRef->DTCRef->OBDDTC < DTCGroupUpper))? TRUE: FALSE;
                    }
                }
                else {
                    result = (DEM_DTC_FORMAT_UDS == dtcFormat) ? (dtc == eventParam->DTCClassRef->DTCRef->UDSDTC) : (dtc == TO_OBD_FORMAT(eventParam->DTCClassRef->DTCRef->OBDDTC));
                }
            }
        }
    }

    return result;
}
#if (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
/**
 * Checks if an event is emission related and MIL activating
 * @param eventParam
 * @return TRUE: event is emission related, FALSE: event is not emission related
 */
static boolean eventIsEmissionRelatedMILActivating(const Dem_EventParameterType *eventParam)
{
    boolean ret = FALSE;
    if( DEM_EMISSION_RELATED_MIL_ACTIVATING == (Dem_Arc_EventDTCKindType) *eventParam->EventDTCKind ) {
        ret = TRUE;
    }
    return ret;
}
#endif

#if (DEM_OBD_DISPLACEMENT_SUPPORT == STD_ON) && (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON) && defined(DEM_DISPLACEMENT_PROCESSING_DEM_INTERNAL) && defined (DEM_USE_MEMORY_FUNCTIONS)
/**
 * Checks if an event is emission related
 * @param eventParam
 * @return TRUE: event is emission related, FALSE: event is not emission related
 */
static boolean eventIsEmissionRelated(const Dem_EventParameterType *eventParam)
{
    boolean ret = FALSE;
    if( (DEM_EMISSION_RELATED_MIL_ACTIVATING == (Dem_Arc_EventDTCKindType) *eventParam->EventDTCKind) || (DEM_EMISSION_RELATED == (Dem_Arc_EventDTCKindType) *eventParam->EventDTCKind) ){
        ret = TRUE;
    }
    return ret;
}
#endif
/*
 * Procedure:   checkDtcOrigin
 * Description: Return TRUE if "dtcOrigin" match any of the events DTCOrigin otherwise FALSE.
 */
static inline boolean checkDtcOrigin(Dem_DTCOriginType dtcOrigin, const Dem_EventParameterType *eventParam, boolean allowPermanentMemory)
{
    boolean originMatch = FALSE;
    if( (TRUE == allowPermanentMemory) && (DEM_DTC_ORIGIN_PERMANENT_MEMORY == dtcOrigin) ) {
#if (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
        /* Check if dtc is stored in permanent memory*/
        /* @req DEM301 */
        if( TRUE == eventIsEmissionRelatedMILActivating(eventParam) ) {
            for( uint32 i = 0; (i < DEM_MAX_NUMBER_EVENT_PERM_MEM) && (FALSE == originMatch); i++) {
                if( eventParam->DTCClassRef->DTCRef->OBDDTC == permMemEventBuffer[i].OBDDTC) {
                    /* DTC is stored */
                    originMatch = TRUE;
                }
            }
        }
#endif
    }
    else {
        originMatch = (eventParam->EventClass->EventDestination == dtcOrigin)? TRUE: FALSE;
    }
    return originMatch;
}

/**
 * Return TRUE if "dtcSeverityMask" match the DTC severity otherwise FALSE.
 * @param dtcSeverityMask
 * @param DTCClass
 * @return
 */
static boolean checkDtcSeverityMask(const Dem_DTCSeverityType dtcSeverityMask, const Dem_DTCClassType *DTCClass)
{
    return ((NULL_PTR != DTCClass) && ( 0 != (DTCClass->DTCSeverity & dtcSeverityMask)))? TRUE: FALSE;
}

/*
 * Procedure:   lookupEventStatusRec
 * Description: Returns the pointer to event id parameters of "eventId" in "*eventStatusBuffer",
 *              if not found NULL is returned.
 */
void lookupEventStatusRec(Dem_EventIdType eventId, EventStatusRecType **const eventStatusRec)
{
    if ( IS_VALID_EVENT_ID(eventId)) {
        *eventStatusRec = &eventStatusBuffer[eventId - 1];
    } else {
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GLOBAL_ID, DEM_E_UNEXPECTED_EXECUTION);
        *eventStatusRec = NULL;
    }
}

/*
 * Procedure:   lookupEventIdParameter
 * Description: Returns the pointer to event id parameters of "eventId" in "*eventIdParam",
 *              if not found NULL is returned.
 */
void lookupEventIdParameter(Dem_EventIdType eventId, const Dem_EventParameterType **const eventIdParam)
{
    const Dem_EventParameterType *EventIdParamList = configSet->EventParameter;
    if (IS_VALID_EVENT_ID(eventId)) {
        *eventIdParam = &EventIdParamList[eventId - 1];
    } else {
        *eventIdParam = NULL;
    }
}
/*
 * Procedure:   checkEntryValid
 * Description: Returns whether event id "eventId" is a valid entry in primary memory
 */
#ifdef DEM_USE_MEMORY_FUNCTIONS
static boolean checkEntryValid(Dem_EventIdType eventId, Dem_DTCOriginType origin, boolean allowCombinedID){
    const Dem_EventParameterType *EventIdParam = NULL;
    EventStatusRecType *eventStatusRec = NULL;
    boolean isValid = FALSE;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( (TRUE == allowCombinedID) && IS_COMBINED_EVENT_ID(eventId) ) {
        if( IS_VALID_COMBINED_ID(TO_COMBINED_EVENT_CFG_IDX(eventId)) ) {
            /* ID is valid, but is it valid for the destination */
            const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(eventId)];
            if( (origin == CombDTCCfg->MemoryDestination) && (TRUE == DTCIsAvailable(CombDTCCfg->DTCClassRef)) ) {
                isValid = TRUE;
            }
        }
    }
    else {
        if( !IS_COMBINED_EVENT_ID(eventId) ) {
            lookupEventIdParameter(eventId, &EventIdParam);
            if (NULL != EventIdParam) {
                // Event was found
                lookupEventStatusRec(eventId, &eventStatusRec);
                // Event should be stored in destination memory?
                isValid = (checkDtcOrigin(origin, EventIdParam, FALSE) && (NULL != eventStatusRec) && eventStatusRec->isAvailable)? TRUE: FALSE;
            } else {
                // The event did not exist
            }
        }
    }
#else
    (void)allowCombinedID;
    lookupEventIdParameter(eventId, &EventIdParam);
    if (NULL != EventIdParam) {
        // Event was found
        lookupEventStatusRec(eventId, &eventStatusRec);
        // Event should be stored in destination memory?
        isValid = ((TRUE == checkDtcOrigin(origin, EventIdParam, FALSE)) && (NULL != eventStatusRec) && (TRUE == eventStatusRec->isAvailable))? TRUE: FALSE;
    } else {
        // The event did not exist
    }
#endif
    return isValid;
}
#endif

/**
 * Checks if an operation cycle is started
 * @param opCycle
 * @return
 */
boolean operationCycleIsStarted(Dem_OperationCycleIdType opCycle)
{
    boolean isStarted = FALSE;
    if (opCycle < DEM_OPERATION_CYCLE_ID_ENDMARK) {
        if (operationCycleStateList[opCycle] == DEM_CYCLE_STATE_START) {
            isStarted = TRUE;
        }
    }
    return isStarted;
}

#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
static boolean failureCycleIsStarted(const Dem_EventParameterType *eventParam)
{
    return operationCycleIsStarted((Dem_OperationCycleIdType)*(eventParam->EventClass->FailureCycleRef));
}

static boolean faultConfirmationCriteriaFulfilled(const Dem_EventParameterType *eventParam, const EventStatusRecType *eventStatusRecPtr)
{
    if(((Dem_OperationCycleIdType)*(eventParam->EventClass->FailureCycleRef) != DEM_OPERATION_CYCLE_ID_ENDMARK) &&
            (eventStatusRecPtr->failureCounter >= eventParam->EventClass->FailureCycleCounterThresholdRef->Threshold) ) {
        return TRUE;
    } else {
        return FALSE;
    }
}
static void handleFaultConfirmation(const Dem_EventParameterType *eventParam, EventStatusRecType *eventStatusRecPtr)
{
    if( (TRUE == failureCycleIsStarted(eventParam)) && (FALSE == eventStatusRecPtr->failedDuringFailureCycle) ) {
        if( eventStatusRecPtr->failureCounter < DEM_FAILURE_CNTR_MAX ) {
            eventStatusRecPtr->failureCounter++;
            eventStatusRecPtr->errorStatusChanged = TRUE;
        }

        /* @req DEM530 */
        if( TRUE == faultConfirmationCriteriaFulfilled(eventParam, eventStatusRecPtr )) {
            eventStatusRecPtr->eventStatusExtended |= DEM_CONFIRMED_DTC;
            eventStatusRecPtr->errorStatusChanged = TRUE;
        }
        eventStatusRecPtr->failedDuringFailureCycle = TRUE;
    }
}
#endif

#if defined(DEM_USE_INDICATORS)

/**
 * Resets healing and failure counter for event indicators
 * @param eventParam
 * @return TRUE: counter value changed, FALSE: no change
 */
static boolean resetIndicatorCounters(const Dem_EventParameterType *eventParam)
{
    boolean countersChanged = FALSE;
    if( (NULL != eventParam->EventClass->IndicatorAttribute )  && (TRUE == (boolean)(*eventParam->EventClass->IndicatorAttribute->IndicatorValid)) ){
        const Dem_IndicatorAttributeType *indAttrPtr = eventParam->EventClass->IndicatorAttribute;
        while( FALSE == indAttrPtr->Arc_EOL) {
            if( (0 != indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].FailureCounter) || (0 != indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].HealingCounter)) {
                countersChanged = TRUE;
            }
            indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].FailureCounter = 0;
            indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].HealingCounter = 0;
            indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus = 0;
            indAttrPtr++;
        }
    }
#if defined(DEM_USE_MEMORY_FUNCTIONS)
    if(TRUE == countersChanged) {
        storeEventIndicators(eventParam);
    }
#endif
    return countersChanged;
}

/**
 * Checks if indicator failure cycle is started
 * @param eventParam
 * @param indAttr
 * @return TRUE: Failure cycle is started, FALSE: failure cycle not started
 */
static boolean indicatorFailureCycleIsStarted(const Dem_EventParameterType *eventParam, const Dem_IndicatorAttributeType *indAttr)
{
    if(DEM_FAILURE_CYCLE_INDICATOR == indAttr->IndicatorFailureCycleSource) {
        return operationCycleIsStarted((Dem_OperationCycleStateType)*(indAttr->IndicatorFailureCycle));
    } else {
        return operationCycleIsStarted((Dem_OperationCycleIdType)*(eventParam->EventClass->FailureCycleRef));
    }
}

/**
 * Checks if failure criteria for indicator is fulfilled
 * @param eventParam
 * @param indicatorAttribute
 * @return TRUE: criteria fulfilled, FALSE: criteria not fulfilled
 */
static boolean indicatorFailFulfilled(const Dem_EventParameterType *eventParam, const Dem_IndicatorAttributeType *indicatorAttribute)
{
    boolean fulfilled = FALSE;
    uint8 thresHold;
    Dem_OperationCycleIdType opCyc;

    if( DEM_FAILURE_CYCLE_INDICATOR == indicatorAttribute->IndicatorFailureCycleSource ) {
        thresHold = indicatorAttribute->IndicatorFailureCycleThreshold;
        opCyc = ((Dem_OperationCycleStateType)*(indicatorAttribute->IndicatorFailureCycle));
    } else {
        thresHold = eventParam->EventClass->FailureCycleCounterThresholdRef->Threshold;
        opCyc = (Dem_OperationCycleIdType)*(eventParam->EventClass->FailureCycleRef);
    }
    /* @req DEM501 */
    if( (opCyc < DEM_OPERATION_CYCLE_ID_ENDMARK) && (indicatorStatusBuffer[indicatorAttribute->IndicatorBufferIndex].FailureCounter >= thresHold) ) {
        fulfilled = TRUE;
    }
    return fulfilled;
}

/**
 * Checks if warningIndicatorOnCriteria is fulfilled for an event
 * @param eventParam
 * @return TRUE: criteria fulfilled, FALSE: criteria not fulfilled
 */
static boolean warningIndicatorOnCriteriaFulfilled(const Dem_EventParameterType *eventParam)
{
    boolean fulfilled = FALSE;
    if( (NULL != eventParam->EventClass->IndicatorAttribute) && (TRUE == (boolean)(*eventParam->EventClass->IndicatorAttribute->IndicatorValid)) ) {
        const Dem_IndicatorAttributeType *indAttrPtr = eventParam->EventClass->IndicatorAttribute;
        /* @req DEM566 */
        while( (FALSE == indAttrPtr->Arc_EOL) && (FALSE == fulfilled) ) {
            fulfilled = indicatorFailFulfilled(eventParam, indAttrPtr);
            indAttrPtr++;
        }
    }
    return fulfilled;
}

/**
 * Checks if indicator healing cycle is started
 * @param indAttr
 * @return TRUE: Healing cycle is started, FALSE: Healing cycle not started
 */
static boolean indicatorHealingCycleIsStarted(const Dem_IndicatorAttributeType *indAttr)
{
    return operationCycleIsStarted(indAttr->IndicatorHealingCycle);
}

/**
 * Prepares indicator status for a new failure/healing cycle
 * @param operationCycleId
 */
static void indicatorOpCycleStart(Dem_OperationCycleIdType operationCycleId) {
    Dem_OperationCycleIdType failCyc;
    const Dem_EventParameterType *eventIdParamList = configSet->EventParameter;
    uint16 indx = 0;
    while( FALSE == eventIdParamList[indx].Arc_EOL ) {
        if( NULL != eventIdParamList[indx].EventClass->IndicatorAttribute  ) {
            const Dem_IndicatorAttributeType *indAttrPtr = eventIdParamList[indx].EventClass->IndicatorAttribute;
            while( FALSE == indAttrPtr->Arc_EOL) {
                if( operationCycleId == indAttrPtr->IndicatorHealingCycle ) {
                    indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus &= (uint8)~(INDICATOR_FAILED_DURING_HEALING_CYCLE | INDICATOR_PASSED_DURING_HEALING_CYCLE);
                }
                if( DEM_FAILURE_CYCLE_INDICATOR == indAttrPtr->IndicatorFailureCycleSource ) {
                    failCyc = ((Dem_OperationCycleStateType)*indAttrPtr->IndicatorFailureCycle);
                } else {
                    failCyc = (Dem_OperationCycleIdType)*(eventIdParamList[indx].EventClass->FailureCycleRef);
                }
                if( operationCycleId == failCyc ) {
                    indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus &= (uint8)~(INDICATOR_FAILED_DURING_FAILURE_CYCLE | INDICATOR_PASSED_DURING_FAILURE_CYCLE);
                }
                indAttrPtr++;
            }
        }
        indx++;
    }
}

/**
 * Checks if warningIndicatorOffCriteria is fulfilled for an event
 * @param eventParam
 * @return TRUE: criteria fulfilled, FALSE: criteria not fulfilled
 */
static boolean warningIndicatorOffCriteriaFulfilled(const Dem_EventParameterType *eventParam)
{
    return (FALSE == warningIndicatorOnCriteriaFulfilled(eventParam))? TRUE :FALSE;
}

/**
 * Handles end of operation cycles for indicators
 * @param operationCycleId
 * @param eventStatusRecPtr
 * @return TRUE: Counter updated for at least one event
 */
static boolean indicatorOpCycleEnd(Dem_OperationCycleIdType operationCycleId, EventStatusRecType *eventStatusRecPtr) {
    /* @req DEM502 */
    /* @req DEM505 */
    Dem_OperationCycleIdType failCyc;
    boolean counterChanged = FALSE;
    uint8 healingCounterOld = 0u;
    uint8 failureCounterOld = 0u;
    const Dem_EventParameterType *eventParam = eventStatusRecPtr->eventParamRef;
    if( (NULL != eventParam) && (NULL != eventParam->EventClass->IndicatorAttribute)&& (TRUE == (boolean)(*eventParam->EventClass->IndicatorAttribute->IndicatorValid))  ) {
        const Dem_IndicatorAttributeType *indAttrPtr = eventParam->EventClass->IndicatorAttribute;
        while( FALSE == indAttrPtr->Arc_EOL) {
            healingCounterOld = indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].HealingCounter;
            failureCounterOld = indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].FailureCounter;
            if( (operationCycleId == indAttrPtr->IndicatorHealingCycle) &&
                    (0 == (indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus & INDICATOR_FAILED_DURING_HEALING_CYCLE)) &&
                    (0 != (indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus & INDICATOR_PASSED_DURING_HEALING_CYCLE)) &&
                    (TRUE == indicatorFailFulfilled(eventParam, indAttrPtr))) {
                /* Passed and didn't fail during the healing cycle */
                if( indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].HealingCounter < DEM_INDICATOR_CNTR_MAX ) {
                    indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].HealingCounter++;
                }
                if( indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].HealingCounter >= indAttrPtr->IndicatorHealingCycleThreshold ) {
                    /* @req DEM503 */
                    /* Healing condition fulfilled.
                     * Should we reset failure counter here? */
                    indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].FailureCounter = 0;
                }
            }

            if( DEM_FAILURE_CYCLE_INDICATOR == indAttrPtr->IndicatorFailureCycleSource ) {
                failCyc = ((Dem_OperationCycleStateType)*indAttrPtr->IndicatorFailureCycle);
            } else {
                failCyc = (Dem_OperationCycleIdType)*(eventParam->EventClass->FailureCycleRef);
            }
            if( (operationCycleId == failCyc) &&
                    (0 == (indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus & INDICATOR_FAILED_DURING_FAILURE_CYCLE)) &&
                    (0 != (indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus & INDICATOR_PASSED_DURING_FAILURE_CYCLE)) &&
                    (FALSE == indicatorFailFulfilled(eventParam, indAttrPtr))) {
                /* Passed and didn't fail during the failure cycle.
                 * Reset failure counter */
                indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].FailureCounter = 0;
            }
            if( (healingCounterOld != indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].HealingCounter) ||
                    (failureCounterOld != indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].FailureCounter) ) {
                counterChanged = TRUE;
            }
            indAttrPtr++;
        }
        /* @req DEM533 */
        if( TRUE == warningIndicatorOffCriteriaFulfilled(eventParam)) {
            eventStatusRecPtr->eventStatusExtended &= ~DEM_WARNING_INDICATOR_REQUESTED;
        }

#if defined(DEM_USE_MEMORY_FUNCTIONS)
        if(TRUE == counterChanged) {
            storeEventIndicators(eventParam);
        }
#endif
    }
    return counterChanged;
}

/**
 * Performs clearing of indicator healing counter if conditions fulfilled.
 * NOTE: This functions should only be called when event is FAILED.
 * @param eventParam
 * @param indAttrPtr
 */
static inline void handleHealingCounterOnFailed(const Dem_EventParameterType *eventParam, const Dem_IndicatorAttributeType *indAttrPtr)
{
#if defined(DEM_HEALING_COUNTER_CLEAR_ON_FAIL_DURING_FAILURE_CYCLE)
    if( TRUE == indicatorFailureCycleIsStarted(eventParam, indAttrPtr) ) {
        indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].HealingCounter = 0;
    }
#elif defined(DEM_HEALING_COUNTER_CLEAR_ON_FAIL_DURING_FAILURE_OR_HEALING_CYCLE)
    if( (TRUE == indicatorFailureCycleIsStarted(eventParam, indAttrPtr)) || (TRUE == indicatorHealingCycleIsStarted(indAttrPtr)) ) {
        indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].HealingCounter = 0;
    }
#elif defined(DEM_HEALING_COUNTER_CLEAR_ON_ALL_FAIL)
    (void)eventParam;/*lint !e920*/
    indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].HealingCounter = 0;
#else
#error "Dem: Unknown healing counter clear behavior"
#endif
}

/**
 * Handles updating of indicator failure counter and status bits
 * @param eventParam
 * @param eventStatusRecPtr
 * @param eventStatus
 * @return TRUE: counter was updated, FALSE: counter was not updated
 */
static boolean handleIndicators(const Dem_EventParameterType *eventParam, EventStatusRecType *eventStatusRecPtr, Dem_EventStatusType eventStatus)
{
    /* @req DEM506 */
    /* @req DEM510 */
    boolean cntrChanged = FALSE;
    if( (NULL != eventParam->EventClass->IndicatorAttribute) && (TRUE == (boolean)(*eventParam->EventClass->IndicatorAttribute->IndicatorValid)) ){
        const Dem_IndicatorAttributeType *indAttrPtr = eventParam->EventClass->IndicatorAttribute;
        while( FALSE == indAttrPtr->Arc_EOL) {
            switch(eventStatus) {
                case DEM_EVENT_STATUS_FAILED:
                    if( TRUE == indicatorFailureCycleIsStarted(eventParam, indAttrPtr) ) {
                        if( (0 == (indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus & INDICATOR_FAILED_DURING_FAILURE_CYCLE)) &&
                                (indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].FailureCounter < DEM_INDICATOR_CNTR_MAX) ) {
                            /* First fail during this failure cycle and incrementing failure counter would not overflow */
                            indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].FailureCounter++;
                            cntrChanged = TRUE;
                        }
                        indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus |= INDICATOR_FAILED_DURING_FAILURE_CYCLE;
                    }
                    if( TRUE == indicatorHealingCycleIsStarted(indAttrPtr) ) {
                        indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus |= INDICATOR_FAILED_DURING_HEALING_CYCLE;
                    }
                    break;
                case DEM_EVENT_STATUS_PASSED:
                    if( TRUE == indicatorFailureCycleIsStarted(eventParam, indAttrPtr) ) {
                        indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus |= INDICATOR_PASSED_DURING_FAILURE_CYCLE;
                    }
                    if( TRUE == indicatorHealingCycleIsStarted(indAttrPtr) ) {
                        indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus |= INDICATOR_PASSED_DURING_HEALING_CYCLE;
                    }
                    break;
                default:
                    break;
            }
            if( DEM_EVENT_STATUS_FAILED == eventStatus ) {
                handleHealingCounterOnFailed(eventParam, indAttrPtr);
            }
            indAttrPtr++;
        }
        /* @req DEM566 */
        if( TRUE == warningIndicatorOnCriteriaFulfilled(eventParam)) {
            eventStatusRecPtr->eventStatusExtended |= DEM_WARNING_INDICATOR_REQUESTED;
        }
    }
    return cntrChanged;
}

#if defined(DEM_USE_MEMORY_FUNCTIONS)
/**
 * Stores indicators for an event in memory destined for NvRam
 * @param eventParam
 */
static void storeEventIndicators(const Dem_EventParameterType *eventParam)
{
    const Dem_IndicatorAttributeType *indAttr;
    if( (NULL != eventParam->EventClass->IndicatorAttribute) && (TRUE == (boolean)(*eventParam->EventClass->IndicatorAttribute->IndicatorValid)) ){
        indAttr = eventParam->EventClass->IndicatorAttribute;
        while( FALSE == indAttr->Arc_EOL) {
            indicatorBuffer[indAttr->IndicatorBufferIndex].EventID = indicatorStatusBuffer[indAttr->IndicatorBufferIndex].EventID;
            indicatorBuffer[indAttr->IndicatorBufferIndex].FailureCounter = indicatorStatusBuffer[indAttr->IndicatorBufferIndex].FailureCounter;
            indicatorBuffer[indAttr->IndicatorBufferIndex].HealingCounter = indicatorStatusBuffer[indAttr->IndicatorBufferIndex].HealingCounter;
            indicatorBuffer[indAttr->IndicatorBufferIndex].IndicatorId = indAttr->IndicatorId;
            indAttr++;
        }
    }
}

/**
 * Merges indicator status read from NvRam with status held in ram
 */
static void mergeIndicatorBuffers(void) {
    const Dem_EventParameterType *eventParam;
    const Dem_IndicatorAttributeType *indAttr;
    for(uint32 i = 0; i < DEM_NOF_EVENT_INDICATORS; i++) {
        if(IS_VALID_EVENT_ID(indicatorBuffer[i].EventID) && IS_VALID_INDICATOR_ID(indicatorBuffer[i].IndicatorId)) {
            /* Valid event and indicator. Check that it is a valid indicator for this event */
            eventParam = NULL;
            lookupEventIdParameter(indicatorBuffer[i].EventID, &eventParam);
            if((NULL != eventParam) && (NULL != eventParam->EventClass->IndicatorAttribute) && (TRUE == (boolean)(*eventParam->EventClass->IndicatorAttribute->IndicatorValid)) ){
                indAttr = eventParam->EventClass->IndicatorAttribute;
                while(FALSE == indAttr->Arc_EOL) {
                    if( indAttr->IndicatorId == indicatorBuffer[i].IndicatorId ) {
                        /* Update healing counter. */
                        if( (0 != (indicatorStatusBuffer[indAttr->IndicatorBufferIndex].OpCycleStatus & INDICATOR_FAILED_DURING_FAILURE_CYCLE)) ||
                                (0 != (indicatorStatusBuffer[indAttr->IndicatorBufferIndex].OpCycleStatus & INDICATOR_FAILED_DURING_HEALING_CYCLE))) {
                            /* Failed at some point during PreInit.  */
                            indicatorStatusBuffer[indAttr->IndicatorBufferIndex].HealingCounter = 0;
                        } else {
                            indicatorStatusBuffer[indAttr->IndicatorBufferIndex].HealingCounter = indicatorBuffer[i].HealingCounter;
                        }

                        /* Update failure counter */
                        if( (DEM_INDICATOR_CNTR_MAX - indicatorBuffer[i].FailureCounter) > indicatorStatusBuffer[indAttr->IndicatorBufferIndex].FailureCounter) {
                            indicatorStatusBuffer[indAttr->IndicatorBufferIndex].FailureCounter += indicatorBuffer[i].FailureCounter;
                        } else {
                            indicatorStatusBuffer[indAttr->IndicatorBufferIndex].FailureCounter = DEM_INDICATOR_CNTR_MAX;
                        }
                    }
                    indAttr++;
                }
            }
        }
    }
#ifdef DEM_USE_MEMORY_FUNCTIONS
    /* Transfer content of indicatorStatusBuffer to indicatorBuffer */
    eventParam = configSet->EventParameter;
    while( FALSE == eventParam->Arc_EOL ) {
        storeEventIndicators(eventParam);
        eventParam++;
    }
    /* IMPROVEMENT: Only call this if the content of memory was changed */
    /* IMPROVEMENT: Add handling of immediate storage */
    Dem_NvM_SetIndicatorBlockChanged(FALSE);
#endif
}

#endif
#endif
#if (DEM_USE_TIMESTAMPS == STD_ON) && defined (DEM_USE_MEMORY_FUNCTIONS)
static void setEventTimeStamp(EventStatusRecType *eventStatusRecPtr)
{
    if( DEM_INITIALIZED == demState ) {
        if( Event_TimeStamp >= DEM_MAX_TIMESTAMP_FOR_REARRANGEMENT ) {
            rearrangeEventTimeStamp(&Event_TimeStamp);

        }
        eventStatusRecPtr->timeStamp = Event_TimeStamp;
        Event_TimeStamp++;
    } else {
        eventStatusRecPtr->timeStamp = Event_TimeStamp;
        if( Event_TimeStamp < DEM_MAX_TIMESTAMP_FOR_PRE_INIT ) {
            Event_TimeStamp++;
        }
    }
}
#endif

#if (defined(DEM_USE_INDICATORS) && (DEM_OBD_DISPLACEMENT_SUPPORT == STD_ON)) || (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
/**
 * Checks if event currently activates MIL
 * @param eventParam
 * @return
 */
static boolean eventActivatesMIL(const Dem_EventParameterType *eventParam)
{
    boolean fulfilled = FALSE;
    if( (NULL != eventParam->EventClass->IndicatorAttribute) && (TRUE == (boolean)(*eventParam->EventClass->IndicatorAttribute->IndicatorValid)) )  {
#if defined(DEM_USE_INDICATORS)
        const Dem_IndicatorAttributeType *indAttrPtr = eventParam->EventClass->IndicatorAttribute;
        /* @req DEM566 */
        while( (FALSE == indAttrPtr->Arc_EOL) && (FALSE == fulfilled) ) {
            if( DEM_MIL_INIDICATOR_ID == indAttrPtr->IndicatorId ) {
                fulfilled |= indicatorFailFulfilled(eventParam, indAttrPtr);
            }
            indAttrPtr++;
        }
#endif
    }
    return fulfilled;
}
#endif

#if (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
/* !req DEM300 Do we handle according to regulations? */

/**
 * Stores a permanent DTC
 * @param DTCRef
 * @return TRUE: buffer was updated, FALSE: buffer NOT updated
 */
static boolean storeDTCPermanentMemory(const Arc_Dem_DTC *DTCRef)
{
    boolean alreadyStored = FALSE;
    boolean freeEntryFound = FALSE;
    boolean stored = FALSE;
    uint32 storeIndex = 0;
    /* Check if it is already stored and if there is a free entry */
    for( uint32 i = 0; (i < DEM_MAX_NUMBER_EVENT_PERM_MEM) && (FALSE == alreadyStored); i++) {
        if( FREE_PERMANENT_DTC_ENTRY == permMemEventBuffer[i].OBDDTC ) {
            /* Found a free entry */
            freeEntryFound = TRUE;
            storeIndex = i;
        }

        if( DTCRef->OBDDTC == permMemEventBuffer[i].OBDDTC) {
            /* DTC already stored */
            alreadyStored = TRUE;
        }
    }
    if( (FALSE == alreadyStored) && (TRUE == freeEntryFound) ) {
        permMemEventBuffer[storeIndex].OBDDTC = DTCRef->OBDDTC;
        stored = TRUE;
    }
    return stored;
}

/**
 * Deletes a permanent DTC
 * @param DTCRef
 * @return TRUE: buffer was updated, FALSE: buffer NOT updated
 */
static boolean deleteDTCPermanentMemory(const Arc_Dem_DTC *DTCRef)
{
    boolean deleted = FALSE;
    /* Check if it is already stored and if there is a free entry */
    for( uint32 i = 0; (i < DEM_MAX_NUMBER_EVENT_PERM_MEM) && (FALSE == deleted); i++) {
        if( DTCRef->OBDDTC == permMemEventBuffer[i].OBDDTC) {
            /* DTC already stored */
            permMemEventBuffer[i].OBDDTC = FREE_PERMANENT_DTC_ENTRY;
            deleted = TRUE;
        }
    }

    return deleted;
}

/**
 * Check if a DTC is a valid OBD DTC
 * @param dtc
 * @return
 */
static boolean isValidPermanentOBDDTC(uint32 dtc)
{
    boolean validDTC = FALSE;
    const Dem_EventParameterType *eventParam = configSet->EventParameter;
    while( (FALSE == eventParam->Arc_EOL) && (FALSE == validDTC)) {
        if( (DEM_EMISSION_RELATED_MIL_ACTIVATING == (Dem_Arc_EventDTCKindType) *eventParam->EventDTCKind) && (NULL != eventParam->DTCClassRef)){
            if( dtc == eventParam->DTCClassRef->DTCRef->OBDDTC ) {
                validDTC = TRUE;
            }
        }
        eventParam++;
    }

    return validDTC;
}

/**
 * Checks if condition for storing a permanent DTC is fulfilled
 * @param eventParam
 * @param evtStatus
 * @return TRUE: condition fulfilled, FALSE: condition NOT fulfilled
 */
static boolean permanentDTCStorageConditionFulfilled(const Dem_EventParameterType *eventParam, Dem_EventStatusExtendedType evtStatus)
{
    boolean fulfilled = FALSE;
    if( TRUE == eventIsEmissionRelatedMILActivating(eventParam) ) {
        /* This event has an emission related DTC.
         * Check if it is confirmed and activates MIL */
        if( (0u != (evtStatus & DEM_CONFIRMED_DTC)) && (TRUE == eventActivatesMIL(eventParam)) ) {
            /* Condition for storing as permanent is fulfilled*/
            fulfilled = TRUE;
        }
    }
    return fulfilled;
}

/**
 * Handles storage of permanent DTCs
 * @param eventParam
 * @param evtStatus
 * @return TRUE: permanent memory updated, FALSE: permanent memory NOT updated
 */
static boolean handlePermanentDTCStorage(const Dem_EventParameterType *eventParam, Dem_EventStatusExtendedType evtStatus)
{
    boolean memoryUpdated = FALSE;
    if( TRUE == permanentDTCStorageConditionFulfilled(eventParam, evtStatus) ) {
        /* Try store as permanent DTC */
        memoryUpdated = storeDTCPermanentMemory(eventParam->DTCClassRef->DTCRef);
    }
    return memoryUpdated;
}

/**
 * Handles erasing permanent DTCs.
 * NOTE: May only be called on operation cycle end!
 * @param eventParam
 * @param evtStatus
 */
static boolean handlePermanentDTCErase(const EventStatusRecType *eventRec, Dem_OperationCycleIdType operationCycleId)
{
    boolean MILHealOK = TRUE;
    const Dem_EventParameterType *eventParam = eventRec->eventParamRef;
    /* Check if emission related */
    boolean permanentMemoryUpdated = FALSE;
    if( TRUE == eventIsEmissionRelatedMILActivating(eventParam) ) {
        /* This event has an emission related DTC.
         * Check if it currently activates MIL */
        if( FALSE == eventActivatesMIL(eventParam)) {
#if defined(DEM_USE_INDICATORS)
            if( (NULL != eventParam->EventClass->IndicatorAttribute) && (TRUE == (boolean)(*eventParam->EventClass->IndicatorAttribute->IndicatorValid)) ) {
                const Dem_IndicatorAttributeType *indAttrPtr = eventParam->EventClass->IndicatorAttribute;
                /* @req DEM566 */
                while( (FALSE == indAttrPtr->Arc_EOL) && (FALSE != MILHealOK)) {
                    if( DEM_MIL_INIDICATOR_ID == indAttrPtr->IndicatorId ) {
                        if( (operationCycleId != indAttrPtr->IndicatorHealingCycle) ||
                            (0u != (indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus & INDICATOR_FAILED_DURING_HEALING_CYCLE)) ||
                            (0u == (indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus & INDICATOR_PASSED_DURING_HEALING_CYCLE))) {
                            /* Failed or did not pass.  */
                            MILHealOK = FALSE;
                        }
                    }
                    indAttrPtr++;
                }
            }
#endif
            if(TRUE == MILHealOK) {
                /* Delete permanent DTC */
                permanentMemoryUpdated = deleteDTCPermanentMemory(eventParam->DTCClassRef->DTCRef);
            }
        }
    }
    return permanentMemoryUpdated;
}

/**
 *
 */
static void ValidateAndUpdatePermanentBuffer(void)
{
    boolean newDataStored = FALSE;
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
    boolean immediateStorage = FALSE;
#endif
    /* Validate entries in the buffer */
    for( uint32 i = 0; i < DEM_MAX_NUMBER_EVENT_PERM_MEM; i++) {
        if( FALSE == isValidPermanentOBDDTC(permMemEventBuffer[i].OBDDTC)) {
            /* DTC is not a valid permanent DTC */
            permMemEventBuffer[i].OBDDTC = FREE_PERMANENT_DTC_ENTRY;
        }
    }

    /* Insert emission relates DTCs which are currently confirmed and activating MIL */
    for (uint16 i = 0; i < DEM_MAX_NUMBER_EVENT; i++) {
        if( DEM_EVENT_ID_NULL != eventStatusBuffer[i].eventId ) {
            if(TRUE == handlePermanentDTCStorage(eventStatusBuffer[i].eventParamRef, eventStatusBuffer[i].eventStatusExtended) ) {
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
                if( (NULL != eventStatusBuffer[i].eventParamRef->DTCClassRef) && (TRUE == eventStatusBuffer[i].eventParamRef->DTCClassRef->DTCRef->ImmediateNvStorage) &&
                        (eventStatusBuffer[i].occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
                    immediateStorage = TRUE;
                }
#endif
                newDataStored = TRUE;
            }
        }
    }
    /* Set block changed if new data was stored */
    if( TRUE == newDataStored ) {
        /* !req DEM590 */
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
        Dem_NvM_SetPermanentBlockChanged(immediateStorage);
#else
        Dem_NvM_SetPermanentBlockChanged(FALSE);
#endif
    }
}
#endif /* DEM_USE_PERMANENT_MEMORY_SUPPORT */

#if (DEM_PRESTORAGE_FF_DATA_IN_MEM)
static void ValidateAndUpdatePreStoredFreezeFramesBuffer(void)
{
    const Dem_EventParameterType *eventParam;
    boolean ffDeleted = FALSE;


    /* Validate entries in the buffer */
    for( uint32 i = 0; i < DEM_MAX_NUMBER_PRESTORED_FF; i++) {
        if (0u != memPreStoreFreezeFrameBuffer[i].eventId) {
            lookupEventIdParameter(memPreStoreFreezeFrameBuffer[i].eventId, &eventParam);

            if (NULL != eventParam ) {
                if (FALSE == eventParam->EventClass->FFPrestorageSupported) {
                    memset(&memPreStoreFreezeFrameBuffer[i], 0, sizeof(FreezeFrameRecType));
                    ffDeleted = TRUE;
                }
            }
            else {
                /* Invalid ID. Deletet it. */
                memset(&memPreStoreFreezeFrameBuffer[i], 0, sizeof(FreezeFrameRecType));
                ffDeleted = TRUE;
            }
        }

    }

   /* Set block changed if new data was stored */
    if( TRUE == ffDeleted ) {
        Dem_NvM_SetPreStoreFreezeFrameBlockChanged(FALSE);
    }
}
#endif

static void notifyEventStatusChange(const Dem_EventParameterType *eventParam, Dem_EventStatusExtendedType oldStatus, Dem_EventStatusExtendedType newStatus)
{
    uint8 j = 0;
    if( NULL != eventParam ) {
        if( NULL != eventParam->CallbackEventStatusChanged ) {
            /* @req Dem016 */ /* @req Dem615 */
            while( FALSE == eventParam->CallbackEventStatusChanged[j].Arc_EOL ) {
                if( TRUE == eventParam->CallbackEventStatusChanged[j].UsePort ) {
                    (void)eventParam->CallbackEventStatusChanged[j].CallbackEventStatusChangedFnc.eventStatusChangedWithoutId(oldStatus, newStatus);
                } else {
                    (void)eventParam->CallbackEventStatusChanged[j].CallbackEventStatusChangedFnc.eventStatusChangedWithId(eventParam->EventID, oldStatus, newStatus);
                }
                j++;
            }
        }
#if defined(USE_RTE) && (DEM_GENERAL_EVENT_STATUS_CB == STD_ON)
        /* @req Dem616 */
        (void)Rte_Call_GeneralCBStatusEvt_EventStatusChanged(eventParam->EventID, oldStatus, newStatus);
#endif

#if (DEM_TRIGGER_DLT_REPORTS == STD_ON)
        /* @req Dem517 */
        Dlt_DemTriggerOnEventStatus(eventParam->EventID, oldStatus, newStatus);
#endif

#if defined(USE_FIM) && (DEM_TRIGGER_FIM_REPORTS == STD_ON)
        /* @req Dem029 */
        if( TRUE == DemFiMInit ) {
            FiM_DemTriggerOnMonitorStatus(eventParam->EventID);
        }
#endif
    }
}

static void notifyEventDataChanged(const Dem_EventParameterType *eventParam) {
    /* @req DEM474 */
    if( (NULL != eventParam) && (NULL != eventParam->CallbackEventDataChanged)) {
        /* @req Dem618 */
        if( TRUE == eventParam->CallbackEventDataChanged->UsePort ) {
            (void)eventParam->CallbackEventDataChanged->CallbackEventDataChangedFnc.eventDataChangedWithoutId();
        } else {
            (void)eventParam->CallbackEventDataChanged->CallbackEventDataChangedFnc.eventDataChangedWithId(eventParam->EventID);
        }
    }

#if defined(USE_RTE) && (DEM_GENERAL_EVENT_DATA_CB == STD_ON)
    /* @req Dem619 */
    if( NULL != eventParam ) {
        (void)Rte_Call_GeneralCBDataEvt_EventDataChanged(eventParam->EventID);
    }
#endif
}

static void setDefaultEventStatus(EventStatusRecType *eventStatusRecPtr)
{
    eventStatusRecPtr->eventId = DEM_EVENT_ID_NULL;
    eventStatusRecPtr->eventParamRef = NULL;
    eventStatusRecPtr->fdcInternal = 0;
    eventStatusRecPtr->UDSFdc = 0;
    eventStatusRecPtr->maxUDSFdc = 0;
    eventStatusRecPtr->occurrence = 0;
    eventStatusRecPtr->eventStatusExtended = DEM_DEFAULT_EVENT_STATUS;
    eventStatusRecPtr->errorStatusChanged = FALSE;
    eventStatusRecPtr->extensionDataChanged = FALSE;
    eventStatusRecPtr->extensionDataStoreBitfield = 0;
#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
    eventStatusRecPtr->failureCounter = 0;
    eventStatusRecPtr->failedDuringFailureCycle = FALSE;
    eventStatusRecPtr->passedDuringFailureCycle = FALSE;
#endif
#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL)
    eventStatusRecPtr->agingCounter = 0;
    eventStatusRecPtr->passedDuringAgingCycle = FALSE;
    eventStatusRecPtr->failedDuringAgingCycle = FALSE;
#endif
    eventStatusRecPtr->timeStamp = 0;
}

#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL)
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1) && defined(DEM_CLEAR_COMBINED_AGING_COUNTERS_ON_FAIL)
#if defined(DEM_USE_MEMORY_FUNCTIONS)
/**
 * Reset an existing aging counter in event memory.
 * @param eventId
 * @param buffer
 * @param bufferSize
 * @param origin
 */
static void resetAgingCounter(Dem_EventIdType eventId, EventRecType* buffer, uint32 bufferSize, Dem_DTCOriginType origin)
{
    boolean positionFound = FALSE;

    for (uint32 i = 0uL; (i < bufferSize) && (FALSE == positionFound); i++){
        if( buffer[i].EventData.eventId == eventId ) {
            buffer[i].EventData.agingCounter = 0u;
            Dem_NvM_SetEventBlockChanged(origin, FALSE);
        }
    }
}
#endif
/**
 * Resets aging counter in memory destination
 * @param eventId
 * @param DTCOrigin
 */
static void resetAgingCounterEvtMem(Dem_EventIdType eventId, Dem_DTCOriginType DTCOrigin)
{
    switch (DTCOrigin) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
            resetAgingCounter(eventId, priMemEventBuffer, DEM_MAX_NUMBER_EVENT_ENTRY_PRI, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
#endif
            break;
        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
            resetAgingCounter(eventId, secMemEventBuffer, DEM_MAX_NUMBER_EVENT_ENTRY_SEC, DEM_DTC_ORIGIN_SECONDARY_MEMORY);
#endif
            break;
        default:
            /* Origin not supported */
            break;
    }
}
/**
 * Clears aging counter for all other sub-events of a combined event
 * @param eventParam
 */
static void clearCombinedEventAgingCounters(const Dem_EventParameterType *eventParam)
{
    EventStatusRecType *eventStatusRecPtr;
    const Dem_CombinedDTCCfgType *CombDTCCfg;
    const Dem_DTCClassType *DTCClass;
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        CombDTCCfg = &configSet->CombinedDTCConfig[eventParam->CombinedDTCCID];
        DTCClass = CombDTCCfg->DTCClassRef;
        for(uint16 i = 0; i < DTCClass->NofEvents; i++) {
            if( eventParam->EventID != DTCClass->Events[i] ) {
                eventStatusRecPtr = NULL_PTR;
                lookupEventStatusRec(DTCClass->Events[i], &eventStatusRecPtr);
                if( NULL_PTR !=  eventStatusRecPtr ) {
                    if( (0u != (eventStatusRecPtr->eventStatusExtended & DEM_CONFIRMED_DTC)) && (0u != eventStatusRecPtr->agingCounter) ) {
                        eventStatusRecPtr->agingCounter = 0u;
                        resetAgingCounterEvtMem(DTCClass->Events[i], CombDTCCfg->MemoryDestination);
                    }
                }
            }
        }
    }
}
#endif
/**
 * Handles clearing of aging counter. Should only be called when operation cycle
 * is start and event is qualified as FAILED
 * @param eventParam
 * @param eventStatusRecPtr
 */
static inline void handleAgingCounterOnFailed(const Dem_EventParameterType *eventParam, EventStatusRecType *eventStatusRecPtr)
{
#if defined(DEM_AGING_COUNTER_CLEAR_ON_FAIL_DURING_FAILURE_CYCLE)
    if( TRUE == operationCycleIsStarted((Dem_OperationCycleIdType)*(eventParam->EventClass->FailureCycleRef)) ) {
        eventStatusRecPtr->agingCounter = 0;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1) && defined(DEM_CLEAR_COMBINED_AGING_COUNTERS_ON_FAIL)
        clearCombinedEventAgingCounters(eventParam);
#endif
    }
#elif defined(DEM_AGING_COUNTER_CLEAR_ON_FAIL_DURING_FAILURE_OR_AGING_CYCLE)
    if( (TRUE == operationCycleIsStarted((Dem_OperationCycleIdType)*(eventParam->EventClass->FailureCycleRef))) || (TRUE == operationCycleIsStarted(eventParam->EventClass->AgingCycleRef)) ) {
        eventStatusRecPtr->agingCounter = 0;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1) && defined(DEM_CLEAR_COMBINED_AGING_COUNTERS_ON_FAIL)
        clearCombinedEventAgingCounters(eventParam);
#endif
    }
#elif defined(DEM_AGING_COUNTER_CLEAR_ON_ALL_FAIL)
    (void)eventParam;/*lint !e920*/
    eventStatusRecPtr->agingCounter = 0;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1) && defined(DEM_CLEAR_COMBINED_AGING_COUNTERS_ON_FAIL)
        clearCombinedEventAgingCounters(eventParam);
#endif
#else
#error "Dem: Unknown aging counter clear behavior"
#endif
}
#endif

#if defined(DEM_USE_IUMPR)
/**
 * Increments IUMPR numerator of a specific component according to legislation
 * @param RatioID
 */
static Std_ReturnType incrementIumprNumerator(Dem_RatioIdType ratioId) {
	Std_ReturnType ret = E_NOT_OK;

	if (ratioId < DEM_IUMPR_REGISTERED_COUNT) { // is valid ID
		if (FALSE ==  iumprBufferLocal[ratioId].numerator.incrementedThisDrivingCycle) { // not incremented this driving cycle
			if (TRUE == operationCycleIsStarted(DEM_OBD_DCY)) {  /* IMPROVEMENT */
				uint16 boundEventId = Dem_RatiosList[ratioId].DiagnosticEventRef->EventID;
				EventStatusRecType* boundEvent;
				lookupEventStatusRec(boundEventId, &boundEvent);
				Dem_EventStatusExtendedType boundEventStatus = GET_STORED_STATUS_BITS(boundEvent->eventStatusExtended);

				/* @req DEM299 */
				if (0u == (boundEventStatus & DEM_PENDING_DTC)) { // not pending event
					// (C) If either the numerator or denominator for a specific component reaches
					// the maximum value of 65,535 ±2, both numbers shall be divided by two
					// before either is incremented again to avoid overflow problems.
					if (iumprBufferLocal[ratioId].numerator.value == 65535) {
						iumprBufferLocal[ratioId].numerator.value = 32767;

						iumprBufferLocal[ratioId].denominator.value = iumprBufferLocal[ratioId].denominator.value / 2;
					}

					iumprBufferLocal[ratioId].numerator.value++;

					iumprBufferLocal[ratioId].numerator.incrementedThisDrivingCycle = TRUE;

					ret = E_OK;
				}
			}
		}
	}

	return ret;
}

/**
 * Increments IUMPR denominator of a specific component according to legislation
 * @param RatioID
 */
static Std_ReturnType incrementIumprDenominator(Dem_RatioIdType ratioId) {
	Std_ReturnType ret = E_NOT_OK;

	if (ratioId < DEM_IUMPR_REGISTERED_COUNT) { // is valid ID
		if (FALSE == iumprBufferLocal[ratioId].denominator.isLocked) { // if denominator is NOT locked
			if (FALSE == iumprBufferLocal[ratioId].denominator.incrementedThisDrivingCycle) { // not incremented this driving cycle
				if (TRUE == operationCycleIsStarted(DEM_OBD_DCY)) {  /* IMPROVEMENT */
					uint16 boundEventId = Dem_RatiosList[ratioId].DiagnosticEventRef->EventID;
					EventStatusRecType* boundEvent;
					lookupEventStatusRec(boundEventId, &boundEvent);
					Dem_EventStatusExtendedType boundEventStatus = GET_STORED_STATUS_BITS(boundEvent->eventStatusExtended);

					/* @req DEM299 */
					if (0u == (boundEventStatus & DEM_PENDING_DTC)) { // not pending event
						// (C) If either the numerator or denominator for a specific component reaches
						// the maximum value of 65,535 ±2, both numbers shall be divided by two
						// before either is incremented again to avoid overflow problems.
						if (iumprBufferLocal[ratioId].denominator.value == 65535) {
							iumprBufferLocal[ratioId].denominator.value = 32767;

							iumprBufferLocal[ratioId].numerator.value = iumprBufferLocal[ratioId].numerator.value / 2;
						}

						iumprBufferLocal[ratioId].denominator.value++;

						iumprBufferLocal[ratioId].denominator.incrementedThisDrivingCycle = TRUE;

						ret = E_OK;
					}
				}
			}
		}
	}


	return ret;
}

/**
 * Resets the incrementedThisDrivingCycle flag to FALSE of all numerators and denominators in IUMPR buffer,
 * as well as general denominator, if current operating cycle is the bound driving cycle.
 *
 * @param operationCycleId
 */
static void resetIumprFlags(Dem_OperationCycleIdType operationCycleId) {
	if (operationCycleId == DEM_OBD_DCY) {
		for (Dem_RatioIdType i = 0; i < DEM_IUMPR_REGISTERED_COUNT; i++) {
			iumprBufferLocal[i].numerator.incrementedThisDrivingCycle = FALSE;
			iumprBufferLocal[i].denominator.incrementedThisDrivingCycle = FALSE;
		}

		// reset general denominator flags and status
		generalDenominatorBuffer.incrementedThisDrivingCycle = FALSE;
		(void) Dem_SetIUMPRDenCondition(DEM_IUMPR_GENERAL_OBDCOND, DEM_IUMPR_DEN_STATUS_NOT_REACHED);
	}
}

/**
 * Increments all unlocked denominators in IUMPR buffer,
 * if current operating cycle is the bound driving cycle.
 * @param operationCycleId
 */
static void incrementUnlockedIumprDenominators(Dem_OperationCycleIdType operationCycleId) {
	if (operationCycleId == DEM_OBD_DCY) {
		for (Dem_RatioIdType i = 0; i < DEM_IUMPR_REGISTERED_COUNT; i++) {
			(void) incrementIumprDenominator(i);
		}
	}
}

/**
 * Increments observer numerator in IUMPR buffer,
 * if its bound event is set to pass or failed, must be qualified.
 */
static void incrementObserverIumprNumerator(Dem_EventIdType eventId, Dem_EventStatusType eventStatus) {
	/* @req DEM359 */
	// find the ratio that has the given event bound to it
	if (eventStatus == DEM_EVENT_STATUS_FAILED || eventStatus == DEM_EVENT_STATUS_PASSED) {
		for (uint16 ratioId = 0; ratioId < DEM_IUMPR_REGISTERED_COUNT; ratioId++) {
			if (Dem_RatiosList[ratioId].RatioKind == DEM_RATIO_OBSERVER && eventId == Dem_RatiosList[ratioId].DiagnosticEventRef->EventID) {
				(void) incrementIumprNumerator(ratioId);
			}
		}
	}
}

/**
 * Initialise IUMPR additional denominator conditions buffer
 */
static void initIumprAddiDenomCondBuffer() {
	iumprAddiDenomCondBuffer[0].condition = DEM_IUMPR_DEN_COND_COLDSTART;
	iumprAddiDenomCondBuffer[1].condition = DEM_IUMPR_DEN_COND_EVAP;
	iumprAddiDenomCondBuffer[2].condition = DEM_IUMPR_DEN_COND_500MI;
	iumprAddiDenomCondBuffer[3].condition = DEM_IUMPR_GENERAL_OBDCOND;

	for (uint8 i = 0; i < DEM_IUMPR_ADDITIONAL_DENOMINATORS_COUNT; i++) {
		iumprAddiDenomCondBuffer[i].status = DEM_IUMPR_DEN_STATUS_NOT_REACHED;
	}
}

/**
 * Increment ignition cycle counter at the start of ignition cycle
 */
static void incrementIgnitionCycleCounter(Dem_OperationCycleIdType operationCycleId) {
	// If the ignition cycle counter reaches the maximum value of 65,535 ±2, the
	// ignition cycle counter shall rollover and increment to zero on the next
	// ignition cycle to avoid overflow problems.
	if (operationCycleId == DEM_IGNITION) {
		if (ignitionCycleCountBuffer == 65535) {
			ignitionCycleCountBuffer = 0;
		} else  {
			ignitionCycleCountBuffer++;
		}
	}
}
#endif

/**
 * Performs event updates when event is qualified as FAILED
 * @param eventParam
 * @param eventStatusRecPtr
 */
static inline void updateEventOnFAILED(const Dem_EventParameterType *eventParam, EventStatusRecType *eventStatusRecPtr) {
    if (0 == (eventStatusRecPtr->eventStatusExtended & DEM_TEST_FAILED)) {
        if( eventStatusRecPtr->occurrence < DEM_OCCURENCE_COUNTER_MAX ) {
            eventStatusRecPtr->occurrence++;/* @req DEM523 *//* @req DEM524 *//* !req DEM625 */
        }
        eventStatusRecPtr->errorStatusChanged = TRUE;
    }
#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL)
    if( TRUE == operationCycleIsStarted(eventParam->EventClass->AgingCycleRef) ) {
        eventStatusRecPtr->failedDuringAgingCycle = TRUE;
    }

    handleAgingCounterOnFailed(eventParam, eventStatusRecPtr);
#endif
#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
    /* Handle fault confirmation *//** @req DEM379.ConfirmedSet */
    handleFaultConfirmation(eventParam, eventStatusRecPtr);
#endif
    /** @req DEM036 */ /** @req DEM379.PendingSet */
    eventStatusRecPtr->eventStatusExtended |= (DEM_TEST_FAILED | DEM_TEST_FAILED_THIS_OPERATION_CYCLE | DEM_TEST_FAILED_SINCE_LAST_CLEAR | DEM_PENDING_DTC);
    eventStatusRecPtr->eventStatusExtended &= (Dem_EventStatusExtendedType)~(DEM_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR | DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE);
}

/**
 * Performs event updates when event is qualified as PASSED
 * @param eventParam
 * @param eventStatusRecPtr
 */
static inline void updateEventOnPASSED(const Dem_EventParameterType *eventParam, EventStatusRecType *eventStatusRecPtr) {
    if ( 0 != (eventStatusRecPtr->eventStatusExtended & DEM_TEST_FAILED) ) {
        eventStatusRecPtr->errorStatusChanged = TRUE;
    }
    /** @req DEM036 */
    eventStatusRecPtr->eventStatusExtended &= (Dem_EventStatusExtendedType)~DEM_TEST_FAILED;
    eventStatusRecPtr->eventStatusExtended &= (Dem_EventStatusExtendedType)~(DEM_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR | DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE);
#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL)
    if( TRUE == operationCycleIsStarted(eventParam->EventClass->AgingCycleRef) ) {
        eventStatusRecPtr->passedDuringAgingCycle = TRUE;
    }
#endif
#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
    if( TRUE == operationCycleIsStarted((Dem_OperationCycleIdType)*(eventParam->EventClass->FailureCycleRef))) {
        eventStatusRecPtr->passedDuringFailureCycle = TRUE;
    }
#endif
}
/*
 * Procedure:   updateEventStatusRec
 * Description: Update the status of "eventId"
 */
static void updateEventStatusRec(const Dem_EventParameterType *eventParam, Dem_EventStatusType reportedEventStatus, EventStatusRecType *eventStatusRecPtr)
{
    /* IMPROVEMENT: !req DEM544 */
    Dem_EventStatusType eventStatus = reportedEventStatus;

    if (eventStatusRecPtr != NULL) {

        eventStatus = RunPredebounce(reportedEventStatus, eventStatusRecPtr, eventParam);
        eventStatusRecPtr->errorStatusChanged = FALSE;
        eventStatusRecPtr->extensionDataChanged = FALSE;
        eventStatusRecPtr->indicatorDataChanged = FALSE;
        eventStatusRecPtr->extensionDataStoreBitfield = 0;

#if defined(USE_DEM_EXTENSION)
        Dem_EventStatusExtendedType eventStatusExtendedBeforeUpdate = eventStatusRecPtr->eventStatusExtended;
#endif

        switch(eventStatus) {
            case DEM_EVENT_STATUS_FAILED:
                updateEventOnFAILED(eventParam, eventStatusRecPtr);
                break;
            case DEM_EVENT_STATUS_PASSED:
                updateEventOnPASSED(eventParam, eventStatusRecPtr);
                break;
            default:
                break;
        }

#if defined(DEM_USE_IUMPR)
        incrementObserverIumprNumerator(eventParam->EventID, eventStatus);
#endif

#if defined(DEM_USE_INDICATORS)
        /** @req DEM379.WarningIndicatorSet */
        if(TRUE == handleIndicators(eventParam, eventStatusRecPtr, eventStatus)) {
            eventStatusRecPtr->indicatorDataChanged = TRUE;
        }
#endif

#if defined(USE_DEM_EXTENSION)
        Dem_Extension_UpdateEventstatus(eventStatusRecPtr, eventStatusExtendedBeforeUpdate, eventStatus);
#endif

        eventStatusRecPtr->maxUDSFdc = MAX(eventStatusRecPtr->maxUDSFdc, eventStatusRecPtr->UDSFdc);
#if (DEM_USE_TIMESTAMPS == STD_ON) && defined (DEM_USE_MEMORY_FUNCTIONS)
        if( (TRUE == eventStatusRecPtr->errorStatusChanged) && (eventStatus == DEM_EVENT_STATUS_FAILED) ) {
            /* Test just failed. Need to set timestamp */
            setEventTimeStamp(eventStatusRecPtr);
        }
#endif
    }
}


/*
 * Procedure:   mergeEventStatusRec
 * Description: Update the occurrence counter of status, if not exist a new record is created
 */
#ifdef DEM_USE_MEMORY_FUNCTIONS
static boolean mergeEventStatusRec(const EventRecType *eventRec)
{
    EventStatusRecType *eventStatusRecPtr;
    const Dem_EventParameterType *eventParam;
    boolean statusChanged = FALSE;

    // Lookup event ID
    lookupEventStatusRec(eventRec->EventData.eventId, &eventStatusRecPtr);
    lookupEventIdParameter(eventRec->EventData.eventId, &eventParam);

    if (eventStatusRecPtr != NULL) {
        // Update occurrence counter.
        eventStatusRecPtr->occurrence += eventRec->EventData.occurrence;
        // Merge event status extended with stored
        // TEST_FAILED_SINCE_LAST_CLEAR should be set if set if set in either
        eventStatusRecPtr->eventStatusExtended |= (Dem_EventStatusExtendedType)(eventRec->EventData.eventStatusExtended & DEM_TEST_FAILED_SINCE_LAST_CLEAR);
        // DEM_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR should cleared if cleared in either
        if((eventRec->EventData.eventStatusExtended & eventStatusRecPtr->eventStatusExtended & DEM_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR) == 0u) {
            eventStatusRecPtr->eventStatusExtended &= (Dem_EventStatusExtendedType)~(Dem_EventStatusExtendedType)DEM_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR;
        }

        // DEM_PENDING_DTC and DEM_CONFIRMED_DTC should be set if set in either
        eventStatusRecPtr->eventStatusExtended |= (Dem_EventStatusExtendedType)(eventRec->EventData.eventStatusExtended & (DEM_PENDING_DTC | DEM_CONFIRMED_DTC));
        // DEM_WARNING_INDICATOR_REQUESTED should be set criteria fulfilled
#if defined(DEM_USE_INDICATORS)
        if( TRUE == warningIndicatorOnCriteriaFulfilled(eventParam) ) {
            eventStatusRecPtr->eventStatusExtended |= DEM_WARNING_INDICATOR_REQUESTED;
        }
#endif
#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
        // Update confirmation counter
        if( (DEM_FAILURE_CNTR_MAX - eventRec->EventData.failureCounter) < eventStatusRecPtr->failureCounter) {
            /* Would overflow */
            eventStatusRecPtr->failureCounter = DEM_FAILURE_CNTR_MAX;
        } else {
            eventStatusRecPtr->failureCounter += eventRec->EventData.failureCounter;
        }

        if( (NULL != eventParam) && (TRUE == faultConfirmationCriteriaFulfilled(eventParam, eventStatusRecPtr)) ) {
            eventStatusRecPtr->eventStatusExtended |= (Dem_EventStatusExtendedType)DEM_CONFIRMED_DTC;
        }
#endif
#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL)
        // Update confirmation counter
        if( (DEM_AGING_CNTR_MAX - eventRec->EventData.agingCounter) < eventStatusRecPtr->agingCounter) {
            /* Would overflow */
            eventStatusRecPtr->agingCounter = DEM_AGING_CNTR_MAX;
        } else {
            eventStatusRecPtr->agingCounter += eventRec->EventData.agingCounter;
        }

#endif
#if (DEM_TEST_FAILED_STORAGE == STD_ON)
        /* @req DEM387 */
        /* @req DEM388 */
        /* @req DEM525 */
        if( 0 != (eventStatusRecPtr->eventStatusExtended & DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE) ) {
            /* Test has not been completed this operation cycle. Set test failed bit as in stored */
            eventStatusRecPtr->eventStatusExtended |= (eventRec->EventData.eventStatusExtended & DEM_TEST_FAILED);
        }
#endif

#if (DEM_USE_TIMESTAMPS == STD_ON)
        if( 0u == (eventStatusRecPtr->eventStatusExtended & DEM_TEST_FAILED_THIS_OPERATION_CYCLE) ) {
            /* Test has not failed this operation cycle. Means that the that the timestamp
             * should be set to the one read from NvRam */
            eventStatusRecPtr->timeStamp = eventRec->EventData.timeStamp;
        }
#endif

        if( (eventStatusRecPtr->occurrence != eventRec->EventData.occurrence) ||
#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
            (eventStatusRecPtr->failureCounter != eventRec->EventData.failureCounter) ||
#endif
            (GET_STORED_STATUS_BITS(eventStatusRecPtr->eventStatusExtended) != GET_STORED_STATUS_BITS(eventRec->EventData.eventStatusExtended)) ) {
            statusChanged = TRUE;
        }

        if( 0 == (eventStatusRecPtr->eventStatusExtended & DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE) ) {
            /* Test was completed during preInit, means that the eventStatus was changed in some way */
            Dem_EventStatusExtendedType oldStatus = (Dem_EventStatusExtendedType)(GET_STORED_STATUS_BITS(eventRec->EventData.eventStatusExtended));
            notifyEventStatusChange(eventStatusRecPtr->eventParamRef, oldStatus, eventStatusRecPtr->eventStatusExtended);
        }
    }

    return statusChanged;
}

/*
 * Procedure:   resetEventStatusRec
 * Description: Reset the status record of "eventParam->eventId" from "eventStatusBuffer".
 */
static void resetEventStatusRec(const Dem_EventParameterType *eventParam)
{
    EventStatusRecType *eventStatusRecPtr;

    // Lookup event ID
    lookupEventStatusRec(eventParam->EventID, &eventStatusRecPtr);

    if (eventStatusRecPtr != NULL) {
        // Reset event record
        resetDebounceCounter(eventStatusRecPtr);
        eventStatusRecPtr->eventStatusExtended = DEM_DEFAULT_EVENT_STATUS;/** @req DEM385 *//** @req DEM440 */
        eventStatusRecPtr->errorStatusChanged = FALSE;
        eventStatusRecPtr->occurrence = 0;
#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
        eventStatusRecPtr->failureCounter = 0;
        eventStatusRecPtr->failedDuringFailureCycle = FALSE;
        eventStatusRecPtr->passedDuringFailureCycle = FALSE;
#endif
#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL)
        eventStatusRecPtr->agingCounter = 0;
        eventStatusRecPtr->failedDuringAgingCycle = FALSE;
        eventStatusRecPtr->passedDuringAgingCycle = FALSE;
#endif
        eventStatusRecPtr->timeStamp = 0;
    }

}
#endif
/*
 * Procedure:   getEventStatusRec
 * Description: Returns the status record of "eventId" in "eventStatusRec"
 */
static void getEventStatusRec(Dem_EventIdType eventId, EventStatusRecType *eventStatusRec)
{
    EventStatusRecType *eventStatusRecPtr;

    // Lookup event ID
    lookupEventStatusRec(eventId, &eventStatusRecPtr);

    if (eventStatusRecPtr != NULL) {
        // Copy the record
        memcpy(eventStatusRec, eventStatusRecPtr, sizeof(EventStatusRecType));
    }
    else {
        eventStatusRec->eventId = DEM_EVENT_ID_NULL;
    }
}

/**
 * Sets overflow indication for a specific memory
 * @param origin
 * @param overflow
 */
#ifdef DEM_USE_MEMORY_FUNCTIONS
static void setOverflowIndication(Dem_DTCOriginType origin, boolean overflow)
{
    switch (origin) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
            priMemOverflow = overflow;
            if(overflow != priMemEventBuffer[PRI_MEM_EVENT_BUFFER_ADMIN_INDEX].AdminData.overflow) {
                priMemEventBuffer[PRI_MEM_EVENT_BUFFER_ADMIN_INDEX].AdminData.magic = ADMIN_MAGIC;
                priMemEventBuffer[PRI_MEM_EVENT_BUFFER_ADMIN_INDEX].AdminData.overflow = overflow;
                /* Overflow not stored immediately */
                Dem_NvM_SetEventBlockChanged(origin, FALSE);
            }
#endif
            break;
        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
            secMemOverflow = overflow;
            if( overflow != secMemEventBuffer[SEC_MEM_EVENT_BUFFER_ADMIN_INDEX].AdminData.overflow ) {
                secMemEventBuffer[SEC_MEM_EVENT_BUFFER_ADMIN_INDEX].AdminData.magic = ADMIN_MAGIC;
                secMemEventBuffer[SEC_MEM_EVENT_BUFFER_ADMIN_INDEX].AdminData.overflow = overflow;
                /* Overflow not stored immediately */
                Dem_NvM_SetEventBlockChanged(origin, FALSE);
            }
#endif
            break;
        case DEM_DTC_ORIGIN_PERMANENT_MEMORY:
        case DEM_DTC_ORIGIN_MIRROR_MEMORY:
            // Not yet supported
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GLOBAL_ID, DEM_E_NOT_IMPLEMENTED_YET);
            break;
        default:
            break;
    }
}
#endif

/**
 * Returns the overflow indication for a specific memory
 * @param origin
 * @return E_OK: Operation successful, E_NOT_OK: Operation failed
 */
static Std_ReturnType getOverflowIndication(Dem_DTCOriginType origin, boolean *Overflow)
{
    Std_ReturnType ret = E_OK;
    switch (origin) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
            *Overflow = priMemOverflow;
#endif
            break;
        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
            *Overflow = secMemOverflow;
#endif
            break;
        case DEM_DTC_ORIGIN_PERMANENT_MEMORY:
        case DEM_DTC_ORIGIN_MIRROR_MEMORY:
        default:
            /* Not yet supported */
            ret = E_NOT_OK;
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GLOBAL_ID, DEM_E_NOT_IMPLEMENTED_YET);
            break;
    }
    return ret;
}

/**
 * Returns the occurence counter
 * @param eventParameter
 * @return
 */
static uint16 getEventOccurence(const Dem_EventParameterType *eventParameter)
{
    EventStatusRecType *eventStatusRec = NULL_PTR;
    uint16 occurence = 0u;
    if( NULL != eventParameter ) {
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
        if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParameter->CombinedDTCCID ) {
            const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[eventParameter->CombinedDTCCID];
            CombDTCCfg = &configSet->CombinedDTCConfig[eventParameter->CombinedDTCCID];
            for(uint16 evIdx = 0; evIdx < CombDTCCfg->DTCClassRef->NofEvents; evIdx++) {
                eventStatusRec = NULL_PTR;
                lookupEventStatusRec(CombDTCCfg->DTCClassRef->Events[evIdx], &eventStatusRec);
                if( (NULL_PTR != eventStatusRec) && (eventStatusRec->occurrence > occurence) ) {
                    occurence = eventStatusRec->occurrence;
                }
            }
        }
        else {
            lookupEventStatusRec(eventParameter->EventID, &eventStatusRec);
            if( NULL_PTR != eventStatusRec ) {
                occurence = eventStatusRec->occurrence;
            }
        }
#else
        lookupEventStatusRec(eventParameter->EventID, &eventStatusRec);
        if( NULL_PTR != eventStatusRec ) {
            occurence = eventStatusRec->occurrence;
        }
#endif
    }
    return occurence;
}

static sint8 getEventFDC(const Dem_EventParameterType *eventParameter, boolean maxFDC)
{
    EventStatusRecType *eventStatusRec = NULL_PTR;
    sint8 FDC = 0;
    if( NULL != eventParameter ) {
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
        if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParameter->CombinedDTCCID ) {
            const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[eventParameter->CombinedDTCCID];
            sint8 tempFDC;
            for(uint16 evIdx = 0; evIdx < CombDTCCfg->DTCClassRef->NofEvents; evIdx++) {
                eventStatusRec = NULL_PTR;
                lookupEventStatusRec(CombDTCCfg->DTCClassRef->Events[evIdx], &eventStatusRec);
                if( NULL_PTR != eventStatusRec ) {
                    tempFDC = (TRUE == maxFDC) ? eventStatusRec->maxUDSFdc : eventStatusRec->UDSFdc;
                    if( tempFDC > FDC ) {
                        FDC = tempFDC;
                    }
                }
            }
        }
        else {
            lookupEventStatusRec(eventParameter->EventID, &eventStatusRec);
            if( NULL_PTR != eventStatusRec ) {
                FDC = (TRUE == maxFDC) ? eventStatusRec->maxUDSFdc : eventStatusRec->UDSFdc;
            }
        }
#else
        lookupEventStatusRec(eventParameter->EventID, &eventStatusRec);
        if( NULL_PTR != eventStatusRec ) {
            FDC = (TRUE == maxFDC) ? eventStatusRec->maxUDSFdc : eventStatusRec->UDSFdc;
        }
#endif
    }
    return FDC;
}

#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL)
/**
 * Gets the aging counter
 * @param eventParameter
 * @return
 */
uint8 getEventAgingCntr(const Dem_EventParameterType *eventParameter)
{
    /* IMPROVEMENT: External aging.. */
    EventStatusRecType *eventStatusRec = NULL_PTR;
    uint8 agingCnt = 0u;
    if( NULL != eventParameter ) {
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
        if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParameter->CombinedDTCCID ) {
            const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[eventParameter->CombinedDTCCID];
            agingCnt = 0xFFu;
            boolean cntrValid = FALSE;
            for(uint16 evIdx = 0; evIdx < CombDTCCfg->DTCClassRef->NofEvents; evIdx++) {
                eventStatusRec = NULL_PTR;
                lookupEventStatusRec(CombDTCCfg->DTCClassRef->Events[evIdx], &eventStatusRec);
                if( (NULL_PTR != eventStatusRec) && (TRUE == eventStatusRec->eventParamRef->EventClass->AgingAllowed) && (0u != (eventStatusRec->eventStatusExtended & DEM_CONFIRMED_DTC)) ) {
                    if( eventStatusRec->agingCounter < agingCnt ) {
                        agingCnt = eventStatusRec->agingCounter;
                    }
                    cntrValid = TRUE;
                }
            }
            if( FALSE == cntrValid ) {
                /* @req DEM646 */
                agingCnt = 0u;
            }
        }
        else {
            lookupEventStatusRec(eventParameter->EventID, &eventStatusRec);
            if( (NULL_PTR != eventStatusRec) && (TRUE == eventParameter->EventClass->AgingAllowed) ) {
                agingCnt = eventStatusRec->agingCounter;
            }
        }
#else
        lookupEventStatusRec(eventParameter->EventID, &eventStatusRec);
        if( (NULL_PTR != eventStatusRec) && (TRUE == eventParameter->EventClass->AgingAllowed) ) {
            agingCnt = eventStatusRec->agingCounter;
        }
#endif
    }
    return agingCnt;
}
#endif

#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
/**
 * Gets the failure counter
 * @param eventParameter
 * @return
 */
static uint8 getEventFailureCnt(const Dem_EventParameterType *eventParameter)
{
    EventStatusRecType *eventStatusRec = NULL_PTR;
    uint8 failCnt = 0u;
    if( NULL != eventParameter ) {
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
        if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParameter->CombinedDTCCID ) {
            const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[eventParameter->CombinedDTCCID];
            for(uint16 evIdx = 0; evIdx < CombDTCCfg->DTCClassRef->NofEvents; evIdx++) {
                eventStatusRec = NULL_PTR;
                lookupEventStatusRec(CombDTCCfg->DTCClassRef->Events[evIdx], &eventStatusRec);
                if( (NULL_PTR != eventStatusRec) && (eventStatusRec->failureCounter > failCnt) ) {
                    failCnt = eventStatusRec->failureCounter;
                }
            }
        }
        else {
            lookupEventStatusRec(eventParameter->EventID, &eventStatusRec);
            if( NULL_PTR != eventStatusRec ) {
                failCnt = eventStatusRec->failureCounter;
            }
        }
#else
        lookupEventStatusRec(eventParameter->EventID, &eventStatusRec);
        if( NULL_PTR != eventStatusRec ) {
            failCnt = eventStatusRec->failureCounter;
        }
#endif
    }
    return failCnt;
}
#endif

/**
 * Reads internal element
 * @param eventParameter
 * @param elementType
 * @param buf
 * @param size
 */
static void getInternalElement( const Dem_EventParameterType *eventParameter, Dem_InternalDataElementType elementType, uint8* buf, uint16 size )
{
    /* !req DEM592 *//* SIGNIFICANCE not supported */
    EventStatusRecType *eventStatusRec;
    lookupEventStatusRec(eventParameter->EventID, &eventStatusRec);
    uint16 occurrence;

    if( (DEM_EVENT_ID_NULL != eventStatusRec->eventId) && (size > 0) ) {
        memset(buf, 0, (size_t)size);
        switch(elementType) {
            case DEM_OCCCTR:
                /* @req DEM471 */
                occurrence = getEventOccurence(eventParameter);
                if(1 == size) {
                    buf[0] = (uint8)MIN(occurrence, 0xFF);
                } else  {
                    buf[size - 2] = (uint8)((occurrence & 0xff00u) >> 8u);
                    buf[size - 1] = (uint8)(occurrence & 0xffu);
                }
                break;
            case DEM_FAULTDETCTR:
                /* @req OEM_DEM_10185 */
                buf[size - 1] = (uint8)getEventFDC(eventParameter, FALSE);
                break;
            case DEM_MAXFAULTDETCTR:
                buf[size - 1] = (uint8)getEventFDC(eventParameter, TRUE);
                break;
            case DEM_OVFLIND:
            {
                /* @req DEM473 */
                boolean ovflw = FALSE;
                if( E_OK == getOverflowIndication(eventParameter->EventClass->EventDestination, &ovflw) ) {
                    buf[size - 1] = (TRUE == ovflw) ? 1 : 0;
                } else {
                    buf[size - 1] = 0;
                }
            }
                break;
#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL)
            case DEM_AGINGCTR:
                /* @req DEM472 *//* !req DEM644 *//* !req DEM647 */
                /* @req DEM646 */
                /* set to 0 by memset above */
                buf[size - 1] = getEventAgingCntr(eventParameter);
                break;
#endif
#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
            case DEM_CONFIRMATIONCNTR:
                buf[size - 1] = getEventFailureCnt(eventParameter);
                break;
#endif
            default:
#if defined(USE_DEM_EXTENSION)
                Dem_Extension_GetExtendedDataInternalElement(eventParameter->EventID, elementType, buf, size);
#endif
                break;
        }
    }
}

#if defined(USE_DEM_EXTENSION)
/*
 * Procedure:   lookupDtcEvent
 * Description: Returns TRUE if the DTC was found and "eventStatusRec" points
 *              to the event record found.
 */
/* @req 4.2.2/DEM_00915 */
boolean Dem_LookupEventOfUdsDTC(uint32 dtc, EventStatusRecType **eventStatusRec)
{
    boolean dtcFound = FALSE;
    uint16 i;

    *eventStatusRec = NULL;

    for (i = 0; (i < DEM_MAX_NUMBER_EVENT) && (dtcFound == FALSE); i++) {
        if (eventStatusBuffer[i].eventId != DEM_EVENT_ID_NULL) {
            if (eventStatusBuffer[i].eventParamRef->DTCClassRef != NULL) {

                /* Check DTC. Ignore suppressed DTCs *//* @req DEM587 */
                if ((eventStatusBuffer[i].eventParamRef->DTCClassRef->DTCRef->UDSDTC == dtc) &&
                        (DTCIsAvailable(eventStatusBuffer[i].eventParamRef->DTCClassRef))) {
                    *eventStatusRec = &eventStatusBuffer[i];
                    dtcFound = TRUE;
                }
            }
        }
    }

    return dtcFound;
}
#endif

/**
 * Function for finding configuration of UDS DTC.
 * @param dtc
 * @param DTCClass
 * @return
 */
static boolean LookupUdsDTC(uint32 dtc, const Dem_DTCClassType **DTCClass)
{
    boolean DTCFound = FALSE;
    const Dem_DTCClassType *DTCClassPtr = configSet->DTCClass;
    while( FALSE == DTCClassPtr->Arc_EOL ) {
        if( (DTCClassPtr->DTCRef->UDSDTC == dtc) ) {
            *DTCClass = DTCClassPtr;
            DTCFound = DTCIsAvailable(DTCClassPtr);
            break;
        }
        DTCClassPtr++;
    }
    return DTCFound;
}

/**
 * Gets the UDS status of a DTC in a specific origin.
 * @param DTCClass
 * @param DTCOrigin
 * @param status
 * @return
 */
static Dem_ReturnGetStatusOfDTCType GetDTCUDSStatus(const Dem_DTCClassType *DTCClass, Dem_DTCOriginType DTCOrigin, Dem_EventStatusExtendedType *status)
{
    Dem_ReturnGetStatusOfDTCType ret = DEM_STATUS_OK;
    EventStatusRecType *eventStatusRecPtr;
    uint8 mask = 0xFFU;
    uint16 nofEventsInOrigin = 0u;
    Dem_EventStatusExtendedType temp = 0u;

    for(uint16 i = 0; (i < DTCClass->NofEvents) && (DEM_STATUS_OK == ret); i++) {
        eventStatusRecPtr = NULL_PTR;
        lookupEventStatusRec(DTCClass->Events[i], &eventStatusRecPtr);
        if( NULL_PTR != eventStatusRecPtr ) {
            /* Event found for this DTC */
            if( TRUE == checkDtcOrigin(DTCOrigin,eventStatusRecPtr->eventParamRef, TRUE) ) {
                /* NOTE: Should the availability mask be used here? */
                /* @req DEM059 */
                /* @req DEM441 */
                if( TRUE == eventStatusRecPtr->isAvailable ) {
                    temp |= eventStatusRecPtr->eventStatusExtended;
                    nofEventsInOrigin++;
                }
            }
            else {
                /* Event not available in DTCOrigin */
                /** @req DEM172 */
            }
        }
        else {
            /* This is unexpected. Fail operation. */
            ret = DEM_STATUS_FAILED;
        }
    }

    if( DEM_STATUS_OK == ret ) {
        if( 0u == nofEventsInOrigin ) {
            /* No events where found in the origin. */
            ret = DEM_STATUS_WRONG_DTCORIGIN;
        }
        else if( (DTCClass->NofEvents > 1u) && (0u != nofEventsInOrigin) ) {
            /* This is a combined DTC. Bits have already been OR-ed above. Now we should and bits. */
            /* @req DEM441 */
            mask = ((temp & (1u << 5u)) >> 1u) | ((temp & (1u << 1u)) << 5u);
            mask = (uint8)((~mask) & 0xFFu);
        }
        else {
            /* One event found. Do nomasking. */
        }
    }
    *status = (temp & mask);
    return ret;
}
/*
 * Procedure:   matchEventWithDtcFilter
 * Description: Returns TRUE if the event pointed by "event" fulfill
 *              the "dtcFilter" global filter settings.
 */
/**
 * Checks if a DTC matches the current filter settings. If it matches the UDS status is returned.
 * @param DTCClass
 * @param UDSStatus
 * @return
 */
static boolean matchDTCWithDtcFilter(const Dem_DTCClassType *DTCClass, Dem_EventStatusExtendedType *UDSStatus)
{
    boolean dtcMatch = FALSE;
    Dem_EventStatusExtendedType DTCStatus;

    if( DEM_STATUS_OK == GetDTCUDSStatus(DTCClass, dtcFilter.dtcOrigin, &DTCStatus) ) {
        /* Status available in origin. */
        if ( TRUE == checkDtcStatusMask(dtcFilter.dtcStatusMask, DTCStatus) ) {
            /* Check the DTC kind. */
            if( (dtcFilter.dtcKind == DEM_DTC_KIND_ALL_DTCS) || (DEM_NO_DTC != DTCClass->DTCRef->OBDDTC) ) {

                /* Check severity */
                if ((dtcFilter.filterWithSeverity == DEM_FILTER_WITH_SEVERITY_NO) ||
                    ((dtcFilter.filterWithSeverity == DEM_FILTER_WITH_SEVERITY_YES) && (TRUE == checkDtcSeverityMask(dtcFilter.dtcSeverityMask, DTCClass)))) {

                    /* Check fault detection counter. No support for DEM_FILTER_FOR_FDC_YES */
                    if( dtcFilter.filterForFaultDetectionCounter == DEM_FILTER_FOR_FDC_NO) {

                        /* Check the DTC */
                        if( (TRUE == DTCIsAvailable(DTCClass)) && (TRUE == DTCISAvailableOnFormat(DTCClass, dtcFilter.dtcFormat)) ) {
                            dtcMatch = TRUE;
                            *UDSStatus = DTCStatus;
                        }
                    }
                }
            }
        }
    }

    return dtcMatch;
}

/* Function: eventDTCRecordDataUpdateDisabled
 * Description: Checks if update of event related data (extended data or freezeframe data) has been disabled
 */
static boolean eventDTCRecordDataUpdateDisabled(const Dem_EventParameterType *eventParam)
{
    boolean disabled = FALSE;
    if( (NO_DTC_DISABLED != DTCRecordDisabled.DTC) && /* There is a disabled DTC */
            (NULL != eventParam) && /* Argument ok */
            (NULL != eventParam->DTCClassRef) && /* Event has a DTC */
            (DEM_NO_DTC != eventParam->DTCClassRef->DTCRef->UDSDTC) && /* And a DTC on UDS format */
            (DTCRecordDisabled.DTC == eventParam->DTCClassRef->DTCRef->UDSDTC) && /* The disabled DTC is the DTC of the event */
            (DTCRecordDisabled.Origin == eventParam->EventClass->EventDestination)) { /* The disabled origin is the origin of the event */
        /* DTC record update for this event is disabled */
        disabled = TRUE;
    }
    return disabled;
}
#if (DEM_USE_TIMESTAMPS == STD_ON)
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM) || \
    ((DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_SEC_MEM) || \
    (DEM_FF_DATA_IN_PRE_INIT)
/*
 * Functions for rearranging timestamps
 *
 * */
/**
 * Sorts entries in freeze frame buffers. Oldest freeze frame first, etc. Sets a new timestamp (0-"nof ff entries")
 * Returns the timestamp to use for next stored FF.
 * @param timeStamp, pointer to timestamp used by caller. Shall be set to the next timestamp to use.
 */
#if (DEM_UNIT_TEST == STD_ON)
void rearrangeFreezeFrameTimeStamp(uint32 *timeStamp)
#else
static void rearrangeFreezeFrameTimeStamp(uint32 *timeStamp )
#endif
{
    FreezeFrameRecType temp;
    uint32 i;
    uint32 j = 0;
    uint32 k = 0;
    uint32 bufferIndex;

    /* These two arrays are looped below must have the same size */
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM && (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_SEC_MEM
    uint32 ffBufferSizes[2] = {DEM_MAX_NUMBER_FF_DATA_PRI_MEM, DEM_MAX_NUMBER_FF_DATA_SEC_MEM};
    FreezeFrameRecType* ffBuffers[2] = {priMemFreezeFrameBuffer, secMemFreezeFrameBuffer};
    uint32 nofSupportedDestinations = 2;
#elif (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON && DEM_FF_DATA_IN_PRI_MEM)
    uint32 ffBufferSizes[1] = {DEM_MAX_NUMBER_FF_DATA_PRI_MEM};
    FreezeFrameRecType* ffBuffers[1] = {priMemFreezeFrameBuffer};
    uint32 nofSupportedDestinations = 1;
#elif (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_SEC_MEM
    uint32 ffBufferSizes[1] = {DEM_MAX_NUMBER_FF_DATA_SEC_MEM};
    FreezeFrameRecType* ffBuffers[1] = {secMemFreezeFrameBuffer};
    uint32 nofSupportedDestinations = 1;
#else
    uint32 ffBufferSizes[1] = {0};
    FreezeFrameRecType* ffBuffers[1] = {NULL};
    uint32 nofSupportedDestinations = 0;
#endif

    for (bufferIndex = 0; bufferIndex < nofSupportedDestinations; bufferIndex++) {

        FreezeFrameRecType* ffBuffer = ffBuffers[bufferIndex];
        uint32 ffBufferSize = ffBufferSizes[bufferIndex];

        /* Bubble sort:rearrange ffBuffer from little to big */
        for(i = 0; i < ffBufferSize; i++){
            if(ffBuffer[i].eventId != DEM_EVENT_ID_NULL){
                for( j = ffBufferSize - 1; j > i; j--){
                    if(ffBuffer[j].eventId != DEM_EVENT_ID_NULL){
                        if(ffBuffer[i].timeStamp > ffBuffer[j].timeStamp){
                            //exchange buffer data
                            memcpy(&temp,&ffBuffer[i],sizeof(FreezeFrameRecType));
                            memcpy(&ffBuffer[i],&ffBuffer[j],sizeof(FreezeFrameRecType));
                            memcpy(&ffBuffer[j],&temp,sizeof(FreezeFrameRecType));
                        }

                    }

                }
                ffBuffer[i].timeStamp = k++;
            }
        }
    }

    /* update the current timeStamp */
    *timeStamp = k;

}
#endif
#if (DEM_EXT_DATA_IN_PRE_INIT || DEM_EXT_DATA_IN_PRI_MEM || DEM_EXT_DATA_IN_SEC_MEM ) && defined(DEM_USE_MEMORY_FUNCTIONS)
/**
 * Sorts entries in extended buffers. Oldest extended data first, etc. Sets a new timestamp (0-"nof ff entries")
 * Returns the timestamp to use for next stored extended data.
 * @param timeStamp, pointer to timestamp used by caller. Shall be set to the next timestamp to use.
 */
#if (DEM_UNIT_TEST == STD_ON)
void rearrangeExtDataTimeStamp(uint32 *timeStamp)
#else
static void rearrangeExtDataTimeStamp(uint32 *timeStamp)
#endif
{
    ExtDataRecType temp;
    uint32 i;
    uint32 j = 0;
    uint32 k = 0;
    uint32 bufferIndex;

    /* These two arrays are looped below must have the same size */
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_EXT_DATA_IN_PRI_MEM && (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_EXT_DATA_IN_SEC_MEM
    uint32 extBufferSizes[2] = {DEM_MAX_NUMBER_EXT_DATA_PRI_MEM, DEM_MAX_NUMBER_EXT_DATA_SEC_MEM};
    ExtDataRecType* extBuffers[2] = {priMemExtDataBuffer, secMemExtDataBuffer};
    uint32 nofDestinations = 2;
#elif (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON && DEM_EXT_DATA_IN_PRI_MEM)
    uint32 extBufferSizes[1] = {DEM_MAX_NUMBER_EXT_DATA_PRI_MEM};
    ExtDataRecType* extBuffers[1] = {priMemExtDataBuffer};
    uint32 nofDestinations = 1;
#elif (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON && DEM_EXT_DATA_IN_SEC_MEM)
    uint32 extBufferSizes[1] = {DEM_MAX_NUMBER_EXT_DATA_SEC_MEM};
    ExtDataRecType* extBuffers[1] = {secMemExtDataBuffer};
    uint32 nofDestinations = 1;
#else
    uint32 extBufferSizes[1] = {0};
    ExtDataRecType* extBuffers[1] = {NULL};
    uint32 nofDestinations = 0;
#endif

    for (bufferIndex = 0; bufferIndex < nofDestinations; bufferIndex++) {

        ExtDataRecType* extBuffer = extBuffers[bufferIndex];
        uint32 extBufferSize = extBufferSizes[bufferIndex];

        /* Bubble sort:rearrange Buffer from little to big */
        for( i = 0; i < extBufferSize; i++ ){
            if( DEM_EVENT_ID_NULL != extBuffer[i].eventId ){
                for( j = extBufferSize - 1; j > i; j-- ){
                    if( DEM_EVENT_ID_NULL != extBuffer[j].eventId ){
                        if( extBuffer[i].timeStamp > extBuffer[j].timeStamp ){
                            //exchange buffer data
                            memcpy(&temp, &extBuffer[i], sizeof(ExtDataRecType));
                            memcpy(&extBuffer[i], &extBuffer[j], sizeof(ExtDataRecType));
                            memcpy(&extBuffer[j], &temp, sizeof(ExtDataRecType));
                        }
                    }
                }
                extBuffer[i].timeStamp = k++;
            }
        }
    }

    /* update the current timeStamp */
    *timeStamp = k;
}
#endif

#if defined(DEM_USE_MEMORY_FUNCTIONS)
static void initCurrentEventTimeStamp(uint32 *timeStampPtr)
{
    uint32 highestTimeStamp = 0;

    /* Rearrange events */
    rearrangeEventTimeStamp(timeStampPtr);

    for (uint16 i = 0; i < DEM_MAX_NUMBER_EVENT; i++){
        if( DEM_EVENT_ID_NULL != eventStatusBuffer[i].eventId ){
            eventStatusBuffer[i].timeStamp += *timeStampPtr;
            if( eventStatusBuffer[i].timeStamp > highestTimeStamp ) {
                highestTimeStamp = eventStatusBuffer[i].timeStamp;
            }
        }
    }
    *timeStampPtr = highestTimeStamp + 1;
}

/*
 * Functions for initializing timestamps
 * */
static void initCurrentFreezeFrameTimeStamp(uint32 *timeStampPtr)
{
#if ( DEM_FF_DATA_IN_PRE_INIT )
    uint32 highestTimeStamp = 0;

    /* Rearrange freeze frames */
    rearrangeFreezeFrameTimeStamp(timeStampPtr);

    for (uint16 i = 0; i<DEM_MAX_NUMBER_FF_DATA_PRE_INIT; i++){
        if(preInitFreezeFrameBuffer[i].eventId != DEM_EVENT_ID_NULL){
            preInitFreezeFrameBuffer[i].timeStamp += *timeStampPtr;
            if( preInitFreezeFrameBuffer[i].timeStamp > highestTimeStamp ) {
                highestTimeStamp = preInitFreezeFrameBuffer[i].timeStamp;
            }
        }
    }
    *timeStampPtr = highestTimeStamp + 1;
#endif
}

static void initCurrentExtDataTimeStamp(uint32 *timeStampPtr)
{
#if ( DEM_EXT_DATA_IN_PRE_INIT && DEM_FF_DATA_IN_PRE_INIT )
    uint32 highestTimeStamp = 0;

    /* Rearrange extended data in primary memory */
    rearrangeExtDataTimeStamp(timeStampPtr);

    /* Increment the timestamps in the pre init ext data buffer */
    for (uint16 i = 0; i < DEM_MAX_NUMBER_EXT_DATA_PRE_INIT; i++){
        if( DEM_EVENT_ID_NULL != preInitExtDataBuffer[i].eventId ){
            preInitExtDataBuffer[i].timeStamp += *timeStampPtr;
            if( preInitExtDataBuffer[i].timeStamp > highestTimeStamp ) {
                highestTimeStamp = preInitExtDataBuffer[i].timeStamp;
            }
        }
    }
    *timeStampPtr = highestTimeStamp + 1;
#else
    (void)timeStampPtr;
#endif
}

#if (DEM_UNIT_TEST == STD_ON)
void rearrangeEventTimeStamp(uint32 *timeStamp)
#else
static void rearrangeEventTimeStamp(uint32 *timeStamp)
#endif
{
    FreezeFrameRecType temp;
    uint32 bufferIndex;
    uint32 i;
    uint32 j = 0;
    uint32 k = 0;

    /* These two arrays are looped below must have the same size */
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
    uint32 eventBufferSizes[2] = {DEM_MAX_NUMBER_EVENT_ENTRY_PRI, DEM_MAX_NUMBER_EVENT_ENTRY_SEC};
    EventRecType* eventBuffers[2] = {priMemEventBuffer, secMemEventBuffer};
    uint32 nofDestinations = 2;
#elif (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
    uint32 eventBufferSizes[1] = {DEM_MAX_NUMBER_EVENT_ENTRY_PRI};
    EventRecType* eventBuffers[1] = {priMemEventBuffer};
    uint32 nofDestinations = 1;
#elif (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
    uint32 eventBufferSizes[1] = {DEM_MAX_NUMBER_EVENT_ENTRY_SEC};
    EventRecType* eventBuffers[1] = {secMemEventBuffer};
    uint32 nofDestinations = 1;
#else
    uint32 eventBufferSizes[1] = {0};
    EventRecType* eventBuffers[1] = {NULL};
    uint32 nofDestinations = 0;
#endif

    for (bufferIndex = 0; bufferIndex < nofDestinations; bufferIndex++) {

        EventRecType* eventBuffer = eventBuffers[bufferIndex];
        uint32 eventBufferSize = eventBufferSizes[bufferIndex];

        /* Bubble sort:rearrange event buffer from little to big */
        for( i = 0; i < eventBufferSize; i++ ){
            if( DEM_EVENT_ID_NULL != eventBuffer[i].EventData.eventId ){
                for( j = (eventBufferSize - 1); j > i; j-- ) {
                    if( DEM_EVENT_ID_NULL != eventBuffer[j].EventData.eventId ) {
                        if( eventBuffer[i].EventData.timeStamp > eventBuffer[j].EventData.timeStamp ) {
                            //exchange buffer data
                            memcpy(&temp, &eventBuffer[i].EventData, sizeof(EventRecType));
                            memcpy(&eventBuffer[i].EventData, &eventBuffer[j].EventData, sizeof(EventRecType));
                            memcpy(&eventBuffer[j].EventData, &temp, sizeof(EventRecType));
                        }
                    }
                }
                eventBuffer[i].EventData.timeStamp = k++;
            }
        }
    }

    /* update the current timeStamp */
    *timeStamp = k;
}
#endif /* DEM_USE_MEMORY_FUNCTIONS */
#endif /* (DEM_USE_TIMESTAMPS == STD_ON) */



#if (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON) && defined (DEM_USE_MEMORY_FUNCTIONS)
#if defined(DEM_DISPLACEMENT_PROCESSING_DEM_INTERNAL)
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM && (DEM_OBD_DISPLACEMENT_SUPPORT == STD_ON))
/**
 * Check if an event currently hold the OBD FF
 * @param eventParam
 * @return TRUE: Event holds OBD FF. FALSE: event does NOT hold the OBD FF
 */
static boolean eventHoldsOBDFF(const Dem_EventParameterType *eventParam)
{
    boolean ret = FALSE;
    Dem_EventIdType idToFind;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        idToFind = TO_COMBINED_EVENT_ID(eventParam->CombinedDTCCID);
    }
    else {
        idToFind = eventParam->EventID;
    }
#else
    idToFind = eventParam->EventID;
#endif
    for( uint32 i = 0; (i < DEM_MAX_NUMBER_FF_DATA_PRI_MEM) && (FALSE == ret); i++ ) {
        if( (idToFind == priMemFreezeFrameBuffer[i].eventId) && (DEM_FREEZE_FRAME_OBD == priMemFreezeFrameBuffer[i].kind) ) {
            ret = TRUE;
        }
    }
    return ret;
}
#endif /* ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM && (DEM_OBD_DISPLACEMENT_SUPPORT == STD_ON)) */

/**
 * Checks if an event is an event may be displaced by another
 * @param candidateEventParam
 * @param eventParam
 * @return TRUE: Event may be displaced, FALSE: event may NOT be displaced
 */
static boolean obdEventDisplacementProhibited(const Dem_EventParameterType *candidateEventParam, const Dem_EventParameterType *eventParam)
{
#if (DEM_OBD_DISPLACEMENT_SUPPORT == STD_ON)
    Dem_EventStatusExtendedType evtStatus;
    boolean prohibited = FALSE;
    (void)getEventStatus(candidateEventParam->EventID, &evtStatus);
    if( (TRUE == eventIsEmissionRelated(candidateEventParam)) &&
        (
#if defined(DEM_USE_INDICATORS)
                (TRUE == eventActivatesMIL(candidateEventParam)) ||
#endif
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM)
                ((TRUE == eventHoldsOBDFF(candidateEventParam)) && (candidateEventParam->EventClass->EventPriority <= eventParam->EventClass->EventPriority)) ||
#endif
        (0u != (evtStatus & DEM_PENDING_DTC))) ) {
        prohibited = TRUE;
    }
    return prohibited;
#else
    return FALSE;
#endif
}

/**
 * Returns whether event is considered passive or not
 * @param eventId
 * @return TRUE: Event passive, FALSE: Event NOT passive
 */
static boolean getEventPassive(Dem_EventIdType eventId)
{
    boolean eventPassive = FALSE;
    Dem_EventStatusExtendedType eventStatus = 0u;
    boolean statusFound = FALSE;

#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( IS_COMBINED_EVENT_ID(eventId) ) {
        const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(eventId)];
        CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(eventId)];
        if( DEM_STATUS_OK == GetDTCUDSStatus(CombDTCCfg->DTCClassRef, CombDTCCfg->MemoryDestination, &eventStatus) ) {
            statusFound = TRUE;
        }
    }
    else {
        const Dem_EventParameterType *eventParam = NULL;
        lookupEventIdParameter(eventId, &eventParam);
        if( NULL != eventParam ) {
            if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
                if( DEM_STATUS_OK == GetDTCUDSStatus(eventParam->DTCClassRef, eventParam->EventClass->EventDestination, &eventStatus) ) {
                    statusFound = TRUE;
                }
            }
            else {
                if( E_OK == getEventStatus(eventId, &eventStatus) ) {
                    statusFound = TRUE;
                }
            }
        }
    }
#else
    if( E_OK == getEventStatus(eventId, &eventStatus) ) {
        statusFound = TRUE;
    }
#endif
    if( TRUE == statusFound ) {
#if (DEM_TEST_FAILED_STORAGE == STD_ON)
        eventPassive = (0 == (eventStatus & DEM_TEST_FAILED));
#else
        eventPassive = (0 == (eventStatus & (DEM_TEST_FAILED | DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE)));
#endif
#if defined(CFG_DEM_TREAT_CONFIRMED_EVENTS_AS_ACTIVE)
        eventPassive = (0 != (eventStatus & DEM_CONFIRMED_DTC)) ? FALSE : eventPassive;
#endif
    }
    return eventPassive;

}

/**
 * Return the priority of an event
 * @param eventId
 * @return Priority of event
 */
static uint8 getEventPriority(Dem_EventIdType eventId)
{
    uint8 priority = 0xFF;

    const Dem_EventParameterType *eventParam = NULL;

#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( IS_COMBINED_EVENT_ID(eventId) ) {
        const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(eventId)];
        CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(eventId)];
        priority = CombDTCCfg->Priority;
    }
    else {
        lookupEventIdParameter(eventId, &eventParam);
        if( NULL != eventParam ) {
            if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
                const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[eventParam->CombinedDTCCID];
                priority = CombDTCCfg->Priority;
            }
            else {
                priority = eventParam->EventClass->EventPriority;
            }
        }
    }
#else
    lookupEventIdParameter(eventId, &eventParam);
    if( NULL != eventParam ) {
        priority = eventParam->EventClass->EventPriority;
    }
#endif
    return priority;
}

#if ( DEM_FF_DATA_IN_PRI_MEM || DEM_FF_DATA_IN_SEC_MEM || DEM_FF_DATA_IN_PRE_INIT)
static Std_ReturnType getFFEventForDisplacement(const Dem_EventParameterType *eventParam, const FreezeFrameRecType *ffBuffer, uint32 bufferSize, Dem_EventIdType *eventToRemove)
{
    /* See figure 25 in ASR 4.0.3 */
    /* @req DEM403 */
    /* @req DEM404 */
    /* @req DEM405 */
    /* @req DEM406 */
    /* IMPROVEMENT: How do we handle the case when it is an OBD freeze frame that should be stored?
     * Should they be handled in the same way as "normal" freeze frames? */
    Std_ReturnType ret = E_NOT_OK;
    uint32 removeCandidateIndex = 0;
    uint32 oldestPassive_TimeStamp = DEM_MAX_TIMESTAMP_FOR_REARRANGEMENT;
    uint32 oldestActive_TimeStamp = DEM_MAX_TIMESTAMP_FOR_REARRANGEMENT;
    uint8 eventPrio = 0xFF;
    boolean eventPassive = FALSE;
    boolean passiveCandidateFound = FALSE;
    boolean activeCandidateFound = FALSE;
    boolean eventDataRemovalProhibited = FALSE;
    boolean obdProhibitsRemoval = FALSE;
    const Dem_EventParameterType *candidateEventParam;
    for( uint32 i = 0; i < bufferSize; i++ ) {
        if( (DEM_EVENT_ID_NULL != ffBuffer[i].eventId) && (eventParam->EventID != ffBuffer[i].eventId) && (DEM_FREEZE_FRAME_OBD != ffBuffer[i].kind) ) {
            candidateEventParam = NULL;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
            /* Event id may be a combined id. */
            if( IS_COMBINED_EVENT_ID(ffBuffer[i].eventId) ) {
                const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(ffBuffer[i].eventId)];
                CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(ffBuffer[i].eventId)];
                /* Just grab the first event for this DTC. */
                lookupEventIdParameter(CombDTCCfg->DTCClassRef->Events[0u], &candidateEventParam);
            }
            else {
                lookupEventIdParameter(ffBuffer[i].eventId, &candidateEventParam);
            }
#else
            lookupEventIdParameter(ffBuffer[i].eventId, &candidateEventParam);
#endif
            if(NO_DTC_DISABLED != DTCRecordDisabled.DTC) {
                eventDataRemovalProhibited = eventDTCRecordDataUpdateDisabled(candidateEventParam);
            } else {
                eventDataRemovalProhibited = FALSE;
            }
            obdProhibitsRemoval = obdEventDisplacementProhibited(candidateEventParam, eventParam);
            if( (FALSE == eventDataRemovalProhibited) && (FALSE == obdProhibitsRemoval) ) {
                /* Check if priority if the event is higher or equal (low numerical value of priority means high priority..) *//* @req DEM383 */
                eventPrio = getEventPriority(ffBuffer[i].eventId);
                eventPassive = getEventPassive(ffBuffer[i].eventId);
                /* IMPROVEMENT: Remove the one with lowest priority? */
                if( (eventParam->EventClass->EventPriority <= eventPrio) && (TRUE == eventPassive) && (oldestPassive_TimeStamp > ffBuffer[i].timeStamp) ) {
                    /* This event has lower or equal priority to the reported event, it is passive
                     * and it is the oldest currently found. A candidate for removal. */
                    oldestPassive_TimeStamp = ffBuffer[i].timeStamp;
                    removeCandidateIndex = i;
                    passiveCandidateFound = TRUE;

                }
                if( FALSE == passiveCandidateFound ) {
                    /* Currently, a passive event with lower or equal priority has not been found.
                     * Check if the priority is less than for the reported event. Store the oldest. */
                    if( (eventParam->EventClass->EventPriority < eventPrio) &&  (oldestActive_TimeStamp > ffBuffer[i].timeStamp) ) {
                        oldestActive_TimeStamp = ffBuffer[i].timeStamp;
                        removeCandidateIndex = i;
                        activeCandidateFound = TRUE;
                    }
                }
            }
        }
    }

    if( (TRUE == passiveCandidateFound) || (TRUE == activeCandidateFound) ) {
        *eventToRemove = ffBuffer[removeCandidateIndex].eventId;
        ret = E_OK;
    }
    return ret;

}
#endif /* DEM_FF_DATA_IN_PRI_MEM || DEM_FF_DATA_IN_SEC_MEM || DEM_FF_DATA_IN_PRE_INIT */

#if (DEM_EXT_DATA_IN_PRE_INIT || DEM_EXT_DATA_IN_PRI_MEM || DEM_EXT_DATA_IN_SEC_MEM ) && defined(DEM_USE_MEMORY_FUNCTIONS)
static Std_ReturnType getExtDataEventForDisplacement(const Dem_EventParameterType *eventParam, const ExtDataRecType *extDataBuffer, uint32 bufferSize, Dem_EventIdType *eventToRemove)
{
    /* See figure 25 in ASR 4.0.3 */
    /* @req DEM403 */
    /* @req DEM404 */
    /* @req DEM405 */
    /* @req DEM406 */
    Std_ReturnType ret = E_NOT_OK;
    uint32 removeCandidateIndex = 0;
    uint32 oldestPassive_TimeStamp = DEM_MAX_TIMESTAMP_FOR_REARRANGEMENT;
    uint32 oldestActive_TimeStamp = DEM_MAX_TIMESTAMP_FOR_REARRANGEMENT;
    uint8 eventPrio = 0xFF;
    boolean eventPassive = FALSE;
    boolean passiveCandidateFound = FALSE;
    boolean activeCandidateFound = FALSE;
    boolean eventDataRemovalProhibited = FALSE;
    boolean obdProhibitsRemoval = FALSE;
    const Dem_EventParameterType *candidateEventParam = NULL;
    for( uint32 i = 0; i < bufferSize; i++ ) {
        if( DEM_EVENT_ID_NULL != extDataBuffer[i].eventId ) {
            /* Check if we are allowed to remove data for this event */
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
            /* Event id may be a combined id. */
            if( IS_COMBINED_EVENT_ID(extDataBuffer[i].eventId) ) {
                const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(extDataBuffer[i].eventId)];
                CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(extDataBuffer[i].eventId)];
                /* Just grab the first event for this DTC. */
                lookupEventIdParameter(CombDTCCfg->DTCClassRef->Events[0u], &candidateEventParam);
            }
            else {
                lookupEventIdParameter(extDataBuffer[i].eventId, &candidateEventParam);
            }
#else
            lookupEventIdParameter(extDataBuffer[i].eventId, &candidateEventParam);
#endif
            if(NO_DTC_DISABLED != DTCRecordDisabled.DTC) {
                eventDataRemovalProhibited = eventDTCRecordDataUpdateDisabled(candidateEventParam);
            } else {
                eventDataRemovalProhibited = FALSE;
            }
            obdProhibitsRemoval = obdEventDisplacementProhibited(candidateEventParam, eventParam);
            if( (FALSE == eventDataRemovalProhibited) && (FALSE == obdProhibitsRemoval) ) {
                /* Check if priority if the event is higher or equal (low numerical value of priority means high priority..) *//* @req DEM383 */
                eventPrio = getEventPriority(extDataBuffer[i].eventId);
                eventPassive = getEventPassive(extDataBuffer[i].eventId);
                if( (eventParam->EventClass->EventPriority <= eventPrio) && (TRUE == eventPassive) && (oldestPassive_TimeStamp > extDataBuffer[i].timeStamp) ) {
                    /* This event has lower or equal priority to the reported event, it is passive
                     * and it is the oldest currently found. A candidate for removal. */
                    oldestPassive_TimeStamp = extDataBuffer[i].timeStamp;
                    removeCandidateIndex = i;
                    passiveCandidateFound = TRUE;

                }
                if( FALSE == passiveCandidateFound ) {
                    /* Currently, a passive event with lower or equal priority has not been found.
                     * Check if the priority is less than for the reported event. Store the oldest. */
                    if( (eventParam->EventClass->EventPriority < eventPrio) &&  (oldestActive_TimeStamp > extDataBuffer[i].timeStamp) ) {
                        oldestActive_TimeStamp = extDataBuffer[i].timeStamp;
                        removeCandidateIndex = i;
                        activeCandidateFound = TRUE;
                    }
                }
            }
        }
    }

    if( (TRUE == passiveCandidateFound) || (TRUE == activeCandidateFound) ) {
        *eventToRemove = extDataBuffer[removeCandidateIndex].eventId;
        ret = E_OK;
    }
    return ret;
}
#endif /* (DEM_EXT_DATA_IN_PRE_INIT || DEM_EXT_DATA_IN_PRI_MEM || DEM_EXT_DATA_IN_SEC_MEM ) && defined(DEM_USE_MEMORY_FUNCTIONS) */
#ifdef DEM_USE_MEMORY_FUNCTIONS
static Std_ReturnType getEventForDisplacement(const Dem_EventParameterType *eventParam, const EventRecType *eventBuffer,
                                              uint32 bufferSize, Dem_EventIdType *eventToRemove)
{
    /* See figure 25 in ASR 4.0.3 */
    /* @req DEM403 */
    /* @req DEM404 */
    /* @req DEM405 */
    /* @req DEM406 */
    Std_ReturnType ret = E_NOT_OK;
    uint32 removeCandidateIndex = 0;
    uint32 oldestPassive_TimeStamp = DEM_MAX_TIMESTAMP_FOR_REARRANGEMENT;
    uint32 oldestActive_TimeStamp = DEM_MAX_TIMESTAMP_FOR_REARRANGEMENT;
    uint8 eventPrio = 0xFF;
    boolean eventPassive = FALSE;
    boolean passiveCandidateFound = FALSE;
    boolean activeCandidateFound = FALSE;
    boolean eventDataRemovalProhibited = FALSE;
    boolean obdProhibitsRemoval = FALSE;
    const Dem_EventParameterType *candidateEventParam = NULL;
    for( uint32 i = 0; i < bufferSize; i++ ) {
        if( DEM_EVENT_ID_NULL != eventBuffer[i].EventData.eventId ) {
            lookupEventIdParameter(eventBuffer[i].EventData.eventId, &candidateEventParam);
            if(NO_DTC_DISABLED != DTCRecordDisabled.DTC) {
                eventDataRemovalProhibited = eventDTCRecordDataUpdateDisabled(candidateEventParam);
            } else {
                eventDataRemovalProhibited = FALSE;
            }
            obdProhibitsRemoval = obdEventDisplacementProhibited(candidateEventParam, eventParam);
            if( (FALSE == eventDataRemovalProhibited) && (FALSE == obdProhibitsRemoval) ) {
                /* Check if priority if the event is higher or equal (low numerical value of priority means high priority..) *//* @req DEM383 */
                eventPrio = getEventPriority(eventBuffer[i].EventData.eventId);
                eventPassive = getEventPassive(eventBuffer[i].EventData.eventId);
                if( (eventParam->EventClass->EventPriority <= eventPrio) && (TRUE == eventPassive) && (oldestPassive_TimeStamp > eventBuffer[i].EventData.timeStamp) ) {
                    /* This event has lower or equal priority to the reported event, it is passive
                     * and it is the oldest currently found. A candidate for removal. */
                    oldestPassive_TimeStamp = eventBuffer[i].EventData.timeStamp;
                    removeCandidateIndex = i;
                    passiveCandidateFound = TRUE;

                }
                if( FALSE == passiveCandidateFound ) {
                    /* Currently, a passive event with lower or equal priority has not been found.
                     * Check if the priority is less than for the reported event. Store the oldest. */
                    if( (eventParam->EventClass->EventPriority < eventPrio) &&  (oldestActive_TimeStamp > eventBuffer[i].EventData.timeStamp) ) {
                        oldestActive_TimeStamp = eventBuffer[i].EventData.timeStamp;
                        removeCandidateIndex = i;
                        activeCandidateFound = TRUE;
                    }
                }
            }
        }
    }

    if( (TRUE == passiveCandidateFound) || (TRUE == activeCandidateFound) ) {
        *eventToRemove = eventBuffer[removeCandidateIndex].EventData.eventId;
        ret = E_OK;
    }
    return ret;
}
#endif

#endif /* DEM_DISPLACEMENT_PROCESSING_DEM_INTERNAL */

#if ( DEM_FF_DATA_IN_PRE_INIT )
static boolean lookupFreezeFrameForDisplacementPreInit(const Dem_EventParameterType *eventParam, FreezeFrameRecType **freezeFrame)
{
    boolean freezeFrameFound = FALSE;
    Dem_EventIdType eventToRemove = DEM_EVENT_ID_NULL;

#if defined(DEM_DISPLACEMENT_PROCESSING_DEM_EXTENSION)
    Dem_Extension_GetFFEventForDisplacement(eventParam, preInitFreezeFrameBuffer, DEM_MAX_NUMBER_FF_DATA_PRE_INIT, &eventToRemove);
#elif defined(DEM_DISPLACEMENT_PROCESSING_DEM_INTERNAL)
    if( E_OK != getFFEventForDisplacement(eventParam, preInitFreezeFrameBuffer, DEM_MAX_NUMBER_FF_DATA_PRE_INIT, &eventToRemove) ) {
        eventToRemove = DEM_EVENT_ID_NULL;
    }
#else
#warning Unsupported displacement
#endif
    if( DEM_EVENT_ID_NULL != eventToRemove ) {
        /* Freeze frame for a less significant event was found.
         * Find the all entries in pre init freeze frame buffer and remove these. */
        for (uint16 indx = 0; (indx < DEM_MAX_NUMBER_FF_DATA_PRE_INIT); indx++) {
            if( preInitFreezeFrameBuffer[indx].eventId == eventToRemove ) {
                memset(&preInitFreezeFrameBuffer[indx], 0, sizeof(FreezeFrameRecType));
                if( FALSE == freezeFrameFound ) {
                    *freezeFrame = &preInitFreezeFrameBuffer[indx];
                    freezeFrameFound = TRUE;
                }
            }
        }
#if defined(USE_DEM_EXTENSION)
        if( TRUE == freezeFrameFound ) {
            Dem_Extension_EventFreezeFrameDataDisplaced(eventToRemove);
        }
#endif
    } else {
        /* Buffer is full and the currently stored data is more significant */
    }
    return freezeFrameFound;
}
#endif

#ifdef DEM_USE_MEMORY_FUNCTIONS
#if ( DEM_FF_DATA_IN_PRI_MEM || DEM_FF_DATA_IN_SEC_MEM || DEM_FF_DATA_IN_PRE_INIT)
static boolean lookupFreezeFrameForDisplacement(const Dem_EventParameterType *eventParam, FreezeFrameRecType **freezeFrame,
                                                FreezeFrameRecType* freezeFrameBuffer, uint32 freezeFrameBufferSize)
{
    boolean freezeFrameFound = FALSE;
    Dem_EventIdType eventToRemove = DEM_EVENT_ID_NULL;
    const Dem_EventParameterType *eventToRemoveParam = NULL;

#if defined(DEM_DISPLACEMENT_PROCESSING_DEM_EXTENSION)
    Dem_Extension_GetFFEventForDisplacement(eventParam, freezeFrameBuffer, freezeFrameBufferSize, &eventToRemove);
#elif defined(DEM_DISPLACEMENT_PROCESSING_DEM_INTERNAL)
    if( E_OK != getFFEventForDisplacement(eventParam, freezeFrameBuffer, freezeFrameBufferSize, &eventToRemove) ) {
        eventToRemove = DEM_EVENT_ID_NULL;
    }
#else
#warning Unsupported displacement
#endif
    if( DEM_EVENT_ID_NULL != eventToRemove ) {
        /* Freeze frame for a less significant event was found.
         * Find the all entries in freeze frame buffer and remove these. */
        for (uint16 indx = 0; (indx < freezeFrameBufferSize); indx++) {
            if( freezeFrameBuffer[indx].eventId == eventToRemove ) {
                memset(&freezeFrameBuffer[indx], 0, sizeof(FreezeFrameRecType));
                if( FALSE == freezeFrameFound ) {
                    *freezeFrame = &freezeFrameBuffer[indx];
                    freezeFrameFound = TRUE;
                }
            }
        }
#if defined(USE_DEM_EXTENSION)
        if( TRUE == freezeFrameFound ) {
            Dem_Extension_EventFreezeFrameDataDisplaced(eventToRemove);
        }
#endif
        if( TRUE == freezeFrameFound ) {
            lookupEventIdParameter(eventToRemove, &eventToRemoveParam);
            /* @req DEM475 */
            notifyEventDataChanged(eventToRemoveParam);
        }
    } else {
        /* Buffer is full and the currently stored data is more significant */
    }

    return freezeFrameFound;
}
#endif /* DEM_FF_DATA_IN_PRI_MEM || DEM_FF_DATA_IN_SEC_MEM || DEM_FF_DATA_IN_PRE_INIT */
#endif /* USE_MEMORY_FUNCTIONS */
#endif /* DEM_EVENT_DISPLACEMENT_SUPPORT */

static Std_ReturnType getNofStoredNonOBDFreezeFrames(const Dem_EventParameterType *eventParam, Dem_DTCOriginType origin, uint8 *nofStored)
{
    uint8 nofFound = 0;
    const FreezeFrameRecType *ffBufferPtr = NULL;
    uint8 bufferSize = 0;
    if( DEM_INITIALIZED == demState ) {
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM)
        if (origin == DEM_DTC_ORIGIN_PRIMARY_MEMORY) {
            ffBufferPtr = &priMemFreezeFrameBuffer[0];
            bufferSize = DEM_MAX_NUMBER_FF_DATA_PRI_MEM;
        }
#endif
#if ((DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_SEC_MEM)
        if (origin == DEM_DTC_ORIGIN_SECONDARY_MEMORY) {
            ffBufferPtr = &secMemFreezeFrameBuffer[0];
            bufferSize = DEM_MAX_NUMBER_FF_DATA_SEC_MEM;
        }
#endif

#if !((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM) && \
     !((DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_SEC_MEM)
   /* Avoid compiler warning when no FF is used */
   (void)origin;
#endif

    } else {
#if ( DEM_FF_DATA_IN_PRE_INIT )
        ffBufferPtr = &preInitFreezeFrameBuffer[0];
        bufferSize = DEM_MAX_NUMBER_FF_DATA_PRE_INIT;
#endif
    }
    Dem_EventIdType idToFind;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        idToFind = TO_COMBINED_EVENT_ID(eventParam->CombinedDTCCID);
    }
    else {
        idToFind = eventParam->EventID;
    }
#else
    idToFind = eventParam->EventID;
#endif
    for( uint8 i = 0; (i < bufferSize) && (ffBufferPtr != NULL); i++) {
        if((ffBufferPtr[i].eventId == idToFind) && (DEM_FREEZE_FRAME_NON_OBD == ffBufferPtr[i].kind)) {
            nofFound++;
        }
    }
    *nofStored = nofFound;
    return E_OK;
}

#if (DEM_USE_TIMESTAMPS == STD_ON)
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM) || \
    ((DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_SEC_MEM) || \
    (DEM_FF_DATA_IN_PRE_INIT)
static void setFreezeFrameTimeStamp(FreezeFrameRecType *freezeFrame)
{
    if( DEM_INITIALIZED == demState ) {
        if(FF_TimeStamp >= DEM_MAX_TIMESTAMP_FOR_REARRANGEMENT){
            rearrangeFreezeFrameTimeStamp(&FF_TimeStamp);
        }
        freezeFrame->timeStamp = FF_TimeStamp;
        FF_TimeStamp++;
    } else {
        freezeFrame->timeStamp = FF_TimeStamp;
        if( FF_TimeStamp < DEM_MAX_TIMESTAMP_FOR_PRE_INIT ) {
            FF_TimeStamp++;
        }
    }
}
#endif
#endif

static void getFFClassReference(const Dem_EventParameterType *eventParam, Dem_FreezeFrameClassType **ffClassTypeRef)
{
    Dem_FreezeFrameClassTypeRefIndex ffIdx = getFFIdx(eventParam);
    if ((ffIdx != DEM_FF_NULLREF) ) {
        *ffClassTypeRef = (Dem_FreezeFrameClassType *) &(configSet->GlobalFreezeFrameClassRef[ffIdx]);
    }
}

#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM) || \
    ((DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_SEC_MEM) || \
    (DEM_FF_DATA_IN_PRE_INIT)
/*
 * Procedure:   getPidData
 * Description: get OBD FF data,only called by getFreezeFrameData()
 */
static void getPidData(const Dem_PidOrDidType *const *const *pidClassPtr, FreezeFrameRecType *const *freezeFrame, uint16 *storeIndexPtr)
{
    uint16 storeIndex = 0;
#if (DEM_MAX_NR_OF_PIDS_IN_FREEZEFRAME_DATA > 0)
    const Dem_PidOrDidType *const *FFIdClassRef;
    Std_ReturnType callbackReturnCode;
    uint16 recordSize = 0u;
    boolean detError = FALSE;
    FFIdClassRef = *pidClassPtr;
    //get all pids
    for (uint16 i = 0; ((i < DEM_MAX_NR_OF_PIDS_IN_FREEZEFRAME_DATA) && (FALSE == FFIdClassRef[i]->Arc_EOL) && (FALSE == detError)); i++) {
        //get pid length
        recordSize = FFIdClassRef[i]->PidOrDidSize;
        /* read out the pid data */
        if ((storeIndex + recordSize + DEM_PID_IDENTIFIER_SIZE_OF_BYTES) <= DEM_MAX_SIZE_FF_DATA) {
            /* store PID */
            (*freezeFrame)->data[storeIndex] = FFIdClassRef[i]->PidIdentifier;
            storeIndex++;

            /* store data */
            if(FFIdClassRef[i]->PidReadFnc != NULL){
                callbackReturnCode = FFIdClassRef[i]->PidReadFnc(&(*freezeFrame)->data[storeIndex]);
                if (callbackReturnCode != E_OK) {
                    memset(&(*freezeFrame)->data[storeIndex], DEM_FREEZEFRAME_DEFAULT_VALUE, (size_t)recordSize);
                    DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GET_FREEZEFRAME_ID, DEM_E_NODATAAVAILABLE);
                }
                storeIndex += recordSize;

            } else {
                memset(&(*freezeFrame)->data[storeIndex], DEM_FREEZEFRAME_DEFAULT_VALUE, (size_t)recordSize);
                storeIndex += recordSize;
            }
        } else {
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GET_FREEZEFRAME_ID, DEM_E_FF_TOO_BIG);
            detError = TRUE;/* Will break the loop */
        }

    }
#else
    (void)pidClassPtr;/*lint !e920*/
    (void)freezeFrame;/*lint !e920*/
#endif
    //store storeIndex,it will be used for judge whether FF contains valid data.
    *storeIndexPtr = storeIndex;

}

/*
 * Procedure:   getDidData
 * Description: get UDS FF data,only called by getFreezeFrameData()
 */
 static void getDidData(const Dem_PidOrDidType *const *const *didClassPtr, FreezeFrameRecType *const *freezeFrame, uint16 *storeIndexPtr)
{
    const Dem_PidOrDidType *const *FFIdClassRef;
    Std_ReturnType callbackReturnCode;
    uint16 storeIndex = 0u;
    uint16 recordSize = 0u;
    boolean detError = FALSE;

    FFIdClassRef = *didClassPtr;
    //get all dids
    for (uint16 i = 0u; ((i < DEM_MAX_NR_OF_DIDS_IN_FREEZEFRAME_DATA) && (FALSE == FFIdClassRef[i]->Arc_EOL) && (FALSE == detError)); i++) {
        recordSize = FFIdClassRef[i]->PidOrDidSize;
        /* read out the did data */
        if ((storeIndex + recordSize + DEM_DID_IDENTIFIER_SIZE_OF_BYTES) <= DEM_MAX_SIZE_FF_DATA) {
            /* store DID */
            (*freezeFrame)->data[storeIndex] = (uint8)(FFIdClassRef[i]->DidIdentifier>> 8u) & 0xFFu;
            storeIndex++;
            (*freezeFrame)->data[storeIndex] = (uint8)(FFIdClassRef[i]->DidIdentifier & 0xFFu);
            storeIndex++;
            /* store data */
            if(FFIdClassRef[i]->DidReadFnc!= NULL) {
                callbackReturnCode = FFIdClassRef[i]->DidReadFnc(&(*freezeFrame)->data[storeIndex]);
                if (callbackReturnCode != E_OK) {
                    /* @req DEM463 */
                    memset(&(*freezeFrame)->data[storeIndex], DEM_FREEZEFRAME_DEFAULT_VALUE, (size_t)recordSize);
                    DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GET_FREEZEFRAME_ID, DEM_E_NODATAAVAILABLE);
                }
                storeIndex += recordSize;
            } else {
                memset(&(*freezeFrame)->data[storeIndex], DEM_FREEZEFRAME_DEFAULT_VALUE, (size_t)recordSize);
                storeIndex += recordSize;
            }
        } else {
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GET_FREEZEFRAME_ID, DEM_E_FF_TOO_BIG);
            detError = TRUE;/* Will break the loop */
        }
    }

    //store storeIndex,it will be used for judge whether FF contains valid data.
    *storeIndexPtr = storeIndex;
}

 static Std_ReturnType getNextFFRecordNumber(const Dem_EventParameterType *eventParam, uint8 *recNum, Dem_FreezeFrameKindType ffKind, Dem_DTCOriginType origin)
 {
     /* @req DEM574 */
     Std_ReturnType ret = E_OK;
     uint8 nofStored = 0;
     uint8 maxNofRecords;
     const Dem_FreezeFrameRecNumClass *FreezeFrameRecNumClass;
     if( DEM_FREEZE_FRAME_NON_OBD == ffKind) {
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
        if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
            const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[eventParam->CombinedDTCCID];
            maxNofRecords = CombDTCCfg->MaxNumberFreezeFrameRecords;
            FreezeFrameRecNumClass = CombDTCCfg->FreezeFrameRecNumClassRef;
        }
        else {
            maxNofRecords = eventParam->MaxNumberFreezeFrameRecords;
            FreezeFrameRecNumClass = eventParam->FreezeFrameRecNumClassRef;
        }
#else
        maxNofRecords = eventParam->MaxNumberFreezeFrameRecords;
        FreezeFrameRecNumClass = eventParam->FreezeFrameRecNumClassRef;
#endif
        if( 0 != maxNofRecords ) {
            if( E_OK == getNofStoredNonOBDFreezeFrames(eventParam, origin, &nofStored) ) {
                /* Was ok! */
                if( nofStored < maxNofRecords ) {
                    *recNum = FreezeFrameRecNumClass->FreezeFrameRecordNumber[nofStored];
                } else {
                    /* @req DEM585 *//* All records stored so update the latest */
                    if( (0u == nofStored) || (maxNofRecords > 1u) ) {
                        *recNum = FreezeFrameRecNumClass->FreezeFrameRecordNumber[maxNofRecords - 1];
                    }
                    else {
                        /* Event only has one record and it is already stored */
                        ret = E_NOT_OK;
                    }
                }
            }
         } else {
             /* No freeze frames should be stored for this event */
             ret = E_NOT_OK;
         }
     } else {
         /* Always record 0 for OBD freeze frames */
         *recNum = 0;/* @req DEM291 */
     }
     return ret;
 }

#if (DEM_PRESTORAGE_FF_DATA_IN_MEM)
 static boolean findPreStoredFreezeFrame(Dem_EventIdType eventId,  Dem_FreezeFrameKindType ffKind, FreezeFrameRecType **freezeFrame)
 {
     boolean ffFound = FALSE;
     FreezeFrameRecType *freezeFrameBuffer = &memPreStoreFreezeFrameBuffer[0];
     if (freezeFrameBuffer != NULL) {
         for (uint16 i = 0; (i < DEM_MAX_NUMBER_PRESTORED_FF) && (FALSE == ffFound); i++) {
             ffFound = ((freezeFrameBuffer[i].eventId == eventId) && (freezeFrameBuffer[i].kind == ffKind) )? TRUE: FALSE;
             if(TRUE == ffFound) {
                 *freezeFrame = &freezeFrameBuffer[i];
             }
         }
     }
     return ffFound;
 }
#endif

/*
 * Procedure:   getFreezeFrameData
 * Description: get FF data according configuration
 */
static void getFreezeFrameData(const Dem_EventParameterType *eventParam,
                               FreezeFrameRecType *freezeFrame,
                               Dem_FreezeFrameKindType ffKind,
                               Dem_DTCOriginType origin,
                               boolean useCombinedID)
{
    uint16 storeIndex = 0;
    Dem_FreezeFrameClassType *FreezeFrameLocalClass = NULL;
#if (DEM_PRESTORAGE_FF_DATA_IN_MEM)
    FreezeFrameRecType *preStoredFF = NULL;
#endif
    boolean preStoredFFfound = FALSE;

    /* clear FF data record */
    memset(freezeFrame, 0, sizeof(FreezeFrameRecType ));

    /* Find out the corresponding FF class */
    Dem_FreezeFrameClassTypeRefIndex ffIdx = getFFIdx(eventParam);

    if( (DEM_FREEZE_FRAME_NON_OBD == ffKind) && (ffIdx != DEM_FF_NULLREF) ) {
        getFFClassReference(eventParam, &FreezeFrameLocalClass);
    } else if((DEM_FREEZE_FRAME_OBD == ffKind) && (NULL != configSet->GlobalOBDFreezeFrameClassRef)) {
        FreezeFrameLocalClass = (Dem_FreezeFrameClassType *) configSet->GlobalOBDFreezeFrameClassRef;
    } else {
        FreezeFrameLocalClass = NULL;
    }

    /* get the dids */
    if(FreezeFrameLocalClass != NULL){
        if(FreezeFrameLocalClass->FFIdClassRef != NULL){
            if( DEM_FREEZE_FRAME_NON_OBD == ffKind ) {
                getDidData(&FreezeFrameLocalClass->FFIdClassRef, &freezeFrame, &storeIndex);
            } else if(DEM_FREEZE_FRAME_OBD == ffKind) {
                /* Get the pids */
                getPidData(&FreezeFrameLocalClass->FFIdClassRef, &freezeFrame, &storeIndex);
            } else {
                /* IMPROVEMENT: Det error */
            }
        }
    } else {
        /* create an empty FF */
        freezeFrame->eventId = DEM_EVENT_ID_NULL;
    }

#if (DEM_PRESTORAGE_FF_DATA_IN_MEM)
    /* @req DEM464 */
    if (eventParam->EventClass->FFPrestorageSupported == TRUE) {
        /* Look for pre-stored FF in pre-stored buffer */
        if (findPreStoredFreezeFrame(eventParam->EventID, ffKind, (FreezeFrameRecType **) &preStoredFF) == TRUE) {
            if ( preStoredFF != NULL) {
                /* @req DEM465 */
                /* Remove FF from preStored buffer */
                memcpy(freezeFrame, preStoredFF, sizeof(FreezeFrameRecType));
                memset(preStoredFF, 0, sizeof(FreezeFrameRecType ));

                Dem_NvM_SetPreStoreFreezeFrameBlockChanged(FALSE);

                preStoredFFfound = TRUE;
            }
        }
    }
#endif

    if (preStoredFFfound == FALSE) {
        uint8 recNum = 0;
        /* Check if any data has been stored and that there is a record number */
        if ( (storeIndex != 0) && (E_OK == getNextFFRecordNumber(eventParam, &recNum, ffKind, origin))) {/*lint !e9007 */
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
            if( (DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID) && (TRUE == useCombinedID) ) {
                freezeFrame->eventId = TO_COMBINED_EVENT_ID(eventParam->CombinedDTCCID);
            }
            else {
                freezeFrame->eventId = eventParam->EventID;
            }
#else
            freezeFrame->eventId = eventParam->EventID;
#endif

            freezeFrame->dataSize = storeIndex;
            freezeFrame->recordNumber = recNum;
            freezeFrame->kind = ffKind;
#if (DEM_USE_TIMESTAMPS == STD_ON)
            setFreezeFrameTimeStamp(freezeFrame);
#endif
        } else {
            freezeFrame->eventId = DEM_EVENT_ID_NULL;
            freezeFrame->dataSize = storeIndex;
        }
    }
#if !defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    (void)useCombinedID;
#endif
}
#endif

#if (defined(DEM_FREEZE_FRAME_CAPTURE_EXTENSION) && DEM_FF_DATA_IN_PRE_INIT)
static void deleteFreezeFrameDataPreInit(const Dem_EventParameterType *eventParam)
{
    /* Delete all freeze frames */
    for (uint16 i = 0; i < DEM_MAX_NUMBER_FF_DATA_PRE_INIT; i++){
        if(preInitFreezeFrameBuffer[i].eventId == eventParam->EventID) {
            memset(&preInitFreezeFrameBuffer[i], 0, sizeof(FreezeFrameRecType));
        }
    }
}
#endif
/*
 * Procedure:   storeFreezeFrameDataPreInit
 * Description: store FF in before  preInitFreezeFrameBuffer DEM's full initialization
 */
#if ( DEM_FF_DATA_IN_PRE_INIT )
static void storeFreezeFrameDataPreInit(const Dem_EventParameterType *eventParam, const FreezeFrameRecType *freezeFrame)
{
    boolean eventIdFound = FALSE;
    boolean eventIdFreePositionFound=FALSE;
    uint16 i;
    /* Check if already stored */
    Dem_EventIdType idToFind;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        idToFind = TO_COMBINED_EVENT_ID(eventParam->CombinedDTCCID);
    }
    else {
        idToFind = eventParam->EventID;
    }
#else
    idToFind = eventParam->EventID;
#endif
    for (i = 0; (i < DEM_MAX_NUMBER_FF_DATA_PRE_INIT) && (FALSE == eventIdFound); i++) {
        if( DEM_FREEZE_FRAME_NON_OBD == freezeFrame->kind ) {
            eventIdFound = ( (preInitFreezeFrameBuffer[i].eventId == idToFind) && (preInitFreezeFrameBuffer[i].recordNumber == freezeFrame->recordNumber))? TRUE: FALSE;
        } else {
            eventIdFound = ((DEM_EVENT_ID_NULL != preInitFreezeFrameBuffer[i].eventId) && (DEM_FREEZE_FRAME_OBD == preInitFreezeFrameBuffer[i].kind))? TRUE: FALSE;
        }
    }

    if( TRUE == eventIdFound ) {
        /* Entry found. Overwrite if not an OBD freeze frame*/
        if( DEM_FREEZE_FRAME_NON_OBD == preInitFreezeFrameBuffer[i-1].kind ) {
            /* overwrite existing */
            memcpy(&preInitFreezeFrameBuffer[i-1], freezeFrame, sizeof(FreezeFrameRecType));
        }
    }
    else {
        /* lookup first free position */
        for (i = 0; (i < DEM_MAX_NUMBER_FF_DATA_PRE_INIT) && (FALSE == eventIdFreePositionFound); i++) {
            if( preInitFreezeFrameBuffer[i].eventId == DEM_EVENT_ID_NULL ) {
                eventIdFreePositionFound = TRUE;
            }
        }

        if ( TRUE == eventIdFreePositionFound ) {
            memcpy(&preInitFreezeFrameBuffer[i-1], freezeFrame, sizeof(FreezeFrameRecType));
        } else {
#if (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON)
            /* @req DEM400 */
            /* @req DEM407 */
            /* do displacement */
            FreezeFrameRecType *freezeFrameLocal = NULL;
            if( TRUE == lookupFreezeFrameForDisplacementPreInit(eventParam, &freezeFrameLocal) ) {
                memcpy(freezeFrameLocal, freezeFrame, sizeof(FreezeFrameRecType));
            }
#else
            /* @req DEM402*/ /* Req is not the Det-error.. */
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_STORE_FF_DATA_PRE_INIT_ID, DEM_E_PRE_INIT_FF_DATA_BUFF_FULL);
#endif
        }
    }

}
#endif

#ifdef DEM_USE_MEMORY_FUNCTIONS
#if ( DEM_FF_DATA_IN_PRE_INIT )
static boolean isValidRecordNumber(const Dem_FreezeFrameRecNumClass *FreezeFrameRecNumClass, uint8 maxNumRecords, uint8 recordNumber)
{
    boolean isValid = FALSE;
    if( NULL != FreezeFrameRecNumClass ) {
        for( uint8 i = 0; (i < maxNumRecords) && (FALSE == isValid); i++ ) {
            if( FreezeFrameRecNumClass->FreezeFrameRecordNumber[i] == recordNumber ) {
                isValid = TRUE;
            }
        }
    }
    return isValid;
}
#endif

#if ( DEM_FF_DATA_IN_PRE_INIT )
/**
 * Transfers non-OBD freeze frame stored in pre-Init buffer to event destination
 * @param eventId
 * @param freezeFrameBuffer
 * @param freezeFrameBufferSize
 * @param eventHandled
 * @param origin
 * @param removeOldFFRecords
 * @return
 */
static boolean transferNonOBDFreezeFramesEvtMem(Dem_EventIdType eventId, FreezeFrameRecType* freezeFrameBuffer, uint32 freezeFrameBufferSize,
                                                boolean* eventHandled, Dem_DTCOriginType origin, boolean removeOldFFRecords)
{
    /* Note: Don't touch the OBD freeze frame */
    uint16 nofStoredMemory = 0u;
    uint16 nofStoredPreInit = 0u;
    uint16 nofFFToMove;
    const Dem_EventParameterType *eventParam;
    uint8 recordToFind;
    uint16 findRecordStartIndex = 0u;
    uint16 setRecordStartIndex = 0u;
    boolean memoryChanged = FALSE;
    boolean dataUpdated = FALSE;

    uint8 maxNofRecords;
    const Dem_FreezeFrameRecNumClass *FreezeFrameRecNumClass;

#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    /* Event id may be a combined id. */
    if( IS_COMBINED_EVENT_ID(eventId) ) {
        const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(eventId)];
        CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(eventId)];
        maxNofRecords = CombDTCCfg->MaxNumberFreezeFrameRecords;
        FreezeFrameRecNumClass = CombDTCCfg->FreezeFrameRecNumClassRef;
        /* Just grab the first event for this DTC. */
        lookupEventIdParameter(CombDTCCfg->DTCClassRef->Events[0u], &eventParam);
    }
    else {
        lookupEventIdParameter(eventId, &eventParam);
        maxNofRecords = eventParam->MaxNumberFreezeFrameRecords;
        FreezeFrameRecNumClass = eventParam->FreezeFrameRecNumClassRef;
    }
#else
    lookupEventIdParameter(eventId, &eventParam);
    maxNofRecords = eventParam->MaxNumberFreezeFrameRecords;
    FreezeFrameRecNumClass = eventParam->FreezeFrameRecNumClassRef;
#endif
    /* Count the number of entries in destination memory for the event */
    for( uint32 j = 0u; j < freezeFrameBufferSize; j++ ) {
        if( (eventId == freezeFrameBuffer[j].eventId) && (DEM_FREEZE_FRAME_NON_OBD == freezeFrameBuffer[j].kind)) {
            if( TRUE == isValidRecordNumber(FreezeFrameRecNumClass, maxNofRecords, freezeFrameBuffer[j].recordNumber) ) {
                if( TRUE == removeOldFFRecords) {
                    /* We should remove the old records. Don't increment nofStoredMemory
                     * since no records will be stored in the buffer after all have been cleared */
                    memset(&freezeFrameBuffer[j], 0, sizeof(FreezeFrameRecType));
                    memoryChanged = TRUE;
                } else {
                    nofStoredMemory++;
                }
            } else {
                /* Invalid ff record number */
                memset(&freezeFrameBuffer[j], 0, sizeof(FreezeFrameRecType));
                memoryChanged = TRUE;
            }
        }
    }

    /* Count the number of entries in the pre init memory for the event */
    for( uint16 k = 0; k < DEM_MAX_NUMBER_FF_DATA_PRE_INIT; k++ ) {
        if( (eventId == preInitFreezeFrameBuffer[k].eventId) && (DEM_FREEZE_FRAME_NON_OBD == preInitFreezeFrameBuffer[k].kind)) {
            nofStoredPreInit++;
        }
    }

    /* Find out the number of FF to transfer from preInit buffer to memory */
    /* We can assume that we should transfer at least on record from the preInitBuffer
     * since we found this event in the preInit buffer*/
    if( 0u ==  maxNofRecords) {
        nofFFToMove = 0u;
    } else if( maxNofRecords == nofStoredMemory ) {
        /* All records already stored in primary memory. Just update the last record */
        nofFFToMove = 1u;
        findRecordStartIndex = nofStoredPreInit - 1u;
        setRecordStartIndex = maxNofRecords - 1u;
    } else if( maxNofRecords > nofStoredMemory ) {
        nofFFToMove = (uint16)(MIN(nofStoredPreInit, (uint16)(maxNofRecords - nofStoredMemory)));
        findRecordStartIndex = nofStoredPreInit - nofFFToMove;
        setRecordStartIndex = nofStoredMemory;
    } else {
        /* To many records stored in primary memory. And they are all valid records
         * as we check this.
         * IMPROVEMENT: How do we handle this?
         * For now, throw all records in buffer and store the ones
         * in the preinit buffer. */
        for( uint32 i = 0u; i < freezeFrameBufferSize; i++ ) {
            if( (eventId == freezeFrameBuffer[i].eventId) && (DEM_FREEZE_FRAME_NON_OBD == freezeFrameBuffer[i].kind)) {
                memset(&freezeFrameBuffer[i], 0, sizeof(FreezeFrameRecType));
            }
        }
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GLOBAL_ID, DEM_E_MEMORY_CORRUPT);
        nofFFToMove = nofStoredPreInit;
        findRecordStartIndex = 0u;
        setRecordStartIndex = 0u;
    }

    if( 0u != nofFFToMove) {
        boolean ffStored = FALSE;
        for( uint16 offset = 0u; offset < nofFFToMove; offset++ ) {
            recordToFind = FreezeFrameRecNumClass->FreezeFrameRecordNumber[findRecordStartIndex + offset];
            ffStored = FALSE;
            for( uint16 indx = 0u; (indx < DEM_MAX_NUMBER_FF_DATA_PRE_INIT) && (FALSE == ffStored); indx++ ) {
                if( (preInitFreezeFrameBuffer[indx].eventId == eventId) && (preInitFreezeFrameBuffer[indx].recordNumber == recordToFind) &&
                        (DEM_FREEZE_FRAME_NON_OBD == preInitFreezeFrameBuffer[indx].kind) && (FALSE == eventHandled[indx]) ) {
                    /* Found the record to update */
                    preInitFreezeFrameBuffer[indx].recordNumber =
                            FreezeFrameRecNumClass->FreezeFrameRecordNumber[setRecordStartIndex + offset];
                    /* Store the freeze frame */
                    if( TRUE == storeFreezeFrameDataMem(eventParam, &preInitFreezeFrameBuffer[indx], freezeFrameBuffer, freezeFrameBufferSize, origin) ) {
                        dataUpdated = TRUE;
                    }
                    /* Clear the event id in the preInit buffer */
                    eventHandled[indx] = TRUE;
                    ffStored = TRUE;
                }
            }
        }

        if( nofFFToMove < nofStoredPreInit ) {
            /* Did not move all freeze frames from the preInit buffer.
             * Need to clear the remaining */
            for( uint16 indx = 0u; indx < DEM_MAX_NUMBER_FF_DATA_PRE_INIT; indx++ ) {
                if( (preInitFreezeFrameBuffer[indx].eventId == eventId) && (DEM_FREEZE_FRAME_NON_OBD == preInitFreezeFrameBuffer[indx].kind) ) {
                    eventHandled[indx] = TRUE;
                }
            }
        }
    }
    if( TRUE == dataUpdated ) {
        /* Use errorStatusChanged in eventsStatusBuffer to signal that the event data was updated */
        EventStatusRecType *eventStatusRecPtr;
        lookupEventStatusRec(eventParam->EventID, &eventStatusRecPtr);
        if( NULL != eventStatusRecPtr ) {
            eventStatusRecPtr->errorStatusChanged = TRUE;
        }
    }
    return memoryChanged;
}
#endif

#if ( DEM_FF_DATA_IN_PRE_INIT )
static void transferOBDFreezeFramesEvtMem(const FreezeFrameRecType *freezeFrame, FreezeFrameRecType* freezeFrameBuffer,
                                          uint32 freezeFrameBufferSize, Dem_DTCOriginType origin)
{
    const Dem_EventParameterType *eventParam;
    lookupEventIdParameter(freezeFrame->eventId, &eventParam);

    if (origin != DEM_DTC_ORIGIN_PRIMARY_MEMORY) {
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GLOBAL_ID, DEM_E_OBD_NOT_ALLOWED_IN_SEC_MEM);
    } else if( NULL != eventParam ) {
        /* Assuming that this function will not store the OBD freeze frame
         * if there already is one stored. */
        if( TRUE == storeOBDFreezeFrameDataMem(eventParam, freezeFrame, freezeFrameBuffer, freezeFrameBufferSize, origin) ) {
            /* Use errorStatusChanged in eventsStatusBuffer to signal that the event data was updated */
            EventStatusRecType *eventStatusRecPtr;
            lookupEventStatusRec(eventParam->EventID, &eventStatusRecPtr);
            if( NULL != eventStatusRecPtr ) {
                eventStatusRecPtr->errorStatusChanged = TRUE;
            }
        }
    } else {
        /* Bad origin and no config available.. */
    }
}
#endif

#if ( DEM_FF_DATA_IN_PRE_INIT )
static boolean transferPreInitFreezeFramesEvtMem(FreezeFrameRecType* freezeFrameBuffer, uint32 freezeFrameBufferSize, EventRecType* eventBuffer, uint32 eventBufferSize, Dem_DTCOriginType origin)
{
    boolean priMemChanged = FALSE;
    boolean removeOldFFRecords;
    boolean skipFF;
    const Dem_EventParameterType *eventParam;
    EventStatusRecType* eventStatusRec;
    boolean eventHandled[DEM_MAX_NUMBER_FF_DATA_PRE_INIT] = {FALSE};
    Dem_DTCOriginType eventOrigin;
    Dem_EventStatusExtendedType eventStatus;

    for( uint16 i = 0; i < DEM_MAX_NUMBER_FF_DATA_PRE_INIT; i++ ) {
        if( (DEM_EVENT_ID_NULL != preInitFreezeFrameBuffer[i].eventId) && (TRUE == checkEntryValid(preInitFreezeFrameBuffer[i].eventId, origin, TRUE)) ) {
            /* Check if this is a freeze frame which should be processed.
             * Ignore freeze frames for event not stored in memory or OBD
             * freeze frames for event which are not confirmed  */
            eventStatusRec = NULL;
            eventParam = NULL;
            skipFF = TRUE;
            eventOrigin = DEM_DTC_ORIGIN_NOT_USED;;
            eventStatus = 0xFFu;/* This combination of bits is invalid */
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
            if( IS_COMBINED_EVENT_ID(preInitFreezeFrameBuffer[i].eventId) ) {
                const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(preInitFreezeFrameBuffer[i].eventId)];
                eventOrigin = CombDTCCfg->MemoryDestination;
                if( DEM_STATUS_OK != GetDTCUDSStatus(CombDTCCfg->DTCClassRef, origin, &eventStatus) ) {
                    eventStatus = 0xFFu;
                }
            }
            else {
                lookupEventStatusRec(preInitFreezeFrameBuffer[i].eventId, &eventStatusRec);
                lookupEventIdParameter(preInitFreezeFrameBuffer[i].eventId, &eventParam);
                if( (NULL != eventStatusRec) && (NULL != eventParam) ) {
                    eventOrigin = eventParam->EventClass->EventDestination;
                    eventStatus = eventStatusRec->eventStatusExtended;
                }
            }
#else
            lookupEventStatusRec(preInitFreezeFrameBuffer[i].eventId, &eventStatusRec);
            lookupEventIdParameter(preInitFreezeFrameBuffer[i].eventId, &eventParam);
            if( (NULL != eventStatusRec) && (NULL != eventParam) ) {
                eventOrigin = eventParam->EventClass->EventDestination;
                eventStatus = eventStatusRec->eventStatusExtended;
            }
#endif

            if( (DEM_DTC_ORIGIN_NOT_USED != eventOrigin) && (0xFFu != eventStatus) ) {
                skipFF = FALSE;
                if( origin == eventOrigin ) {
                    if( (FALSE == eventIsStoredInMem(preInitFreezeFrameBuffer[i].eventId, eventBuffer, eventBufferSize)) ||
                            ((DEM_FREEZE_FRAME_OBD == preInitFreezeFrameBuffer[i].kind) && (0u == (eventStatus & DEM_CONFIRMED_DTC))) ) {
                        /* Event is not stored or FF is OBD and event is not confirmed. Skip FF */
                        skipFF = TRUE;
                    }
                }
            }
            if( FALSE == skipFF ) {
                removeOldFFRecords = FALSE;
#if defined(USE_DEM_EXTENSION)
                Dem_Extension_PreTransferPreInitFreezeFrames(preInitFreezeFrameBuffer[i].eventId, &removeOldFFRecords, origin);
#endif
                if( DEM_FREEZE_FRAME_NON_OBD == preInitFreezeFrameBuffer[i].kind ) {
                    if (TRUE == transferNonOBDFreezeFramesEvtMem(preInitFreezeFrameBuffer[i].eventId,
                                                         freezeFrameBuffer,
                                                         freezeFrameBufferSize,
                                                         eventHandled,
                                                         origin,
                                                         removeOldFFRecords)) {
                        priMemChanged = TRUE;
                    }
                } else {
                    transferOBDFreezeFramesEvtMem(&preInitFreezeFrameBuffer[i], freezeFrameBuffer,
                                                  freezeFrameBufferSize, origin);
                }
            }
        }
    }
    return priMemChanged;
}
#endif

#endif /* DEM_USE_MEMORY_FUNCTIONS */


#if (DEM_USE_TIMESTAMPS == STD_ON)
static void setExtDataTimeStamp(ExtDataRecType *extData)
{

    if( DEM_INITIALIZED == demState ) {
        if(ExtData_TimeStamp >= DEM_MAX_TIMESTAMP_FOR_REARRANGEMENT){
            rearrangeExtDataTimeStamp(&ExtData_TimeStamp);
        }
        extData->timeStamp = ExtData_TimeStamp;
        ExtData_TimeStamp++;
    } else {
        extData->timeStamp = ExtData_TimeStamp;
        if( ExtData_TimeStamp < DEM_MAX_TIMESTAMP_FOR_PRE_INIT ) {
            ExtData_TimeStamp++;
        }
    }

}
#endif
#if (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON) && (DEM_EXT_DATA_IN_PRE_INIT || DEM_EXT_DATA_IN_PRI_MEM || DEM_EXT_DATA_IN_SEC_MEM ) && defined(DEM_USE_MEMORY_FUNCTIONS)
static Std_ReturnType lookupExtDataForDisplacement(const Dem_EventParameterType *eventParam, ExtDataRecType *extDataBufPtr, uint32 bufferSize, ExtDataRecType **extData )
{
    const Dem_EventParameterType *eventToRemoveParam = NULL;
    Std_ReturnType ret = E_NOT_OK;
    /* @req DEM400 */
    Dem_EventIdType eventToRemove = DEM_EVENT_ID_NULL;

#if defined(DEM_DISPLACEMENT_PROCESSING_DEM_EXTENSION)
    Dem_Extension_GetExtDataEventForDisplacement(eventParam, extDataBufPtr, bufferSize, &eventToRemove);
#elif defined(DEM_DISPLACEMENT_PROCESSING_DEM_INTERNAL)
    if( E_OK != getExtDataEventForDisplacement(eventParam, extDataBufPtr, bufferSize, &eventToRemove) ) {
        eventToRemove = DEM_EVENT_ID_NULL;
    }
#else
#warning Unsupported displacement
#endif
    if( DEM_EVENT_ID_NULL != eventToRemove ) {
        /* Extended data for a less significant event was found.
         * Find the entry in ext data buffer. */
        for (uint32 indx = 0; (indx < bufferSize) && (E_OK != ret); indx++) {
            if( extDataBufPtr[indx].eventId == eventToRemove ) {
                memset(&extDataBufPtr[indx], 0, sizeof(ExtDataRecType));
                *extData = &extDataBufPtr[indx];
                ret = E_OK;
#if defined(USE_DEM_EXTENSION)
                Dem_Extension_EventExtendedDataDisplaced(eventToRemove);
#endif
                lookupEventIdParameter(eventToRemove, &eventToRemoveParam);
                /* @req DEM475 */
                notifyEventDataChanged(eventToRemoveParam);
            }
        }
    } else {
        /* Buffer is full and the currently stored data is more significant *//* @req DEM407 */
    }
    return ret;
}
#endif /* (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON) && (DEM_EXT_DATA_IN_PRE_INIT || DEM_EXT_DATA_IN_PRI_MEM || DEM_EXT_DATA_IN_SEC_MEM ) && defined(DEM_USE_MEMORY_FUNCTIONS) */




#if ( DEM_EXT_DATA_IN_PRE_INIT || DEM_EXT_DATA_IN_PRI_MEM || DEM_EXT_DATA_IN_SEC_MEM ) && defined(DEM_USE_MEMORY_FUNCTIONS)
static boolean StoreExtDataInMem( const Dem_EventParameterType *eventParam, ExtDataRecType *extDataMem, uint16 bufferSize, Dem_DTCOriginType origin, boolean overrideOldData) {

    uint16 storeIndex = 0;
    uint16 recordSize;
    const Dem_ExtendedDataRecordClassType *extendedDataRecord;
    const Dem_ExtendedDataClassType *ExtendedDataClass;
    ExtDataRecType *extData = NULL;
    boolean eventIdFound = FALSE;
    boolean bStoredData = FALSE;

    Dem_EventIdType idToFind;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        idToFind = TO_COMBINED_EVENT_ID(eventParam->CombinedDTCCID);
        ExtendedDataClass = configSet->CombinedDTCConfig[eventParam->CombinedDTCCID].ExtendedDataClassRef;
    }
    else {
        idToFind = eventParam->EventID;
        ExtendedDataClass = eventParam->ExtendedDataClassRef;
    }
#else
    idToFind = eventParam->EventID;
    ExtendedDataClass = eventParam->ExtendedDataClassRef;
#endif


    // Check if already stored
    for (uint16 i = 0; (i < bufferSize) && (FALSE == eventIdFound); i++){
        eventIdFound = (extDataMem[i].eventId == idToFind)? TRUE: FALSE;
        extData = &extDataMem[i];
    }

    if( FALSE == eventIdFound ) {
        extData = NULL;
        for (uint16 i = 0; (i < bufferSize) && (NULL == extData); i++){
            if( extDataMem[i].eventId == DEM_EVENT_ID_NULL ) {
                extData = &extDataMem[i];
            }
        }
        if( NULL == extData ) {
#if (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON)
            /* @req DEM400 *//* @req DEM407 */
            if( E_OK != lookupExtDataForDisplacement(eventParam, extDataMem, bufferSize, &extData) ) {
                setOverflowIndication(eventParam->EventClass->EventDestination, TRUE);
                return FALSE;
            }
#else
            /* @req DEM402*//* Displacement supported disabled */
            setOverflowIndication(eventParam->EventClass->EventDestination, TRUE);
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_STORE_EXT_DATA_MEM_ID, DEM_E_MEM_EXT_DATA_BUFF_FULL);
            return FALSE;
#endif /* DEM_EVENT_DISPLACEMENT_SUPPORT */
        }
    }

    // Check if any pointer to extended data class
    if ( (NULL != extData) && (ExtendedDataClass != NULL) ) {
        // Request extended data and copy it to the buffer
        for (uint32 i = 0uL; (i < DEM_MAX_NR_OF_RECORDS_IN_EXTENDED_DATA) && (ExtendedDataClass->ExtendedDataRecordClassRef[i] != NULL); i++) {
            extendedDataRecord = ExtendedDataClass->ExtendedDataRecordClassRef[i];
            if( DEM_UPDATE_RECORD_VOLATILE != extendedDataRecord->UpdateRule ) {
                recordSize = extendedDataRecord->DataSize;
                if ((storeIndex + recordSize) <= DEM_MAX_SIZE_EXT_DATA) {
                    if( (DEM_UPDATE_RECORD_YES == extendedDataRecord->UpdateRule) ||
                            ((DEM_UPDATE_RECORD_NO == extendedDataRecord->UpdateRule) && (extData->eventId != eventParam->EventID)) ||
                            (TRUE == overrideOldData)) {
                        /* Either update rule YES, or update rule is NO and extended data was not previously stored for this event */
                        if( NULL != extendedDataRecord->CallbackGetExtDataRecord ) {
                            /** @req DEM282 */
                            if (E_OK != extendedDataRecord->CallbackGetExtDataRecord(&extData->data[storeIndex])) {
                                // Callback data currently not available, clear space.
                                memset(&extData->data[storeIndex], 0xFF, (size_t)recordSize);
                            }
                            bStoredData = TRUE;
                        } else if( DEM_NO_ELEMENT != extendedDataRecord->InternalDataElement ) {
                            getInternalElement( eventParam, extendedDataRecord->InternalDataElement, &extData->data[storeIndex], extendedDataRecord->DataSize );
                            bStoredData = TRUE;
                        } else {
                            /* No callback and not internal element..
                             * IMPROVMENT: Det error */
                        }
                    } else {
                        /* Should not update */
                    }
                    storeIndex += recordSize;
                } else {
                    // Error: Size of extended data record is bigger than reserved space.
                    DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GET_EXTENDED_DATA_ID, DEM_E_EXT_DATA_TOO_BIG);
                    break;  // Break the loop
                }
            }
        }
    }

    // Check if any data has been stored
    if ( (NULL != extData) && (TRUE == bStoredData) ) {
        extData->eventId = idToFind;
#if (DEM_USE_TIMESTAMPS == STD_ON)
        setExtDataTimeStamp(extData);
#endif

#ifdef DEM_USE_MEMORY_FUNCTIONS
        if( DEM_PREINITIALIZED != demState ) {
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
            boolean immediateStorage = FALSE;
            EventStatusRecType *eventStatusRecPtr = NULL;
            lookupEventStatusRec(eventParam->EventID, &eventStatusRecPtr);
            if( (NULL != eventStatusRecPtr) && (NULL != eventParam->DTCClassRef) && (TRUE == eventParam->DTCClassRef->DTCRef->ImmediateNvStorage)
                    && (eventStatusRecPtr->occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
                immediateStorage = TRUE;
            }
            Dem_NvM_SetExtendedDataBlockChanged(origin, immediateStorage);
#else
            Dem_NvM_SetExtendedDataBlockChanged(origin, FALSE);
#endif
        }
#endif
    }
    return bStoredData;
}
#endif /* DEM_EXT_DATA_IN_PRE_INIT || DEM_EXT_DATA_IN_PRI_MEM || DEM_EXT_DATA_IN_SEC_MEM */


/*
 * Procedure:   getExtendedData
 * Description: Collects the extended data according to "eventParam" and return it in "extData",
 *              if not found eventId is set to DEM_EVENT_ID_NULL.
 */
static boolean storeExtendedData(const Dem_EventParameterType *eventParam, boolean overrideOldData)
{
    boolean ret = FALSE;
    if( DEM_PREINITIALIZED == demState ) {
#if ( DEM_EXT_DATA_IN_PRE_INIT )
        (void)StoreExtDataInMem(eventParam, preInitExtDataBuffer, DEM_MAX_NUMBER_EXT_DATA_PRE_INIT, DEM_DTC_ORIGIN_NOT_USED, overrideOldData);/* @req DEM468 */
#endif
    } else {
        switch (eventParam->EventClass->EventDestination) {
            case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_EXT_DATA_IN_PRI_MEM)
                ret = StoreExtDataInMem(eventParam, priMemExtDataBuffer, DEM_MAX_NUMBER_EXT_DATA_PRI_MEM, DEM_DTC_ORIGIN_PRIMARY_MEMORY, overrideOldData);/* @req DEM468 */
#endif
                break;
            case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if ((DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_EXT_DATA_IN_SEC_MEM)
                ret = StoreExtDataInMem(eventParam, secMemExtDataBuffer, DEM_MAX_NUMBER_EXT_DATA_SEC_MEM, DEM_DTC_ORIGIN_SECONDARY_MEMORY, overrideOldData);/* @req DEM468 */
#endif
                break;
            case DEM_DTC_ORIGIN_PERMANENT_MEMORY:
            case DEM_DTC_ORIGIN_MIRROR_MEMORY:
                // Not yet supported
                DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GLOBAL_ID, DEM_E_NOT_IMPLEMENTED_YET);
                break;
            default:
                break;
        }
    }
    return ret;
}

#ifdef DEM_USE_MEMORY_FUNCTIONS
#if (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON)
static Std_ReturnType findAndDisplaceEvent(const Dem_EventParameterType *eventParam, EventRecType *eventBuffer, uint32 bufferSize,
        EventRecType **memEventStatusRec, Dem_DTCOriginType origin)
{
    Std_ReturnType ret = E_NOT_OK;
    for (uint32 i = 0; (i < bufferSize) && (E_OK != ret); i++) {
        if( eventBuffer[i].EventData.eventId == eventParam->EventID ) {
            memset(&eventBuffer[i].EventData, 0, sizeof(EventRecType));
            *memEventStatusRec = &eventBuffer[i];
            ret = E_OK;
#if defined(USE_DEM_EXTENSION)
            Dem_Extension_EventDataDisplaced(eventParam->EventID);
#endif
        }
    }
    /* Reset it */
    EventStatusRecType *eventStatusRecPtr;
    lookupEventStatusRec(eventParam->EventID, &eventStatusRecPtr);
    if( NULL != eventStatusRecPtr ) {
        Dem_EventStatusExtendedType oldStatus = eventStatusRecPtr->eventStatusExtended;
        /* @req DEM409 *//* @req DEM538 */
        eventStatusRecPtr->eventStatusExtended &= (Dem_EventStatusExtendedType)(~DEM_CONFIRMED_DTC);
#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
        eventStatusRecPtr->failureCounter = 0;
        eventStatusRecPtr->failedDuringFailureCycle = FALSE;
        eventStatusRecPtr->passedDuringFailureCycle = FALSE;
#endif
#if defined(DEM_aging_PROCESSING_DEM_INTERNAL)
        eventStatusRecPtr->agingCounter = 0;
        eventStatusRecPtr->failedDuringAgingCycle = FALSE;
        eventStatusRecPtr->passedDuringAgingCycle = FALSE;
#endif
        if( oldStatus != eventStatusRecPtr->eventStatusExtended ) {
            /* @req DEM016 */
            notifyEventStatusChange(eventParam, oldStatus, eventStatusRecPtr->eventStatusExtended);
        }
    }

    /* Remove all event related data */
    /* @req DEM408 */
    /* @req DEM542 */
    boolean combinedDTC = FALSE;
    const Dem_ExtendedDataClassType *ExtendedDataClass;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        ExtendedDataClass = configSet->CombinedDTCConfig[eventParam->CombinedDTCCID].ExtendedDataClassRef;
        combinedDTC = TRUE;
    }
    else {
        ExtendedDataClass = eventParam->ExtendedDataClassRef;
    }
#else
    ExtendedDataClass = eventParam->ExtendedDataClassRef;
#endif
    if( DEM_FF_NULLREF != getFFIdx(eventParam) ) {
        (void)deleteFreezeFrameDataMem(eventParam, origin, combinedDTC);
    }
    if( NULL_PTR != ExtendedDataClass ) {
        (void)deleteExtendedDataMem(eventParam, origin, combinedDTC);
    }
#if defined(DEM_USE_INDICATORS)
    if( TRUE == resetIndicatorCounters(eventParam) ) {
#ifdef DEM_USE_MEMORY_FUNCTIONS
        /* IPROVEMENT: Immediate storage when deleting events? */
        Dem_NvM_SetIndicatorBlockChanged(FALSE);
#endif
    }
#endif
    /* @req DEM475 */
    notifyEventDataChanged(eventParam);

    return ret;

}

Std_ReturnType lookupEventForDisplacement(const Dem_EventParameterType *eventParam, EventRecType *eventBuffer, uint32 bufferSize,
        EventRecType **memEventStatusRec, Dem_DTCOriginType origin)
{
    Std_ReturnType ret = E_NOT_OK;
    Dem_EventIdType eventToRemove = DEM_EVENT_ID_NULL;
    /* No free position found. See if any of the stored events may be removed */
#if defined(DEM_DISPLACEMENT_PROCESSING_DEM_EXTENSION)
    Dem_Extension_GetEventForDisplacement(eventParam, eventBuffer, bufferSize, &eventToRemove);
#elif defined(DEM_DISPLACEMENT_PROCESSING_DEM_INTERNAL)
    if( E_OK != getEventForDisplacement(eventParam, eventBuffer, bufferSize, &eventToRemove) ) {
        eventToRemove = DEM_EVENT_ID_NULL;
    }
#else
#warning Unsupported displacement
#endif
    if( DEM_EVENT_ID_NULL != eventToRemove ) {
        const Dem_EventParameterType *removeEventParam = NULL;
        lookupEventIdParameter(eventToRemove, &removeEventParam);
        if( NULL != removeEventParam ) {
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
            const Dem_DTCClassType *DTCClass = removeEventParam->DTCClassRef;
            if( NULL_PTR != DTCClass ) {
                /* @req DEM443 */
                for(uint16 evIdx = 0; evIdx < DTCClass->NofEvents; evIdx++) {
                    removeEventParam = NULL;
                    lookupEventIdParameter(DTCClass->Events[evIdx], &removeEventParam);
                    if( NULL != removeEventParam ) {
                        if( E_OK == findAndDisplaceEvent(removeEventParam, eventBuffer, bufferSize, memEventStatusRec, origin) ) {
                            ret = E_OK;
                        }
                    }
                }
            }
            else {
                ret = findAndDisplaceEvent(removeEventParam, eventBuffer, bufferSize, memEventStatusRec, origin);
            }
#else
            ret = findAndDisplaceEvent(removeEventParam, eventBuffer, bufferSize, memEventStatusRec, origin);
#endif /* DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1 */
        }
    } else {
        /* Buffer is full and the currently stored data is more significant */
    }
    return ret;
}
#endif

/*
 * Procedure:   storeEventMem
 * Description: Store the event data of "eventStatus->eventId" in eventBuffer (i.e.primary or secondary memory),
 *              if non existent a new entry is created.
 */
static Std_ReturnType storeEventMem(const Dem_EventParameterType *eventParam, const EventStatusRecType *eventStatus,
        EventRecType* buffer, uint32 bufferSize, Dem_DTCOriginType origin, boolean failedNow)
{
    boolean positionFound = FALSE;
    EventRecType *eventStatusRec = NULL;
    Std_ReturnType ret = E_OK;
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
    boolean immediateStorage = FALSE;
#endif

    // Lookup event ID
    for (uint32 i = 0uL; (i < bufferSize) && (FALSE == positionFound); i++){
        if( buffer[i].EventData.eventId == eventStatus->eventId ) {
            eventStatusRec = &buffer[i];
            positionFound = TRUE;
        }
    }

    if( FALSE == positionFound ) {
        /* Event is not already stored, Search for free position */
        for (uint32 i  = 0uL; (i < bufferSize) && (FALSE == positionFound); i++){
            if( buffer[i].EventData.eventId == DEM_EVENT_ID_NULL ) {
                eventStatusRec = &buffer[i];
                positionFound = TRUE;
            }
        }
        if( FALSE == positionFound ) {
#if (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON)
        /* @req DEM400 *//* @req DEM407 */
            if( E_OK == lookupEventForDisplacement(eventParam, buffer, bufferSize, &eventStatusRec, origin) ) {
                positionFound = TRUE;
            } else {
                setOverflowIndication(eventParam->EventClass->EventDestination, TRUE);
            }
#else
            /* @req DEM402*/ /* No displacement should be done */
            setOverflowIndication(eventParam->EventClass->EventDestination, TRUE);
#endif /* DEM_EVENT_DISPLACEMENT_SUPPORT */
        }
    }

    if ((TRUE == positionFound) && (NULL != eventStatusRec)) {
        // Update event found
        eventStatusRec->EventData.eventId = eventStatus->eventId;
        eventStatusRec->EventData.occurrence = eventStatus->occurrence;
        eventStatusRec->EventData.eventStatusExtended = eventStatus->eventStatusExtended;
#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
        eventStatusRec->EventData.failureCounter = eventStatus->failureCounter;
#endif
#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL)
        eventStatusRec->EventData.agingCounter = eventStatus->agingCounter;
#endif
#if (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON) && defined(DEM_DISPLACEMENT_PROCESSING_DEM_INTERNAL)
        eventStatusRec->EventData.timeStamp = eventStatus->timeStamp;
#endif
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
        if( (TRUE == failedNow) && (NULL != eventParam->DTCClassRef) && (TRUE == eventParam->DTCClassRef->DTCRef->ImmediateNvStorage)
                && (eventStatus->occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
            immediateStorage = TRUE;
        }
        Dem_NvM_SetEventBlockChanged(origin, immediateStorage);
#else
        (void)failedNow;/* Only used for immediate storage */
        Dem_NvM_SetEventBlockChanged(origin, FALSE);
#endif
    } else {
#if (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON) && defined(DEM_DISPLACEMENT_PROCESSING_DEM_INTERNAL)
        /* Error: mem event buffer full */
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_STORE_EVENT_MEM_ID, DEM_E_MEM_EVENT_BUFF_FULL);
#endif
        /* Buffer is full and all stored events are more significant */
        ret = E_NOT_OK;
    }

    return ret;
}

/*
 * Procedure:   deleteEventMem
 * Description: Delete the event data of "eventParam->eventId" from event buffer".
 */
static boolean deleteEventMem(const Dem_EventParameterType *eventParam, EventRecType* eventMemory, uint32 eventMemorySize, Dem_DTCOriginType origin)
{
    boolean eventIdFound = FALSE;
    uint32 i;

    for (i = 0uL; (i < eventMemorySize) && (FALSE == eventIdFound); i++){
        eventIdFound = (eventMemory[i].EventData.eventId == eventParam->EventID)? TRUE: FALSE;
    }

    if (TRUE == eventIdFound) {
        memset(&eventMemory[i-1], 0, sizeof(EventRecType));
        /* IMPROVEMENT: Immediate storage when delecting event? */
        Dem_NvM_SetEventBlockChanged(origin, FALSE);
    }
    return eventIdFound;
}
#endif /* DEM_USE_MEMORY_FUNCTIONS */


/**
 * Deletes DTC data i.e. event data, ff data and extended data for an event.
 * @param eventParam - the event which data shall be deleted for
 * @param dtcOriginFound - TRUE if origin found otherwise FALSE
 * @return TRUE if any data deleted otherwise FALSE
 */
static boolean DeleteDTCData(const Dem_EventParameterType *eventParam, boolean resetEventstatus, boolean *dtcOriginFound, boolean combinedDTC) {

    boolean dataDeleted = FALSE;

    switch (eventParam->EventClass->EventDestination)
    {
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
    case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
        /** @req DEM077 */
        if( TRUE == deleteEventMem(eventParam, priMemEventBuffer, DEM_MAX_NUMBER_EVENT_PRI_MEM, DEM_DTC_ORIGIN_PRIMARY_MEMORY) ) {
            dataDeleted = TRUE;
        }
        if( TRUE == deleteFreezeFrameDataMem(eventParam, DEM_DTC_ORIGIN_PRIMARY_MEMORY, combinedDTC) ) {
            dataDeleted = TRUE;
        }
        if( TRUE == deleteExtendedDataMem(eventParam, DEM_DTC_ORIGIN_PRIMARY_MEMORY, combinedDTC) ) {
            dataDeleted = TRUE;
        }
        if( TRUE == resetEventstatus ) {
            resetEventStatusRec(eventParam);
        }
        *dtcOriginFound = TRUE;
        break;
#endif
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
    case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
        if( TRUE == deleteEventMem(eventParam, secMemEventBuffer, DEM_MAX_NUMBER_EVENT_SEC_MEM, DEM_DTC_ORIGIN_SECONDARY_MEMORY) ) {
            dataDeleted = TRUE;
        }
        if( TRUE == deleteFreezeFrameDataMem(eventParam, DEM_DTC_ORIGIN_SECONDARY_MEMORY, combinedDTC) ) {
            dataDeleted = TRUE;
        }
        if( TRUE == deleteExtendedDataMem(eventParam, DEM_DTC_ORIGIN_SECONDARY_MEMORY, combinedDTC) ) {
            dataDeleted = TRUE;
        }
        if( TRUE == resetEventstatus ) {
            resetEventStatusRec(eventParam);
        }
        *dtcOriginFound = TRUE;
        break;
#endif
    default:
        *dtcOriginFound = FALSE;
        break;
    }
    return dataDeleted;
}

#if defined(DEM_USE_MEMORY_FUNCTIONS)
/**
 * Checks if an event is stored in its event destination
 * @param eventParam
 * @return TRUE: Event is stored in event memory, FALSE: Event not stored in event memory
 */
static boolean isInEventMemory(const Dem_EventParameterType *eventParam)
{
    boolean found = FALSE;
    uint16 memSize = 0;
    const EventRecType *mem = NULL;
    switch (eventParam->EventClass->EventDestination) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
            mem = priMemEventBuffer;
            memSize = DEM_MAX_NUMBER_EVENT_ENTRY_PRI;
#endif
            break;
        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
            mem = secMemEventBuffer;
            memSize = DEM_MAX_NUMBER_EVENT_ENTRY_SEC;
#endif
            break;
        default:
            break;
    }

    if( NULL != mem ) {
        for(uint16 i = 0; (i < memSize) && (FALSE == found); i++) {
            if( eventParam->EventID == mem[i].EventData.eventId ) {
                found = TRUE;
            }
        }
    }
    return found;
}
#endif

/*
 * Procedure:   storeEventEvtMem
 * Description: Store the event data of "eventStatus->eventId" in event memory according to
 *              "eventParam" destination option.
 */
static Std_ReturnType storeEventEvtMem(const Dem_EventParameterType *eventParam, const EventStatusRecType *eventStatus, boolean failedNow)
{
    Std_ReturnType ret = E_NOT_OK;

    switch (eventParam->EventClass->EventDestination) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
            ret = storeEventMem(eventParam, eventStatus, priMemEventBuffer, DEM_MAX_NUMBER_EVENT_ENTRY_PRI, DEM_DTC_ORIGIN_PRIMARY_MEMORY, failedNow); /** @req DEM010 */
#endif
            break;
        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
            ret = storeEventMem(eventParam, eventStatus, secMemEventBuffer, DEM_MAX_NUMBER_EVENT_ENTRY_SEC, DEM_DTC_ORIGIN_SECONDARY_MEMORY, failedNow);    /** @req DEM548 */
#endif
            break;
        case DEM_DTC_ORIGIN_PERMANENT_MEMORY:
        case DEM_DTC_ORIGIN_MIRROR_MEMORY:
            // Not yet supported
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GLOBAL_ID, DEM_E_NOT_IMPLEMENTED_YET);
            break;
        default:
            break;
    }

    return ret;
}


/*
 * Procedure:   getExtendedDataMem
 * Description: Get record from buffer if it exists, or pick next free if it doesn't
 */
#ifdef DEM_USE_MEMORY_FUNCTIONS
#if (DEM_EXT_DATA_IN_PRE_INIT )
static void getExtendedDataMem(const Dem_EventParameterType *eventParam, ExtDataRecType ** const extendedData,
                                  ExtDataRecType* extendedDataBuffer, uint32 extendedDataBufferSize) /** @req DEM041 */
{
    boolean eventIdFound = FALSE;
    boolean eventIdFreePositionFound=FALSE;
    uint16 i;
    Dem_EventIdType idToFind;

#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        idToFind = TO_COMBINED_EVENT_ID(eventParam->CombinedDTCCID);
    }
    else {
        idToFind = eventParam->EventID;
    }
#else
    idToFind = eventParam->EventID;
#endif
    // Check if already stored
    for (i = 0; (i < extendedDataBufferSize) && (FALSE == eventIdFound); i++){
        if( extendedDataBuffer[i].eventId == idToFind ) {
            *extendedData = &extendedDataBuffer[i];
            eventIdFound = TRUE;
        }
    }

    if ( FALSE == eventIdFound ) {
        // No, lookup first free position
        for (i = 0; (i < extendedDataBufferSize) && (FALSE == eventIdFreePositionFound); i++){
            eventIdFreePositionFound =  (extendedDataBuffer[i].eventId == DEM_EVENT_ID_NULL);
        }
        if ( TRUE == eventIdFreePositionFound ) {
            *extendedData = &extendedDataBuffer[i-1];
        } else {
#if (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON)
            if(E_OK != lookupExtDataForDisplacement(eventParam, extendedDataBuffer, extendedDataBufferSize, extendedData)) {
                *extendedData = NULL;
            }
#else
            /* Displacement supported disabled */
            /* Error: mem extended data buffer full */
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_STORE_EXT_DATA_MEM_ID, DEM_E_MEM_EXT_DATA_BUFF_FULL);

#endif /* DEM_EVENT_DISPLACEMENT_SUPPORT */
        }
    }
}
#endif

/*
 * Procedure:   deleteExtendedDataMem
 * Description: Delete the extended data of "eventParam->eventId" from "priMemExtDataBuffer".
 */
static boolean deleteExtendedDataMem(const Dem_EventParameterType *eventParam, Dem_DTCOriginType origin, boolean combinedDTC)
{
    boolean eventIdFound = FALSE;
    uint32 i;

    ExtDataRecType* extBuffer = NULL;
    uint32 bufferSize = 0;
    Dem_EventIdType idToFind;

    switch (origin) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_EXT_DATA_IN_PRI_MEM)
            extBuffer = priMemExtDataBuffer;
            bufferSize = DEM_MAX_NUMBER_EXT_DATA_PRI_MEM;
#endif
            break;
        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if ((DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_EXT_DATA_IN_SEC_MEM)
            extBuffer = secMemExtDataBuffer;
            bufferSize = DEM_MAX_NUMBER_EXT_DATA_SEC_MEM;
#endif
            break;
        case DEM_DTC_ORIGIN_PERMANENT_MEMORY:
        case DEM_DTC_ORIGIN_MIRROR_MEMORY:
            // Not yet supported
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GLOBAL_ID, DEM_E_NOT_IMPLEMENTED_YET);
            break;
        default:
            break;
    }
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( (TRUE == combinedDTC) && (DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID) ) {
        idToFind = TO_COMBINED_EVENT_ID(eventParam->CombinedDTCCID);
    }
    else {
        idToFind = eventParam->EventID;
    }
#else
    (void)combinedDTC;
    idToFind = eventParam->EventID;
#endif
    if( NULL != extBuffer ) {
        // Check if already stored
        for (i = 0uL; (i < bufferSize) && (FALSE == eventIdFound); i++){
            eventIdFound = (extBuffer[i].eventId == idToFind)? TRUE: FALSE;
        }

        if (TRUE == eventIdFound) {
            // Yes, clear record
            memset(&extBuffer[i-1], 0, sizeof(ExtDataRecType));
            /* IMPROVEMENT: Immediate storage when deleting data? */
            Dem_NvM_SetExtendedDataBlockChanged(origin, FALSE);
        }
    }
    return eventIdFound;
}

/*
 * Procedure:   storeExtendedDataEvtMem
 * Description: Store the extended data in event memory according to
 *              "eventParam" destination option
 */
#if ( DEM_EXT_DATA_IN_PRE_INIT )
static boolean mergeExtendedDataEvtMem(const ExtDataRecType *extendedData, ExtDataRecType* extendedDataBuffer,
                                    uint32 extendedDataBufferSize, Dem_DTCOriginType origin, boolean updateAllExtData)
{
    uint16 i;
    const Dem_ExtendedDataRecordClassType *extendedDataRecordClass;
    ExtDataRecType *memExtDataRec = NULL;
    uint16 storeIndex = 0;
    boolean bCopiedData = FALSE;
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
    boolean immediateStorage = FALSE;
#endif
    const Dem_EventParameterType *eventParam = NULL_PTR;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( IS_COMBINED_EVENT_ID(extendedData->eventId) ) {
        /* This is a combined event entry. */
        /* Get DTC config */
        const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(extendedData->eventId)];
        /* Just grab the first event for this DTC. */
        lookupEventIdParameter(CombDTCCfg->DTCClassRef->Events[0u], &eventParam);
    }
    else {
        lookupEventIdParameter(extendedData->eventId, &eventParam);
    }
#else
    lookupEventIdParameter(extendedData->eventId, &eventParam);
#endif
    if( (NULL_PTR != eventParam) && (eventParam->EventClass->EventDestination == origin) ) {
        /* Management is only relevant for events stored in destinatio mem (i.e. nvram) */

        getExtendedDataMem(eventParam, &memExtDataRec, extendedDataBuffer, extendedDataBufferSize);

        if( NULL != memExtDataRec ) {
            /* We found an old record or could allocate a new slot */

            /* Only copy extended data related to event set during pre-init */
            for(i = 0; (i < DEM_MAX_NR_OF_RECORDS_IN_EXTENDED_DATA) && (eventParam->ExtendedDataClassRef->ExtendedDataRecordClassRef[i] != NULL); i++) {
                extendedDataRecordClass = eventParam->ExtendedDataClassRef->ExtendedDataRecordClassRef[i];
                if( DEM_UPDATE_RECORD_VOLATILE != extendedDataRecordClass->UpdateRule ) {
                    if( DEM_UPDATE_RECORD_YES == extendedDataRecordClass->UpdateRule ) {
                        /* Copy records that failed during pre init */
                        memcpy(&memExtDataRec->data[storeIndex], &extendedData->data[storeIndex],extendedDataRecordClass->DataSize);
                        bCopiedData = TRUE;
                    }
                    else if( DEM_UPDATE_RECORD_NO == extendedDataRecordClass->UpdateRule ) {
                        if( (eventParam->EventID != memExtDataRec->eventId)  || (TRUE == updateAllExtData) ) {
                            /* Extended data was not previously stored for this event. */
                            memcpy(&memExtDataRec->data[storeIndex], &extendedData->data[storeIndex],extendedDataRecordClass->DataSize);
                            bCopiedData = TRUE;
                        }
                    }
                    else {
                        /* DET FEL */
                    }
                    storeIndex += extendedDataRecordClass->DataSize;
                }
            }
            if( TRUE == bCopiedData ) {
                memExtDataRec->eventId = extendedData->eventId;
#if (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON) && defined(DEM_DISPLACEMENT_PROCESSING_DEM_INTERNAL)
                memExtDataRec->timeStamp = extendedData->timeStamp;
#endif
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
                EventStatusRecType *eventStatusRecPtr = NULL;
                lookupEventStatusRec(eventParam->EventID, &eventStatusRecPtr);
                if( (NULL != eventStatusRecPtr) && (NULL != eventParam->DTCClassRef) &&
                        (TRUE == eventParam->DTCClassRef->DTCRef->ImmediateNvStorage) && (eventStatusRecPtr->occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
                    immediateStorage = TRUE;
                }
                Dem_NvM_SetExtendedDataBlockChanged(origin, immediateStorage);
#else
                Dem_NvM_SetExtendedDataBlockChanged(origin, FALSE);
#endif
            }
        }
        else {
            /* DET FEL */
        }
    }
    return bCopiedData;
}
#endif
#endif /* DEM_USE_MEMORY_FUNCTIONS */


/*
 * Procedure:   lookupExtendedDataRecNumParam
 * Description: Returns TRUE if the requested extended data number was found among the configured records for the event.
 *              "extDataRecClassPtr" returns a pointer to the record class, "posInExtData" returns the position in stored extended data.
 */
static boolean lookupExtendedDataRecNumParam(uint8 extendedDataNumber, const Dem_EventParameterType *eventParam, Dem_ExtendedDataRecordClassType const **extDataRecClassPtr, uint16 *posInExtData)
{
    boolean recNumFound = FALSE;
    const Dem_ExtendedDataClassType *ExtendedDataClass;

#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        ExtendedDataClass = configSet->CombinedDTCConfig[eventParam->CombinedDTCCID].ExtendedDataClassRef;
    }
    else {
        ExtendedDataClass = eventParam->ExtendedDataClassRef;
    }
#else
    ExtendedDataClass = eventParam->ExtendedDataClassRef;
#endif

    if (ExtendedDataClass != NULL) {
        uint16  byteCnt = 0;
        uint32 i;

        // Request extended data and copy it to the buffer
        for (i = 0uL; (i < DEM_MAX_NR_OF_RECORDS_IN_EXTENDED_DATA) && (ExtendedDataClass->ExtendedDataRecordClassRef[i] != NULL) && (recNumFound == FALSE); i++) {
            if (ExtendedDataClass->ExtendedDataRecordClassRef[i]->RecordNumber == extendedDataNumber) {
                *extDataRecClassPtr =  ExtendedDataClass->ExtendedDataRecordClassRef[i];
                *posInExtData = byteCnt;
                recNumFound = TRUE;
            }
            if(DEM_UPDATE_RECORD_VOLATILE != ExtendedDataClass->ExtendedDataRecordClassRef[i]->UpdateRule) {
                byteCnt += ExtendedDataClass->ExtendedDataRecordClassRef[i]->DataSize;
            }
        }
    }

    return recNumFound;
}


/*
 * Procedure:   lookupExtendedDataMem
 * Description: Returns TRUE if the requested event id is found, "extData" points to the found data.
 */
static boolean lookupExtendedDataMem(Dem_EventIdType eventId, ExtDataRecType **extData, Dem_DTCOriginType origin)
{
    boolean eventIdFound = FALSE;
    uint32 i;
    ExtDataRecType* extBuffer = NULL;
    uint32 extBufferSize = 0;

    switch (origin) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_EXT_DATA_IN_PRI_MEM)
            extBuffer = priMemExtDataBuffer;
            extBufferSize = DEM_MAX_NUMBER_EXT_DATA_PRI_MEM;
#endif
            break;
        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if ((DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_EXT_DATA_IN_SEC_MEM)
            extBuffer = secMemExtDataBuffer;
            extBufferSize = DEM_MAX_NUMBER_EXT_DATA_SEC_MEM;
#endif
            break;
        default:
            break;
    }
    if( NULL == extBuffer ) {
        return FALSE;
    }
    // Lookup corresponding extended data
    for (i = 0uL; (i < extBufferSize) && (FALSE == eventIdFound); i++) {
        eventIdFound = (extBuffer[i].eventId == eventId)? TRUE: FALSE;
    }

    if (TRUE == eventIdFound) {
        // Yes, return pointer
        *extData = &extBuffer[i-1];
    }

    return eventIdFound;
}

#if (DEM_USE_TIMESTAMPS == STD_ON) && (DEM_EVENT_COMB_TYPE2_REPORT_OLDEST_DATA == STD_ON)
/**
 * Searcher buffer for older (compared to timestamp provided) extended data entry. Timestamp check is done based on parameter checkTimeStamp.
 * @param eventId
 * @param extData
 * @param origin
 * @param timestamp
 * @param checkTimeStamp
 * @return
 */
static boolean lookupOlderExtendedDataMem(Dem_EventIdType eventId, ExtDataRecType **extData, Dem_DTCOriginType origin, uint32 *timestamp, boolean checkTimeStamp)
{
    boolean eventIdFound = FALSE;
    uint32 i;
    ExtDataRecType* extBuffer = NULL;
    uint32 extBufferSize = 0;

    switch (origin) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_EXT_DATA_IN_PRI_MEM)
            extBuffer = priMemExtDataBuffer;
            extBufferSize = DEM_MAX_NUMBER_EXT_DATA_PRI_MEM;
#endif
            break;
        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if ((DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_EXT_DATA_IN_SEC_MEM)
            extBuffer = secMemExtDataBuffer;
            extBufferSize = DEM_MAX_NUMBER_EXT_DATA_SEC_MEM;
#endif
            break;
        default:
            break;
    }
    if( NULL == extBuffer ) {
        return FALSE;
    }
    // Lookup corresponding extended data
    for (i = 0uL; (i < extBufferSize) && (FALSE == eventIdFound); i++) {
        eventIdFound = (extBuffer[i].eventId == eventId)? TRUE: FALSE;
    }

    if ( TRUE == eventIdFound ) {
        if( (FALSE == checkTimeStamp) || (*timestamp > extBuffer[i-1u].timeStamp) ) {
            // Yes, return pointer
            *timestamp = extBuffer[i-1u].timeStamp;
            *extData = &extBuffer[i-1u];
        }
        else {
            eventIdFound = FALSE;
        }
    }

    return eventIdFound;
}
#endif
/*
 * Procedure:   storeFreezeFrameDataMem
 * Description: store FreezeFrame data record in primary memory
 */
#ifdef DEM_USE_MEMORY_FUNCTIONS
#if ( DEM_FF_DATA_IN_PRE_INIT || DEM_FF_DATA_IN_PRI_MEM || DEM_FF_DATA_IN_SEC_MEM )
static boolean storeFreezeFrameDataMem(const Dem_EventParameterType *eventParam, const FreezeFrameRecType *freezeFrame,
                                       FreezeFrameRecType* freezeFrameBuffer, uint32 freezeFrameBufferSize,
                                       Dem_DTCOriginType  origin)
{
    boolean eventIdFound = FALSE;
    boolean eventIdFreePositionFound=FALSE;
    boolean ffUpdated = FALSE;
    uint32 i;

    /* Check if already stored */
    for (i = 0uL; (i < freezeFrameBufferSize) && (FALSE == eventIdFound); i++){
        eventIdFound = ((freezeFrameBuffer[i].eventId == freezeFrame->eventId) && (freezeFrameBuffer[i].recordNumber == freezeFrame->recordNumber))? TRUE: FALSE;
    }

    if ( TRUE == eventIdFound ) {
        memcpy(&freezeFrameBuffer[i-1], freezeFrame, sizeof(FreezeFrameRecType));
        ffUpdated = TRUE;
    }
    else {
        for (i = 0uL; (i < freezeFrameBufferSize) && (FALSE == eventIdFreePositionFound); i++){
            eventIdFreePositionFound =  (freezeFrameBuffer[i].eventId == DEM_EVENT_ID_NULL)? TRUE: FALSE;
        }
        if ( TRUE == eventIdFreePositionFound ) {
            memcpy(&freezeFrameBuffer[i-1], freezeFrame, sizeof(FreezeFrameRecType));
            ffUpdated = TRUE;
        } else {
#if (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON)
            /* @req DEM400 *//* @req DEM407 */
            FreezeFrameRecType *freezeFrameLocal;
            if( TRUE == lookupFreezeFrameForDisplacement(eventParam, &freezeFrameLocal, freezeFrameBuffer, freezeFrameBufferSize) ){
                memcpy(freezeFrameLocal, freezeFrame, sizeof(FreezeFrameRecType));
                ffUpdated = TRUE;
            } else {
                setOverflowIndication(eventParam->EventClass->EventDestination, TRUE);
            }
#else
            /* @req DEM402*/ /* Req is not the Det-error.. */
            setOverflowIndication(eventParam->EventClass->EventDestination, TRUE);
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_STORE_FF_DATA_MEM_ID, DEM_E_MEM_FF_DATA_BUFF_FULL);
#endif
        }
    }

    if( TRUE == ffUpdated ) {
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
        boolean immediateStorage = FALSE;
        EventStatusRecType *eventStatusRecPtr = NULL;
        lookupEventStatusRec(eventParam->EventID, &eventStatusRecPtr);
        if( (NULL != eventStatusRecPtr) && (NULL != eventParam->DTCClassRef) &&
                (TRUE == eventParam->DTCClassRef->DTCRef->ImmediateNvStorage) && (eventStatusRecPtr->occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
            immediateStorage = TRUE;
        }
#if (DEM_PRESTORAGE_FF_DATA_IN_MEM)
        if ( (TRUE == eventParam->EventClass->FFPrestorageSupported) && (origin == DEM_DTC_ORIGIN_NOT_USED) ) {
        	Dem_NvM_SetPreStoreFreezeFrameBlockChanged(immediateStorage);
        } else {
        	Dem_NvM_SetFreezeFrameBlockChanged(origin, immediateStorage);
        }
#else
        Dem_NvM_SetFreezeFrameBlockChanged(origin, immediateStorage);
#endif
#else
#if (DEM_PRESTORAGE_FF_DATA_IN_MEM)
        if (eventParam->EventClass->FFPrestorageSupported && origin == DEM_DTC_ORIGIN_NOT_USED ) {
        	Dem_NvM_SetPreStoreFreezeFrameBlockChanged(FALSE);
        } else {
        	Dem_NvM_SetFreezeFrameBlockChanged(origin, FALSE);
        }
#else
        Dem_NvM_SetFreezeFrameBlockChanged(origin, FALSE);
#endif
#endif
    }
    return ffUpdated;
}
#endif

static boolean deleteFreezeFrameDataMem(const Dem_EventParameterType *eventParam, Dem_DTCOriginType origin, boolean combinedDTC)
{
    uint32 i;
    boolean ffDeleted = FALSE;
    FreezeFrameRecType* freezeFrameBuffer = NULL_PTR;
    uint32 bufferSize = 0uL;
    Dem_EventIdType idToFind;

    switch (origin) {
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if ( DEM_FF_DATA_IN_PRI_MEM )
            freezeFrameBuffer = priMemFreezeFrameBuffer;
            bufferSize = DEM_MAX_NUMBER_FF_DATA_PRI_MEM;
#else
            freezeFrameBuffer = NULL_PTR;
            bufferSize = 0uL;
#endif
            break;
#endif
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if ( DEM_FF_DATA_IN_SEC_MEM )
            freezeFrameBuffer = secMemFreezeFrameBuffer;
            bufferSize = DEM_MAX_NUMBER_FF_DATA_SEC_MEM;
#endif
            break;
#endif
        case DEM_DTC_ORIGIN_NOT_USED:
#if (DEM_PRESTORAGE_FF_DATA_IN_MEM)
        	 /* It could be a pre-stored freeze frame */
			if (TRUE == eventParam->EventClass->FFPrestorageSupported ) {
				freezeFrameBuffer = memPreStoreFreezeFrameBuffer;
				bufferSize = DEM_MAX_NUMBER_PRESTORED_FF;
			}
#endif
			break;
        default:
            break;
    }

#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( (TRUE == combinedDTC) && (DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID) ) {
        idToFind = TO_COMBINED_EVENT_ID(eventParam->CombinedDTCCID);
    }
    else {
        idToFind = eventParam->EventID;
    }
#else
    (void)combinedDTC;
    idToFind = eventParam->EventID;
#endif

    if( NULL != freezeFrameBuffer ) {
        for (i = 0uL; i < bufferSize; i++){
            if (freezeFrameBuffer[i].eventId == idToFind){
                memset(&freezeFrameBuffer[i], 0, sizeof(FreezeFrameRecType));
                ffDeleted = TRUE;
            }
        }

        if( TRUE == ffDeleted ) {
            /* IMPROVEMENT: Immediate storage when deleting data? */
#if (DEM_PRESTORAGE_FF_DATA_IN_MEM)
        	if ((TRUE == eventParam->EventClass->FFPrestorageSupported) && (origin == DEM_DTC_ORIGIN_NOT_USED) ) {
        		Dem_NvM_SetPreStoreFreezeFrameBlockChanged(FALSE);
        	} else {
        		 Dem_NvM_SetFreezeFrameBlockChanged(origin, FALSE);
        	}
#else
        	Dem_NvM_SetFreezeFrameBlockChanged(origin, FALSE);
#endif
        }
    }
    return ffDeleted;
}
#endif /* DEM_USE_MEMORY_FUNCTIONS */
/*
 * Procedure:   storeFreezeFrameDataEvtMem
 * Description: Store the freeze frame data in event memory according to
 *              "eventParam" destination option
 */
static boolean storeFreezeFrameDataEvtMem(const Dem_EventParameterType *eventParam, FreezeFrameRecType *freezeFrame,
                                       Dem_FreezeFrameKindType ffKind)
{
    boolean ret = FALSE;
    switch (eventParam->EventClass->EventDestination) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM)
            getFreezeFrameData(eventParam, freezeFrame, ffKind, DEM_DTC_ORIGIN_PRIMARY_MEMORY, TRUE);
            if (freezeFrame->eventId != DEM_EVENT_ID_NULL) {
                if(freezeFrame->kind == DEM_FREEZE_FRAME_OBD){
                    ret = storeOBDFreezeFrameDataMem(eventParam, freezeFrame,priMemFreezeFrameBuffer,
                            DEM_MAX_NUMBER_FF_DATA_PRI_MEM, DEM_DTC_ORIGIN_PRIMARY_MEMORY);
                }
                else {
                    ret = storeFreezeFrameDataMem(eventParam, freezeFrame, priMemFreezeFrameBuffer,
                                            DEM_MAX_NUMBER_FF_DATA_PRI_MEM, DEM_DTC_ORIGIN_PRIMARY_MEMORY); /** @req DEM190 */
                }
            }
#endif
            break;
        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if ((DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_SEC_MEM)
            getFreezeFrameData(eventParam, freezeFrame, ffKind, DEM_DTC_ORIGIN_SECONDARY_MEMORY, TRUE);
            if (freezeFrame->eventId != DEM_EVENT_ID_NULL) {
                if(freezeFrame->kind == DEM_FREEZE_FRAME_OBD){
                    DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GLOBAL_ID, DEM_E_OBD_NOT_ALLOWED_IN_SEC_MEM);
                }
                else {
                    ret = storeFreezeFrameDataMem(eventParam, freezeFrame, secMemFreezeFrameBuffer,
                                            DEM_MAX_NUMBER_FF_DATA_SEC_MEM, DEM_DTC_ORIGIN_SECONDARY_MEMORY); /** @req DEM190 */
                }
            }
#endif
            break;
        case DEM_DTC_ORIGIN_PERMANENT_MEMORY:
        case DEM_DTC_ORIGIN_MIRROR_MEMORY:
            // Not yet supported
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GLOBAL_ID, DEM_E_NOT_IMPLEMENTED_YET);
            break;
        default:
            (void)freezeFrame; /*lint !e920 Avoid compiler warning (variable not used) */
            (void)ffKind;
            break;
    }
    return ret;
}

/*
 * Procedure:   lookupFreezeFrameDataRecNumParam
 * Description: Returns TRUE if the requested freezeFrame data number was found among the configured records for the event.
 *              "freezeFrameClassPtr" returns a pointer to the record class.
 */
static boolean lookupFreezeFrameDataRecNumParam(uint8 recordNumber, const Dem_EventParameterType *eventParam, Dem_FreezeFrameClassType const **freezeFrameClassPtr)
{
    boolean recNumFound = FALSE;
    uint8 maxNofRecords;
    const Dem_FreezeFrameRecNumClass *FreezeFrameRecNumClass;
    Dem_FreezeFrameClassTypeRefIndex ffIdx = DEM_FF_NULLREF;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[eventParam->CombinedDTCCID];
        maxNofRecords = CombDTCCfg->MaxNumberFreezeFrameRecords;
        FreezeFrameRecNumClass = CombDTCCfg->FreezeFrameRecNumClassRef;
        ffIdx = CombDTCCfg->Calib->FreezeFrameClassIdx;
    }
    else {
        maxNofRecords = eventParam->MaxNumberFreezeFrameRecords;
        FreezeFrameRecNumClass = eventParam->FreezeFrameRecNumClassRef;
        ffIdx = *(eventParam->FreezeFrameClassRefIdx);
    }
#else
    maxNofRecords = eventParam->MaxNumberFreezeFrameRecords;
    FreezeFrameRecNumClass = eventParam->FreezeFrameRecNumClassRef;
    ffIdx = *(eventParam->FreezeFrameClassRefIdx);
#endif

    if ( (ffIdx != DEM_FF_NULLREF) && (NULL != FreezeFrameRecNumClass)) {
        for( uint8 i = 0; (i < maxNofRecords) && (FALSE == recNumFound); i++ ) {
            if( FreezeFrameRecNumClass->FreezeFrameRecordNumber[i] == recordNumber ) {
                recNumFound = TRUE;
                getFFClassReference(eventParam, (Dem_FreezeFrameClassType **) freezeFrameClassPtr);
            }
        }
    }

    return recNumFound;
}

/*
 * Procedure:   lookupFreezeFrameDataSize
 * Description: Returns TRUE if the requested freezeFrame data size was obtained successfully from the configuration.
 *              "dataSize" returns a pointer to the data size.
 */
static boolean lookupFreezeFrameDataSize(uint8 recordNumber, const Dem_FreezeFrameClassType const  * const *freezeFrameClassPtr, uint16 *dataSize)
{
    boolean dataSizeFound = FALSE;
    uint16 i;

    (void)recordNumber; /* Avoid compiler warning - can this be removed */
    *dataSize = 0;
    if (*freezeFrameClassPtr != NULL) {
        dataSizeFound = TRUE;
        for (i = 0u; (i < DEM_MAX_NR_OF_DIDS_IN_FREEZEFRAME_DATA) && ((*freezeFrameClassPtr)->FFIdClassRef[i]->Arc_EOL != TRUE); i++) {
            *dataSize += (uint16)(*freezeFrameClassPtr)->FFIdClassRef[i]->PidOrDidSize + DEM_DID_IDENTIFIER_SIZE_OF_BYTES;
        }
    }

    return dataSizeFound;
}

/**
 * Looks for a stored freeze frame with correct record number has been stored for an event
 * @param eventId
 * @param recordNumber
 * @param dtcOrigin
 * @param freezeFrame
 * @return TRUE: if a freeze frame with the correct record number was stored for this event, FALSE: Otherwise
 */
static boolean getStoredFreezeFrame(Dem_EventIdType eventId, uint8 recordNumber, Dem_DTCOriginType dtcOrigin, FreezeFrameRecType **freezeFrame)
{
    boolean ffFound = FALSE;

    FreezeFrameRecType* freezeFrameBuffer = NULL;
    uint32 freezeFrameBufferSize = 0;

    switch (dtcOrigin) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM)
            freezeFrameBuffer = priMemFreezeFrameBuffer;
            freezeFrameBufferSize = DEM_MAX_NUMBER_FF_DATA_PRI_MEM;
#endif
            break;
        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if ((DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_SEC_MEM)
            freezeFrameBuffer = secMemFreezeFrameBuffer;
            freezeFrameBufferSize = DEM_MAX_NUMBER_FF_DATA_SEC_MEM;
#endif
            break;
        default:
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GLOBAL_ID, DEM_E_NOT_IMPLEMENTED_YET);
            break;
    }

    if (freezeFrameBuffer != NULL) {

        for (uint32 i = 0uL; (i < freezeFrameBufferSize) && (FALSE == ffFound); i++) {
            ffFound = ((freezeFrameBuffer[i].eventId == eventId) && (freezeFrameBuffer[i].recordNumber == recordNumber))? TRUE: FALSE;
            if(TRUE == ffFound) {
                *freezeFrame = &freezeFrameBuffer[i];
            }
        }
    }
    return ffFound;
}

#if (DEM_USE_TIMESTAMPS == STD_ON) && (DEM_EVENT_COMB_TYPE2_REPORT_OLDEST_DATA == STD_ON)
/**
 * Gets and updates freeze frame data if older than provided timestamp
 * @param eventId
 * @param recordNumber
 * @param dtcOrigin
 * @param destBuffer
 * @param bufSize
 * @param FFDataSize
 * @param timestamp
 * @param checkTimeStamp
 * @return
 */
static boolean getOlderFreezeFrameRecord(Dem_EventIdType eventId, uint8 recordNumber, Dem_DTCOriginType dtcOrigin, uint8* destBuffer, uint16* bufSize,
        const uint16 FFDataSize, uint32 *timestamp, boolean checkTimeStamp)
{

    boolean ffFound = FALSE;

    FreezeFrameRecType* freezeFrame = NULL;

    if( TRUE == getStoredFreezeFrame(eventId, recordNumber, dtcOrigin, &freezeFrame) ) {
        if( (FALSE == checkTimeStamp) || (*timestamp > freezeFrame->timeStamp) ) {
            memcpy(destBuffer, freezeFrame->data, FFDataSize); /** @req DEM071 */
            *bufSize = FFDataSize;
            *timestamp = freezeFrame->timeStamp;

            ffFound = TRUE;
        }
    }
    return ffFound;
}
#else
/*
 * Procedure:   getFreezeFrameRecord
 * Description: Returns TRUE if the requested event id is found, "freezeFrame" points to the found data.
 */
static boolean getFreezeFrameRecord(Dem_EventIdType eventId, uint8 recordNumber, Dem_DTCOriginType dtcOrigin, uint8* destBuffer, uint16*  bufSize, const uint16 FFDataSize)
{

    boolean ffFound = FALSE;

    FreezeFrameRecType* freezeFrame = NULL;

    if(TRUE == getStoredFreezeFrame(eventId, recordNumber, dtcOrigin, &freezeFrame)) {

        memcpy(destBuffer, freezeFrame->data, FFDataSize); /** @req DEM071 */
        *bufSize = FFDataSize;

        ffFound = TRUE;
    }
    return ffFound;
}
#endif
/**
 * Gets conditions for data storage
 * @param eventFailedNow
 * @param eventDataUppdated
 * @param extensionStorageBitfield
 * @param storeFFData
 * @param storeExtData
 * @param overrideOldExtdata
 */
static void getStorageConditions(boolean eventFailedNow, boolean eventDataUpdated, uint8 extensionStorageBitfield, boolean *storeFFData, boolean *storeExtData, boolean *overrideOldExtData)
{
#if defined(DEM_EXTENDED_DATA_CAPTURE_EVENT_MEMORY_STORAGE)
                    *storeExtData = eventDataUpdated;
                    *overrideOldExtData = FALSE;
#elif defined(DEM_EXTENDED_DATA_CAPTURE_TESTFAILED)
                    *storeExtData = eventFailedNow;
                    *overrideOldExtData = FALSE;
#elif defined(DEM_EXTENDED_DATA_CAPTURE_EXTENSION)
                    /* @req OEM_DEM_10169 DEM_TRIGGER_EXTENSION */
                    *overrideOldExtData = (0 != (extensionStorageBitfield & DEM_EXT_CLEAR_BEFORE_STORE_EXT_DATA_BIT));
                    *storeExtData = (0 != (extensionStorageBitfield & DEM_EXT_STORE_EXT_DATA_BIT));
#else
                    *storeExtData = FALSE;
                    *overrideOldExtData = FALSE;
#endif
#if defined(DEM_FREEZE_FRAME_CAPTURE_EVENT_MEMORY_STORAGE)
                    *storeFFData = eventDataUpdated;
#elif defined(DEM_FREEZE_FRAME_CAPTURE_TESTFAILED)
                    *storeFFData = eventFailedNow;
#elif defined(DEM_FREEZE_FRAME_CAPTURE_EXTENSION)
                    /* @req OEM_DEM_10170 DEM_TRIGGER_EXTENSION */
                    *storeFFData = (0 != (extensionStorageBitfield & DEM_EXT_STORE_FF_BIT));
#else
                    *storeFFData = FALSE;
                    (void)eventFailedNow;  /* Avoid compiler warning */
                    (void)eventDataUpdated; /* Avoid compiler warning */
                    (void)extensionStorageBitfield; /* Avoid compiler warning */
#endif
}

/*
 * Procedure:   handlePreInitEvent
 * Description: Handle the updating of event status and storing of
 *              event related data in preInit buffers.
 */
static void handlePreInitEvent(Dem_EventIdType eventId, Dem_EventStatusType eventStatus)
{
    const Dem_EventParameterType *eventParam;
    EventStatusRecType *eventStatusRec = NULL;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    Dem_EventStatusExtendedType newDTCStatus = DEM_DEFAULT_EVENT_STATUS;
    Dem_EventStatusExtendedType oldDTCStatus = DEM_DEFAULT_EVENT_STATUS;
#endif
    lookupEventIdParameter(eventId, &eventParam);
    if (eventParam != NULL) {
        if ( TRUE == operationCycleIsStarted(eventParam->EventClass->OperationCycleRef) ) {
            lookupEventStatusRec(eventId, &eventStatusRec);
            if( NULL != eventStatusRec ) {
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
                    if( DEM_STATUS_OK != GetDTCUDSStatus(eventParam->DTCClassRef, eventParam->EventClass->EventDestination, &oldDTCStatus) ) {
                        oldDTCStatus = eventStatusRec->eventStatusExtended;
                    }
                }
                else {
                    oldDTCStatus = eventStatusRec->eventStatusExtended;
                }
#endif
                updateEventStatusRec(eventParam, eventStatus, eventStatusRec);
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
                    if( DEM_STATUS_OK != GetDTCUDSStatus(eventParam->DTCClassRef, eventParam->EventClass->EventDestination, &newDTCStatus) ) {
                        newDTCStatus = eventStatusRec->eventStatusExtended;
                    }
                }
                else {
                    newDTCStatus = eventStatusRec->eventStatusExtended;
                }
#endif
                if ( (0 != eventStatusRec->errorStatusChanged) || (0 != eventStatusRec->extensionDataChanged) ) {
                    boolean storeExtData;
                    boolean overrideOldExtData;
                    boolean storeFFData;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                    boolean dtcFailedNow = (((0u == (oldDTCStatus & DEM_TEST_FAILED))) && ((0 != (newDTCStatus & DEM_TEST_FAILED)))) ? TRUE: FALSE;
#else
                    boolean eventFailedNow = ((TRUE == eventStatusRec->errorStatusChanged) && (0 != (eventStatusRec->eventStatusExtended & DEM_TEST_FAILED)))? TRUE: FALSE;
#endif
                    /* Get conditions for data storage */
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                    /* @req DEM163 */
                    getStorageConditions(dtcFailedNow, TRUE, eventStatusRec->extensionDataStoreBitfield, &storeFFData, &storeExtData, &overrideOldExtData);
#else
                    /* @req DEM539 */
                    getStorageConditions(eventFailedNow, TRUE, eventStatusRec->extensionDataStoreBitfield, &storeFFData, &storeExtData, &overrideOldExtData);
#endif
                    if( (TRUE == storeExtData) && (NULL != eventParam->ExtendedDataClassRef) ) {
                        (void)storeExtendedData(eventParam, overrideOldExtData);
                    }
#if( DEM_FF_DATA_IN_PRE_INIT )
                    if( TRUE == storeFFData) {
                        FreezeFrameRecType freezeFrameLocal;
                        if( DEM_FF_NULLREF != getFFIdx(eventParam) ) {
#if defined(DEM_FREEZE_FRAME_CAPTURE_EXTENSION)
                            /* Allow extension to decide if ffs should be deleted before storing */
                            if( 0 != (eventStatusRec->extensionDataStoreBitfield & DEM_EXT_CLEAR_BEFORE_STORE_FF_BIT) ) {
                                deleteFreezeFrameDataPreInit(eventParam);
                            }
#endif
                            getFreezeFrameData(eventParam, &freezeFrameLocal, DEM_FREEZE_FRAME_NON_OBD, DEM_DTC_ORIGIN_NOT_USED, TRUE);
                            if (freezeFrameLocal.eventId != DEM_EVENT_ID_NULL) {
                                storeFreezeFrameDataPreInit(eventParam, &freezeFrameLocal);
                            }
                        }
                        if( (NULL != eventParam->DTCClassRef) && (DEM_NON_EMISSION_RELATED != (Dem_Arc_EventDTCKindType) *eventParam->EventDTCKind) ) {
                            getFreezeFrameData(eventParam, &freezeFrameLocal, DEM_FREEZE_FRAME_OBD, DEM_DTC_ORIGIN_NOT_USED, TRUE);
                            if (freezeFrameLocal.eventId != DEM_EVENT_ID_NULL) {
                                storeFreezeFrameDataPreInit(eventParam, &freezeFrameLocal);
                            }
                        }
                    }
#endif /* DEM_FF_DATA_IN_PRE_INIT */
                }
            }
        }
        else {
            // Operation cycle not set or not started
            // IMPROVEMENT: Report error?
        }
    }
    else {
        // Event ID not configured
        // IMPROVEMENT: Report error?
    }
}

#if (DEM_ENABLE_CONDITION_SUPPORT == STD_ON)
static boolean enableConditionsSet(const Dem_EventClassType *eventClass)
{
    /* @req DEM449 */
    /* @req DEM450 */
    boolean conditionsSet = TRUE;
    if( NULL != eventClass->EnableConditionGroupRef ) {
        /* Each group must reference at least one enable condition. Or this won't work.. */
        const Dem_EnableConditionGroupType *enableConditionGroupPtr = eventClass->EnableConditionGroupRef;
        for( uint8 i = 0; i < enableConditionGroupPtr->nofEnableConditions; i++ ) {
            if( FALSE == DemEnableConditions[enableConditionGroupPtr->EnableCondition[i]->EnableConditionID] ) {
                conditionsSet = FALSE;
            }
        }
    }
    return conditionsSet;
}
#endif

/**
 * Checks whether DTC setting for event is disabled. If the event does not have a DTC (or its DTC is suppressed)
 * setting is NOT disabled.
 * @param eventParam
 * @return TRUE: Disabled, FALSE: NOT disabled
 */
static boolean DTCSettingDisabled(const Dem_EventParameterType *eventParam)
{
    /* @req DEM587 */
    boolean eventDTCSettingDisabled = FALSE;
    if( (disableDtcSetting.settingDisabled == TRUE) && (NULL != eventParam->DTCClassRef) && (DTCIsAvailable(eventParam->DTCClassRef) == TRUE) ) {
        /* DTC setting is disabled and the event has a DTC (which is not suppressed) */
        if( (checkDtcGroup(disableDtcSetting.dtcGroup, eventParam, DEM_DTC_FORMAT_UDS) == TRUE) &&
                (checkDtcKind(disableDtcSetting.dtcKind, eventParam) == TRUE) ) {
            /* Setting of DTC for this event is disabled. */
            eventDTCSettingDisabled = TRUE;
        }
    }
    return eventDTCSettingDisabled;
}

/**
 * Checks whether event processing is allowed
 * @param eventParam
 * @return TRUE: Processing allowed, FALSE: Processing not allowed
 */
static boolean eventProcessingAllowed(const Dem_EventParameterType *eventParam)
{
    /* Event processing is not allowed if event has DTC and DTC setting has been disabled,
     * or if event has enable conditions and these are not set. */
    if ( ( FALSE == DTCSettingDisabled(eventParam) ) /* @req DEM626 */
#if (DEM_ENABLE_CONDITION_SUPPORT == STD_ON)
            && (TRUE == enableConditionsSet(eventParam->EventClass))/* @req DEM447 */
#endif
        )  {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * Checks if condition for storing OBD freeze frame is fulfilled
 * @param eventParam
 * @param oldStatus
 * @param status
 * @return
 */
static boolean checkOBDFFStorageCondition(Dem_EventStatusExtendedType oldStatus, Dem_EventStatusExtendedType status)
{
    if( (0u == (oldStatus & DEM_CONFIRMED_DTC)) && (0u != (status & DEM_CONFIRMED_DTC)) ) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
 * Get index for event FF
 * @param eventParam
 * @return DEM_FF_NULLREF: no FF configured
 */
static Dem_FreezeFrameClassTypeRefIndex getFFIdx(const Dem_EventParameterType *eventParam)
{
    Dem_FreezeFrameClassTypeRefIndex ffIdx = DEM_FF_NULLREF;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[eventParam->CombinedDTCCID];
        ffIdx = CombDTCCfg->Calib->FreezeFrameClassIdx;
    }
    else {
        if( NULL_PTR != eventParam->FreezeFrameClassRefIdx ) {
            ffIdx = *(eventParam->FreezeFrameClassRefIdx);
        }
    }
#else
    if( NULL_PTR != eventParam->FreezeFrameClassRefIdx ) {
        ffIdx = *(eventParam->FreezeFrameClassRefIdx);
    }
#endif
    return ffIdx;
}

#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
/**
 * Gets the status of DTC referenced bu event
 * @param eventParam
 * @param eventStatus
 * @return Status of DTC
 */
static Dem_EventStatusExtendedType getEventDTCStatus(const Dem_EventParameterType *eventParam, Dem_EventStatusExtendedType eventStatus)
{
    Dem_EventStatusExtendedType DTCStatus = DEM_DEFAULT_EVENT_STATUS;
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        if( DEM_STATUS_OK != GetDTCUDSStatus(eventParam->DTCClassRef, eventParam->EventClass->EventDestination, &DTCStatus) ) {
            DTCStatus = eventStatus;
        }
    }
    else {
        DTCStatus = eventStatus;
    }
    return DTCStatus;
}
#endif

/*
 * Procedure:   handleEvent
 * Description: Handle the updating of event status and storing of
 *              event related data in event memory.
 */
Std_ReturnType handleEvent(Dem_EventIdType eventId, Dem_EventStatusType eventStatus)
{
    Std_ReturnType returnCode = E_OK;
    const Dem_EventParameterType *eventParam;
    EventStatusRecType *eventStatusRec;
    FreezeFrameRecType freezeFrameLocal;
    Dem_EventStatusExtendedType oldEventStatus = DEM_DEFAULT_EVENT_STATUS;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
    Dem_EventStatusExtendedType newDTCStatus = DEM_DEFAULT_EVENT_STATUS;
    Dem_EventStatusExtendedType oldDTCStatus = DEM_DEFAULT_EVENT_STATUS;
#endif
    Std_ReturnType eventStoreStatus = E_OK;

    lookupEventIdParameter(eventId, &eventParam);
    lookupEventStatusRec(eventId, &eventStatusRec);
    if ( (eventParam != NULL) && (NULL != eventStatusRec) && (TRUE == eventStatusRec->isAvailable) ) {
        if ( TRUE == operationCycleIsStarted(eventParam->EventClass->OperationCycleRef) ) {/* @req DEM481 */
            /* Check if event processing is allowed (DTC setting, enable condition) */
            if ( TRUE == eventProcessingAllowed(eventParam))  {
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                oldDTCStatus = getEventDTCStatus(eventParam, eventStatusRec->eventStatusExtended);
#endif
                oldEventStatus = eventStatusRec->eventStatusExtended;

                updateEventStatusRec(eventParam, eventStatus, eventStatusRec);
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                newDTCStatus = getEventDTCStatus(eventParam, eventStatusRec->eventStatusExtended);
#endif
                if ( (0 != eventStatusRec->errorStatusChanged) || (0 != eventStatusRec->extensionDataChanged) ) {
                    boolean eventFailedNow = ((TRUE == eventStatusRec->errorStatusChanged) && (0 != (eventStatusRec->eventStatusExtended & DEM_TEST_FAILED)))? TRUE: FALSE;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                    boolean dtcFailedNow = (((0u == (oldDTCStatus & DEM_TEST_FAILED))) && ((0 != (newDTCStatus & DEM_TEST_FAILED)))) ? TRUE: FALSE;
#endif
                    eventStoreStatus = storeEventEvtMem(eventParam, eventStatusRec, eventFailedNow); /** @req DEM184 *//** @req DEM396 */
                    boolean storeExtData;
                    boolean storeFFData;
                    boolean overrideOldExtData;
                    boolean eventDataUpdated = (E_OK == eventStoreStatus)? TRUE: FALSE;

                    /* Get conditions for data storage */
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                    /* @req DEM163 */
                    getStorageConditions(dtcFailedNow, eventDataUpdated, eventStatusRec->extensionDataStoreBitfield, &storeFFData, &storeExtData, &overrideOldExtData);
#else
                    /* @req DEM539 */
                    getStorageConditions(eventFailedNow, eventDataUpdated, eventStatusRec->extensionDataStoreBitfield, &storeFFData, &storeExtData, &overrideOldExtData);
#endif
                    if( FALSE == eventDTCRecordDataUpdateDisabled(eventParam) ) {
                        if ( (TRUE == storeExtData) && (NULL != eventParam->ExtendedDataClassRef) ) {
                            if( TRUE == storeExtendedData(eventParam, overrideOldExtData) ) {
                                eventDataUpdated = TRUE;
                            }
                        }
                        if ( TRUE == storeFFData ) {
                            if( DEM_FF_NULLREF != getFFIdx(eventParam) ) {
#if defined(DEM_USE_MEMORY_FUNCTIONS) && defined(DEM_FREEZE_FRAME_CAPTURE_EXTENSION)
                                /* Allow extension to decide if ffs should be deleted before storing */
                                if( 0 != (eventStatusRec->extensionDataStoreBitfield & DEM_EXT_CLEAR_BEFORE_STORE_FF_BIT) ) {
                                    if( TRUE == deleteFreezeFrameDataMem(eventParam, eventParam->EventClass->EventDestination, FALSE) ) {
                                        eventDataUpdated = TRUE;
                                    }
                                }
#endif
                                if( TRUE == storeFreezeFrameDataEvtMem(eventParam, &freezeFrameLocal, DEM_FREEZE_FRAME_NON_OBD) ) { /** @req DEM190 */
                                    eventDataUpdated = TRUE;
                                }
                        	}
                            if( (DEM_NON_EMISSION_RELATED != (Dem_Arc_EventDTCKindType) *(eventParam->EventDTCKind)) &&
                                    (TRUE == eventDataUpdated) &&
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                                    (TRUE == checkOBDFFStorageCondition(oldDTCStatus, newDTCStatus))
#else
                                    (TRUE == checkOBDFFStorageCondition(oldEventStatus, eventStatusRec->eventStatusExtended))
#endif
                                    ) {
                                if( TRUE == storeFreezeFrameDataEvtMem(eventParam, &freezeFrameLocal, DEM_FREEZE_FRAME_OBD) ) { /** @req DEM190 */
                                    eventDataUpdated = TRUE;
                                }
                            }
                        }
                    }
                    if( TRUE == eventDataUpdated ) {
                        /* @req DEM475 */
                        notifyEventDataChanged(eventParam);
                    }
                }
                if( E_NOT_OK == eventStoreStatus ) {
                    /* Tried to store event but did not succeed (eventStoreStatus initialized to E_OK).
                     * Make sure confirmed bit is not set. */
                    eventStatusRec->eventStatusExtended &= ~DEM_CONFIRMED_DTC;
                }
                if( oldEventStatus != eventStatusRec->eventStatusExtended ) {
                    /* @req DEM016 */
                    notifyEventStatusChange(eventStatusRec->eventParamRef, oldEventStatus, eventStatusRec->eventStatusExtended);
                }
#if defined(DEM_USE_INDICATORS) && defined(DEM_USE_MEMORY_FUNCTIONS)
                if((TRUE == eventStatusRec->indicatorDataChanged) && (TRUE == isInEventMemory(eventParam))) {
                    storeEventIndicators(eventParam);
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
                    boolean immediateStorage = FALSE;
                    if( (NULL != eventParam->DTCClassRef) && (TRUE == eventParam->DTCClassRef->DTCRef->ImmediateNvStorage) &&
                            (eventStatusRec->occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
                        immediateStorage = TRUE;
                    }
                    Dem_NvM_SetIndicatorBlockChanged(immediateStorage);
#else
                    Dem_NvM_SetIndicatorBlockChanged(FALSE);
#endif
                }
#endif
#if (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
                if(TRUE == handlePermanentDTCStorage(eventParam, eventStatusRec->eventStatusExtended)) {
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
                    boolean immediateStorage = FALSE;
                    if( (NULL != eventParam->DTCClassRef) && (TRUE == eventParam->DTCClassRef->DTCRef->ImmediateNvStorage) && (eventStatusRec->occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
                        immediateStorage = TRUE;
                    }
                    Dem_NvM_SetPermanentBlockChanged(immediateStorage);
#else
                    Dem_NvM_SetPermanentBlockChanged(FALSE);
#endif
                }
#endif
            } else {
                /* Enable conditions not set or DTC disabled */
                returnCode = E_NOT_OK;
            }
        } else {
            returnCode = E_NOT_OK; // Operation cycle not valid or not started /* @req DEM482 */
        }
    } else {
        returnCode = E_NOT_OK; // Event ID not configured or set to not available
    }

    return returnCode;
}

/*
 * Procedure:   resetEventStatus
 * Description: Resets the events status of eventId.
 */
static Std_ReturnType resetEventStatus(Dem_EventIdType eventId)
{
    EventStatusRecType *eventStatusRecPtr;
    Std_ReturnType ret = E_OK;
    lookupEventStatusRec(eventId, &eventStatusRecPtr);
    if (eventStatusRecPtr != NULL) {
        if( (TRUE == eventStatusRecPtr->isAvailable) && (0 != (eventStatusRecPtr->eventStatusExtended & DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE)) ) {
            Dem_EventStatusExtendedType oldStatus = eventStatusRecPtr->eventStatusExtended;
            eventStatusRecPtr->eventStatusExtended &= (Dem_EventStatusExtendedType)~DEM_TEST_FAILED; /** @req DEM187 */
            resetDebounceCounter(eventStatusRecPtr);
            if( oldStatus != eventStatusRecPtr->eventStatusExtended ) {
                /* @req DEM016 */
                notifyEventStatusChange(eventStatusRecPtr->eventParamRef, oldStatus, eventStatusRecPtr->eventStatusExtended);
            }
            /* NOTE: Should we store in "event destination" if DEM_TEST_FAILED_STORAGE == STD_ON) */
#if 0
#if DEM_TEST_FAILED_STORAGE == STD_ON
            if((0 != (oldStatus & DEM_TEST_FAILED)) && (E_OK == storeEventEvtMem(eventStatusRecPtr->eventParamRef, eventStatusRecPtr))) {
                notifyEventDataChanged(eventStatusRecPtr->eventParamRef);
            }
#endif
#endif
        } else {
            /* @req DEM638 */
            ret = E_NOT_OK;
        }
    }
    return ret;
}


/*
 * Procedure:   getEventStatus
 * Description: Returns the extended event status bitmask of eventId in "eventStatusExtended".
 */
static Std_ReturnType getEventStatus(Dem_EventIdType eventId, Dem_EventStatusExtendedType *eventStatusExtended)
{
    Std_ReturnType ret = E_OK;
    EventStatusRecType eventStatusLocal;

    // Get recorded status
    getEventStatusRec(eventId, &eventStatusLocal);
    if ( (eventStatusLocal.eventId == eventId) && (TRUE == eventStatusLocal.isAvailable) ) {
        *eventStatusExtended = eventStatusLocal.eventStatusExtended; /** @req DEM051 */
    }
    else {
        // Event Id not found, no report received.
        *eventStatusExtended = DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE | DEM_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR;
        ret = E_NOT_OK;
    }
    return ret;
}

/*
 * Procedure:   getEventFailed
 * Description: Returns the TRUE or FALSE of "eventId" in "eventFailed" depending on current status.
 */
static Std_ReturnType getEventFailed(Dem_EventIdType eventId, boolean *eventFailed)
{
    Std_ReturnType ret = E_OK;
    EventStatusRecType eventStatusLocal;

    // Get recorded status
    getEventStatusRec(eventId, &eventStatusLocal);
    if ( (eventStatusLocal.eventId == eventId) && (TRUE == eventStatusLocal.isAvailable) ) {
        if ( 0 != (eventStatusLocal.eventStatusExtended & DEM_TEST_FAILED)) { /** @req DEM052 */
            *eventFailed = TRUE;
        }
        else {
            *eventFailed = FALSE;
        }
    }
    else {
        // Event Id not found or not available.
        *eventFailed = FALSE;
        ret = E_NOT_OK;
    }
    return ret;
}

/*
 * Procedure:   getEventTested
 * Description: Returns the TRUE or FALSE of "eventId" in "eventTested" depending on
 *              current status the "test not completed this operation cycle" bit.
 */
static Std_ReturnType getEventTested(Dem_EventIdType eventId, boolean *eventTested)
{
    Std_ReturnType ret = E_OK;
    EventStatusRecType eventStatusLocal;

    // Get recorded status
    getEventStatusRec(eventId, &eventStatusLocal);
    if ( (eventStatusLocal.eventId == eventId) && (TRUE == eventStatusLocal.isAvailable) )  {
        if ( 0 == (eventStatusLocal.eventStatusExtended & DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE)) { /** @req DEM053 */
            *eventTested = TRUE;
        }
        else {
            *eventTested = FALSE;
        }
    }
    else {
        // Event Id not found, not tested.
        *eventTested = FALSE;
        ret = E_NOT_OK;
    }
    return ret;
}

#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL) && defined(DEM_USE_MEMORY_FUNCTIONS)
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1) && defined(DEM_COMBINED_DTC_STATUS_AGING)
/**
 * Gets the combined status (passed/failed) during aging cycle
 * @param eventParam
 * @param passed
 * @param failed
 * @return
 */
static Std_ReturnType getCombTypeStatusDuringAgingCycle(const Dem_EventParameterType *eventParam, boolean *passed, boolean *failed)
{
    Std_ReturnType ret = E_NOT_OK;
    const Dem_CombinedDTCCfgType *CombDTCCfg;
    const Dem_DTCClassType *DTCClass;
    EventStatusRecType* eventStatusRecPtr;
    *failed = FALSE;
    *passed = TRUE;
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        ret = E_OK;
        CombDTCCfg = &configSet->CombinedDTCConfig[eventParam->CombinedDTCCID];
        DTCClass = CombDTCCfg->DTCClassRef;
        for(uint16 i = 0; (i < DTCClass->NofEvents) && (E_OK == ret); i++) {
            if( eventParam->EventID != DTCClass->Events[i] ) {
                eventStatusRecPtr = NULL_PTR;
                lookupEventStatusRec(DTCClass->Events[i], &eventStatusRecPtr);
                if( NULL_PTR != eventStatusRecPtr ) {
                    if( FALSE == eventStatusRecPtr->passedDuringAgingCycle  ) {
                        *passed = FALSE;
                    }
                    if( TRUE == eventStatusRecPtr->failedDuringAgingCycle  ) {
                        *failed = FALSE;
                    }
                }
                else {
                    /* This is unexpected */
                    ret = E_NOT_OK;
                }
            }
        }
    }
    return ret;
}
#endif

#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
/**
 * Deletes all DTC data if a combined DTC is aged
 * @param eventParam
 * @return TRUE: data was deleted, FALSE: No data deleted
 */
static boolean deletDTCDataIfAged(const Dem_EventParameterType *eventParam)
{
    boolean dataDeleted = FALSE;
    boolean dummy;
    if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
        Dem_EventStatusExtendedType DTCStatus;
        if( DEM_STATUS_OK == GetDTCUDSStatus(eventParam->DTCClassRef, eventParam->EventClass->EventDestination, &DTCStatus) ) {
            if( 0u == (DTCStatus & DEM_CONFIRMED_DTC) ) {
                /* Confirmed bit was cleared for combined event *//* @req DEM442 */
                if( TRUE == DeleteDTCData(eventParam, FALSE, &dummy, TRUE) ) {
                    dataDeleted = TRUE;
                }
            }
        }
    }
    return dataDeleted;
}
#endif

static boolean ageEvent(EventStatusRecType* evtStatusRecPtr) {
    /* @req DEM643 */
    boolean updatedMemory = FALSE;
    boolean dummy;
    boolean eventDeleted = FALSE;
    Dem_EventStatusExtendedType oldStatus = evtStatusRecPtr->eventStatusExtended;
    boolean passedDuringAgingCycle;
    boolean failedDuringAgingCycle;

    /* @req DEM489 *//* If it is confirmed it should be stored in event memory */
    if( 0u != (evtStatusRecPtr->eventStatusExtended & DEM_CONFIRMED_DTC)) {
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1) && defined(DEM_COMBINED_DTC_STATUS_AGING)
        if( DEM_COMBINED_EVENT_NO_DTC_ID != evtStatusRecPtr->eventParamRef->CombinedDTCCID ) {
            passedDuringAgingCycle = evtStatusRecPtr->passedDuringAgingCycle;
            failedDuringAgingCycle = evtStatusRecPtr->failedDuringAgingCycle;
            if( E_OK != getCombTypeStatusDuringAgingCycle(evtStatusRecPtr->eventParamRef, &passedDuringAgingCycle, &failedDuringAgingCycle) ) {
                passedDuringAgingCycle = FALSE;
                failedDuringAgingCycle = FALSE;
            }
        }
        else {
            passedDuringAgingCycle = evtStatusRecPtr->passedDuringAgingCycle;
            failedDuringAgingCycle = evtStatusRecPtr->failedDuringAgingCycle;
        }
#else
        passedDuringAgingCycle = evtStatusRecPtr->passedDuringAgingCycle;
        failedDuringAgingCycle = evtStatusRecPtr->failedDuringAgingCycle;
#endif
        if( (TRUE == passedDuringAgingCycle) && (FALSE == evtStatusRecPtr->failedDuringAgingCycle) ) {
            /* Event was PASSED but NOT FAILED during aging cycle.
             * Increment aging counter */
            if( evtStatusRecPtr->agingCounter < DEM_AGING_CNTR_MAX ) {
                evtStatusRecPtr->agingCounter++;
                /* Set the flag,start up the storage of NVRam in main function. */
                updatedMemory = TRUE;
            }
            if((NULL != evtStatusRecPtr->eventParamRef->EventClass->AgingCycleCounterThresholdPtr) &&
                     (evtStatusRecPtr->agingCounter >= *evtStatusRecPtr->eventParamRef->EventClass->AgingCycleCounterThresholdPtr) ) { /* @req DEM493 */
                /* @req DEM497 *//* Delete ff and ext data */
                /* @req DEM161 */
                /* @req DEM541 */
                evtStatusRecPtr->agingCounter = 0;
                /* !req DEM498 *//* IMPROVEMNT: Only reset confirmed bit */
                evtStatusRecPtr->eventStatusExtended &= (Dem_EventStatusExtendedType)(~DEM_CONFIRMED_DTC);
                evtStatusRecPtr->eventStatusExtended &= (Dem_EventStatusExtendedType)(~DEM_PENDING_DTC);
                if(TRUE == DeleteDTCData(evtStatusRecPtr->eventParamRef, FALSE, &dummy, FALSE)) {
                    /* Set the flag,start up the storage of NVRam in main function. */
                    updatedMemory = TRUE;
                    eventDeleted = TRUE;
                }

#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                if( TRUE == deletDTCDataIfAged(evtStatusRecPtr->eventParamRef ) ) {
                    /* Set the flag,start up the storage of NVRam in main function. */
                    updatedMemory = TRUE;
                }
#endif
#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
                evtStatusRecPtr->failureCounter = 0;
#endif
#if defined(USE_DEM_EXTENSION)
                Dem_Extension_HealedEvent(evtStatusRecPtr->eventId);
#endif
            }
        }
        else if( (TRUE == failedDuringAgingCycle) && (0u != evtStatusRecPtr->agingCounter)) {
            /* Event failed during the aging cycle. Reset aging counter */
            evtStatusRecPtr->agingCounter = 0;
            updatedMemory = TRUE;
        } else {
            /* Do nothing.. */
        }
    }
    if( oldStatus != evtStatusRecPtr->eventStatusExtended ) {
        /* @req DEM016 */
        notifyEventStatusChange(evtStatusRecPtr->eventParamRef, oldStatus, evtStatusRecPtr->eventStatusExtended);
    }
    if( TRUE == updatedMemory ) {
        if( FALSE == eventDeleted ) {
            if(E_OK == storeEventEvtMem(evtStatusRecPtr->eventParamRef, evtStatusRecPtr, FALSE) ) {
                /* @req DEM475 */
                notifyEventDataChanged(evtStatusRecPtr->eventParamRef);
            }
        } else {
            /* Event was deleted. So don't store again but notify event data changed */
            notifyEventDataChanged(evtStatusRecPtr->eventParamRef);
#if defined(DEM_USE_MEMORY_FUNCTIONS)
            /* IMPROVEMENT: Immediate storage when deleting data? */
            Dem_NvM_SetEventBlockChanged(evtStatusRecPtr->eventParamRef->EventClass->EventDestination, FALSE);
#endif
        }

    }
    return updatedMemory;
}


/*
 * Procedure:   handleAging
 * Description: according to the operation state of "operationCycleId" to "cycleState" , handle the aging relatived data
 *              Returns E_OK if operation was successful else E_NOT_OK.
 */
static Std_ReturnType handleAging(Dem_OperationCycleIdType operationCycleId)
{
    uint16 i;
    Std_ReturnType returnCode = E_OK;
    boolean agingUpdatedSecondaryMemory = FALSE;
    boolean agingUpdatedPrimaryMemory = FALSE;
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
    boolean immediateStoragePrimary = FALSE;
    boolean immediateStorageSecondary = FALSE;
#endif
    if (operationCycleId < DEM_OPERATION_CYCLE_ID_ENDMARK) {
        /** @req Dem490 */
        for (i = 0; i < DEM_MAX_NUMBER_EVENT; i++) {
            if(eventStatusBuffer[i].eventId != DEM_EVENT_ID_NULL){
                if(eventStatusBuffer[i].eventParamRef != NULL){
                    if(eventStatusBuffer[i].eventParamRef->EventClass != NULL){
                        if((eventStatusBuffer[i].eventParamRef->EventClass->AgingAllowed == TRUE)
                            && (eventStatusBuffer[i].eventParamRef->EventClass->AgingCycleRef == operationCycleId)) {
                            /* Loop all destination memories e.g. primary and secondary */
                            Dem_DTCOriginType origin = eventStatusBuffer[i].eventParamRef->EventClass->EventDestination;
                            if (origin == DEM_DTC_ORIGIN_SECONDARY_MEMORY) {
                                agingUpdatedSecondaryMemory = ageEvent(&eventStatusBuffer[i]);
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
                                if( (NULL != eventStatusBuffer[i].eventParamRef->DTCClassRef) && (TRUE == eventStatusBuffer[i].eventParamRef->DTCClassRef->DTCRef->ImmediateNvStorage) && (eventStatusBuffer[i].occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
                                    immediateStorageSecondary = TRUE;
                                }
#endif
                            } else if (origin == DEM_DTC_ORIGIN_PRIMARY_MEMORY) {
                                agingUpdatedPrimaryMemory = ageEvent(&eventStatusBuffer[i]);
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
                                if( (NULL != eventStatusBuffer[i].eventParamRef->DTCClassRef) && (TRUE == eventStatusBuffer[i].eventParamRef->DTCClassRef->DTCRef->ImmediateNvStorage) && (eventStatusBuffer[i].occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
                                    immediateStoragePrimary = TRUE;
                                }
#endif
                            }
                        }
                    }
                }
            }
        }
    } else {
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_SETOPERATIONCYCLESTATE_ID, DEM_E_PARAM_DATA);
        returnCode = E_NOT_OK;
    }
#if defined(DEM_USE_MEMORY_FUNCTIONS)
    if( TRUE == agingUpdatedPrimaryMemory ) {
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
        Dem_NvM_SetEventBlockChanged(DEM_DTC_ORIGIN_PRIMARY_MEMORY, immediateStoragePrimary);
#else
        Dem_NvM_SetEventBlockChanged(DEM_DTC_ORIGIN_PRIMARY_MEMORY, FALSE);
#endif
    }
    if( TRUE == agingUpdatedSecondaryMemory ) {
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
        Dem_NvM_SetEventBlockChanged(DEM_DTC_ORIGIN_SECONDARY_MEMORY, immediateStorageSecondary);
#else
        Dem_NvM_SetEventBlockChanged(DEM_DTC_ORIGIN_SECONDARY_MEMORY, FALSE);
#endif
    }
#endif
    return returnCode;

}
#endif

/**
 * Handles starting an operation cycle
 * @param operationCycleId
 */
static void operationCycleStart(Dem_OperationCycleIdType operationCycleId)
{
    Dem_EventStatusExtendedType oldStatus;
    operationCycleStateList[operationCycleId] = DEM_CYCLE_STATE_START;
    // Lookup event ID
    for (uint16 i = 0; i < DEM_MAX_NUMBER_EVENT; i++) {
        if( (eventStatusBuffer[i].eventId != DEM_EVENT_ID_NULL) && (TRUE == eventStatusBuffer[i].isAvailable) ) {
            if( eventStatusBuffer[i].eventParamRef->EventClass->OperationCycleRef == operationCycleId ) {
                oldStatus = eventStatusBuffer[i].eventStatusExtended;
                eventStatusBuffer[i].eventStatusExtended &= (Dem_EventStatusExtendedType)~DEM_TEST_FAILED_THIS_OPERATION_CYCLE;
                eventStatusBuffer[i].eventStatusExtended |= DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE;
                resetDebounceCounter(&eventStatusBuffer[i]);
                eventStatusBuffer[i].isDisabled = FALSE;

                if( oldStatus != eventStatusBuffer[i].eventStatusExtended ) {
                    /* @req DEM016 */
                    notifyEventStatusChange(eventStatusBuffer[i].eventParamRef, oldStatus, eventStatusBuffer[i].eventStatusExtended);
                }
                if( NULL != eventStatusBuffer[i].eventParamRef->CallbackInitMforE ) {
                    /* @req DEM376 */
                    (void)eventStatusBuffer[i].eventParamRef->CallbackInitMforE(DEM_INIT_MONITOR_RESTART);
                }

            }
#if defined(USE_DEM_EXTENSION)
            Dem_Extension_OperationCycleStart(operationCycleId, &eventStatusBuffer[i]);
#endif
        }
#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
        if( (eventStatusBuffer[i].eventId != DEM_EVENT_ID_NULL) && ((Dem_OperationCycleIdType)*eventStatusBuffer[i].eventParamRef->EventClass->FailureCycleRef == operationCycleId) ) {
            eventStatusBuffer[i].failedDuringFailureCycle = FALSE;
            eventStatusBuffer[i].passedDuringFailureCycle = FALSE;
        }
#endif
#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL)
        if( (eventStatusBuffer[i].eventId != DEM_EVENT_ID_NULL) && (eventStatusBuffer[i].eventParamRef->EventClass->AgingCycleRef == operationCycleId) ) {
            eventStatusBuffer[i].passedDuringAgingCycle = FALSE;
            eventStatusBuffer[i].failedDuringAgingCycle = FALSE;
        }
#endif
    }
#if defined(DEM_USE_INDICATORS)
    indicatorOpCycleStart(operationCycleId);
#endif

#if defined(DEM_USE_IUMPR)
    resetIumprFlags(operationCycleId);

    incrementUnlockedIumprDenominators(operationCycleId);

    incrementIgnitionCycleCounter(operationCycleId);
#endif
}

#if (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
/**
 * Check if an event is stored in permanent memory
 * @param eventParam
 * @return
 */
static boolean isStoredInPermanentMemory(const Dem_EventParameterType *eventParam)
{
    boolean isStored = FALSE;
    for( uint32 i = 0; (i < DEM_MAX_NUMBER_EVENT_PERM_MEM) && (FALSE == isStored); i++) {
        if( (NULL != eventParam->DTCClassRef) && (eventParam->DTCClassRef->DTCRef->OBDDTC == permMemEventBuffer[i].OBDDTC)) {
            /* DTC is stored */
            isStored = TRUE;
        }
    }
    return isStored;
}

/**
 * Transfers a number of events to permanent memory
 * @param nofEntries
 */
static boolean transferEventToPermanent(uint16 nofEntries)
{
    uint32 oldestTimestamp;
    uint16 storeIndex = 0;
    boolean candidateFound = TRUE;
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
    boolean immediateStorage = FALSE;
#endif
    /* Find the oldest */
    for( uint16 cnt = 0; (cnt < nofEntries) && (TRUE == candidateFound); cnt++ ) {
        candidateFound = FALSE;
        oldestTimestamp = 0xFFFFFFFF;
        for (uint16 i = 0; (i < DEM_MAX_NUMBER_EVENT) ; i++) {
            if( (eventStatusBuffer[i].eventId != DEM_EVENT_ID_NULL) && (TRUE == eventStatusBuffer[i].isAvailable) ) {
                if( (TRUE == permanentDTCStorageConditionFulfilled(eventStatusBuffer[i].eventParamRef, eventStatusBuffer[i].eventStatusExtended)) &&
                        (FALSE == isStoredInPermanentMemory(eventStatusBuffer[i].eventParamRef)) &&
                        (eventStatusBuffer[i].timeStamp < oldestTimestamp)) {
                    storeIndex = i;
                    oldestTimestamp = eventStatusBuffer[i].timeStamp;
                    candidateFound = TRUE;
                }
            }
        }
        if( TRUE == candidateFound ) {
            if( TRUE == handlePermanentDTCStorage(eventStatusBuffer[storeIndex].eventParamRef, eventStatusBuffer[storeIndex].eventStatusExtended) ) {
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
                if( (NULL != eventStatusBuffer[storeIndex].eventParamRef->DTCClassRef) &&
                        (TRUE == eventStatusBuffer[storeIndex].eventParamRef->DTCClassRef->DTCRef->ImmediateNvStorage) && (eventStatusBuffer[storeIndex].occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
                    immediateStorage = TRUE;
                }
#endif
            }
        }
    }
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
    return immediateStorage;
#else
    return FALSE;
#endif
}
#endif

/**
 * Handles ending an operation cycle
 * @param operationCycleId
 */
static void operationCycleEnd(Dem_OperationCycleIdType operationCycleId)
{
    Dem_EventStatusExtendedType oldStatus;
#if (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
    boolean permanentMemoryUpdated = FALSE;
    uint16 nofErasedPermanentDTCs = 0;
#endif
#if defined(DEM_USE_INDICATORS)
    boolean indicatorsUpdated = FALSE;
#endif
    operationCycleStateList[operationCycleId] = DEM_CYCLE_STATE_END;
    // Lookup event ID
    for (uint16 i = 0; i < DEM_MAX_NUMBER_EVENT; i++) {
        boolean storeEvtMem = FALSE;

        if ((eventStatusBuffer[i].eventId != DEM_EVENT_ID_NULL) && (TRUE == eventStatusBuffer[i].isAvailable)) {
            oldStatus = eventStatusBuffer[i].eventStatusExtended;
#if defined(DEM_USE_INDICATORS)
            if(TRUE == indicatorOpCycleEnd(operationCycleId, &eventStatusBuffer[i])) {
                indicatorsUpdated = TRUE;
            }
#endif
#if (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
            /* Handle the permanent DTC */
            if(TRUE == handlePermanentDTCErase(&eventStatusBuffer[i], operationCycleId) ) {
                nofErasedPermanentDTCs++;
                permanentMemoryUpdated = TRUE;
            }
#endif
            if ((0 == (eventStatusBuffer[i].eventStatusExtended & DEM_TEST_FAILED_THIS_OPERATION_CYCLE)) && (0 == (eventStatusBuffer[i].eventStatusExtended & DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE))) {
                if( eventStatusBuffer[i].eventParamRef->EventClass->OperationCycleRef == operationCycleId ) {
                    eventStatusBuffer[i].eventStatusExtended &= (Dem_EventStatusExtendedType)~DEM_PENDING_DTC;      // Clear pendingDTC bit /** @req DEM379.PendingClear
                    if( oldStatus != eventStatusBuffer[i].eventStatusExtended ) {
                        storeEvtMem = TRUE;
                    }
                }
            }
#if defined(DEM_FAILURE_PROCESSING_DEM_INTERNAL)
            if( ((Dem_OperationCycleIdType)*(eventStatusBuffer[i].eventParamRef->EventClass->FailureCycleRef) == operationCycleId) &&
                    (TRUE == eventStatusBuffer[i].passedDuringFailureCycle) && (FALSE == eventStatusBuffer[i].failedDuringFailureCycle) ) {
                /* @dev DEM: Spec. does not say when this counter should be cleared */
                if( 0 != eventStatusBuffer[i].failureCounter ) {
                    eventStatusBuffer[i].failureCounter = 0;
                    storeEvtMem = TRUE;
                }
            }
#endif
#if defined(USE_DEM_EXTENSION)
            Dem_Extension_OperationCycleEnd(operationCycleId, &eventStatusBuffer[i]);
#endif
            if( oldStatus != eventStatusBuffer[i].eventStatusExtended ) {
                /* @req DEM016 */
                notifyEventStatusChange(eventStatusBuffer[i].eventParamRef, oldStatus, eventStatusBuffer[i].eventStatusExtended);
            }
            if( TRUE == storeEvtMem ) {
                /* Transfer to event memory.  */
                if( E_OK == storeEventEvtMem(eventStatusBuffer[i].eventParamRef, &eventStatusBuffer[i], FALSE) ) {
                    notifyEventDataChanged(eventStatusBuffer[i].eventParamRef);
                }
            }
        }
    }
#if defined(DEM_USE_INDICATORS)
    if( TRUE == indicatorsUpdated ) {
#ifdef DEM_USE_MEMORY_FUNCTIONS
        /* IMPROVEMENT: Immediate storage? */
        Dem_NvM_SetIndicatorBlockChanged(FALSE);
#endif
    }
#endif
#if (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
    if( TRUE == permanentMemoryUpdated ) {
        /* DTC was deleted from permanent memory. Check if we should store some other DTC */
        boolean immediateStorage = transferEventToPermanent(nofErasedPermanentDTCs);
        Dem_NvM_SetPermanentBlockChanged(immediateStorage);
    }
#endif
}
/*
 * Procedure:   setOperationCycleState
 * Description: Change the operation state of "operationCycleId" to "cycleState" and updates stored
 *              event connected to this cycle id.
 *              Returns E_OK if operation was successful else E_NOT_OK.
 */
static Std_ReturnType setOperationCycleState(Dem_OperationCycleIdType operationCycleId, Dem_OperationCycleStateType cycleState) /** @req DEM338 */
{
    Std_ReturnType returnCode = E_OK;
    /* @req DEM338 */
    if (operationCycleId < DEM_OPERATION_CYCLE_ID_ENDMARK) {
        switch (cycleState) {
        case DEM_CYCLE_STATE_START:
            /* @req DEM483 */
            operationCycleStart(operationCycleId);
            break;

        case DEM_CYCLE_STATE_END:
            if(operationCycleStateList[operationCycleId] != DEM_CYCLE_STATE_END) {
                /* @req DEM484 */
                operationCycleEnd(operationCycleId);
#if defined(DEM_AGING_PROCESSING_DEM_INTERNAL) && defined(DEM_USE_MEMORY_FUNCTIONS)
                (void)handleAging(operationCycleId);
#endif
            }
            break;
        default:
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_SETOPERATIONCYCLESTATE_ID, DEM_E_PARAM_DATA);
            returnCode = E_NOT_OK;
            break;
        }
    } else {
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_SETOPERATIONCYCLESTATE_ID, DEM_E_PARAM_DATA);
        returnCode = E_NOT_OK;
    }

    return returnCode;
}


static inline void initEventStatusBuffer(const Dem_EventParameterType *eventIdParamList)
{
    // Insert all supported events into event status buffer
    const Dem_EventParameterType *eventParam = eventIdParamList;
    EventStatusRecType *eventStatusRecPtr;
    while( FALSE == eventParam->Arc_EOL ) {
        // Find next free position in event status buffer
        lookupEventStatusRec(eventParam->EventID, &eventStatusRecPtr);
        if(NULL != eventStatusRecPtr) {
            eventStatusRecPtr->eventId = eventParam->EventID;
            eventStatusRecPtr->eventParamRef = eventParam;
            sint8 startUdsFdc = getDefaultUDSFdc(eventParam->EventID);
            eventStatusRecPtr->UDSFdc = startUdsFdc;/* @req DEM438 */
            eventStatusRecPtr->maxUDSFdc = startUdsFdc;
            eventStatusRecPtr->fdcInternal = 0;
            eventStatusRecPtr->isAvailable = *eventParam->EventClass->EventAvailableByCalibration;
            if(FALSE == eventStatusRecPtr->isAvailable) {
                eventStatusRecPtr->eventStatusExtended = 0x0u;
            }
#if (DEM_DTC_SUPPRESSION_SUPPORT == STD_ON)
            /* Check if suppression of DTC is affected */
            boolean suppressed = TRUE;
            const Dem_EventParameterType *dtcEventParam;
            if( (NULL != eventParam->DTCClassRef) && (NULL != eventParam->DTCClassRef->Events) ) {
                for( uint16 i = 0; (i < eventParam->DTCClassRef->NofEvents) && (TRUE == suppressed); i++ ) {
                    dtcEventParam = NULL;
                    lookupEventIdParameter(eventParam->DTCClassRef->Events[i], &dtcEventParam);
                    if( (NULL != dtcEventParam) && (TRUE == *dtcEventParam->EventClass->EventAvailableByCalibration) ) {
                        /* Event is available -> DTC NOT suppressed */
                        suppressed = FALSE;
                    }
                }
                if( 0 != eventParam->DTCClassRef->NofEvents ) {
                    DemDTCSuppressed[eventParam->DTCClassRef->DTCIndex].SuppressedByEvent = suppressed;
                }
            }
#endif
        }
        eventParam++;
    }
}

#if ( DEM_FF_DATA_IN_PRE_INIT )
static inline void initPreInitFreezeFrameBuffer(void)
{
    for (uint16 i = 0; i < DEM_MAX_NUMBER_FF_DATA_PRE_INIT; i++) {
        preInitFreezeFrameBuffer[i].eventId = DEM_EVENT_ID_NULL;
        preInitFreezeFrameBuffer[i].dataSize = 0;
#if (DEM_USE_TIMESTAMPS == STD_ON)
        preInitFreezeFrameBuffer[i].timeStamp = 0;
#endif
        for (uint16 j = 0; j < DEM_MAX_SIZE_FF_DATA;j++){
            preInitFreezeFrameBuffer[i].data[j] = 0;
        }
    }
}
#endif

static inline void initPreInitExtDataBuffer(void)
{
#if ( DEM_EXT_DATA_IN_PRE_INIT )
    for (uint16 i = 0; i < DEM_MAX_NUMBER_EXT_DATA_PRE_INIT; i++) {
#if (DEM_USE_TIMESTAMPS == STD_ON)
        preInitExtDataBuffer[i].timeStamp = 0;
#endif
        preInitExtDataBuffer[i].eventId = DEM_EVENT_ID_NULL;
        for (uint16 j = 0; j < DEM_MAX_SIZE_EXT_DATA;j++){
            preInitExtDataBuffer[i].data[j] = 0;
        }
    }
#endif
}

#if (DEM_ENABLE_CONDITION_SUPPORT == STD_ON)
static inline void initEnableConditions(void)
{
    /* Initialize the enable conditions */
    const Dem_EnableConditionType *enableCondition = configSet->EnableCondition;
    while( enableCondition->EnableConditionID != DEM_ENABLE_CONDITION_EOL) {
        DemEnableConditions[enableCondition->EnableConditionID] = enableCondition->EnableConditionStatus;
        enableCondition++;
    }
}
#endif

#ifdef DEM_USE_MEMORY_FUNCTIONS
static boolean validateFreezeFrames(FreezeFrameRecType* freezeFrameBuffer, uint32 freezeFrameBufferSize, Dem_DTCOriginType origin)
{
    /* IMPROVEMENT: Delete OBD freeze frames if the event is not emission related */
    boolean freezeFrameBlockChanged = FALSE;
    // Validate freeze frame records stored in primary memory
    for (uint32 i = 0u; i < freezeFrameBufferSize; i++) {
        if ((freezeFrameBuffer[i].eventId == DEM_EVENT_ID_NULL) || (FALSE == checkEntryValid(freezeFrameBuffer[i].eventId, origin, TRUE))) {
            // Unlegal record, clear the record
            memset(&freezeFrameBuffer[i], 0, sizeof(FreezeFrameRecType));
            freezeFrameBlockChanged = TRUE;
        }
    }
    return freezeFrameBlockChanged;
}

static boolean validateExtendedData(ExtDataRecType* extendedDataBuffer, uint32 extendedDataBufferSize, Dem_DTCOriginType origin)
{
    boolean extendedDataBlockChanged = FALSE;
    for (uint32 i = 0uL; i < extendedDataBufferSize; i++) {
        if ((extendedDataBuffer[i].eventId == DEM_EVENT_ID_NULL) || (FALSE == checkEntryValid(extendedDataBuffer[i].eventId, origin, TRUE))) {
            // Unlegal record, clear the record
            memset(&extendedDataBuffer[i], 0, sizeof(ExtDataRecType));
            extendedDataBlockChanged = TRUE;
        }
    }
    return extendedDataBlockChanged;
}
#endif /* DEM_USE_MEMORY_FUNCTIONS */

/**
 * Looks for freeze frame data for a specific record number (a specific record or the most recent). Returns pointer to data,
 * the record number found and the type of freeze frame data (OBD or NON-OBD)
 * @param eventParam
 * @param recNum
 * @param freezeFrameData
 * @param ffRecNumFound
 * @param ffKind
 * @return TRUE: freeze frame data found, FALSE: freeze frame data not found
 */
static boolean getFFRecData(const Dem_EventParameterType *eventParam, uint8 recNum, uint8 **freezeFrameData, uint8 *ffRecNumFound, Dem_FreezeFrameKindType *ffKind)
{
    boolean isStored = FALSE;
    uint8 nofStoredRecord = 0;
    uint8 recordToFind = recNum;
    boolean failed = FALSE;
    if( (NULL != eventParam->FreezeFrameClassRefIdx) && (*(eventParam->FreezeFrameClassRefIdx) != DEM_FF_NULLREF) && (NULL != eventParam->FreezeFrameRecNumClassRef) ) {
        if( MOST_RECENT_FF_RECORD == recNum ) {
            /* Should find the most recent record */
            if(E_OK == getNofStoredNonOBDFreezeFrames(eventParam, eventParam->EventClass->EventDestination, &nofStoredRecord)){
                if( 0 == nofStoredRecord ) {
                    failed = TRUE;
                } else {
                    recordToFind = eventParam->FreezeFrameRecNumClassRef->FreezeFrameRecordNumber[nofStoredRecord - 1];
                }
            }
        }
        if( FALSE == failed ) {
            /* Have a record number to look for */
            FreezeFrameRecType *freezeFrame = NULL;
            if((TRUE == getStoredFreezeFrame(eventParam->EventID, recordToFind, eventParam->EventClass->EventDestination, &freezeFrame)) && (NULL != freezeFrame)) {
                *freezeFrameData = freezeFrame->data;
                *ffKind = DEM_FREEZE_FRAME_NON_OBD;
                isStored = TRUE;
            }
        }
    } else {
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM)
        /* Could be OBD... */
        recordToFind = 0; /* Always 0 for OBD freeze frame */
        if( (NULL != configSet->GlobalOBDFreezeFrameClassRef) && (NULL != eventParam->DTCClassRef) && (DEM_NON_EMISSION_RELATED != (Dem_Arc_EventDTCKindType) *(eventParam->EventDTCKind))) {
            /* Event is event related */
            if( (0 == recNum) || (MOST_RECENT_FF_RECORD == recNum) ) {
                /*find the corresponding FF in FF buffer*/
                for(uint16 i = 0; i < DEM_MAX_NUMBER_FF_DATA_PRI_MEM; i++){
                    if((DEM_FREEZE_FRAME_OBD == priMemFreezeFrameBuffer[i].kind) && (priMemFreezeFrameBuffer[i].eventId == eventParam->EventID)){
                        *freezeFrameData = priMemFreezeFrameBuffer[i].data;
                        *ffKind = DEM_FREEZE_FRAME_OBD;
                        isStored = TRUE;
                        break;
                    }
                }
            }
        }
#endif
    }
    *ffRecNumFound = recordToFind;
    return isStored;
}

/**
 * Checks if an event may be cleared
 * @param eventParam
 * @return TRUE: Event may be cleared, FALSE: Event may NOT be cleared
 */
static boolean clearEventAllowed(const Dem_EventParameterType *eventParam)
{
    boolean clearAllowed = TRUE;
    /* @req DEM514 */
    if(NULL != eventParam->CallbackClearEventAllowed) {
        /* @req DEM515 */
        if( E_OK != eventParam->CallbackClearEventAllowed(&clearAllowed)) {
            /* @req DEM516 */
            clearAllowed = TRUE;
        }
    }
    return clearAllowed;
}
//==============================================================================//
//                                                                              //
//                    E X T E R N A L   F U N C T I O N S                       //
//                                                                              //
//==============================================================================//

/*********************************************
 * Interface for upper layer modules (8.3.1) *
 *********************************************/

/*
 * Procedure:   Dem_GetVersionInfo
 * Reentrant:   Yes
 */
// Defined in Dem.h


/***********************************************
 * Interface ECU State Manager <-> DEM (8.3.2) *
 ***********************************************/

#if defined(DEM_USE_INDICATORS)
static void initIndicatorStatusBuffer(const Dem_EventParameterType *eventIdParamList)
{
    uint16 indx = 0;
    while( FALSE == eventIdParamList[indx].Arc_EOL ) {
        if( NULL != eventIdParamList[indx].EventClass->IndicatorAttribute  ) {
            const Dem_IndicatorAttributeType *indAttrPtr = eventIdParamList[indx].EventClass->IndicatorAttribute;
            while( FALSE == indAttrPtr->Arc_EOL) {
                indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].EventID = eventIdParamList[indx].EventID;
                indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].InternalIndicatorId = indAttrPtr->IndicatorBufferIndex;
                indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].FailureCounter = 0;
                indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].HealingCounter = 0;
                indicatorStatusBuffer[indAttrPtr->IndicatorBufferIndex].OpCycleStatus = 0;
                indAttrPtr++;
            }
        }
        indx++;
    }
}
#endif

#if defined(USE_NVM) && (DEM_USE_NVM == STD_ON)
/**
 * Validates configured NvM block sizes
 */

/*lint --e{522} CONFIGURATION */
static void validateNvMBlockSizes(void)
{
    /* Check sizes of used NvM blocks */
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
    DEM_ASSERT( (0 == DEM_EVENT_PRIMARY_NVM_BLOCK_HANDLE) ||
                (DEM_EVENT_PRIMARY_NVM_BLOCK_SIZE == sizeof(priMemEventBuffer)));/*lint !e506 CONFIGURATION */
#if ( DEM_FF_DATA_IN_PRI_MEM )
    DEM_ASSERT( (0 == DEM_FREEZE_FRAME_PRIMARY_NVM_BLOCK_HANDLE) ||
                (DEM_FREEZE_FRAME_PRIMARY_NVM_BLOCK_SIZE == sizeof(priMemFreezeFrameBuffer)));/*lint !e506 CONFIGURATION */
#endif

#if ( DEM_EXT_DATA_IN_PRI_MEM )
    DEM_ASSERT( (0 == DEM_EXTENDED_DATA_PRIMARY_NVM_BLOCK_HANDLE) ||
                (DEM_EXTENDED_DATA_PRIMARY_NVM_BLOCK_SIZE == sizeof(priMemExtDataBuffer)));/*lint !e506 CONFIGURATION */
#endif
#endif

#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
    DEM_ASSERT( (0 == DEM_EVENT_SECONDARY_NVM_BLOCK_HANDLE) ||
                (DEM_EVENT_SECONDARY_NVM_BLOCK_SIZE == sizeof(secMemEventBuffer)));/*lint !e506 CONFIGURATION */
#if ( DEM_FF_DATA_IN_SEC_MEM )
    DEM_ASSERT( (0 == DEM_FREEZE_FRAME_SECONDARY_NVM_BLOCK_HANDLE) ||
                (DEM_FREEZE_FRAME_SECONDARY_NVM_BLOCK_SIZE == sizeof(secMemFreezeFrameBuffer)));/*lint !e506 CONFIGURATION */
#endif
#if ( DEM_EXT_DATA_IN_SEC_MEM )
    DEM_ASSERT( (0 == DEM_EXTENDED_DATA_SECONDARY_NVM_BLOCK_HANDLE ) ||
                (DEM_EXTENDED_DATA_SECONDARY_NVM_BLOCK_SIZE == sizeof(secMemExtDataBuffer)));/*lint !e506 CONFIGURATION */
#endif
#endif

#if defined(DEM_USE_INDICATORS) && defined(DEM_USE_MEMORY_FUNCTIONS)
    DEM_ASSERT( (0 == DEM_INDICATOR_NVM_BLOCK_HANDLE ) ||
                (DEM_INDICATOR_NVM_BLOCK_SIZE == sizeof(indicatorBuffer)));/*lint !e506 CONFIGURATION */
#endif

#if (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
    DEM_ASSERT( (0 == DEM_PERMANENT_NVM_BLOCK_HANDLE ) ||
                (DEM_PERMANENT_NVM_BLOCK_SIZE == sizeof(permMemEventBuffer)));/*lint !e506 CONFIGURATION */
#endif

#if (DEM_PRESTORAGE_FF_DATA_IN_MEM)
    DEM_ASSERT( (0 == DEM_PRESTORE_FF_NVM_BLOCK_HANDLE ) ||
                  (DEM_PRESTORE_FF_NVM_BLOCK_SIZE == sizeof(memPreStoreFreezeFrameBuffer)));/*lint !e506 CONFIGURATION */
#endif
}
#endif

/*
 * Procedure:   Dem_PreInit
 * Reentrant:   No
 */
