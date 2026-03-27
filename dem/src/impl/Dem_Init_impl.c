void Dem_PreInit(const Dem_ConfigType *ConfigPtr)
{
    /** @req DEM180 */
    uint16 i;

    VALIDATE_NO_RV(ConfigPtr != NULL, DEM_PREINIT_ID, DEM_E_CONFIG_PTR_INVALID);
    VALIDATE_NO_RV(ConfigPtr->ConfigSet != NULL, DEM_PREINIT_ID, DEM_E_CONFIG_PTR_INVALID);


#if defined(USE_NVM) && (DEM_USE_NVM == STD_ON)
    validateNvMBlockSizes();
#endif

    configSet = ConfigPtr->ConfigSet;

#if (DEM_DTC_SUPPRESSION_SUPPORT == STD_ON)
    const Dem_DTCClassType *DTCClass = configSet->DTCClass;
    while(FALSE == DTCClass->Arc_EOL) {
        DemDTCSuppressed[DTCClass->DTCIndex].SuppressedByDTC = FALSE;
        DemDTCSuppressed[DTCClass->DTCIndex].SuppressedByEvent = FALSE;
        DTCClass++;
    }
#endif

    // Initializion of operation cycle states.
    for (i = 0; i < DEM_OPERATION_CYCLE_ID_ENDMARK; i++) {
        operationCycleStateList[i] = DEM_CYCLE_STATE_END;
    }

    // Initialize the event status buffer
    for (i = 0; i < DEM_MAX_NUMBER_EVENT; i++) {
        setDefaultEventStatus(&eventStatusBuffer[i]);
    }

#if defined(DEM_USE_TIME_BASE_PREDEBOUNCE)
    InitTimeBasedDebounce();
#endif

#if (DEM_STORE_UDS_STATUS_BIT_SUBSET_FOR_ALL_EVENTS == STD_ON) && defined(DEM_USE_MEMORY_FUNCTIONS)
    SetDefaultUDSStatusBitSubset();
#endif

    // Initialize the eventstatus buffer (Insert all supported events into event status buffer)
    initEventStatusBuffer(configSet->EventParameter);

    /* Initialize the preInit freeze frame buffer */
#if( DEM_FF_DATA_IN_PRE_INIT )
    initPreInitFreezeFrameBuffer();
#endif

    /* Initialize the preInit extended data buffer */
    initPreInitExtDataBuffer();

#if (DEM_ENABLE_CONDITION_SUPPORT == STD_ON)
    /* Initialize the enable conditions */
    initEnableConditions();
#endif

#if defined(USE_DEM_EXTENSION)
    Dem_Extension_PreInit(ConfigPtr);
#endif

#if (DEM_USE_TIMESTAMPS == STD_ON)
    /* Reset freze frame time stamp */
    FF_TimeStamp = 0;

    /* Reset event time stamp */
    Event_TimeStamp = 0;

    /* Reset extended data timestamp */
    ExtData_TimeStamp = 0;
#endif

#if defined(DEM_USE_INDICATORS)
    initIndicatorStatusBuffer(configSet->EventParameter);
#endif

    disableDtcSetting.settingDisabled = FALSE;

    (void)setOperationCycleState(DEM_ACTIVE, DEM_CYCLE_STATE_START);

    /* Init the DTC record update disable */
    DTCRecordDisabled.DTC = NO_DTC_DISABLED;

#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
    priMemOverflow = FALSE;
#endif
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
    secMemOverflow = FALSE;
#endif

    Dem_NvM_Init();
#if defined(USE_FIM)
    DemFiMInit = FALSE;
#endif
    demState = DEM_PREINITIALIZED;
}

