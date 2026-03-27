#if (DEM_ENABLE_CONDITION_SUPPORT == STD_ON)
/* @req DEM202 */
Std_ReturnType Dem_SetEnableCondition(uint8 EnableConditionID, boolean ConditionFulfilled)
{
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_SETENABLECONDITION_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV((EnableConditionID < DEM_NUM_ENABLECONDITIONS), DEM_SETENABLECONDITION_ID, DEM_E_PARAM_DATA, E_NOT_OK);

    DemEnableConditions[EnableConditionID] = ConditionFulfilled;

    return E_OK;
}
#endif

/* Function: Dem_GetSeverityOfDTC
 * Description: Gets the severity of a DTC
 */
Dem_ReturnGetSeverityOfDTCType Dem_GetSeverityOfDTC(uint32 DTC, Dem_DTCSeverityType* DTCSeverity)
{
    /* NOTE: DTC is on UDS format according to DEM232 */
    Dem_ReturnGetSeverityOfDTCType ret = DEM_GET_SEVERITYOFDTC_WRONG_DTC;
    boolean isDone = FALSE;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETSEVERITYOFDTC_ID, DEM_E_UNINIT, DEM_GET_SEVERITYOFDTC_PENDING);
    VALIDATE_RV((NULL != DTCSeverity), DEM_GETSEVERITYOFDTC_ID, DEM_E_PARAM_POINTER, DEM_GET_SEVERITYOFDTC_PENDING);

    const Dem_DTCClassType *DTCPtr = configSet->DTCClass;
    while( (DTCPtr->Arc_EOL == FALSE) && (isDone == FALSE) ) {
        if( (DEM_NO_DTC != DTCPtr->DTCRef->UDSDTC) && (DTC == DTCPtr->DTCRef->UDSDTC) ) {
            /* Dtc found */
            isDone = TRUE;
            if( DTCIsAvailable(DTCPtr) == TRUE ) {
                *DTCSeverity = DTCPtr->DTCSeverity;
                if( DEM_SEVERITY_NO_SEVERITY == DTCPtr->DTCSeverity ) {
                    ret = DEM_GET_SEVERITYOFDTC_NOSEVERITY;
                } else {
                    ret = DEM_GET_SEVERITYOFDTC_OK;
                }
            }
            else {
                /* @req 4.2.2/SWS_Dem_01100 */
                /* Ignore suppressed DTCs *//* @req DEM587 */
                ret = DEM_GET_SEVERITYOFDTC_WRONG_DTC;
            }
        }
        DTCPtr++;
    }

    return ret;
}

/* Function: Dem_DisableDTCRecordUpdate
 * Description: Disables the event memory update of a specific DTC (only one at one time)
 */
Dem_ReturnDisableDTCRecordUpdateType Dem_DisableDTCRecordUpdate(uint32 DTC, Dem_DTCOriginType DTCOrigin)
{
    /* NOTE: DTC in in UDS format according to DEM233 */
    Dem_ReturnDisableDTCRecordUpdateType ret = DEM_DISABLE_DTCRECUP_WRONG_DTC;

    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_DISABLEDTCRECORDUPDATE_ID, DEM_E_UNINIT, DEM_DISABLE_DTCRECUP_PENDING);

    if(NO_DTC_DISABLED == DTCRecordDisabled.DTC) {
        const Dem_EventParameterType *eventIdParamPtr =  configSet->EventParameter;
        while( (eventIdParamPtr->Arc_EOL == FALSE) && (DEM_DISABLE_DTCRECUP_OK != ret)) {
            if( (NULL != eventIdParamPtr->DTCClassRef) && (eventHasDTCOnFormat(eventIdParamPtr, DEM_DTC_FORMAT_UDS) == TRUE) &&
                    (eventIdParamPtr->DTCClassRef->DTCRef->UDSDTC == DTC) && (DTCIsAvailable(eventIdParamPtr->DTCClassRef) == TRUE)) {
                /* Event references this DTC */
                ret = DEM_DISABLE_DTCRECUP_WRONG_DTCORIGIN;
                if( eventIdParamPtr->EventClass->EventDestination == DTCOrigin ) {
                    /* Event destination match. Disable update for this DTC and the event destination */
                    /* @req DEM270 */
                    DTCRecordDisabled.DTC = DTC;
                    DTCRecordDisabled.Origin = DTCOrigin;
                    ret = DEM_DISABLE_DTCRECUP_OK;
                }
            }
            eventIdParamPtr++;
        }
    } else if (DTCRecordDisabled.DTC != DTC) {
        /* The previously disabled DTC has not been enabled */
        /* @req DEM648 *//* @req DEM518 */
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_DISABLEDTCRECORDUPDATE_ID, DEM_E_WRONG_CONDITION);
        ret = DEM_DISABLE_DTCRECUP_PENDING;
    } else {
        ret = DEM_DISABLE_DTCRECUP_OK;
    }

    return ret;
}

/* Function: Dem_EnableDTCRecordUpdate
 * Description: Enables the event memory update of the DTC disabled by Dem_DisableDTCRecordUpdate() before.
 */
Std_ReturnType Dem_EnableDTCRecordUpdate(void)
{
    Std_ReturnType ret = E_NOT_OK;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_ENABLEDTCRECORDUPDATE_ID, DEM_E_UNINIT, E_NOT_OK);

    if(NO_DTC_DISABLED != DTCRecordDisabled.DTC) {
        /* @req DEM271 */
        DTCRecordDisabled.DTC = NO_DTC_DISABLED;
        ret = E_OK;
    } else {
        /* No DTC record update has been disabled */
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_ENABLEDTCRECORDUPDATE_ID, DEM_E_SEQUENCE_ERROR);
    }

    return ret;
}


/**
 * Gets the data of a freeze frame by event
 * @param EventId
 * @param RecordNumber
 * @param ReportTotalRecord
 * @param DataId
 * @param DestBuffer
 * @param BufSize
 * @param CareAboutBufsize
 * @return E_OK: Operation was successful, E_NOT_OK: Operation failed
 */
static Std_ReturnType Dem_GetEventFreezeFrameData_Internal(Dem_EventIdType EventId, uint8 RecordNumber, boolean ReportTotalRecord,
        uint16 DataId, uint8* DestBuffer, uint8* BufSize, boolean CareAboutBufsize)
{
    /* @req DEM478*/
    /* @req DEM479 */
    Std_ReturnType ret = E_NOT_OK;
    const Dem_EventParameterType *eventIdParamPtr = NULL_PTR;
    uint8 recordToReport = 0u;
    uint8 destBufferIndex = 0u;
    uint8 *ffRecordData;
    EventStatusRecType * eventStatusRec = NULL_PTR;
    Dem_FreezeFrameKindType ffKind = DEM_FREEZE_FRAME_NON_OBD;
    if( DEM_INITIALIZED == demState  ) {
        lookupEventIdParameter(EventId, &eventIdParamPtr);

        lookupEventStatusRec(EventId, &eventStatusRec);
        if( (NULL != eventIdParamPtr) && (NULL != eventStatusRec) && (TRUE == eventStatusRec->isAvailable) ) {
            /* Event has freeze frames configured */
            if(getFFRecData(eventIdParamPtr, RecordNumber, &ffRecordData, &recordToReport, &ffKind) == TRUE) {
                /* And the record we are looking for was found */
                const Dem_FreezeFrameClassType *freezeFrameClass = NULL_PTR;
                const Dem_PidOrDidType * const *xidPtr;
                if(DEM_FREEZE_FRAME_NON_OBD == ffKind ) {
                	getFFClassReference(eventIdParamPtr, (Dem_FreezeFrameClassType **) &freezeFrameClass);
                    xidPtr = freezeFrameClass->FFIdClassRef;
                } else {
                    freezeFrameClass = configSet->GlobalOBDFreezeFrameClassRef;
                    xidPtr = freezeFrameClass->FFIdClassRef;
                }
                uint16 ffDataIndex = 0u;
                boolean done = FALSE;
                while( ((*xidPtr)->Arc_EOL == FALSE) && (done == FALSE)) {
                    if(DEM_FREEZE_FRAME_NON_OBD == ffKind ) {
                        ffDataIndex += DEM_DID_IDENTIFIER_SIZE_OF_BYTES;
                    } else {
                        ffDataIndex += DEM_PID_IDENTIFIER_SIZE_OF_BYTES;
                    }
                    if( (ReportTotalRecord == TRUE) ||
                       ((DEM_FREEZE_FRAME_NON_OBD == ffKind) && (DataId == (*xidPtr)->DidIdentifier)) ||
                       ((DEM_FREEZE_FRAME_NON_OBD != ffKind) && (DataId == (*xidPtr)->PidIdentifier))) {
                        if(CareAboutBufsize == TRUE){
                            if(((*xidPtr)->PidOrDidSize + destBufferIndex) > *BufSize){
                                *BufSize = destBufferIndex;
                                return E_NOT_OK; /* buffer full, no info in DLT spec what to do for this case so we return operation failed */
                            }
                        }
                        memcpy(&DestBuffer[destBufferIndex], &ffRecordData[ffDataIndex], (size_t)((*xidPtr)->PidOrDidSize));
                        destBufferIndex += (*xidPtr)->PidOrDidSize;
                        done = (ReportTotalRecord == FALSE)? TRUE: FALSE;
                        ret = E_OK;
                    }
                    ffDataIndex += (uint16)((*xidPtr)->PidOrDidSize);
                    xidPtr++;
                }
            }
        }
    } else {
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GETEVENTFREEZEFRAMEDATA_ID, DEM_E_UNINIT);
    }

    *BufSize = destBufferIndex;
    return ret;
}

/**
 * Gets the data of a freeze frame by event
 * @param EventId
 * @param RecordNumber
 * @param ReportTotalRecord
 * @param DataId
 * @param DestBuffer
 * @return E_OK: Operation was successful, E_NOT_OK: Operation failed
 */
Std_ReturnType Dem_GetEventFreezeFrameData(Dem_EventIdType EventId, uint8 RecordNumber, boolean ReportTotalRecord, uint16 DataId, uint8* DestBuffer)
{
    uint8 BufSize = 0x0; /* dummy size not used */
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETEVENTFREEZEFRAMEDATA_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV((NULL != DestBuffer), DEM_GETEVENTFREEZEFRAMEDATA_ID, DEM_E_PARAM_POINTER, E_NOT_OK);
    return Dem_GetEventFreezeFrameData_Internal(EventId,RecordNumber,ReportTotalRecord,DataId,DestBuffer, &BufSize, FALSE);
}

#if (DEM_TRIGGER_DLT_REPORTS == STD_ON)
/**
 * Gets the most recent data of a freeze frame by event for DLT
 * @param EventId
 * @param DestBuffer
 * @param BufSize
 * @return E_OK: Operation was successful, E_NOT_OK: Operation failed
 */
Std_ReturnType Dem_DltGetMostRecentFreezeFrameRecordData(Dem_EventIdType EventId, uint8* DestBuffer, uint8* BufSize){
    /* @req DEM632 */
    /* @req DEM633 */
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_DLTGETMOSTRECENTFREEZEFRAMERECORDDATA_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV(((NULL != DestBuffer) && (NULL != BufSize)), DEM_DLTGETMOSTRECENTFREEZEFRAMERECORDDATA_ID, DEM_E_PARAM_POINTER, E_NOT_OK);
	return Dem_GetEventFreezeFrameData_Internal(EventId, MOST_RECENT_FF_RECORD, TRUE,0, DestBuffer, BufSize, TRUE);
}
#endif


/**
 * Gets the data of an extended data record by event
 * @param EventId
 * @param RecordNumber
 * @param DestBuffer
 * @param BufSize
 * @param CareAboutBufsize
 * @return E_OK: Operation was successful, E_NOT_OK: Operation failed
 */