#ifdef DEM_USE_MEMORY_FUNCTIONS
static boolean ValidateAndMergeEventRecords(EventRecType* eventBuffer, uint32 eventBufferSize, boolean* eventEntryChanged,
                                    uint32* Evt_TimeStamp, Dem_DTCOriginType origin ) {


    boolean eventBlockChanged = FALSE;
    uint32 i;

    // Validate event records stored in memory
    for (i = 0; i < eventBufferSize; i++) {
        eventEntryChanged[i] = FALSE;

        if ((eventBuffer[i].EventData.eventId == DEM_EVENT_ID_NULL) || (checkEntryValid(eventBuffer[i].EventData.eventId, origin, FALSE) == FALSE )) {
            // Unlegal record, clear the record
            memset(&eventBuffer[i], 0, sizeof(EventRecType));
            eventBlockChanged = TRUE;
        }
    }

#if (DEM_USE_TIMESTAMPS == STD_ON)
    /* initialize the current timestamp and update the timestamp in pre init */
    initCurrentEventTimeStamp(Evt_TimeStamp);
#else
    (void)Evt_TimeStamp;/*lint !e920 *//* Avoid compiler warning */
#endif
    /* Merge events read from NvRam */
    for (i = 0; i < eventBufferSize; i++) {
        eventEntryChanged[i] = FALSE;
        if( DEM_EVENT_ID_NULL != eventBuffer[i].EventData.eventId ) {
            eventEntryChanged[i] = mergeEventStatusRec(&eventBuffer[i]);
        }
    }

    return eventBlockChanged;

}

static void MergeBuffer(Dem_DTCOriginType origin) {

    uint32 i;
    boolean eventBlockChanged;
    boolean extendedDataBlockChanged;
    boolean freezeFrameBlockChanged;
    boolean eventEntryChanged[DEM_MAX_NUMBER_EVENT_ENTRY] = {FALSE};/*lint !e506 */
    const Dem_EventParameterType *eventParam;
    EventRecType* eventBuffer = NULL;
    uint32 eventBufferSize = 0;
    FreezeFrameRecType* freezeFrameBuffer = NULL;
    uint32 freezeFrameBufferSize = 0;
    ExtDataRecType* extendedDataBuffer = NULL;
    uint32 extendedDataBufferSize = 0;
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
    boolean immediateEventStorage = FALSE;
    boolean immediateFFStorage = FALSE;
    boolean immediateExtDataStorage = FALSE;
#endif

    /* Setup variables for merging */
    switch (origin) {
        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
            eventBuffer = priMemEventBuffer;
            eventBufferSize = DEM_MAX_NUMBER_EVENT_PRI_MEM;
#if ( DEM_FF_DATA_IN_PRI_MEM )
            freezeFrameBuffer = priMemFreezeFrameBuffer;
            freezeFrameBufferSize = DEM_MAX_NUMBER_FF_DATA_PRI_MEM;
#endif
#if ( DEM_EXT_DATA_IN_PRI_MEM )
            extendedDataBuffer = priMemExtDataBuffer;
            extendedDataBufferSize = DEM_MAX_NUMBER_EXT_DATA_PRI_MEM;
#endif
#endif
            break;

        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
            eventBuffer = secMemEventBuffer;
            eventBufferSize = DEM_MAX_NUMBER_EVENT_SEC_MEM;
#if ( DEM_FF_DATA_IN_SEC_MEM )
            freezeFrameBuffer = secMemFreezeFrameBuffer;
            freezeFrameBufferSize = DEM_MAX_NUMBER_FF_DATA_SEC_MEM;
#endif
#if ( DEM_EXT_DATA_IN_SEC_MEM )
            extendedDataBuffer = secMemExtDataBuffer;
            extendedDataBufferSize = DEM_MAX_NUMBER_EXT_DATA_SEC_MEM;
#endif
#endif
            break;
        default:
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GLOBAL_ID, DEM_E_NOT_IMPLEMENTED_YET);
            break;

    }

#if (DEM_USE_TIMESTAMPS != STD_ON)
        /* The timestamp isn't actually used. This just to make it compile.. */
    uint32 Event_TimeStamp = 0;
#endif
    eventBlockChanged = ValidateAndMergeEventRecords(eventBuffer, eventBufferSize, eventEntryChanged, &Event_TimeStamp, origin);

#if defined(USE_DEM_EXTENSION)
    Dem_Extension_Init_PostEventMerge(origin);
#endif

#if (DEM_USE_TIMESTAMPS == STD_ON)
    //initialize the current timestamp and update the timestamp in pre init
    initCurrentFreezeFrameTimeStamp(&FF_TimeStamp);
#endif

    /* Validate freeze frames stored in memory */
    freezeFrameBlockChanged = validateFreezeFrames(freezeFrameBuffer, freezeFrameBufferSize, origin);

    /* Transfer updated event data to event memory */
    for (i = 0u; (i < eventBufferSize) && (NULL != eventBuffer); i++) {
        if ( (eventBuffer[i].EventData.eventId != DEM_EVENT_ID_NULL) && (TRUE == eventEntryChanged[i]) ) {
            EventStatusRecType *eventStatusRecPtr = NULL;
            eventParam = NULL;
            lookupEventIdParameter(eventBuffer[i].EventData.eventId, &eventParam);
             /* Transfer to event memory. */
            lookupEventStatusRec(eventBuffer[i].EventData.eventId, &eventStatusRecPtr);
            if( (NULL != eventStatusRecPtr) && (NULL != eventParam) ) {
                if( E_OK == storeEventEvtMem(eventParam, eventStatusRecPtr, FALSE) ) {
                    /* Use errorStatusChanged in eventsStatusBuffer to signal that the event data was updated */
                    eventStatusRecPtr->errorStatusChanged = TRUE;
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
                    if( (NULL != eventParam->DTCClassRef) && (TRUE == eventParam->DTCClassRef->DTCRef->ImmediateNvStorage) && (eventStatusRecPtr->occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
                        immediateEventStorage = TRUE;
                    }
#endif
                }
            }
        }
    }

    /* Now we need to store events that was reported during preInit.
     * That is, events not already stored in eventBuffer.  */
    for (i = 0; i < DEM_MAX_NUMBER_EVENT; i++) {
        if( (DEM_EVENT_ID_NULL != eventStatusBuffer[i].eventId) &&
             (0 == (eventStatusBuffer[i].eventStatusExtended & DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE)) &&
             (FALSE == eventIsStoredInMem(eventStatusBuffer[i].eventId, eventBuffer, eventBufferSize)) ) {

            lookupEventIdParameter(eventStatusBuffer[i].eventId, &eventParam);
            if( (NULL != eventParam) && (eventParam->EventClass->EventDestination == origin)) {
                /* Destination check is needed two avoid notifying status change twice */
                notifyEventStatusChange(eventParam, DEM_DEFAULT_EVENT_STATUS, eventStatusBuffer[i].eventStatusExtended);
            }
            if( 0 != (eventStatusBuffer[i].eventStatusExtended & DEM_TEST_FAILED_THIS_OPERATION_CYCLE) ) {
                if( E_OK == storeEventEvtMem(eventParam, &eventStatusBuffer[i], FALSE) ) {
                    /* Use errorStatusChanged in eventsStatusBuffer to signal that the event data was updated */
                    eventStatusBuffer[i].errorStatusChanged = TRUE;
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
                    if( (NULL != eventParam) && (NULL != eventParam->DTCClassRef) &&
                            (TRUE == eventParam->DTCClassRef->DTCRef->ImmediateNvStorage) && (eventStatusBuffer[i].occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
                        immediateEventStorage = TRUE;
                    }
#endif
                }
            }
        }
    }

    // Validate extended data records stored in primary memory
    extendedDataBlockChanged = validateExtendedData(extendedDataBuffer, extendedDataBufferSize, origin);

#if (DEM_USE_TIMESTAMPS == STD_ON)
    //initialize the current timestamp and update the timestamp in pre init
    initCurrentExtDataTimeStamp(&ExtData_TimeStamp);
#endif

#if ( DEM_EXT_DATA_IN_PRE_INIT )
    /* Transfer extended data to event memory if necessary */
    for (i = 0; i < DEM_MAX_NUMBER_EXT_DATA_PRE_INIT; i++) {
        if ( preInitExtDataBuffer[i].eventId != DEM_EVENT_ID_NULL ) {
            boolean updateAllExtData = FALSE;
#if defined(USE_DEM_EXTENSION)
            Dem_Extension_PreMergeExtendedData(preInitExtDataBuffer[i].eventId, &updateAllExtData);
#endif
            if( TRUE == mergeExtendedDataEvtMem(&preInitExtDataBuffer[i], extendedDataBuffer, extendedDataBufferSize, origin, updateAllExtData) ) {
                /* Use errorStatusChanged in eventsStatusBuffer to signal that the event data was updated */
                EventStatusRecType *eventStatusRecPtr = NULL_PTR;
                eventParam = NULL_PTR;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                /* Event id may be a combined id. */
                if( IS_COMBINED_EVENT_ID(preInitExtDataBuffer[i].eventId) ) {
                    const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(preInitExtDataBuffer[i].eventId)];
                    CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(preInitExtDataBuffer[i].eventId)];
                    /* Just grab the first event for this DTC. */
                    lookupEventIdParameter(CombDTCCfg->DTCClassRef->Events[0u], &eventParam);
                }
                else {
                    lookupEventIdParameter(preInitExtDataBuffer[i].eventId, &eventParam);
                }
#else
                lookupEventIdParameter(preInitExtDataBuffer[i].eventId, &eventParam);
#endif
                if( NULL_PTR != eventParam ) {
                    lookupEventStatusRec(eventParam->EventID, &eventStatusRecPtr);
                }
                if( NULL_PTR != eventStatusRecPtr ) {
                    eventStatusRecPtr->errorStatusChanged = TRUE;
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
                    if( (NULL_PTR != eventParam) && (NULL != eventParam->DTCClassRef) &&
                            (TRUE == eventParam->DTCClassRef->DTCRef->ImmediateNvStorage) && (eventStatusRecPtr->occurrence <= DEM_IMMEDIATE_NV_STORAGE_LIMIT)) {
                        immediateExtDataStorage = TRUE;
                    }
#endif
                }
            }
        }
    }