static Std_ReturnType Dem_GetEventExtendedDataRecord_Internal(Dem_EventIdType EventId, uint8 RecordNumber, uint8* DestBuffer, uint8* BufSize, boolean CareAboutBufsize)
{
    /* @req DEM476 */
    /* @req DEM477 */
    Std_ReturnType ret = E_NOT_OK;
    const Dem_EventParameterType *eventIdParamPtr = NULL_PTR;
    Dem_ExtendedDataRecordClassType const *extendedDataRecordClass = NULL_PTR;
    ExtDataRecType *extData;
    uint16 posInExtData = 0u;
    uint16 extdataIndex = 0u;
    uint8 extendedDataNumber = RecordNumber;
    boolean done = FALSE;
    boolean readFailed = FALSE;
    uint8 destBufferIndex = 0u;
    EventStatusRecType * eventStatusRec = NULL_PTR;

    if( DEM_INITIALIZED == demState  ) {
        if( IS_VALID_EXT_DATA_RECORD(RecordNumber) || (ALL_EXTENDED_DATA_RECORDS == RecordNumber) ) {
            /* Record number ok */
            lookupEventIdParameter(EventId, &eventIdParamPtr);
            lookupEventStatusRec(EventId, &eventStatusRec);
            if( (NULL != eventIdParamPtr) && (NULL != eventIdParamPtr->ExtendedDataClassRef) &&
                    (NULL != eventStatusRec) && (TRUE == eventStatusRec->isAvailable) ) {
                /* Event ok and has extended data */
                ret = E_OK;
                while( (done == FALSE) && (NULL != eventIdParamPtr->ExtendedDataClassRef->ExtendedDataRecordClassRef[extdataIndex])) {
                    readFailed = TRUE;
                    if( ALL_EXTENDED_DATA_RECORDS == RecordNumber) {
                        extendedDataNumber = eventIdParamPtr->ExtendedDataClassRef->ExtendedDataRecordClassRef[extdataIndex]->RecordNumber;
                    } else {
                        /* Should only read one specific record */
                        done = TRUE;
                    }
                    if (lookupExtendedDataRecNumParam(extendedDataNumber, eventIdParamPtr, &extendedDataRecordClass, &posInExtData) == TRUE) {
                        if(CareAboutBufsize == TRUE){
                            if((extendedDataRecordClass->DataSize + destBufferIndex) > *BufSize){
                                *BufSize = destBufferIndex;
                                return E_NOT_OK; /* buffer full, no info in DLT spec what to do for this case so we return operation failed */
                            }
                        }
                        if( extendedDataRecordClass->UpdateRule != DEM_UPDATE_RECORD_VOLATILE ) {
                            if (lookupExtendedDataMem(EventId, &extData, eventIdParamPtr->EventClass->EventDestination) == TRUE ) {
                                // Yes all conditions met, copy the extended data record to destination buffer.
                                memcpy(&DestBuffer[destBufferIndex], &extData->data[posInExtData], (size_t)extendedDataRecordClass->DataSize); /** @req DEM075 */
                                destBufferIndex += (uint8)extendedDataRecordClass->DataSize;
                                readFailed = FALSE;
                            }
                        } else {
                            if( NULL != extendedDataRecordClass->CallbackGetExtDataRecord ) {
                                if(E_OK == extendedDataRecordClass->CallbackGetExtDataRecord(&DestBuffer[destBufferIndex])) {
                                    readFailed = FALSE;
                                }
                                destBufferIndex += (uint8)extendedDataRecordClass->DataSize;
                            }  else if (DEM_NO_ELEMENT != extendedDataRecordClass->InternalDataElement ) {
                                getInternalElement(eventIdParamPtr, extendedDataRecordClass->InternalDataElement, &DestBuffer[destBufferIndex], extendedDataRecordClass->DataSize );
                                destBufferIndex += (uint8)extendedDataRecordClass->DataSize;
                                readFailed = FALSE;
                            } else {
                               /* No callback and no internal element.
                                * IMPROVMENT: Det_error */
                            }
                        }
                    }
                    if( readFailed == TRUE) {
                        /* Something failed reading the data */
                        done = TRUE;
                        ret = E_NOT_OK;
                    }
                    extdataIndex++;
                }
            }
        }
    } else {
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GETEVENTEXTENDEDDATARECORD_ID, DEM_E_UNINIT);
    }

    *BufSize = destBufferIndex;
    return ret;
}

/**
 * Gets the data of an extended data record by event
 * @param EventId
 * @param RecordNumber
 * @param DestBuffer
 * @return E_OK: Operation was successful, E_NOT_OK: Operation failed
 */
Std_ReturnType Dem_GetEventExtendedDataRecord(Dem_EventIdType EventId, uint8 RecordNumber, uint8* DestBuffer)
{
    uint8 Bufsize = 0; /* Dummy size not used */
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETEVENTEXTENDEDDATARECORD_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV((NULL != DestBuffer), DEM_GETEVENTEXTENDEDDATARECORD_ID, DEM_E_PARAM_POINTER, E_NOT_OK);
    return Dem_GetEventExtendedDataRecord_Internal(EventId, RecordNumber, DestBuffer, &Bufsize, FALSE);
}

#if (DEM_TRIGGER_DLT_REPORTS == STD_ON)
/**
 * Gets all the data of an extended data record by event for DLT
 * @param EventId
 * @param DestBuffer
 * @param BufSize
 * @return E_OK: Operation was successful, E_NOT_OK: Operation failed
 */
Std_ReturnType Dem_DltGetAllExtendedDataRecords(Dem_EventIdType EventId, uint8* DestBuffer, uint8* BufSize){
    /* @req DEM634 */
    /* @req DEM635 */
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_DLTGETALLEXTENDEDDATARECORDS_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV(((NULL != DestBuffer) && (NULL != BufSize)), DEM_DLTGETALLEXTENDEDDATARECORDS_ID, DEM_E_PARAM_POINTER, E_NOT_OK);
 	return Dem_GetEventExtendedDataRecord_Internal(EventId, ALL_EXTENDED_DATA_RECORDS, DestBuffer, BufSize, TRUE);
}
#endif

#if ( DEM_PRESTORAGE_FF_DATA_IN_MEM )
/**
 * Captures the freeze frame data for a specific event.
 * @param EventId
 * @return E_OK: Freeze frame prestorage was successful, E_NOT_OK: Freeze frame prestorage failed
 */
/* @req DEM188 */
/* @req DEM189 */
/* @req DEM334 */
Std_ReturnType Dem_PrestoreFreezeFrame(Dem_EventIdType EventId)
{
    Std_ReturnType ret = E_NOT_OK;
    const Dem_EventParameterType *eventParam;
    EventStatusRecType *eventStatusRec = NULL;
    FreezeFrameRecType freezeFrame = {0};

    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_PRE_STORE_FF_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV(IS_VALID_EVENT_ID(EventId), DEM_PRE_STORE_FF_ID, DEM_E_PARAM_DATA, E_NOT_OK);

    /* Find eventParameter to each eventId belongs */
    lookupEventIdParameter(EventId, &eventParam);

    /* Find event status */
    lookupEventStatusRec(EventId, &eventStatusRec);

    if ( (eventParam != NULL) && (eventStatusRec != NULL) ) {
        if (eventStatusRec->isAvailable == TRUE) {
            if (eventParam->EventClass->FFPrestorageSupported == TRUE) {
                /* To see if it is NON-OBD check if there is a FF class configured for this event */
                if( DEM_FF_NULLREF != getFFIdx(eventParam)) {
                    getFreezeFrameData(eventParam, &freezeFrame, DEM_FREEZE_FRAME_NON_OBD, DEM_DTC_ORIGIN_NOT_USED, TRUE);

                    if (freezeFrame.eventId != DEM_EVENT_ID_NULL) {
                        /* is there already pre-stored FFs */
                        if (storeFreezeFrameDataMem(eventParam, &freezeFrame, memPreStoreFreezeFrameBuffer, DEM_MAX_NUMBER_PRESTORED_FF, DEM_DTC_ORIGIN_NOT_USED) == TRUE) { /** @req DEM190 */
                            ret = E_OK;
                        }
                    }
                }

                /* check for OBD */
                if( DEM_NON_EMISSION_RELATED != (Dem_Arc_EventDTCKindType) *eventParam->EventDTCKind ) {
                    getFreezeFrameData(eventParam, &freezeFrame, DEM_FREEZE_FRAME_OBD, DEM_DTC_ORIGIN_NOT_USED, FALSE);

                    if (freezeFrame.eventId != DEM_EVENT_ID_NULL) {
                        /* is there already pre-stored OBD FFs */
                        if (storeFreezeFrameDataMem(eventParam, &freezeFrame, memPreStoreFreezeFrameBuffer, DEM_MAX_NUMBER_PRESTORED_FF, DEM_DTC_ORIGIN_NOT_USED) == TRUE) { /** @req DEM190 */
                            ret = E_OK;
                        }
                    }
                }
            }
        }
    }
    return ret;
}

/**
 * Clears a prestored freeze frame of a specific event.
 * @param EventId
 * @return E_OK: Clear prestored freeze frame was successful, E_NOT_OK: Clear prestored freeze frame failed
 */
/* @req DEM193 */
/* @req DEM050 */
/* @req DEM334 */
Std_ReturnType Dem_ClearPrestoredFreezeFrame(Dem_EventIdType EventId)
{
    Std_ReturnType ret = E_NOT_OK;
    const Dem_EventParameterType *eventParam;

    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_CLEAR_PRE_STORED_FF_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV(IS_VALID_EVENT_ID(EventId), DEM_CLEAR_PRE_STORED_FF_ID, DEM_E_PARAM_DATA, E_NOT_OK);

    /* Find eventParameter to each eventId belongs */
    lookupEventIdParameter(EventId, &eventParam);

    if ( eventParam != NULL ) {
        if (eventParam->EventClass->FFPrestorageSupported == TRUE) {
            boolean combinedDTC = FALSE;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
            if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
                combinedDTC = TRUE;
            }
#endif
            /* It could be OBD or non-OBD */
            if (deleteFreezeFrameDataMem(eventParam, DEM_DTC_ORIGIN_NOT_USED, combinedDTC) == TRUE) { /** @req DEM190 */
                ret = E_OK;
            }
        } else {
            ret = E_NOT_OK; // DemFFPrestorage is not supported for this EventClass
        }
    } else {
        ret = E_NOT_OK; // Event ID not configured or set to not available
    }

    return ret;
}
#endif

/**
 * Gets the event memory overflow indication status
 * @param DTCOrigin
 * @param OverflowIndication
 * @return E_OK: Operation was successful, E_NOT_OK: Operation failed or is not supported
 */
/* @req DEM559 *//* @req DEM398 */
Std_ReturnType Dem_GetEventMemoryOverflow(Dem_DTCOriginType DTCOrigin, boolean *OverflowIndication)
{
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETEVENTMEMORYOVERFLOW_ID, DEM_E_UNINIT, E_NOT_OK)
    VALIDATE_RV(NULL != OverflowIndication, DEM_GETEVENTMEMORYOVERFLOW_ID, DEM_E_PARAM_POINTER, E_NOT_OK);
    return getOverflowIndication(DTCOrigin, OverflowIndication);
}

#if (DEM_UNIT_TEST == STD_ON)
#if ( DEM_FF_DATA_IN_PRE_INIT )
void getFFDataPreInit(FreezeFrameRecType **buf);
void getFFDataPreInit(FreezeFrameRecType **buf)
{
    *buf = &preInitFreezeFrameBuffer[0];
    return;
}
#endif
#if (DEM_USE_TIMESTAMPS == STD_ON)
uint32 getCurTimeStamp(void)
{
    return FF_TimeStamp;
}
#endif
void getEventStatusBufPtr(EventStatusRecType **buf);
void getEventStatusBufPtr(EventStatusRecType **buf)
{
    *buf = &eventStatusBuffer[0];
    return;
}
#endif /* DEM_UNIT_TEST */





/****************
 * OBD-specific *
 ***************/
/*
 * Procedure:   Dem_GetDTCOfOBDFreezeFrame
 * Reentrant:   No
 */
 /* @req OBD_DEM_REQ_3 */
Std_ReturnType Dem_GetDTCOfOBDFreezeFrame(uint8 FrameNumber, uint32* DTC )
{
    /* @req DEM623 */
    const FreezeFrameRecType *freezeFrame = NULL;
    const Dem_EventParameterType *eventParameter = NULL;
    Std_ReturnType returnCode = E_NOT_OK;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETDTCOFOBDFREEZEFRAME_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV(NULL != DTC, DEM_GETDTCOFOBDFREEZEFRAME_ID, DEM_E_PARAM_POINTER, E_NOT_OK);
    VALIDATE_RV(0 == FrameNumber, DEM_GETDTCOFOBDFREEZEFRAME_ID, DEM_E_PARAM_DATA, E_NOT_OK);

    /* find the corresponding FF in FF buffer */
    /* @req OBD_DEM_REQ_1 */
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM)
        for(uint16 i = 0; i < DEM_MAX_NUMBER_FF_DATA_PRI_MEM; i++){
            if((priMemFreezeFrameBuffer[i].eventId != DEM_EVENT_ID_NULL)
                && (DEM_FREEZE_FRAME_OBD == priMemFreezeFrameBuffer[i].kind)){
                freezeFrame = &priMemFreezeFrameBuffer[i];
                break;
            }
        }
#endif
    /*if FF found,find the corresponding eventParameter*/
    if( freezeFrame != NULL ) {
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
        if( !IS_COMBINED_EVENT_ID(freezeFrame->eventId) ) {
            /* Only do this if the entry is NOT a combined event entry. This to avoid Det error. */
            lookupEventIdParameter(freezeFrame->eventId, &eventParameter);
        }
#else
        lookupEventIdParameter(freezeFrame->eventId, &eventParameter);
#endif

        if(eventParameter != NULL){
            /* if DTCClass configured,get DTC value */
            if((eventParameter->DTCClassRef != NULL) && (DTCIsAvailable(eventParameter->DTCClassRef) == TRUE) && (eventHasDTCOnFormat(eventParameter, DEM_DTC_FORMAT_OBD) == TRUE)){
                *DTC = TO_OBD_FORMAT(eventParameter->DTCClassRef->DTCRef->OBDDTC);
                returnCode = E_OK;
            }
            else {
                /* Event has no DTC or DTC is suppressed */
                /* @req 4.2.2/SWS_Dem_01101 *//* @req DEM587 */
            }
        }
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
        else {
            if( IS_COMBINED_EVENT_ID(freezeFrame->eventId) ) {
                const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(freezeFrame->eventId)];
                if( (TRUE == DTCISAvailableOnFormat(CombDTCCfg->DTCClassRef, DEM_DTC_FORMAT_OBD)) &&
                        (TRUE == DTCIsAvailable(CombDTCCfg->DTCClassRef)) ) {
                    *DTC = TO_OBD_FORMAT(CombDTCCfg->DTCClassRef->DTCRef->OBDDTC);
                    returnCode = E_OK;
                }
            }
        }
#endif

    }

    return returnCode;

}

/*
 * Procedure:   Dem_ReadDataOfOBDFreezeFrame
 * Reentrant:   No
 */
 /* @req OBD_DEM_REQ_2 */
/*lint -efunc(818,Dem_ReadDataOfOBDFreezeFrame) Pointers cannot be declared as pointing to const as API defined by AUTOSAR  */
Std_ReturnType Dem_ReadDataOfOBDFreezeFrame(uint8 PID, uint8 DataElementIndexOfPid, uint8* DestBuffer, uint8* BufSize)
{
    /* IMPROVEMENT: Validate parameters */
    /* @req DEM596 */
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_READDATAOFOBDFREEZEFRAME_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV(NULL != DestBuffer, DEM_READDATAOFOBDFREEZEFRAME_ID, DEM_E_PARAM_POINTER, E_NOT_OK);
    VALIDATE_RV(NULL != BufSize, DEM_READDATAOFOBDFREEZEFRAME_ID, DEM_E_PARAM_POINTER, E_NOT_OK);
    Std_ReturnType returnCode = E_NOT_OK;

    /* IMPROVEMENT: DataElementIndexOfPid should be used to get the data of the Pid. But we only support 1 data element
     * per Pid.. */
    (void)DataElementIndexOfPid;

#if (DEM_MAX_NR_OF_PIDS_IN_FREEZEFRAME_DATA > 0)
    const FreezeFrameRecType *freezeFrame = NULL;
    const Dem_FreezeFrameClassType *freezeFrameClass;
    boolean pidFound = FALSE;
    uint16 offset = 0;
    uint8 pidDataSize = 0u;
    SchM_Enter_Dem_EA_0();
    freezeFrameClass = configSet->GlobalOBDFreezeFrameClassRef;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE2) && ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM)
    uint8 bufSizeLeft = *BufSize;
    uint32 timestamp = 0u;
    for(uint16 i = 0u; i < DEM_MAX_NUMBER_FF_DATA_PRI_MEM; i++) {
        /* @req OBD_DEM_REQ_1 */
        if((priMemFreezeFrameBuffer[i].eventId != DEM_EVENT_ID_NULL) && (DEM_FREEZE_FRAME_OBD == priMemFreezeFrameBuffer[i].kind)) {
            freezeFrame = &priMemFreezeFrameBuffer[i];
            if( (freezeFrameClass->FFIdClassRef != NULL) && ((FALSE == pidFound) || (priMemFreezeFrameBuffer[i].timeStamp < timestamp)) ) {
                timestamp = priMemFreezeFrameBuffer[i].timeStamp;
                offset = 0u;
                for(uint16 pidIdx = 0u; (pidIdx < DEM_MAX_NR_OF_PIDS_IN_FREEZEFRAME_DATA) && ((freezeFrameClass->FFIdClassRef[pidIdx]->Arc_EOL) == FALSE); pidIdx++) {
                    offset += DEM_PID_IDENTIFIER_SIZE_OF_BYTES;
                    if(freezeFrameClass->FFIdClassRef[pidIdx]->PidIdentifier == PID) {
                        pidFound = TRUE;
                        /* Found. Copy the data. */
                        if( (bufSizeLeft >= freezeFrameClass->FFIdClassRef[pidIdx]->PidOrDidSize) && (PID == (freezeFrame->data[offset - DEM_PID_IDENTIFIER_SIZE_OF_BYTES]))
                            && ((offset + (uint16)freezeFrameClass->FFIdClassRef[pidIdx]->PidOrDidSize) <= (uint16)(freezeFrame->dataSize))
                            && ((offset + (uint16)freezeFrameClass->FFIdClassRef[pidIdx]->PidOrDidSize) <= DEM_MAX_SIZE_FF_DATA)) {
                            memcpy(&DestBuffer[pidDataSize], &freezeFrame->data[offset], (size_t)freezeFrameClass->FFIdClassRef[pidIdx]->PidOrDidSize);
                            returnCode = E_OK;
                        }
                        else {
                            /* Something wrong */
                            returnCode = E_NOT_OK;
                        }
                        pidDataSize = freezeFrameClass->FFIdClassRef[pidIdx]->PidOrDidSize;
                        break;
                    }
                    else {
                        offset += (uint16)freezeFrameClass->FFIdClassRef[pidIdx]->PidOrDidSize;
                    }
                }
            }
        }
    }
    if( (E_OK == returnCode) && (TRUE == pidFound) ) {
        *BufSize = pidDataSize;
    }
#else
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM)
    /*find the corresponding FF in FF buffer*/
    for(uint16 i = 0u; i < DEM_MAX_NUMBER_FF_DATA_PRI_MEM; i++) {
        /* @req OBD_DEM_REQ_1 */
        if((priMemFreezeFrameBuffer[i].eventId != DEM_EVENT_ID_NULL)
            && (DEM_FREEZE_FRAME_OBD == priMemFreezeFrameBuffer[i].kind)) {
            freezeFrame = &priMemFreezeFrameBuffer[i];
            break;
        }
    }
#endif
    /*if FF class found,find the corresponding PID*/
    if(NULL != freezeFrame) {
        offset = 0u;
        if(freezeFrameClass->FFKind == DEM_FREEZE_FRAME_OBD) {
            if(freezeFrameClass->FFIdClassRef != NULL){
                for(uint16 i = 0u; (i < DEM_MAX_NR_OF_PIDS_IN_FREEZEFRAME_DATA) && ((freezeFrameClass->FFIdClassRef[i]->Arc_EOL) == FALSE); i++) {
                    offset += DEM_PID_IDENTIFIER_SIZE_OF_BYTES;
                    if(freezeFrameClass->FFIdClassRef[i]->PidIdentifier == PID){
                        pidDataSize = freezeFrameClass->FFIdClassRef[i]->PidOrDidSize;
                        pidFound = TRUE;
                        break;
                    } else{
                        offset += (uint16)freezeFrameClass->FFIdClassRef[i]->PidOrDidSize;
                    }
                }
            }
        }
    }

    if( (TRUE == pidFound) && (NULL != freezeFrame) && (offset >= DEM_PID_IDENTIFIER_SIZE_OF_BYTES) ) {
        if(((*BufSize) >= pidDataSize) && (PID == (freezeFrame->data[offset - DEM_PID_IDENTIFIER_SIZE_OF_BYTES]))
            && ((offset + (uint16)pidDataSize) <= (uint16)(freezeFrame->dataSize))
			&& ((offset + (uint16)pidDataSize) <= DEM_MAX_SIZE_FF_DATA)) {
            memcpy(DestBuffer, &freezeFrame->data[offset], (size_t)pidDataSize);
            *BufSize = pidDataSize;
            returnCode = E_OK;
        }
    }
#endif
    SchM_Exit_Dem_EA_0();
#else
    (void)PID;
#endif

    return returnCode;
}

/*
 * Procedure:   storeOBDFreezeFrameDataMem
 * Description: store OBD FreezeFrame data record in primary memory
 */
#ifdef DEM_USE_MEMORY_FUNCTIONS
#if ( DEM_FF_DATA_IN_PRE_INIT || DEM_FF_DATA_IN_PRI_MEM )
static boolean storeOBDFreezeFrameDataMem(const Dem_EventParameterType *eventParam, const FreezeFrameRecType *freezeFrame,
                                       FreezeFrameRecType* freezeFrameBuffer, uint32 freezeFrameBufferSize,
                                       Dem_DTCOriginType origin)
{
    boolean eventIdFound = FALSE;
    boolean eventIdFreePositionFound = FALSE;
    uint32 i;
    boolean dataStored = FALSE;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE2) && (DEM_MAX_NUM_OBD_FFS > 1)
    const Dem_EventParameterType *eventParameter;
#endif
    (void)origin;

    /* Check if already stored */
    for (i = 0uL; (i < freezeFrameBufferSize) && (FALSE == eventIdFound); i++){
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE2) && (DEM_MAX_NUM_OBD_FFS > 1)
        /* Need to be able to store more than one OBD FF. Allow storage if not already stored for this
         * event or for some other DTC. */
        if( (freezeFrameBuffer[i].eventId != DEM_EVENT_ID_NULL) && (freezeFrameBuffer[i].kind == DEM_FREEZE_FRAME_OBD) ) {
            /* Found an OBD freeze frame. Check event ID. */
            if( freezeFrameBuffer[i].eventId != eventParam->EventID ) {
                eventParameter = NULL_PTR;
                lookupEventIdParameter(freezeFrameBuffer[i].eventId, &eventParameter);
                if( NULL_PTR != eventParameter ) {
                    if( (NULL_PTR != eventParameter->DTCClassRef) && (NULL_PTR != eventParam->DTCClassRef) && (eventParameter->DTCClassRef != eventParam->DTCClassRef) ) {
                        /* Freeze frame is for different DTC. */
                        eventIdFound = TRUE;
                    }
                }
            }
            else {
                /* Same ID. */
                eventIdFound = TRUE;
            }
        }
#else
        eventIdFound = ((freezeFrameBuffer[i].eventId != DEM_EVENT_ID_NULL)
            && (freezeFrameBuffer[i].kind == DEM_FREEZE_FRAME_OBD))? TRUE: FALSE;
#endif /* DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE2 */
    }

    if ( FALSE == eventIdFound ) {
        /* find the first free position */
        for (i = 0uL; (i < freezeFrameBufferSize) && (FALSE == eventIdFreePositionFound); i++){
            eventIdFreePositionFound =  (freezeFrameBuffer[i].eventId == DEM_EVENT_ID_NULL)? TRUE: FALSE;
        }
        /* if found,copy it to this position */
        if ( TRUE == eventIdFreePositionFound ) {
            memcpy(&freezeFrameBuffer[i-1], freezeFrame, sizeof(FreezeFrameRecType));
        } else {
#if (DEM_EVENT_DISPLACEMENT_SUPPORT == STD_ON)
            /* if not found,do displacement */
            FreezeFrameRecType *freezeFrameLocal = NULL;
            if( TRUE == lookupFreezeFrameForDisplacement(eventParam, &freezeFrameLocal, freezeFrameBuffer, freezeFrameBufferSize) ) {
                if(freezeFrameLocal != NULL){
                    memcpy(freezeFrameLocal, freezeFrame, sizeof(FreezeFrameRecType));
                    dataStored = TRUE;
                } else {
                    setOverflowIndication(eventParam->EventClass->EventDestination, TRUE);
                }
            }
#else
            setOverflowIndication(eventParam->EventClass->EventDestination, TRUE);
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_STORE_FF_DATA_MEM_ID, DEM_E_MEM_FF_DATA_BUFF_FULL);
#endif /* DEM_EVENT_DISPLACEMENT_SUPPORT */
        }
    } else {
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE2) && (DEM_MAX_NUM_OBD_FFS > 1)
        /* OBD freeze frame was already stored. Check if we should replace it.
         * We replace it if the new event has higher priority than all the ones
         * previously stored. */
        /* Check all OBD FFs stored */
        boolean replaceOBDFF = TRUE;
        const Dem_EventParameterType *storedEventParam;
        for (uint32 idx = 0uL; (idx < freezeFrameBufferSize) && (TRUE == replaceOBDFF); idx++){
            if( (freezeFrameBuffer[idx].eventId != DEM_EVENT_ID_NULL) && (freezeFrameBuffer[idx].kind == DEM_FREEZE_FRAME_OBD) ) {
                storedEventParam = NULL_PTR;
                lookupEventIdParameter(freezeFrameBuffer[idx].eventId, &storedEventParam);
                if( NULL_PTR != storedEventParam ) {
                    if( storedEventParam->EventClass->EventPriority <= eventParam->EventClass->EventPriority ) {
                        /* Priority of stored event is higher. We should not replace the FF. */
                        replaceOBDFF = FALSE;
                    }
                }
            }
        }
        if( TRUE == replaceOBDFF ) {
            /* We should replace the currently stored OBF FF. There could be more than one stored so
             * we need to loop through the whole buffer again. */
            for (uint32 idx = 0UL; idx < freezeFrameBufferSize; idx++){
                if( (freezeFrameBuffer[idx].eventId != DEM_EVENT_ID_NULL) && (freezeFrameBuffer[idx].kind == DEM_FREEZE_FRAME_OBD) ) {
#if 0
                    memset(&freezeFrameBuffer[idx], 0, sizeof(FreezeFrameRecType));
#else
                    freezeFrameBuffer[idx].eventId = DEM_EVENT_ID_NULL;
#endif
                }
            }
            memcpy(&freezeFrameBuffer[i-1], freezeFrame, sizeof(FreezeFrameRecType));
            dataStored = TRUE;
        }