#endif

    /* Transfer freeze frames stored during preInit to event memory */
#if ( DEM_FF_DATA_IN_PRE_INIT )
    if( TRUE == transferPreInitFreezeFramesEvtMem(freezeFrameBuffer, freezeFrameBufferSize, eventBuffer, eventBufferSize, origin) ){
        freezeFrameBlockChanged = TRUE;
    }
#endif
    if( TRUE == eventBlockChanged ) {
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
        Dem_NvM_SetEventBlockChanged(origin, immediateEventStorage);
#else
        Dem_NvM_SetEventBlockChanged(origin, FALSE);
#endif
    }
    if( TRUE == extendedDataBlockChanged ) {
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
        Dem_NvM_SetExtendedDataBlockChanged(origin, immediateExtDataStorage);
#else
        Dem_NvM_SetExtendedDataBlockChanged(origin, FALSE);
#endif
    }
    if( TRUE == freezeFrameBlockChanged ) {
#if defined(DEM_USE_IMMEDIATE_NV_STORAGE)
        /* IMPROVEMENT: Immediate storage */
        Dem_NvM_SetFreezeFrameBlockChanged(origin, immediateFFStorage);
#else
        Dem_NvM_SetFreezeFrameBlockChanged(origin, FALSE);
#endif
    }
}

#if (DEM_STORE_UDS_STATUS_BIT_SUBSET_FOR_ALL_EVENTS == STD_ON)
/**
 * Set the default value for UDS status bit subset if buffer is considered
 * invalid.
 */
static void SetDefaultUDSStatusBitSubset(void)
{
    if( UDS_STATUS_BIT_MAGIC != statusBitSubsetBuffer[UDS_STATUS_BIT_MAGIC_INDEX] ) {
        memset(statusBitSubsetBuffer, 0u, sizeof(statusBitSubsetBuffer));
        for(Dem_EventIdType i = (Dem_EventIdType)0u; i < DEM_MAX_NUMBER_EVENT; i++) {
            statusBitSubsetBuffer[GET_UDSBIT_BYTE_INDEX(i+1u)] |= 1u<<(GET_UDS_STARTBIT(i+1u) + UDS_TNCSLC_BIT);
        }
    }

}

/**
 * Merges UDS status bit subset to event buffer
 */
static void MergeUDSStatusBitSubset(void)
{
    EventStatusRecType *eventStatusRec;
    if( UDS_STATUS_BIT_MAGIC == statusBitSubsetBuffer[UDS_STATUS_BIT_MAGIC_INDEX] ) {
        for(Dem_EventIdType i = (Dem_EventIdType)0; i < DEM_MAX_NUMBER_EVENT; i++) {
            eventStatusRec = NULL;
            lookupEventStatusRec(i + 1u, &eventStatusRec);
            if( (NULL != eventStatusRec) && (TRUE == eventStatusRec->isAvailable) ) {
                if( 0 == (statusBitSubsetBuffer[GET_UDSBIT_BYTE_INDEX(i+1u)] & (1u<<(GET_UDS_STARTBIT(i+1u) + UDS_TNCSLC_BIT))) ) {
                    eventStatusRec->eventStatusExtended &= ~(DEM_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR);
                }
                if( 0 != (statusBitSubsetBuffer[GET_UDSBIT_BYTE_INDEX(i+1u)] & (1u<<(GET_UDS_STARTBIT(i+1u) + UDS_TFSLC_BIT))) ) {
                    eventStatusRec->eventStatusExtended |= DEM_TEST_FAILED_SINCE_LAST_CLEAR;
                }
            }
        }
    }
}

/**
 * Transfers subset of UDS status bits from event buffer to buffer for NvM storage
 */
static void StoreUDSStatusBitSubset(void)
{
    Dem_EventStatusExtendedType eventStatus;
    const Dem_EventParameterType *eventParam;

    memset(statusBitSubsetBuffer, 0u, sizeof(statusBitSubsetBuffer));
    for(Dem_EventIdType i = (Dem_EventIdType)0; i < DEM_MAX_NUMBER_EVENT; i++) {
        if(E_OK == getEventStatus(i + 1u, &eventStatus)) {
            eventParam = NULL;
            lookupEventIdParameter(i + 1u, &eventParam);
            if( (NULL != eventParam) && (DEM_DTC_ORIGIN_NOT_USED != eventParam->EventClass->EventDestination)) {
                if( 0 != (eventStatus & DEM_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR) ) {
                    statusBitSubsetBuffer[GET_UDSBIT_BYTE_INDEX(i+1u)] |= 1u<<(GET_UDS_STARTBIT(i+1u) + UDS_TNCSLC_BIT);
                }
                if( 0 != (eventStatus & DEM_TEST_FAILED_SINCE_LAST_CLEAR) ) {
                    statusBitSubsetBuffer[GET_UDSBIT_BYTE_INDEX(i+1u)] |= 1u<<(GET_UDS_STARTBIT(i+1u) + UDS_TFSLC_BIT);
                }
            }
        }
    }
    statusBitSubsetBuffer[UDS_STATUS_BIT_MAGIC_INDEX] = UDS_STATUS_BIT_MAGIC;
    /* IPROVEMENT: Immediate storage? */
    Dem_NvM_SetStatusBitSubsetBlockChanged(FALSE);
}
#endif
#endif /* DEM_USE_MEMORY_FUNCTIONS */

#if defined(DEM_USE_IUMPR)
static void mergeIumprBuffer(void) {
    for (Dem_RatioIdType i = 0; i < DEM_IUMPR_REGISTERED_COUNT; i++) {
		iumprBufferLocal[i].denominator.value = iumprBuffer.ratios[i].denominator;
		iumprBufferLocal[i].numerator.value = iumprBuffer.ratios[i].numerator;
    }

    generalDenominatorBuffer.value = iumprBuffer.generalDenominatorCount;

	ignitionCycleCountBuffer = iumprBuffer.ignitionCycleCount;
}

static void storeIumprBuffer(void) {
    for (Dem_RatioIdType i = 0; i < DEM_IUMPR_REGISTERED_COUNT; i++) {
    	iumprBuffer.ratios[i].denominator = iumprBufferLocal[i].denominator.value;
    	iumprBuffer.ratios[i].numerator = iumprBufferLocal[i].numerator.value;
    }

    iumprBuffer.generalDenominatorCount = generalDenominatorBuffer.value;

    iumprBuffer.ignitionCycleCount = ignitionCycleCountBuffer;

    Dem_Nvm_SetIumprBlockChanged(TRUE);
}
#endif

/*
 * Procedure:   Dem_Init
 * Reentrant:   No
 */