#else
        /* OBD freeze frame was already stored. Check if we should replace it.
         * We replace it if the new event has higher priority. */
        const Dem_EventParameterType *storedEventParam = NULL;
        boolean replaceOBDFF = TRUE;
        uint8 priorityStored = 0xFFu;/* Lowest prio. */
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
        if( IS_COMBINED_EVENT_ID(freezeFrameBuffer[i-1].eventId) ) {
            const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(freezeFrameBuffer[i-1].eventId)];
            priorityStored = CombDTCCfg->Priority;
        }
        else {
            lookupEventIdParameter(freezeFrameBuffer[i-1].eventId, &storedEventParam);
            if( NULL != storedEventParam ) {
                priorityStored = storedEventParam->EventClass->EventPriority;
            }
        }
#else
        lookupEventIdParameter(freezeFrameBuffer[i-1].eventId, &storedEventParam);
        if( NULL != storedEventParam ) {
            priorityStored = storedEventParam->EventClass->EventPriority;
        }
#endif
        if( priorityStored <= eventParam->EventClass->EventPriority  ) {
            /* Priority of the new event is lower or equal to the stored event.
             * Should NOT replace the FF. */
            replaceOBDFF = FALSE;
        }
        if( TRUE == replaceOBDFF ) {
            memcpy(&freezeFrameBuffer[i-1], freezeFrame, sizeof(FreezeFrameRecType));
            dataStored = TRUE;
        }
#endif/* DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE2 */
    }
    return dataStored;
}
#endif /* DEM_FF_DATA_IN_PRE_INIT || DEM_FF_DATA_IN_PRI_MEM  */
#endif /* DEM_USE_MEMORY_FUNCTIONS */

#if (DEM_OBD_SUPPORT == STD_ON)
/**
 * Service for reporting the event as disabled to the Dem for the PID $41 computation.
 *
 * @param EventId: identification of an event by assigned EventId.
 * @return E_OK set of event to disabled was successful.
 */
Std_ReturnType Dem_SetEventDisabled(Dem_EventIdType EventId )
{
	/* @req Dem312 */
	/* @req Dem348 */
	/* @req Dem294 */
	EventStatusRecType *eventStatusRec = NULL;

	SchM_Enter_Dem_EA_0();
	lookupEventStatusRec(EventId, &eventStatusRec);
	eventStatusRec->isDisabled = 1;
	SchM_Exit_Dem_EA_0();

    return E_OK;
}

/**
 * Gets the number of confirmed OBD DTCs.
 *
 * @return the number of confirmed OBD DTCs, between 0 and 127.
 */
static uint8_t getNumberOfConfirmedObdDTCs(void) {
	/* @req DEM351 */
	uint8_t confirmedBitMask = DEM_CONFIRMED_DTC;
	uint16_t confirmedDTCs = 0;
	if (DEM_FILTER_ACCEPTED == Dem_SetDTCFilter(confirmedBitMask, DEM_DTC_KIND_EMISSION_REL_DTCS, DEM_DTC_FORMAT_OBD, DEM_DTC_ORIGIN_PRIMARY_MEMORY, DEM_FILTER_WITH_SEVERITY_NO, DEM_SEVERITY_NO_SEVERITY, DEM_FILTER_FOR_FDC_NO)) {
		while (Dem_GetNumberOfFilteredDtc(&confirmedDTCs) == DEM_NUMBER_PENDING) {
			// wait until it either succeeded or failed
		}

		// Can store at most a non-negative number of 6 bits (127)
		if (confirmedDTCs > 127) {
			confirmedDTCs = 127;
		}
	}

	return (uint8_t) confirmedDTCs;
}

/**
 * Gets the MIL status.
 *
 * @param numberOfConfirmedDtcs: The number of confirmed OBD DTCs.
 * @return TRUE if MIL is ON, otherwise FALSE.
 */
static uint8_t isObdMilOn(uint8_t numberOfConfirmedDtcs) {
	/* @req DEM352 */
	uint8 milStatus = 0;
	// According to req. DEM544, SAE J1979 and CARB OBD legislation,
	// this status should reflect if there is any confirmed DTC,
	// NOT if the MIL bulb is lit up, as it can be ON for different reasons as well (see SAE J1979).
	if (0u != numberOfConfirmedDtcs) {
		milStatus = 1;
	}

	return milStatus;
}

/**
 * Checks if the the event (test) has been marked as completed since last clear.
 * If the event (test) failed, it will be reported as incomplete.
 *
 * @param eventIndex: the event's current location in the event status buffer.
 * @return TRUE if test is complete, otherwise FALSE.
 */
static boolean isMonitoringNotCompleteSinceLastClear(uint16 eventIndex) {
	boolean testFailedSinceLastClear = (0 != (eventStatusBuffer[eventIndex].eventStatusExtended & DEM_TEST_FAILED_SINCE_LAST_CLEAR))? TRUE: FALSE;
	boolean monitoringNotComplete = (0 != (eventStatusBuffer[eventIndex].eventStatusExtended & DEM_TEST_NOT_COMPLETED_SINCE_LAST_CLEAR))? TRUE: FALSE;
	monitoringNotComplete |= testFailedSinceLastClear;
	return monitoringNotComplete;
}

#if (DEM_OBD_ENGINE_TYPE == DEM_IGNITION_SPARK)
/**
 * Service to report the value of PID $01 computed by the Dem.
 * Reentrant: Yes
 *
 * FOR SPARK ENGINE CONFIGURATION
 *
 * @param PID01value: Buffer containing the contents of PID $01 computed by the Dem.
 * @return Always E_OK is returned, as E_NOT_OK will never appear..
 */
Std_ReturnType Dem_DcmReadDataOfPID01(uint8* PID01value) {
	// Reset PID value
	PID01value[0] = 0; // Byte A
	PID01value[1] = 0; // Byte B
	PID01value[2] = 0; // Byte C
	PID01value[3] = 0; // Byte D

	/**
	 * Get number of confirmed OBD DTCs (Byte A)
	 */
	uint8_t confirmedDTCs = getNumberOfConfirmedObdDTCs();

	/**
	 * Get MIL status (Byte A)
	 */
	uint8_t milStatus = isObdMilOn(confirmedDTCs);

	/**
	 * Get engine systems' monitors availability (Byte B and C)
	 *
	 * Get engine systems' monitors readiness (Byte B and D)
	 */
	boolean ENG_TYPE  	= 0; // Compression ignition monitoring supported

	boolean MIS_SUP		= 0; // Misfire monitoring supported (All)
	boolean FUEL_SUP	= 0; // Fuel system monitoring supported (All)
	boolean CCM_SUP		= 0; // Comprehensive component monitoring supported (All)
	boolean CAT_SUP		= 0; // Catalyst monitoring supported (Gasoline)
	boolean HCAT_SUP	= 0; // Heated catalyst monitoring supported (Gasoline)
	boolean EVAP_SUP	= 0; // Evaporative system monitoring supported (Gasoline)
	boolean AIR_SUP		= 0; // Secondary air system monitoring supported (Gasoline)
	boolean ACRF_SUP	= 0; // A/C system refrigerant monitoring supported (Gasoline)
	boolean O2S_SUP		= 0; // Oxygen sensor monitoring supported (Gasoline)
	boolean HTR_SUP		= 0; // Oxygen sensor heater monitoring supported (Gasoline)
	boolean EGR_SUP		= 0; // EGR system monitoring supported (All)

	boolean MIS_RDY		= 0; // Misfire monitoring ready (All)
	boolean FUEL_RDY	= 0; // Fuel system monitoring ready (All)
	boolean CCM_RDY		= 0; // Comprehensive component monitoring ready (All)
	boolean CAT_RDY		= 0; // Catalyst monitoring ready (Gasoline)
	boolean HCAT_RDY	= 0; // Heated catalyst monitoring ready (Gasoline)
	boolean EVAP_RDY	= 0; // Evaporative system monitoring ready (Gasoline)
	boolean AIR_RDY		= 0; // Secondary air system monitoring ready (Gasoline)
	boolean ACRF_RDY	= 0; // A/C system refrigerant monitoring ready (Gasoline)
	boolean O2S_RDY		= 0; // Oxygen sensor monitoring ready (Gasoline)
	boolean HTR_RDY		= 0; // Oxygen sensor heater monitoring ready (Gasoline)
	boolean EGR_RDY		= 0; // EGR system monitoring ready (All)

	// Quoted from ASR 4.3:
	// According to SAEJ1979, the group AirCondition Component (ACRF) shall not be supported anymore.
	// However, it is still included in ISO 15031-5.

	/* @req DEM354 */
	for (uint16 i = 0; i < DEM_MAX_NUMBER_EVENT; i++) {
		if (DEM_EVENT_ID_NULL != eventStatusBuffer[i].eventId) {
			Dem_EventOBDReadinessGroup readinessGroup = eventStatusBuffer[i].eventParamRef->EventClass->OBDReadinessGroup;
			boolean monitoringNotComplete = isMonitoringNotCompleteSinceLastClear(i);

			switch(readinessGroup) {
				case DEM_OBD_RDY_MISF:
					MIS_SUP   = 1;
					MIS_RDY	  = 0; // Always complete
					break;
				case DEM_OBD_RDY_FLSYS:
					FUEL_SUP  = 1;
					FUEL_RDY  = 0; // Always complete
					break;
				case DEM_OBD_RDY_CMPRCMPT:
					CCM_SUP   = 1;
					CCM_RDY	  = 0; // Always complete
					break;
				case DEM_OBD_RDY_CAT:
					CAT_SUP   = 1;
					CAT_RDY  |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_HTCAT:
					HCAT_SUP  = 1;
					HCAT_RDY |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_EVAP:
					EVAP_SUP  = 1;
					EVAP_RDY |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_SECAIR:
					AIR_SUP   = 1;
					AIR_RDY  |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_AC:
					ACRF_SUP  = 1;
					ACRF_RDY |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_O2SENS:
					O2S_SUP   = 1;
					O2S_RDY  |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_O2SENSHT:
					HTR_SUP   = 1;
					HTR_RDY  |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_ERG:
					EGR_SUP   = 1;
					EGR_RDY  |= monitoringNotComplete;
					break;
			}
		}
	}

	/**
	 * Pack availability and readiness values
	 */
	// Byte A
	PID01value[0] = confirmedDTCs  | (milStatus << 6);
	// Byte B
	PID01value[1] = (uint8)((MIS_SUP << 0) | (FUEL_SUP << 1) | (CCM_SUP << 2)  | (ENG_TYPE << 3) | (MIS_RDY << 4)  | (FUEL_RDY << 5) | (CCM_RDY << 6)); // Bit 7 reserved
	// Byte C
	PID01value[2] =  (uint8)((CAT_SUP << 0) | (HCAT_SUP << 1) | (EVAP_SUP << 2) | (AIR_SUP << 3)  | (ACRF_SUP << 4) | (O2S_SUP << 5)  | (HTR_SUP << 6) | (EGR_SUP << 7));
	// Byte D
	PID01value[3] =  (uint8)((CAT_RDY << 0) | (HCAT_RDY << 1) | (EVAP_RDY << 2) | (AIR_RDY << 3)  | (ACRF_RDY << 4) | (O2S_RDY << 5)  | (HTR_RDY << 6) | (EGR_RDY << 7));

	return E_OK;
}
#elif (DEM_OBD_ENGINE_TYPE == DEM_IGNITION_COMPR)
/**
 * Service to report the value of PID $01 computed by the Dem.
 * Reentrant: Yes
 *
 * FOR COMPRESSION ENGINE CONFIGURATION
 *
 * @param PID01value: buffer containing the contents of PID $01 computed by the Dem.
 * @return Always E_OK is returned, as E_NOT_OK will never appear.
 */