void Dem_Init(void)
{
    /* @req DEM340 */
   //// SchM_Enter_Dem_EA_0();
    for(uint16 i = 0; i < DEM_MAX_NUMBER_EVENT; i++) {
        eventStatusBuffer[i].errorStatusChanged = FALSE;
    }
    if(DEM_PREINITIALIZED != demState) {
        /*
         * Dem_PreInit was has not been called since last time Dem_Shutdown was called.
         * This suggests that we are resuming from sleep. According to section 5.7 in
         * EcuM specification, RAM content is assumed to be still valid from the previous cycle.
         * Do not read from saved error log since buffers already contains this data.
         */
        (void)setOperationCycleState(DEM_ACTIVE, DEM_CYCLE_STATE_START);

    } else {
#if defined(DEM_USE_MEMORY_FUNCTIONS) && (DEM_STORE_UDS_STATUS_BIT_SUBSET_FOR_ALL_EVENTS == STD_ON)
        MergeUDSStatusBitSubset();
#endif
#if defined(DEM_USE_INDICATORS) && defined(DEM_USE_MEMORY_FUNCTIONS)
        mergeIndicatorBuffers();
#endif
#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
        MergeBuffer(DEM_DTC_ORIGIN_PRIMARY_MEMORY);
#endif
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
        MergeBuffer(DEM_DTC_ORIGIN_SECONDARY_MEMORY);
#endif
#if (DEM_USE_PERMANENT_MEMORY_SUPPORT == STD_ON)
        ValidateAndUpdatePermanentBuffer();
#endif
#if (DEM_PRESTORAGE_FF_DATA_IN_MEM)
        ValidateAndUpdatePreStoredFreezeFramesBuffer();
#endif

    }
#if defined(USE_FIM)
    /* @req 4.3.0/SWS_Dem_01189 */
    if( FALSE == DemFiMInit ) {
        FiM_DemInit();
        DemFiMInit = TRUE;
    }
#endif

    /* Notify application if event data was updated */
    for(uint16 i = 0; i < DEM_MAX_NUMBER_EVENT; i++) {
        /* @req DEM475 */
        if( 0 != eventStatusBuffer[i].errorStatusChanged ) {
            notifyEventDataChanged(eventStatusBuffer[i].eventParamRef);
            eventStatusBuffer[i].errorStatusChanged = FALSE;
        }
    }
#if defined(USE_DEM_EXTENSION)
    Dem_Extension_Init_Complete();
#endif

#if (DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON)
    if(ADMIN_MAGIC == priMemEventBuffer[PRI_MEM_EVENT_BUFFER_ADMIN_INDEX].AdminData.magic) {
        priMemOverflow = priMemEventBuffer[PRI_MEM_EVENT_BUFFER_ADMIN_INDEX].AdminData.overflow;
    }
#endif
#if (DEM_USE_SECONDARY_MEMORY_SUPPORT == STD_ON)
    if(ADMIN_MAGIC == secMemEventBuffer[SEC_MEM_EVENT_BUFFER_ADMIN_INDEX].AdminData.magic) {
        secMemOverflow = secMemEventBuffer[SEC_MEM_EVENT_BUFFER_ADMIN_INDEX].AdminData.overflow;
    }
#endif
    // Init the dtc filter
    dtcFilter.dtcStatusMask = DEM_DTC_STATUS_MASK_ALL;                  // All allowed
    dtcFilter.dtcKind = DEM_DTC_KIND_ALL_DTCS;                          // All kinds of DTCs
    dtcFilter.dtcOrigin = DEM_DTC_ORIGIN_PRIMARY_MEMORY;                // Primary memory
    dtcFilter.filterWithSeverity = DEM_FILTER_WITH_SEVERITY_NO;         // No Severity filtering
    dtcFilter.dtcSeverityMask = DEM_SEVERITY_NO_SEVERITY;               // Not used when filterWithSeverity is FALSE
    dtcFilter.filterForFaultDetectionCounter = DEM_FILTER_FOR_FDC_NO;   // No fault detection counter filtering
    dtcFilter.DTCIndex = 0u;

    disableDtcSetting.settingDisabled = FALSE;

    ffRecordFilter.ffIndex = DEM_MAX_NUMBER_FF_DATA_PRI_MEM;
    ffRecordFilter.dtcFormat = 0xff;

#if defined(DEM_USE_IUMPR)
    mergeIumprBuffer();

    initIumprAddiDenomCondBuffer();
#endif

    demState = DEM_INITIALIZED;

  ////  SchM_Exit_Dem_EA_0();
}


/*
 * Procedure:   Dem_shutdown
 * Reentrant:   No
 */
void Dem_Shutdown(void)
{
    VALIDATE_NO_RV(DEM_INITIALIZED == demState, DEM_SHUTDOWN_ID, DEM_E_UNINIT);
    /* @req DEM102 */
    SchM_Enter_Dem_EA_0();

    (void)setOperationCycleState(DEM_ACTIVE, DEM_CYCLE_STATE_END);
#if defined(DEM_USE_MEMORY_FUNCTIONS) && (DEM_STORE_UDS_STATUS_BIT_SUBSET_FOR_ALL_EVENTS == STD_ON)
    StoreUDSStatusBitSubset();
#endif
#if defined(DEM_USE_IUMPR)
    storeIumprBuffer();
#endif
#if defined(USE_DEM_EXTENSION)
    Dem_Extension_Shutdown();
#endif
    demState = DEM_SHUTDOWN; /** @req DEM368 */

    SchM_Exit_Dem_EA_0();
}

/*
 * Interface for basic software scheduler
 */