Std_ReturnType Dem_DcmReadDataOfPID01(uint8* PID01value) {
	// Reset PID value
	PID01value[0] = 0; // Byte A
	PID01value[1] = 0; // Byte B
	PID01value[2] = 0; // Byte C
	PID01value[3] = 0; // Byte D

	/**
	 * Get number of confirmed OBD DTCs (Byte A)
	 */
	uint8_t confirmedDTCs = getNumberOfConfirmedObdDTCs();

	/**
	 * Get MIL status (Byte A)
	 */
	uint8_t milStatus = isObdMilOn(confirmedDTCs);

	/**
	 * Get engine systems' monitors availability (Byte B and C)
	 *
	 * Get engine systems' monitors readiness (Byte B and D)
	 */
	boolean ENG_TYPE  	= 0; // Compression ignition monitoring supported

	boolean MIS_SUP		= 0; // Misfire monitoring supported (All)
	boolean FUEL_SUP	= 0; // Fuel system monitoring supported (All)
	boolean CCM_SUP		= 0; // Comprehensive component monitoring supported (All)
	boolean HCCATSUP  	= 0; // NMHC catalyst monitoring supported (Diesel)
	boolean NCAT_SUP	= 0; // NOx aftertreatment monitoring supported (Diesel)
	boolean BP_SUP    	= 0; // Boost pressure system monitoring supported (Diesel)
	boolean EGS_SUP		= 0; // Exhaust gas sensor monitoring supported (Diesel)
	boolean PM_SUP		= 0; // PM Filter monitoring supported (Diesel)
	boolean EGR_SUP		= 0; // EGR system monitoring supported (All)

	boolean MIS_RDY		= 0; // Misfire monitoring ready (All)
	boolean FUEL_RDY	= 0; // Fuel system monitoring ready (All)
	boolean CCM_RDY		= 0; // Comprehensive component monitoring ready (All)
	boolean HCCATRDY  	= 0; // NMHC catalyst monitoring ready (Diesel)
	boolean NCAT_RDY	= 0; // NOx aftertreatment monitoring ready (Diesel)
	boolean BP_RDY    	= 0; // Boost pressure system monitoring ready (Diesel)
	boolean EGS_RDY		= 0; // Exhaust gas sensor monitoring ready (Diesel)
	boolean PM_RDY		= 0; // PM Filter monitoring ready (Diesel)
	boolean EGR_RDY		= 0; // EGR system monitoring ready (All)

	ENG_TYPE = 1;

	/* @req DEM354 */
	for (uint16 i = 0; i < DEM_MAX_NUMBER_EVENT; i++) {
		if (DEM_EVENT_ID_NULL != eventStatusBuffer[i].eventId) {
			Dem_EventOBDReadinessGroup readinessGroup = eventStatusBuffer[i].eventParamRef->EventClass->OBDReadinessGroup;
			boolean monitoringNotComplete = isMonitoringNotCompleteSinceLastClear(i);

			switch(readinessGroup) {
				case DEM_OBD_RDY_MISF:
					MIS_SUP   = 1;
					MIS_RDY	 |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_FLSYS:
					FUEL_SUP  = 1;
					FUEL_RDY |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_CMPRCMPT:
					CCM_SUP   = 1;
					CCM_RDY	  = 0;
					break;
				case DEM_OBD_RDY_HCCAT:
					HCCATSUP  = 1;
					HCCATRDY |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_NOXCAT:
					NCAT_SUP  = 1;
					NCAT_RDY |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_BOOSTPR:
					BP_SUP    = 1;
					BP_RDY   |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_EGSENS:
					EGS_SUP   = 1;
					EGS_RDY  |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_PMFLT:
					PM_SUP    = 1;
					PM_RDY   |= monitoringNotComplete;
					break;
				case DEM_OBD_RDY_ERG:
					EGR_SUP   = 1;
					EGR_RDY  |= monitoringNotComplete;
					break;
			}
		}
	}

	/**
	 * Pack availability and readiness values
	 */
	// Byte A
	PID01value[0] = ((uint8) confirmedDTCs) | (milStatus << 6);
	// Byte B
	PID01value[1] = (MIS_SUP << 0)  | (FUEL_SUP << 1) | (CCM_SUP << 2) | (ENG_TYPE << 3) | (MIS_RDY << 4) | (FUEL_RDY << 5) | (CCM_RDY << 6); // Bit 7 reserved
	// Byte C
	PID01value[2] = (HCCATSUP << 0) | (NCAT_SUP << 1) | (BP_SUP << 3)  | (EGS_SUP << 5)  | (PM_SUP << 6)  | (EGR_SUP << 7); // bit 2 reserved // bit 4 reserved
	// Byte D
	PID01value[3] = (HCCATRDY << 0) | (NCAT_RDY << 1) | (BP_RDY << 3)  | (EGS_RDY << 5)  | (PM_RDY << 6)  | (EGR_RDY << 7); // bit 2 reserved // bit 4 reserved

	return E_OK;
}
#endif

/**
 * Checks whether the event (test) has been marked as completed in current driving cycle.
 * If the event (test) failed, it will be reported as incomplete.
 *
 * @param eventIndex: the event's current location in the event status buffer.
 * @return TRUE if test is complete, otherwise FALSE.
 */
static DEM_TRISTATE isMonitoringNotCompleteThisDrivingCycle(uint16 eventIndex) {
	DEM_TRISTATE testFailedThisOperationCycle = (0 != (eventStatusBuffer[eventIndex].eventStatusExtended & DEM_TEST_FAILED_THIS_OPERATION_CYCLE))? 1: 0;
	DEM_TRISTATE monitoringNotComplete = (0 != (eventStatusBuffer[eventIndex].eventStatusExtended & DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE))? 1: 0;
	monitoringNotComplete |= testFailedThisOperationCycle;

	DEM_TRISTATE isEnabledThisDrivingCycle = (FALSE == eventStatusBuffer[eventIndex].isDisabled)? 1: 0;
	// If the monitor for the component is disabled for this driving cycle then
	// set all the relevant tests to complete
	if (isEnabledThisDrivingCycle == DEM_T_FALSE) {
		monitoringNotComplete = DEM_T_FALSE;
	}

	return monitoringNotComplete;
}

/**
 * Sets all NULL values to FALSE.
 *
 * @param total: number of values to check.
 * @param monitorValues: monitor values to reset or keep.
 */
static void resetOrKeepMonitorValues(uint8_t total, DEM_TRISTATE **monitorValues) {
	for (uint8_t i = 0; i < total; i++)  {
		*monitorValues[i] = (*monitorValues[i] == DEM_T_NULL) ? DEM_T_FALSE : *monitorValues[i];
	}
}

#if (DEM_OBD_ENGINE_TYPE == DEM_IGNITION_SPARK)
/**
 * Service to report the value of PID $41 computed by the Dem.
 * Reentrant: Yes
 *
 * FOR SPARK ENGINE CONFIGURATION
 *
 * @param PID41value: buffer containing the contents of PID $41 computed by the Dem.
 * @return Always E_OK is returned, as E_NOT_OK will never appear.
 */
Std_ReturnType Dem_DcmReadDataOfPID41(uint8* PID41value) {
	// Reset PID value
	PID41value[0] = 0; // Byte A
	PID41value[1] = 0; // Byte B
	PID41value[2] = 0; // Byte C
	PID41value[3] = 0; // Byte D

	/**
	 * Get number of confirmed OBD DTCs (Byte A)
	 */
	// Does not report the number of confirmed DTCs, keep at 0

	/**
	 * Get MIL status (Byte A)
	 */
	// Does not report the MIL status, keep at 0

	/**
	 * Get engine systems' monitors status (Byte B and C)
	 *
	 * Get engine systems' monitors completion (Byte B and D)
	 */
	DEM_TRISTATE  ENG_TYPE  = DEM_T_FALSE; // Compression ignition monitoring supported

	DEM_TRISTATE  MIS_ENA	= DEM_T_NULL; // Misfire monitoring enabled (All)
	DEM_TRISTATE  FUEL_ENA	= DEM_T_NULL; // Fuel system monitoring enabled (All)
	DEM_TRISTATE  CCM_ENA	= DEM_T_NULL; // Comprehensive component monitoring enabled (All)
	DEM_TRISTATE  CAT_ENA	= DEM_T_NULL; // Catalyst monitoring enabled (Gasoline)
	DEM_TRISTATE  HCAT_ENA	= DEM_T_NULL; // Heated catalyst monitoring enabled (Gasoline)
	DEM_TRISTATE  EVAP_ENA	= DEM_T_NULL; // Evaporative system monitoring enabled (Gasoline)
	DEM_TRISTATE  AIR_ENA	= DEM_T_NULL; // Secondary air system monitoring enabled (Gasoline)
	DEM_TRISTATE  ACRF_ENA	= DEM_T_NULL; // A/C system refrigerant monitoring enabled (Gasoline)
	DEM_TRISTATE  O2S_ENA	= DEM_T_NULL; // Oxygen sensor monitoring enabled (Gasoline)
	DEM_TRISTATE  HTR_ENA	= DEM_T_NULL; // Oxygen sensor heater monitoring enabled (Gasoline)
	DEM_TRISTATE  EGR_ENA	= DEM_T_NULL; // EGR system monitoring enabled (All)

	DEM_TRISTATE  MIS_CMPL	= DEM_T_NULL; // Misfire monitoring complete (All)
	DEM_TRISTATE  FUELCMPL	= DEM_T_NULL; // Fuel system monitoring complete (All)
	DEM_TRISTATE  CCM_CMPL	= DEM_T_NULL; // Comprehensive component monitoring complete (All)
	DEM_TRISTATE  CAT_CMPL	= DEM_T_NULL; // Catalyst monitoring complete (Gasoline)
	DEM_TRISTATE  HCATCMPL	= DEM_T_NULL; // Heated catalyst monitoring complete (Gasoline)
	DEM_TRISTATE  EVAPCMPL	= DEM_T_NULL; // Evaporative system monitoring complete (Gasoline)
	DEM_TRISTATE  AIR_CMPL	= DEM_T_NULL; // Secondary air system monitoring complete (Gasoline)
	DEM_TRISTATE  ACRFCMPL	= DEM_T_NULL; // A/C system refrigerant monitoring complete (Gasoline)
	DEM_TRISTATE  O2S_CMPL	= DEM_T_NULL; // Oxygen sensor monitoring complete (Gasoline)
	DEM_TRISTATE  HTR_CMPL	= DEM_T_NULL; // Oxygen sensor heater monitoring complete (Gasoline)
	DEM_TRISTATE  EGR_CMPL	= DEM_T_NULL; // EGR system monitoring complete (All)

	// Quoted from ASR 4.3:
	// According to SAEJ1979, the group AirCondition Component shall not be supported anymore.
	// However, it is still included in ISO 15031-5.

	/* @req DEM355 */
	for (uint16 i = 0u; i < DEM_MAX_NUMBER_EVENT; i++){
		if (DEM_EVENT_ID_NULL != eventStatusBuffer[i].eventId) {
			Dem_EventOBDReadinessGroup readinessGroup = eventStatusBuffer[i].eventParamRef->EventClass->OBDReadinessGroup;

			DEM_TRISTATE monitoringNotComplete = isMonitoringNotCompleteThisDrivingCycle(i);
			DEM_TRISTATE isEnabledThisDrivingCycle = (FALSE == eventStatusBuffer[i].isDisabled)? 1 : 0;

			switch(readinessGroup) {
				case DEM_OBD_RDY_MISF:
					MIS_ENA  = (MIS_ENA   == DEM_T_NULL) ? isEnabledThisDrivingCycle  : (MIS_ENA & isEnabledThisDrivingCycle);
					MIS_CMPL = DEM_T_FALSE; // Always on

					break;
				case DEM_OBD_RDY_FLSYS:
					FUEL_ENA  = (FUEL_ENA == DEM_T_NULL) ? isEnabledThisDrivingCycle  : (FUEL_ENA  & isEnabledThisDrivingCycle);
					FUELCMPL = DEM_T_FALSE; // Always on

					break;
				case DEM_OBD_RDY_CMPRCMPT:
					CCM_ENA  = (CCM_ENA   == DEM_T_NULL) ? isEnabledThisDrivingCycle  : (CCM_ENA  & isEnabledThisDrivingCycle);
					CCM_CMPL = DEM_T_FALSE; // Always on

					break;
				case DEM_OBD_RDY_CAT:
					CAT_ENA  = (CAT_ENA   == DEM_T_NULL) ? isEnabledThisDrivingCycle  : (CAT_ENA  & isEnabledThisDrivingCycle);
					CAT_CMPL = (CAT_CMPL  == DEM_T_NULL) ? monitoringNotComplete      : (CAT_CMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_HTCAT:
					HCAT_ENA = (HCAT_ENA  == DEM_T_NULL) ? isEnabledThisDrivingCycle  : (HCAT_ENA & isEnabledThisDrivingCycle);
					HCATCMPL = (HCATCMPL  == DEM_T_NULL) ? monitoringNotComplete      : (HCATCMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_EVAP:
					EVAP_ENA = (EVAP_ENA  == DEM_T_NULL) ? isEnabledThisDrivingCycle  : (EVAP_ENA & isEnabledThisDrivingCycle);
					EVAPCMPL = (EVAPCMPL  == DEM_T_NULL) ? monitoringNotComplete      : (EVAPCMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_SECAIR:
					AIR_ENA  = (AIR_ENA   == DEM_T_NULL) ? isEnabledThisDrivingCycle  : (AIR_ENA  & isEnabledThisDrivingCycle);
					AIR_CMPL = (AIR_CMPL  == DEM_T_NULL) ? monitoringNotComplete	  : (AIR_CMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_AC:
					ACRF_ENA = (ACRF_ENA  == DEM_T_NULL) ? isEnabledThisDrivingCycle  : (ACRF_ENA & isEnabledThisDrivingCycle);
					ACRFCMPL = (ACRFCMPL  == DEM_T_NULL) ? monitoringNotComplete 	  : (ACRFCMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_O2SENS:
					O2S_ENA  = (O2S_ENA   == DEM_T_NULL) ? isEnabledThisDrivingCycle  : (O2S_ENA  & isEnabledThisDrivingCycle);
					O2S_CMPL = (O2S_CMPL  == DEM_T_NULL) ? monitoringNotComplete	  : (O2S_CMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_O2SENSHT:
					HTR_ENA  = (HTR_ENA   == DEM_T_NULL) ? isEnabledThisDrivingCycle  : (HTR_ENA  & isEnabledThisDrivingCycle);
					HTR_CMPL = (HTR_CMPL  == DEM_T_NULL) ? monitoringNotComplete      : (HTR_CMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_ERG:
					EGR_ENA  = (EGR_ENA   == DEM_T_NULL) ? isEnabledThisDrivingCycle  : (EGR_ENA  & isEnabledThisDrivingCycle);
					EGR_CMPL = (EGR_CMPL  == DEM_T_NULL) ? monitoringNotComplete      : (EGR_CMPL & monitoringNotComplete);

					break;
			}
		}
	}

	/**
	 * Set remaining NULL systems to FALSE
	 */
	DEM_TRISTATE *monitorsEnabled[]  = {&MIS_ENA, &FUEL_ENA, &CCM_ENA, &CAT_ENA, &HCAT_ENA, &EVAP_ENA, &AIR_ENA, &ACRF_ENA, &O2S_ENA, &HTR_ENA, &EGR_ENA};
	resetOrKeepMonitorValues(11, monitorsEnabled);

	DEM_TRISTATE *monitorsComplete[] = {&MIS_CMPL, &FUELCMPL, &CCM_CMPL, &CAT_CMPL, &HCATCMPL, &EVAPCMPL, &AIR_CMPL, &ACRFCMPL, &O2S_CMPL, &HTR_CMPL, &EGR_CMPL};
	resetOrKeepMonitorValues(11, monitorsComplete);

	/**
	 * Pack status and completion values
	 */
	// Byte A
	// Both 0, does not report
	// Byte B
	PID41value[1] = (uint8)((MIS_ENA << 0)  | (FUEL_ENA << 1) | (CCM_ENA << 2)  | (ENG_TYPE << 3) | (MIS_CMPL << 4) | (FUELCMPL << 5) | (CCM_CMPL << 6)); // Bit 7 reserved
	// Byte C
	PID41value[2] = (uint8)((CAT_ENA << 0)  | (HCAT_ENA << 1) | (EVAP_ENA << 2) | (AIR_ENA << 3)  | (ACRF_ENA << 4) | (O2S_ENA << 5)  | (HTR_ENA << 6) | (EGR_ENA << 7));
	// Byte D
	PID41value[3] = (uint8)((CAT_CMPL << 0) | (HCATCMPL << 1) | (EVAPCMPL << 2) | (AIR_CMPL << 3) | (ACRFCMPL << 4) | (O2S_CMPL << 5) | (HTR_CMPL << 6) | (EGR_CMPL << 7));

	return E_OK;
}
#elif (DEM_OBD_ENGINE_TYPE == DEM_IGNITION_COMPR)
/**
 * Service to report the value of PID $41 computed by the Dem.
 * Reentrant: Yes
 *
 * FOR COMPRESSION ENGINE CONFIGURATION
 *
 * @param PID41value: buffer containing the contents of PID $41 computed by the Dem.
 * @return Always E_OK is returned, as E_NOT_OK will never appear.
 */
Std_ReturnType Dem_DcmReadDataOfPID41(uint8* PID41value) {
	// Reset PID value
	PID41value[0] = 0; // Byte A
	PID41value[1] = 0; // Byte B
	PID41value[2] = 0; // Byte C
	PID41value[3] = 0; // Byte D

	/**
	 * Get number of confirmed OBD DTCs (Byte A)
	 */
	// Does not report the number of confirmed DTCs, keep at 0

	/**
	 * Get MIL status (Byte A)
	 */
	// Does not report the MIL status, keep at 0

	/**
	 * Get engine systems' monitors status (Byte B and C)
	 *
	 * Get engine systems' monitors completion (Byte B and D)
	 */
	DEM_TRISTATE ENG_TYPE  	= DEM_T_FALSE; // Compression ignition monitoring supported

	DEM_TRISTATE MIS_ENA	= DEM_T_NULL; // Misfire monitoring enabled (All)
	DEM_TRISTATE FUEL_ENA	= DEM_T_NULL; // Fuel system monitoring enabled (All)
	DEM_TRISTATE CCM_ENA	= DEM_T_NULL; // Comprehensive component monitoring enabled (All)
	DEM_TRISTATE HCCATENA 	= DEM_T_NULL; // NMHC catalyst monitoring enabled (Diesel)
	DEM_TRISTATE NCAT_ENA	= DEM_T_NULL; // NOx aftertreatment monitoring enabled (Diesel)
	DEM_TRISTATE BP_ENA    	= DEM_T_NULL; // Boost pressure system monitoring enabled (Diesel)
	DEM_TRISTATE EGS_ENA	= DEM_T_NULL; // Exhaust gas sensor monitoring enabled (Diesel)
	DEM_TRISTATE PM_ENA		= DEM_T_NULL; // PM Filter monitoring enabled (Diesel)
	DEM_TRISTATE EGR_ENA	= DEM_T_NULL; // EGR system monitoring enabled (All)

	DEM_TRISTATE MIS_CMPL	= DEM_T_NULL; // Misfire monitoring complete (All)
	DEM_TRISTATE FUELCMPL	= DEM_T_NULL; // Fuel system monitoring complete (All)
	DEM_TRISTATE CCM_CMPL	= DEM_T_NULL; // Comprehensive component monitoring complete (All)
	DEM_TRISTATE HCCATCMP  	= DEM_T_NULL; // NMHC catalyst monitoring complete (Diesel)
	DEM_TRISTATE NCATCMPL	= DEM_T_NULL; // NOx aftertreatment monitoring complete (Diesel)
	DEM_TRISTATE BP_CMPL   	= DEM_T_NULL; // Boost pressure system monitoring complete (Diesel)
	DEM_TRISTATE EGS_CMPL	= DEM_T_NULL; // Exhaust gas sensor monitoring complete (Diesel)
	DEM_TRISTATE PM_CMPL	= DEM_T_NULL; // PM Filter monitoring complete (Diesel)
	DEM_TRISTATE EGR_CMPL	= DEM_T_NULL; // EGR system monitoring complete (All)

	ENG_TYPE = 1;

	/* @req DEM355 */
	for (uint16 i = 0; i < DEM_MAX_NUMBER_EVENT; i++) {
		if (DEM_EVENT_ID_NULL != eventStatusBuffer[i].eventId) {
			Dem_EventOBDReadinessGroup readinessGroup = eventStatusBuffer[i].eventParamRef->EventClass->OBDReadinessGroup;

			DEM_TRISTATE monitoringNotComplete = isMonitoringNotCompleteThisDrivingCycle(i);
			DEM_TRISTATE isEnabledThisDrivingCycle = (! eventStatusBuffer[i].isDisabled);

			switch(readinessGroup) {
				case DEM_OBD_RDY_MISF:
					MIS_ENA   = (MIS_ENA   == DEM_T_NULL) ? isEnabledThisDrivingCycle   : (MIS_ENA  & isEnabledThisDrivingCycle);
					MIS_CMPL  = (MIS_CMPL  == DEM_T_NULL) ? monitoringNotComplete       : (MIS_CMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_FLSYS:
					FUEL_ENA  = (FUEL_ENA  == DEM_T_NULL) ? isEnabledThisDrivingCycle   : (FUEL_ENA & isEnabledThisDrivingCycle);
					FUELCMPL  = (FUELCMPL  == DEM_T_NULL) ? monitoringNotComplete       : (FUELCMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_CMPRCMPT:
					CCM_ENA  = (CCM_ENA == DEM_T_NULL)    ? isEnabledThisDrivingCycle   : (CCM_ENA  & isEnabledThisDrivingCycle);
					CCM_CMPL = DEM_T_FALSE; // Always on

					break;
				case DEM_OBD_RDY_HCCAT:
					HCCATENA  = (HCCATENA  == DEM_T_NULL) ? isEnabledThisDrivingCycle   : (HCCATENA & isEnabledThisDrivingCycle);
					HCCATCMP  = (HCCATCMP  == DEM_T_NULL) ? monitoringNotComplete       : (HCCATCMP & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_NOXCAT:
					NCAT_ENA  = (NCAT_ENA  == DEM_T_NULL) ? isEnabledThisDrivingCycle   : (NCAT_ENA & isEnabledThisDrivingCycle);
					NCATCMPL  = (NCATCMPL  == DEM_T_NULL) ? monitoringNotComplete       : (NCATCMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_BOOSTPR:
					BP_ENA    = (BP_ENA  == DEM_T_NULL)   ? isEnabledThisDrivingCycle   : (BP_ENA & isEnabledThisDrivingCycle);
					BP_CMPL   = (BP_CMPL == DEM_T_NULL)   ? monitoringNotComplete       : (BP_CMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_EGSENS:
					EGS_ENA   = (EGS_ENA  == DEM_T_NULL)  ? isEnabledThisDrivingCycle   : (EGS_ENA & isEnabledThisDrivingCycle);
					EGS_CMPL  = (EGS_CMPL  == DEM_T_NULL) ? monitoringNotComplete       : (EGS_CMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_PMFLT:
					PM_ENA    = (PM_ENA  == DEM_T_NULL)   ? isEnabledThisDrivingCycle   : (PM_ENA & isEnabledThisDrivingCycle);
					PM_CMPL   = (PM_CMPL == DEM_T_NULL)   ? monitoringNotComplete       : (PM_CMPL & monitoringNotComplete);

					break;
				case DEM_OBD_RDY_ERG:
					EGR_ENA   = (EGR_ENA  == DEM_T_NULL)  ? isEnabledThisDrivingCycle   : (EGR_ENA & isEnabledThisDrivingCycle);
					EGR_CMPL  = (EGR_CMPL == DEM_T_NULL)  ? monitoringNotComplete       : (EGR_CMPL & monitoringNotComplete);

					break;
			}
		}
	}

	/**
	 * Set remaining NULL systems to FALSE
	 */
	DEM_TRISTATE *monitorsEnabled[]  = {&MIS_ENA, &FUEL_ENA, &CCM_ENA, &HCCATENA, &NCAT_ENA, &BP_ENA, &EGS_ENA, &PM_ENA, &EGR_ENA};
	resetOrKeepMonitorValues(9, monitorsEnabled);

	DEM_TRISTATE *monitorsComplete[] = {&MIS_CMPL, &FUELCMPL, &CCM_CMPL, &HCCATCMP, &NCATCMPL, &BP_CMPL, &EGS_CMPL, &PM_CMPL, &EGR_CMPL};
	resetOrKeepMonitorValues(9, monitorsComplete);

	/**
	 * Pack status and completion values
	 */
	// Byte A
	// Both 0, does not report
	// Byte B
	PID41value[1] = (MIS_ENA << 0)  | (FUEL_ENA << 1) | (CCM_ENA << 2) | (ENG_TYPE << 3) | (MIS_CMPL << 4) | (FUELCMPL << 5) | (CCM_CMPL << 6); // Bit 7 reserved
	// Byte C
	PID41value[2] = (HCCATENA << 0) | (NCAT_ENA << 1) | (BP_ENA << 3)  | (EGS_ENA << 5)  | (PM_ENA << 6)   | (EGR_ENA << 7); // bit 2 reserved // bit 4 reserved
	// Byte D
	PID41value[3] = (HCCATCMP << 0) | (NCATCMPL << 1) | (BP_CMPL << 3) | (EGS_CMPL << 5) | (PM_CMPL << 6)  | (EGR_CMPL << 7); // bit 2 reserved // bit 4 reserved

	return E_OK;
}
#endif

#if defined(DEM_USE_IUMPR)
/**
 * Writes current denominator and numerator of a specific ratio to the given data buffer.
 *
 * @param dataBuffer
 * @param startIndex
 * @param ratioId
 */
static void getNumeratorDenominator(uint8* dataBuffer, uint8 startIndex, Dem_RatioIdType ratioId) {
	uint8 currentIndex = startIndex;

	// write numerator
	dataBuffer[currentIndex++] = (uint8)(iumprBufferLocal[ratioId].numerator.value   >> 8);
	dataBuffer[currentIndex++] = (uint8)(iumprBufferLocal[ratioId].numerator.value   & 0xFF);
	// write denominator
	dataBuffer[currentIndex++] = (uint8)(iumprBufferLocal[ratioId].denominator.value >> 8);
	dataBuffer[currentIndex]   = (uint8)(iumprBufferLocal[ratioId].denominator.value & 0xFF);
}

/**
 * Writes general denominator count to data buffer
 *
 * @param dataBuffer
 */
static void getGeneralDenominatorCount(uint8* dataBuffer) {
	dataBuffer[0] = (uint8)(generalDenominatorBuffer.value >> 8);
	dataBuffer[1] = (uint8)(generalDenominatorBuffer.value & 0xFF);
}

/**
 * Writes ignition cycle count to data buffer
 *
 * @param dataBuffer
 */
static void getIgnitionCycleCount(uint8* dataBuffer) {
	dataBuffer[2] = (uint8)(ignitionCycleCountBuffer >> 8);
	dataBuffer[3] = (uint8)(ignitionCycleCountBuffer & 0xFF);
}

#if (DEM_OBD_ENGINE_TYPE == DEM_IGNITION_SPARK) /* @req DEM357 */
/**
 * Writes denominator and numerator counts to InfoType0 data buffer depending on IUMPR group
 *
 * @param iumprGroup
 * @param Iumprdata08
 */
static void setInfoType08NumsDenoms(Dem_IUMPRGroup iumprGroup, uint8* Iumprdata08, Dem_RatioIdType ratioId) {
	switch(iumprGroup) {
		case DEM_IUMPR_CAT1:
			getNumeratorDenominator(Iumprdata08, 4, ratioId);
			break;
		case DEM_IUMPR_CAT2:
			getNumeratorDenominator(Iumprdata08, 8, ratioId);
			break;
		case DEM_IUMPR_OXS1:
			getNumeratorDenominator(Iumprdata08, 12, ratioId);
			break;
		case DEM_IUMPR_OXS2:
			getNumeratorDenominator(Iumprdata08, 16, ratioId);
			break;
		case DEM_IUMPR_EGR:
			getNumeratorDenominator(Iumprdata08, 20, ratioId);
			break;
		case DEM_IUMPR_SAIR:
			getNumeratorDenominator(Iumprdata08, 24, ratioId);
			break;
		case DEM_IUMPR_EVAP:
			getNumeratorDenominator(Iumprdata08, 28, ratioId);
			break;
		case DEM_IUMPR_SECOXS1:
			getNumeratorDenominator(Iumprdata08, 32, ratioId);
			break;
		case DEM_IUMPR_SECOXS2:
			getNumeratorDenominator(Iumprdata08, 36, ratioId);
			break;
		case DEM_IUMPR_AFRI1:
			getNumeratorDenominator(Iumprdata08, 40, ratioId);
			break;
		case DEM_IUMPR_AFRI2:
			getNumeratorDenominator(Iumprdata08, 44, ratioId);
			break;
		case DEM_IUMPR_PF1:
			getNumeratorDenominator(Iumprdata08, 48, ratioId);
			break;
		case DEM_IUMPR_PF2:
			getNumeratorDenominator(Iumprdata08, 52, ratioId);
			break;
	}
}
#endif /* DEM_USE_IUMPR */
/**
 * Service is used to request for IUMPR data according to InfoType $08.
 * Reentrant: Yes
 *
 * @param Iumprdata08: Buffer containing the contents of InfoType $08.
 * The buffer is provided by the Dcm.
 * @return Always E_OK is returned, as E_PENDING and E_NOT_OK will never appear.
 */
Std_ReturnType Dem_GetInfoTypeValue08(uint8* Iumprdata08)
{
	/* @req DEM316 */
	/* @req DEM298 */
	if (Iumprdata08 != NULL_PTR && demState == DEM_INITIALIZED) {
		// reset data
		memset(Iumprdata08, 0, 56);

		// get general denominator count
		getGeneralDenominatorCount(Iumprdata08);

		// get ignition cycle count
		getIgnitionCycleCount(Iumprdata08);

		for (Dem_RatioIdType ratioId = 0; ratioId < DEM_IUMPR_REGISTERED_COUNT; ratioId++) {
			Dem_IUMPRGroup iumprGroup = Dem_RatiosList[ratioId].IumprGroup;

			setInfoType08NumsDenoms(iumprGroup, Iumprdata08, ratioId);
		}
	}

	return E_OK;
}
#elif (DEM_OBD_ENGINE_TYPE == DEM_IGNITION_COMPR) /* @req DEM358 */
static void setInfoType0BNumsDenoms(Dem_IUMPRGroup iumprGroup, uint8* Iumprdata0B, Dem_RatioIdType ratioId) {
	switch(iumprGroup) {
		case DEM_IUMPR_NMHCCAT:
			getNumeratorDenominator(Iumprdata0B, 4, ratioId);
			break;
		case DEM_IUMPR_NOXCAT:
			getNumeratorDenominator(Iumprdata0B, 8, ratioId);
			break;
		case DEM_IUMPR_NOXADSORB:
			getNumeratorDenominator(Iumprdata0B, 12, ratioId);
			break;
		case DEM_IUMPR_PMFILTER:
			getNumeratorDenominator(Iumprdata0B, 16, ratioId);
			break;
		case DEM_IUMPR_EGSENSOR:
			getNumeratorDenominator(Iumprdata0B, 20, ratioId);
			break;
		case DEM_IUMPR_EGR:
			getNumeratorDenominator(Iumprdata0B, 24, ratioId);
			break;
		case DEM_IUMPR_BOOSTPRS:
			getNumeratorDenominator(Iumprdata0B, 28, ratioId);
			break;
		case DEM_IUMPR_FLSYS:
			getNumeratorDenominator(Iumprdata0B, 32, ratioId);
			break;
	}
}

/**
 * Service is used to request for IUMPR data according to InfoType $0B.
 * Reentrant: Yes
 *
 * @param Iumprdata08: Buffer containing the contents of InfoType $0B.
 * The buffer is provided by the Dcm.
 * @return Always E_OK is returned, as E_PENDING and E_NOT_OK will never appear.
 */
Std_ReturnType Dem_GetInfoTypeValue0B(uint8* Iumprdata0B)
{
	/* @req DEM317 */
	/* @req DEM298 */
	if ((demState == DEM_INITIALIZED) && (Iumprdata0B != NULL_PTR)) {
		// reset data
		memset(Iumprdata0B, 0, 36);

		// get general denominator count
		getGeneralDenominatorCount(Iumprdata0B);

		// get ignition cycle count
		getIgnitionCycleCount(Iumprdata0B);

		for (Dem_RatioIdType ratioId = 0; ratioId < DEM_IUMPR_REGISTERED_COUNT; ratioId++) {
			Dem_IUMPRGroup iumprGroup = Dem_RatiosList[ratioId].IumprGroup;

			setInfoType0BNumsDenoms(iumprGroup, Iumprdata0B, ratioId);
		}
	}

	return E_OK;
}
#endif

/**
 * Service for reporting that faults are possibly found because all conditions are fulfilled.
 * Reentrant: No
 *
 * @param RatioID: Ratio Identifier reporting that a respective monitor could have
 * found a fault - only used when interface option "API" is selected.
 * @return E_OK report of IUMPR result was successfully reported.
 */
Std_ReturnType Dem_RepIUMPRFaultDetect(Dem_RatioIdType RatioID)
{
	VALIDATE_RV(DEM_INITIALIZED == demState, DEM_REPIUMPRFAULTDETECT_ID, DEM_E_UNINIT, E_NOT_OK);
#if defined(DEM_USE_IUMPR)
	// valid RatioID
	VALIDATE_RV(RatioID < DEM_IUMPR_REGISTERED_COUNT, DEM_REPIUMPRFAULTDETECT_ID, DEM_E_PARAM_DATA, E_NOT_OK);

	/* @req DEM313 */
	/* @req DEM360 */
	Std_ReturnType ret = E_NOT_OK;

	SchM_Enter_Dem_EA_0();
	/* @req DEM296 */
	if (Dem_RatiosList[RatioID].RatioKind == DEM_RATIO_API) { // is API call
		ret = incrementIumprNumerator(RatioID);
	}
	SchM_Exit_Dem_EA_0();

	return ret;
#else
	(void)RatioID;
	return E_NOT_OK;
#endif
}

/**
 * Service is used to lock a denominator of a specific monitor.
 * Reentrant: Yes
 *
 * @param RatioID: Ratio Identifier reporting that specific denominator is locked (for
 * physical reasons - e.g. temperature conditions or minimum activity).
 * @return  E_OK report of IUMPR denominator status was successfully reported.
 * E_NOT_OK report of IUMPR denominator status was not successfully reported.
 */
Std_ReturnType Dem_RepIUMPRDenLock(Dem_RatioIdType RatioID)
{
	Std_ReturnType ret = E_NOT_OK;

	VALIDATE_RV(DEM_INITIALIZED == demState, DEM_REPIUMPRFAULTDETECT_ID, DEM_E_UNINIT, E_NOT_OK);
#if defined(DEM_USE_IUMPR)
	// valid RatioID
	VALIDATE_RV(RatioID < DEM_IUMPR_REGISTERED_COUNT, DEM_REPIUMPRFAULTDETECT_ID, DEM_E_PARAM_DATA, E_NOT_OK);


	/* @req DEM314 */
	/* @req DEM362 */
	/* @req DEM297 */
	if (FALSE == iumprBufferLocal[RatioID].denominator.isLocked) {
		iumprBufferLocal[RatioID].denominator.isLocked = TRUE;

		ret = E_OK;
	}
#else
	(void)RatioID;
#endif
	return ret;
}

/**
 * Service is used to release a denominator of a specific monitor.
 * Reentrant: Yes
 *
 * @param RatioID: Ratio Identifier reporting that specific denominator is released
 * (for physical reasons - e.g. temperature conditions or minimum activity).
 * @return  E_OK report of IUMPR denominator status was successfully reported.
 * E_NOT_OK report of IUMPR denominator status was not successfully reported.
 */
Std_ReturnType Dem_RepIUMPRDenRelease(Dem_RatioIdType RatioID)
{
	Std_ReturnType ret = E_NOT_OK;

	VALIDATE_RV(DEM_INITIALIZED == demState, DEM_REPIUMPRFAULTDETECT_ID, DEM_E_UNINIT, E_NOT_OK);
#if defined(DEM_USE_IUMPR)
	// valid RatioID
	VALIDATE_RV(RatioID < DEM_IUMPR_REGISTERED_COUNT, DEM_REPIUMPRFAULTDETECT_ID, DEM_E_PARAM_DATA, E_NOT_OK);

	/* @req Dem315 */
	/* @req Dem362 */
	/* @req Dem308 */
	if (TRUE == iumprBufferLocal[RatioID].denominator.isLocked) {
		iumprBufferLocal[RatioID].denominator.isLocked = FALSE;

		// consider this as qualified driving cycle (if applicable), increment denominator immediately
		(void) incrementIumprDenominator(RatioID);

		ret = E_OK;
	}
#else
	(void)RatioID;
#endif
	return ret;
}

#if defined(DEM_USE_IUMPR)
/**
 * Increment general denominator if set condition is general denominator and status is reached.
 *
 * @param ConditionId: Identification of a IUMPR denominator condition ID (General Denominator, Cold start, EVAP, 500mi).
 * @param ConditionStatus: Status of the IUMPR denominator condition (Notreached, reached, not reachable / inhibited).
 */
static void incrementGeneralDenominator(Dem_IumprDenomCondIdType ConditionId, Dem_IumprDenomCondStatusType ConditionStatus) {
	if (ConditionId == DEM_IUMPR_GENERAL_OBDCOND && ConditionStatus == DEM_IUMPR_DEN_STATUS_REACHED) {
		if (FALSE == generalDenominatorBuffer.incrementedThisDrivingCycle) {
			// If driving cycle started
			if (TRUE == operationCycleIsStarted(DEM_OBD_DCY)) {
				// If the general denominator reaches the maximum value of 65,535 ±2, the
				// general denominator shall rollover and increment to zero on the next
				// driving cycle that meets the general denominator definition to avoid
				// overflow problems.
				if (generalDenominatorBuffer.value == 65535) {
					generalDenominatorBuffer.value = 0;
				} else {
					generalDenominatorBuffer.value++;
				}

				generalDenominatorBuffer.incrementedThisDrivingCycle = TRUE;
			}
		}
	}
}
#endif

/**
 * In order to communicate the status of the (additional) denominator conditions among the OBD relevant ECUs,
 * the API is used to forward the condition status to a Dem of a particular ECU.
 * API is needed in OBD-relevant ECUs only.
 *
 * @param ConditionId: Identification of a IUMPR denominator condition ID (General Denominator, Cold start, EVAP, 500mi).
 * @param ConditionStatus: Status of the IUMPR denominator condition (Notreached, reached, not reachable / inhibited).
 * @return E_OK: set of IUMPR denominator condition was successful,
 * E_NOT_OK: set of IUMPR denominator condition failed or could not be accepted.
 */
Std_ReturnType Dem_SetIUMPRDenCondition(Dem_IumprDenomCondIdType ConditionId, Dem_IumprDenomCondStatusType ConditionStatus) {
	Std_ReturnType ret = E_NOT_OK;

	// dem started
	VALIDATE_RV(DEM_INITIALIZED == demState, DEM_REPIUMPRFAULTDETECT_ID, DEM_E_UNINIT, E_NOT_OK);

	// valid condition ID
	VALIDATE_RV(((ConditionId == DEM_IUMPR_DEN_COND_COLDSTART) || (ConditionId == DEM_IUMPR_DEN_COND_EVAP) || (ConditionId == DEM_IUMPR_DEN_COND_500MI) || (ConditionId == DEM_IUMPR_GENERAL_OBDCOND)), DEM_REPIUMPRFAULTDETECT_ID, DEM_E_PARAM_DATA, E_NOT_OK);

	// valid condition status
	VALIDATE_RV(ConditionStatus < 0x03, DEM_REPIUMPRFAULTDETECT_ID, DEM_E_PARAM_DATA, E_NOT_OK);
#if defined(DEM_USE_IUMPR)
	for (uint8 i = 0; i < DEM_IUMPR_ADDITIONAL_DENOMINATORS_COUNT; i++) {
		if (iumprAddiDenomCondBuffer[i].condition == ConditionId) {
			if (iumprAddiDenomCondBuffer[i].status != ConditionStatus) {
				iumprAddiDenomCondBuffer[i].status = ConditionStatus;

				ret = E_OK;

				// if applicable
				incrementGeneralDenominator(ConditionId, ConditionStatus);
			}

			break;
		}
	}
#endif
	return ret;
}

/**
 * In order to communicate the status of the (additional) denominator conditions among the OBD relevant ECUs, the API is used to retrieve
 * the condition status from the Dem of the ECU where the conditions are computed. API is needed in OBD-relevant ECUs only.
 *
 * @param ConditionId: Identification of a IUMPR denominator condition ID (General Denominator, Cold start, EVAP, 500mi).
 * @param ConditionStatus: Status of the IUMPR denominator condition (Notreached, reached, not reachable / inhibited)
 * @return E_OK: get of IUMPR denominator condition status was successful,
 * E_NOT_OK: get of condition status failed.
 */
Std_ReturnType Dem_GetIUMPRDenCondition(Dem_IumprDenomCondIdType ConditionId, Dem_IumprDenomCondStatusType* ConditionStatus) {
	Std_ReturnType ret = E_NOT_OK;

	// dem started
	VALIDATE_RV(DEM_INITIALIZED == demState, DEM_REPIUMPRFAULTDETECT_ID, DEM_E_UNINIT, E_NOT_OK);

	// valid condition ID
	VALIDATE_RV(((ConditionId == DEM_IUMPR_DEN_COND_COLDSTART) || (ConditionId == DEM_IUMPR_DEN_COND_EVAP) || (ConditionId == DEM_IUMPR_DEN_COND_500MI) || (ConditionId == DEM_IUMPR_GENERAL_OBDCOND)), DEM_REPIUMPRFAULTDETECT_ID, DEM_E_PARAM_DATA, E_NOT_OK);
#if defined(DEM_USE_IUMPR)
	for (uint8 i = 0; i < DEM_IUMPR_ADDITIONAL_DENOMINATORS_COUNT; i++) {
		if (iumprAddiDenomCondBuffer[i].condition == ConditionId) {
			*ConditionStatus = iumprAddiDenomCondBuffer[i].status;

			ret = E_OK;
		}
	}
#endif
	return ret;
}

#endif

/**
 * Set the suppression status of a specific DTC.
 * @param DTC
 * @param DTCFormat
 * @param SuppressionStatus
 * @return E_OK (Operation was successful), E_NOT_OK (operation failed or event entry for this DTC still exists)
 */
/* @req DEM589 */
/* @req 4.2.2/SWS_Dem_01047 */
#if (DEM_DTC_SUPPRESSION_SUPPORT == STD_ON)
/* @req 4.2.2/SWS_Dem_00586 */
Std_ReturnType Dem_SetDTCSuppression(uint32 DTC, Dem_DTCFormatType DTCFormat, boolean SuppressionStatus)
{
    /* Requirement tag intentionally incorrect. Handled in DEM.py */
    /* !req DEM588 Allowing suppression of DTC even if event memory entry exists (this requirement is removed in ASR 4.2.2) */
    Std_ReturnType ret = E_NOT_OK;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_SETDTCSUPPRESSION_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV(IS_VALID_DTC_FORMAT(DTCFormat), DEM_SETDTCSUPPRESSION_ID, DEM_E_PARAM_DATA, E_NOT_OK);
    const Dem_DTCClassType *DTCClassPtr = configSet->DTCClass;
    while( FALSE == DTCClassPtr->Arc_EOL ) {
        if( ((DEM_DTC_FORMAT_UDS == DTCFormat) && (DTCClassPtr->DTCRef->UDSDTC == DTC)) ||
            ((DEM_DTC_FORMAT_OBD == DTCFormat) && (TO_OBD_FORMAT(DTCClassPtr->DTCRef->OBDDTC) == DTC))) {
            DemDTCSuppressed[DTCClassPtr->DTCIndex].SuppressedByDTC = SuppressionStatus;
            ret = E_OK;
            break;
        }
        DTCClassPtr++;
    }
    return ret;
}
#endif

#if defined(DEM_USE_MEMORY_FUNCTIONS)
/**
 * Check if an event is stored in freeze frame buffer
 * @param eventId
 * @param dtcOrigin
 * @return TRUE: Event stored in FF buffer, FALSE: Event NOT stored in FF buffer
 */
static boolean isInFFBuffer(Dem_EventIdType eventId, Dem_DTCOriginType dtcOrigin)
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
            ffFound = (freezeFrameBuffer[i].eventId == eventId)? TRUE: FALSE;
        }
    }
    return ffFound;
}
#endif
/**
 * Checks if event is stored in event memory
 * @param EventId
 * @return TRUE: Event stored in memory, FALSE: event NOT stored in memory
 */
static boolean EventIsStoredInMemory(Dem_EventIdType EventId)
{
#if defined(DEM_USE_MEMORY_FUNCTIONS)
    const Dem_EventParameterType *eventParam;
    ExtDataRecType *extData;
    boolean isStored = FALSE;

    lookupEventIdParameter(EventId, &eventParam);
    if( (NULL != eventParam) &&  (DEM_DTC_ORIGIN_NOT_USED != eventParam->EventClass->EventDestination) ) {
        isStored = ((TRUE == isInEventMemory(eventParam)) || (TRUE == isInFFBuffer(EventId, eventParam->EventClass->EventDestination)) || (TRUE == lookupExtendedDataMem(EventId, &extData, eventParam->EventClass->EventDestination)))? TRUE: FALSE;
    }
    return isStored;
#else
    (void)EventId;
    return FALSE;
#endif
}

/**
 * Set the available status of a specific Event.
 * @param EventId: Identification of an event by assigned EventId.
 * @param AvailableStatus: This parameter specifies whether the respective Event shall be available (TRUE) or not (FALSE).
 * @return: E_OK: Operation was successful, E_NOT_OK: change of available status not accepted
 */
/* @req 4.2.2/SWS_Dem_01080 */
Std_ReturnType Dem_SetEventAvailable(Dem_EventIdType EventId, boolean AvailableStatus)
{
#if (DEM_SET_EVENT_AVAILABLE_PREINIT == STD_ON)
    VALIDATE_RV(DEM_UNINITIALIZED != demState, DEM_SETEVENTAVAILABLE_ID, DEM_E_UNINIT, E_NOT_OK);
#else
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_SETEVENTAVAILABLE_ID, DEM_E_UNINIT, E_NOT_OK);
#endif
    VALIDATE_RV(IS_VALID_EVENT_ID(EventId), DEM_SETEVENTAVAILABLE_ID, DEM_E_PARAM_DATA, E_NOT_OK);

    const Dem_EventParameterType *eventParam = NULL;
    Dem_EventStatusExtendedType oldStatus;
    EventStatusRecType *eventStatusRec = NULL;
    Std_ReturnType ret = E_NOT_OK;

    if ( (demState == DEM_UNINITIALIZED)
#if (DEM_SET_EVENT_AVAILABLE_PREINIT == STD_OFF)
            || (demState == DEM_PREINITIALIZED)
#endif
    ) {
        return E_NOT_OK;
    }

    SchM_Enter_Dem_EA_0();

    lookupEventStatusRec(EventId, &eventStatusRec);
    lookupEventIdParameter(EventId, &eventParam);
    /* @req 4.2.2/SWS_Dem_01109 */
    if( (NULL != eventStatusRec) && (NULL != eventParam) && (*eventParam->EventClass->EventAvailableByCalibration == TRUE) &&
            (0u == (eventStatusRec->eventStatusExtended & DEM_TEST_FAILED)) && ((DEM_PREINITIALIZED == demState) || (FALSE == EventIsStoredInMemory(EventId))) ) {
        if( eventStatusRec->isAvailable != AvailableStatus ) {
            /* Event availability changed */
            eventStatusRec->isAvailable = AvailableStatus;
            oldStatus = eventStatusRec->eventStatusExtended;
            if( FALSE == AvailableStatus ) {
                /* @req 4.2.2/SWS_Dem_01110 */
                eventStatusRec->eventStatusExtended = 0x00;
            } else {
                /* @req 4.2.2/SWS_Dem_01111 */
                eventStatusRec->eventStatusExtended = 0x50;
            }

            if( oldStatus != eventStatusRec->eventStatusExtended ) {
                notifyEventStatusChange(eventParam, oldStatus, eventStatusRec->eventStatusExtended);
            }
#if (DEM_DTC_SUPPRESSION_SUPPORT == STD_ON)
            /* Check if suppression of DTC is affected */
            boolean suppressed = TRUE;
            EventStatusRecType *dtcEventStatusRec;
            if( (NULL != eventParam->DTCClassRef) && (NULL != eventParam->DTCClassRef->Events) ) {
                for( uint16 i = 0; (i < eventParam->DTCClassRef->NofEvents) && (TRUE == suppressed); i++ ) {
                    dtcEventStatusRec = NULL;
                    lookupEventStatusRec(eventParam->DTCClassRef->Events[i], &dtcEventStatusRec);
                    if( (NULL != dtcEventStatusRec) && (TRUE == dtcEventStatusRec->isAvailable) ) {
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
        ret = E_OK;
    }
    SchM_Exit_Dem_EA_0();
    return ret;
}

/**
 * Gets the current monitor status for an event.
 * @param EventID
 * @param MonitorStatus
 * @return E_OK: Get monitor status was successful, E_NOT_OK: getting the monitor status failed (e.g.an invalid event id was provided).
 */
/* @req 4.3.0/SWS_Dem_91007 */
Std_ReturnType Dem_GetMonitorStatus(Dem_EventIdType EventID, Dem_MonitorStatusType* MonitorStatus)
{
    Std_ReturnType ret = E_NOT_OK;
    EventStatusRecType * eventStatusRec;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETMONITORSTATUS_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV((NULL != MonitorStatus), DEM_GETMONITORSTATUS_ID, DEM_E_PARAM_POINTER, E_NOT_OK);

    if( IS_VALID_EVENT_ID(EventID) ) {
        /* @req 4.3.0/SWS_Dem_01287 */
        lookupEventStatusRec(EventID, &eventStatusRec);
        if( NULL != eventStatusRec ) {
            *MonitorStatus = 0u;
            if( 0u != (eventStatusRec->eventStatusExtended & DEM_TEST_FAILED) ) {
                *MonitorStatus |= DEM_MONITOR_STATUS_TF;
            }
            if( 0u != (eventStatusRec->eventStatusExtended & DEM_TEST_NOT_COMPLETED_THIS_OPERATION_CYCLE) ) {
                *MonitorStatus |= DEM_MONITOR_STATUS_TNCTOC;
            }
            ret = E_OK;
        }
    }
    else {
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GETMONITORSTATUS_ID, DEM_E_PARAM_DATA);
        /* @req 4.3.0/SWS_Dem_01288 */
    }
    return ret;
}
