Dem_ReturnGetExtendedDataRecordByDTCType Dem_GetExtendedDataRecordByDTC(uint32 dtc, Dem_DTCOriginType dtcOrigin, uint8 extendedDataNumber, uint8 *destBuffer, uint16 *bufSize)
{
    /* IMPROVEMENT: Handle record numbers 0xFE and 0xFF */
    /* NOTE: dtc is in UDS format according to DEM239 */
    /* @req DEM540 */
    Dem_ReturnGetExtendedDataRecordByDTCType returnCode = DEM_RECORD_WRONG_DTC;
    const Dem_EventParameterType *eventParam;
    Dem_ExtendedDataRecordClassType const *extendedDataRecordClass = NULL;
    ExtDataRecType *extData;
    uint16 posInExtData = 0;
    uint16 nofBytesCopied = 0;
    uint16 bufSizeLeft ;
    boolean oneRecordOK = FALSE;
#if (DEM_EVENT_COMB_TYPE2_REPORT_OLDEST_DATA == STD_ON)
    uint32 timestamp = 0;
    boolean dataCopied = FALSE;
#endif
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETEXTENDEDDATARECORDBYDTC_ID, DEM_E_UNINIT, DEM_RECORD_WRONG_DTC);
    VALIDATE_RV((NULL != destBuffer), DEM_GETEXTENDEDDATARECORDBYDTC_ID, DEM_E_PARAM_POINTER, DEM_RECORD_WRONG_DTC);
    VALIDATE_RV((NULL != bufSize), DEM_GETEXTENDEDDATARECORDBYDTC_ID, DEM_E_PARAM_POINTER, DEM_RECORD_WRONG_DTC);
    SchM_Enter_Dem_EA_0();
    bufSizeLeft = *bufSize;
    if( extendedDataNumber <= DEM_HIGHEST_EXT_DATA_REC_NUM ) {
        /* Get the DTC config */
        const Dem_DTCClassType *DTCClass;
        if ( TRUE == LookupUdsDTC(dtc, &DTCClass) ) {
            for(uint16 i = 0; i < DTCClass->NofEvents; i++) {
                eventParam = NULL_PTR;
                lookupEventIdParameter(DTCClass->Events[i], &eventParam);
                if( NULL_PTR != eventParam ) {
                    if (checkDtcOrigin(dtcOrigin, eventParam, FALSE)==TRUE) {
                        if (lookupExtendedDataRecNumParam(extendedDataNumber, eventParam, &extendedDataRecordClass, &posInExtData)==TRUE) {
                            if (bufSizeLeft >= extendedDataRecordClass->DataSize) {
                                oneRecordOK = TRUE;
#if (DEM_EVENT_COMB_TYPE2_REPORT_OLDEST_DATA == STD_OFF)
                                Dem_EventIdType idToFind;
#endif
                                if( extendedDataRecordClass->UpdateRule != DEM_UPDATE_RECORD_VOLATILE ) {
                                    switch (dtcOrigin) {
                                        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
                                        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
#if (DEM_EVENT_COMB_TYPE2_REPORT_OLDEST_DATA == STD_ON)
                                            if (lookupOlderExtendedDataMem(eventParam->EventID, &extData, dtcOrigin, &timestamp, dataCopied) == TRUE) {
                                                // Yes all conditions met, copy the extended data record to destination buffer.
                                                memcpy(destBuffer, &extData->data[posInExtData], extendedDataRecordClass->DataSize); /** @req DEM075 */
                                                nofBytesCopied = extendedDataRecordClass->DataSize;/* @req DEM076 */
                                                dataCopied = TRUE;
                                                returnCode = DEM_RECORD_OK;
                                            }

#else

#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                                            if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
                                                /* @req DEM537 */
                                                idToFind = TO_COMBINED_EVENT_ID(eventParam->CombinedDTCCID);
                                            }
                                            else {
                                                idToFind = eventParam->EventID;
                                            }
#else
                                            idToFind = eventParam->EventID;
#endif
                                            if (lookupExtendedDataMem(idToFind, &extData, dtcOrigin) == TRUE) {
                                                // Yes all conditions met, copy the extended data record to destination buffer.
                                                memcpy(&destBuffer[nofBytesCopied], &extData->data[posInExtData], extendedDataRecordClass->DataSize); /** @req DEM075 */
#if !defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                                                bufSizeLeft -= extendedDataRecordClass->DataSize;/* @req DEM076 */
#endif
                                                nofBytesCopied += extendedDataRecordClass->DataSize;
                                                returnCode = DEM_RECORD_OK;
                                            } else {
                                                /* The record number is legal but no record was found for the DTC *//* @req DEM631 */
                                                returnCode = DEM_RECORD_OK;
                                            }
#endif
                                            break;
                                        case DEM_DTC_ORIGIN_PERMANENT_MEMORY:
                                        case DEM_DTC_ORIGIN_MIRROR_MEMORY:
                                            // Not yet supported
                                            returnCode = DEM_RECORD_WRONG_DTCORIGIN;
                                            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GETEXTENDEDDATARECORDBYDTC_ID, DEM_E_NOT_IMPLEMENTED_YET);
                                            break;
                                        default:
                                            returnCode = DEM_RECORD_WRONG_DTCORIGIN;
                                            break;
                                    }
                                } else {
#if (DEM_EVENT_COMB_TYPE2_REPORT_OLDEST_DATA == STD_ON)
                                    if( FALSE == dataCopied ) {
                                        /* No data copied*/
                                        if( NULL != extendedDataRecordClass->CallbackGetExtDataRecord ) {
                                            /* IMPROVEMENT: Handle return value? */
                                            (void)extendedDataRecordClass->CallbackGetExtDataRecord(&destBuffer[nofBytesCopied]);
                                            bufSizeLeft -= extendedDataRecordClass->DataSize;/* @req DEM076 */
                                            nofBytesCopied += extendedDataRecordClass->DataSize;
                                            returnCode = DEM_RECORD_OK;
                                        } else if (DEM_NO_ELEMENT != extendedDataRecordClass->InternalDataElement ) {
                                            getInternalElement( eventParam, extendedDataRecordClass->InternalDataElement, &destBuffer[nofBytesCopied], extendedDataRecordClass->DataSize );
                                            bufSizeLeft -= extendedDataRecordClass->DataSize;/* @req DEM076 */
                                            nofBytesCopied += extendedDataRecordClass->DataSize;
                                            returnCode = DEM_RECORD_OK;
                                        } else {
                                            returnCode = DEM_RECORD_WRONG_DTC;
                                        }
                                    }
                                    else {
                                        /* Data has already been copied. This means that a stored record was found. Assume this to be older than "live" data. */
                                    }
#else
                                    if( NULL != extendedDataRecordClass->CallbackGetExtDataRecord ) {
                                        /* IMPROVEMENT: Handle return value? */
                                        (void)extendedDataRecordClass->CallbackGetExtDataRecord(destBuffer);
#if !defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                                        bufSizeLeft -= extendedDataRecordClass->DataSize;/* @req DEM076 */
#endif
                                        nofBytesCopied += extendedDataRecordClass->DataSize;
                                        returnCode = DEM_RECORD_OK;
                                    } else if (DEM_NO_ELEMENT != extendedDataRecordClass->InternalDataElement ) {
                                        getInternalElement( eventParam, extendedDataRecordClass->InternalDataElement, destBuffer, extendedDataRecordClass->DataSize );
#if !defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                                        bufSizeLeft -= extendedDataRecordClass->DataSize;/* @req DEM076 */
#endif
                                        nofBytesCopied += extendedDataRecordClass->DataSize;
                                        returnCode = DEM_RECORD_OK;
                                    } else {
                                        returnCode = DEM_RECORD_WRONG_DTC;
                                    }
#endif
                                }
                            } else {
                                DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GETEXTENDEDDATARECORDBYDTC_ID, DEM_E_PARAM_LENGTH);
                                returnCode = DEM_RECORD_BUFFERSIZE;
                            }
                        } else {
                            returnCode = DEM_RECORD_NUMBER;
                        }
                    } else {
                        returnCode = DEM_RECORD_WRONG_DTCORIGIN;
                    }
                }
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                break;
#endif
            }
            if( TRUE == oneRecordOK ) {
                returnCode = DEM_RECORD_OK;
                *bufSize = nofBytesCopied;
            }
        } else {
            /* Event has no DTC or DTC is suppressed */
            /* @req 4.2.2/SWS_Dem_01100 */
            /* @req 4.2.2/SWS_Dem_01101 */
            /* @req DEM587 */
            returnCode = DEM_RECORD_WRONG_DTC;
        }
    } else {
        returnCode = DEM_RECORD_NUMBER;
    }

    SchM_Exit_Dem_EA_0();
    return returnCode;
}


/*
 * Procedure:   Dem_GetSizeOfExtendedDataRecordByDTC
 * Reentrant:   No
 */
/*lint -esym(793, Dem_GetSizeOfExtendedDataRecordByDTC) Function name defined by AUTOSAR. */
Dem_ReturnGetSizeOfExtendedDataRecordByDTCType Dem_GetSizeOfExtendedDataRecordByDTC(uint32 dtc, Dem_DTCOriginType dtcOrigin, uint8 extendedDataNumber, uint16 *sizeOfExtendedDataRecord)
{
    /* NOTE: dtc is in UDS format according to DEM240 */
    Dem_ReturnGetExtendedDataRecordByDTCType returnCode = DEM_GET_SIZEOFEDRBYDTC_W_DTC;
    Dem_ExtendedDataRecordClassType const *extendedDataRecordClass = NULL_PTR;
    const Dem_EventParameterType *eventParam;
    uint16 posInExtData;
    boolean oneRecordOK = FALSE;

    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETSIZEOFEXTENDEDDATARECORDBYDTC_ID, DEM_E_UNINIT, DEM_GET_SIZEOFEDRBYDTC_W_DTC);
    VALIDATE_RV(NULL != sizeOfExtendedDataRecord, DEM_GETSIZEOFEXTENDEDDATARECORDBYDTC_ID, DEM_E_PARAM_POINTER, DEM_GET_SIZEOFEDRBYDTC_W_DTC);
    SchM_Enter_Dem_EA_0();

    /* Check if event has DTC and that the DTC is not suppressed *//* @req DEM587 */
    /* @req 4.2.2/SWS_Dem_01100 */
    /* @req 4.2.2/SWS_Dem_01101 */
    /* Get the DTC config */
    const Dem_DTCClassType *DTCClass;
    if ( TRUE == LookupUdsDTC(dtc, &DTCClass) ) {
        *sizeOfExtendedDataRecord = 0u;
        for(uint16 i = 0; i < DTCClass->NofEvents; i++) {
            eventParam = NULL_PTR;
            lookupEventIdParameter(DTCClass->Events[i], &eventParam);
            if( NULL_PTR != eventParam ) {
                if (checkDtcOrigin(dtcOrigin, eventParam, FALSE) == TRUE) {
                    if (lookupExtendedDataRecNumParam(extendedDataNumber, eventParam, &extendedDataRecordClass, &posInExtData) == TRUE) {
#if (DEM_EVENT_COMB_TYPE2_REPORT_OLDEST_DATA == STD_ON)
                        if( extendedDataRecordClass->DataSize > *sizeOfExtendedDataRecord) {
                            *sizeOfExtendedDataRecord = extendedDataRecordClass->DataSize; /** @req DEM076 */
                        }
#else
                        *sizeOfExtendedDataRecord += extendedDataRecordClass->DataSize;
#endif
                        oneRecordOK = TRUE;
                    }
                    else {
                        returnCode = DEM_GET_SIZEOFEDRBYDTC_W_RNUM;
                    }
                }
                else {
                    returnCode = DEM_GET_SIZEOFEDRBYDTC_W_DTCOR;
                }
            }
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
            break;
#endif
        }
        if( TRUE == oneRecordOK ) {
            returnCode = DEM_GET_SIZEOFEDRBYDTC_OK;
        }
    }

    SchM_Exit_Dem_EA_0();
    return returnCode;
}

/*
 * Procedure:   Dem_GetFreezeFrameDataByDTC
 * Reentrant:   No
 */
/** @req DEM236 */
Dem_ReturnGetFreezeFrameDataByDTCType Dem_GetFreezeFrameDataByDTC(uint32 dtc, Dem_DTCOriginType dtcOrigin, uint8 recordNumber, uint8* destBuffer, uint16*  bufSize)
{
    /* !req DEM576 */
    /* @req DEM540 */
    /* NOTE: dtc is in UDS format according to DEM236 */
    Dem_ReturnGetFreezeFrameDataByDTCType returnCode = DEM_GET_FFDATABYDTC_WRONG_DTC;
    const Dem_EventParameterType *eventParam;
    Dem_FreezeFrameClassType const *FFDataRecordClass = NULL;
    uint16 FFDataSize = 0;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETFREEZEFRAMEDATABYDTC_ID, DEM_E_UNINIT, DEM_GET_ID_PENDING);
    VALIDATE_RV((NULL != destBuffer), DEM_GETFREEZEFRAMEDATABYDTC_ID, DEM_E_PARAM_POINTER, DEM_GET_FFDATABYDTC_WRONG_DTC);
    VALIDATE_RV((NULL != bufSize), DEM_GETFREEZEFRAMEDATABYDTC_ID, DEM_E_PARAM_POINTER, DEM_GET_FFDATABYDTC_WRONG_DTC);
    SchM_Enter_Dem_EA_0();

    uint16 bufSizeLeft = *bufSize;
    uint16 bufferSize;
    uint16 nofBytesCopied = 0u;
    boolean oneRecordOK = FALSE;
#if (DEM_EVENT_COMB_TYPE2_REPORT_OLDEST_DATA == STD_ON)
    uint32 timestamp = 0;
    boolean useTimestamp = FALSE;
#endif
    if( *bufSize >= DEM_REC_NUM_AND_NUM_DIDS_SIZE ) {
        bufSizeLeft -= DEM_REC_NUM_AND_NUM_DIDS_SIZE;
        if( recordNumber <= DEM_HIGHEST_FF_REC_NUM ) {
            /* Get the DTC config */
            const Dem_DTCClassType *DTCClass;
            if ( TRUE == LookupUdsDTC(dtc, &DTCClass) ) {
                destBuffer[0] = recordNumber;
                destBuffer[1] = 0u;
                for(uint16 i = 0; i < DTCClass->NofEvents; i++) {
                    eventParam = NULL_PTR;
                    lookupEventIdParameter(DTCClass->Events[i], &eventParam);
                    if( NULL_PTR != eventParam ) {
                        if (checkDtcOrigin(dtcOrigin, eventParam, FALSE) == TRUE) {
                            if (lookupFreezeFrameDataRecNumParam(recordNumber, eventParam, &FFDataRecordClass) == TRUE) {
                                /* NOTE: Handle return value? */
                                (void)lookupFreezeFrameDataSize(recordNumber, &FFDataRecordClass, &FFDataSize);
                                if (bufSizeLeft >= FFDataSize) {
                                    oneRecordOK = TRUE;
                                    switch (dtcOrigin) {
                                        case DEM_DTC_ORIGIN_PRIMARY_MEMORY:
                                        case DEM_DTC_ORIGIN_SECONDARY_MEMORY:
                                            returnCode = DEM_GET_FFDATABYDTC_OK;
                                            bufferSize = bufSizeLeft;
#if (DEM_EVENT_COMB_TYPE2_REPORT_OLDEST_DATA == STD_ON)
                                            if( TRUE == getOlderFreezeFrameRecord(eventParam->EventID, recordNumber, dtcOrigin, &destBuffer[2u], &bufferSize, FFDataSize, &timestamp,  useTimestamp)) {
                                                destBuffer[1] = FFDataRecordClass->NofXids;
                                                useTimestamp = TRUE;
                                                nofBytesCopied = bufferSize;
                                            }
#else
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                                            Dem_EventIdType eventId;
                                            if( DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID ) {
                                                /* @req DEM537 */
                                                eventId = TO_COMBINED_EVENT_ID(eventParam->CombinedDTCCID);
                                            }
                                            else {
                                                eventId = eventParam->EventID;
                                            }
                                            if (getFreezeFrameRecord(eventId, recordNumber, dtcOrigin, &destBuffer[nofBytesCopied + 2u], &bufferSize, FFDataSize) == TRUE) {
#else
                                            if (getFreezeFrameRecord(eventParam->EventID, recordNumber, dtcOrigin, &destBuffer[nofBytesCopied + 2u], &bufferSize, FFDataSize) == TRUE) {
#endif
                                                destBuffer[1] += FFDataRecordClass->NofXids;
#if !defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                                                bufSizeLeft -= bufferSize;
#endif
                                                nofBytesCopied += bufferSize;
                                            } else {
                                                /* @req DEM630 */
                                            }
#endif
                                            break;
                                        case DEM_DTC_ORIGIN_PERMANENT_MEMORY:
                                        case DEM_DTC_ORIGIN_MIRROR_MEMORY:
                                            // Not yet supported
                                            returnCode = DEM_GET_FFDATABYDTC_WRONG_DTCORIGIN;
                                            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GETFREEZEFRAMEDATABYDTC_ID, DEM_E_NOT_IMPLEMENTED_YET);
                                            break;
                                        default:
                                            returnCode = DEM_GET_FFDATABYDTC_WRONG_DTCORIGIN;
                                            break;
                                    }
                                } else {
                                    DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GETFREEZEFRAMEDATABYDTC_ID, DEM_E_PARAM_LENGTH);
                                    returnCode = DEM_GET_FFDATABYDTC_BUFFERSIZE;
                                }
                            } else {
                                returnCode = DEM_GET_FFDATABYDTC_RECORDNUMBER;
                            }
                        } else {
                            returnCode = DEM_GET_FFDATABYDTC_WRONG_DTCORIGIN;
                        }
                    }
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                    break;
#endif
                }
                if( (DEM_GET_FFDATABYDTC_OK == returnCode) || (TRUE == oneRecordOK) ) {
                    *bufSize = nofBytesCopied;
                    if( 0u != nofBytesCopied ) {
                        /* Data was found. Add size of RecordNumber and NumOfDIDs */
                        *bufSize += DEM_REC_NUM_AND_NUM_DIDS_SIZE;
                    }
                    returnCode = DEM_GET_FFDATABYDTC_OK;
                }
            } else {
                /* Event has no DTC or DTC is suppressed */
                /* @req 4.2.2/SWS_Dem_01100 */
                /* @req 4.2.2/SWS_Dem_01101 */
                /* @req DEM587 */
                returnCode = DEM_GET_FFDATABYDTC_WRONG_DTC;

            }
        } else {
            returnCode = DEM_GET_FFDATABYDTC_RECORDNUMBER;
        }
    }
    else {
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GETFREEZEFRAMEDATABYDTC_ID, DEM_E_PARAM_LENGTH);
        returnCode = DEM_GET_FFDATABYDTC_BUFFERSIZE;
    }

    SchM_Exit_Dem_EA_0();

    return returnCode;


}

/*
 * Procedure:   Dem_GetSizeOfFreezeFrame
 * Reentrant:   No
 */
 /** @req DEM238 */
Dem_ReturnGetSizeOfFreezeFrameType Dem_GetSizeOfFreezeFrameByDTC(uint32 dtc, Dem_DTCOriginType dtcOrigin, uint8 recordNumber, uint16* sizeOfFreezeFrame)
{
    /* NOTE: dtc is in UDS format according to DEM238 */
    Dem_ReturnGetSizeOfFreezeFrameType returnCode = DEM_GET_SIZEOFFF_PENDING;
    Dem_FreezeFrameClassType const *FFDataRecordClass = NULL;
    const Dem_EventParameterType *eventParam;
#if (DEM_EVENT_COMB_TYPE2_REPORT_OLDEST_DATA == STD_ON)
    uint16 tempSize;
#endif
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETSIZEOFFREEZEFRAMEBYDTC_ID, DEM_E_UNINIT, DEM_GET_SIZEOFFF_PENDING);
    VALIDATE_RV((NULL != sizeOfFreezeFrame), DEM_GETSIZEOFFREEZEFRAMEBYDTC_ID, DEM_E_PARAM_POINTER, DEM_GET_SIZEOFFF_WRONG_DTC);
    SchM_Enter_Dem_EA_0();
    const Dem_DTCClassType *DTCClass;
    if ( TRUE == LookupUdsDTC(dtc, &DTCClass) ) {
        *sizeOfFreezeFrame = 0u;
        for(uint16 i = 0; i < DTCClass->NofEvents; i++) {
            eventParam = NULL_PTR;
            lookupEventIdParameter(DTCClass->Events[i], &eventParam);
            if( NULL_PTR != eventParam ) {
                if (checkDtcOrigin(dtcOrigin, eventParam, FALSE) == TRUE) {
                    if (lookupFreezeFrameDataRecNumParam(recordNumber, eventParam, &FFDataRecordClass) == TRUE) {
                        if(FFDataRecordClass->FFIdClassRef != NULL){
                            /* Note - there is a function called lookupFreezeFrameDataSize that can be used here */
                            for(uint16 j = 0; (j < DEM_MAX_NR_OF_DIDS_IN_FREEZEFRAME_DATA) && ((FFDataRecordClass->FFIdClassRef[j]->Arc_EOL == FALSE)); j++){
                                /* read out the did size */
#if (DEM_EVENT_COMB_TYPE2_REPORT_OLDEST_DATA == STD_ON)
                                /* Report the biggest */
                                tempSize = (uint16)(FFDataRecordClass->FFIdClassRef[j]->PidOrDidSize + DEM_DID_IDENTIFIER_SIZE_OF_BYTES);
                                if( tempSize > *sizeOfFreezeFrame ) {
                                    *sizeOfFreezeFrame = tempSize;
                                }
#else
                                /* Report the total size */
                                *sizeOfFreezeFrame += (uint16)(FFDataRecordClass->FFIdClassRef[j]->PidOrDidSize + DEM_DID_IDENTIFIER_SIZE_OF_BYTES);/** @req DEM074 */
#endif
                                returnCode = DEM_GET_SIZEOFFF_OK;
                            }
                        } else {
                            returnCode = DEM_GET_SIZEOFFF_WRONG_RNUM;
                        }
                    } else {
                        returnCode = DEM_GET_SIZEOFFF_WRONG_RNUM;
                    }
                } else {
                    returnCode = DEM_GET_SIZEOFFF_WRONG_DTCOR;
                }
            }
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
            break;
#endif
        }
    } else {
        /* Event has no DTC or DTC is suppressed */
        /* @req 4.2.2/SWS_Dem_01100 */
        /* @req 4.2.2/SWS_Dem_01101 */
        /* @req DEM587 */
        returnCode = DEM_GET_SIZEOFFF_WRONG_DTC;
    }

    SchM_Exit_Dem_EA_0();
    return returnCode;


}

/**
 *
 * @param DTCFormat
 * @param NumberOfFilteredRecords
 * @return
 */
Dem_ReturnSetFilterType Dem_SetFreezeFrameRecordFilter(Dem_DTCFormatType DTCFormat, uint16 *NumberOfFilteredRecords)
{
    Dem_ReturnSetFilterType ret = DEM_WRONG_FILTER;
    uint16 nofRecords = 0;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_SETFREEZEFRAMERECORDFILTER_ID, DEM_E_UNINIT, DEM_WRONG_FILTER);
    VALIDATE_RV((NULL != NumberOfFilteredRecords), DEM_SETFREEZEFRAMERECORDFILTER_ID, DEM_E_PARAM_POINTER, DEM_WRONG_FILTER);
    VALIDATE_RV(IS_VALID_DTC_FORMAT(DTCFormat), DEM_SETFREEZEFRAMERECORDFILTER_ID, DEM_E_PARAM_DATA, DEM_WRONG_FILTER);

    SchM_Enter_Dem_EA_0();

    /* @req DEM210 Only applies to primary memory */
#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM)
    for( uint16 i = 0; i < DEM_MAX_NUMBER_FF_DATA_PRI_MEM; i++ ) {
        if( DEM_EVENT_ID_NULL != priMemFreezeFrameBuffer[i].eventId ) {
            EventStatusRecType *eventStatusRecPtr = NULL_PTR;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
            if( !IS_COMBINED_EVENT_ID(priMemFreezeFrameBuffer[i].eventId) ) {
                /* Only do this if the entry is NOT a combined event entry. This to avoid Det error. */
                lookupEventStatusRec(priMemFreezeFrameBuffer[i].eventId, &eventStatusRecPtr);
            }
#else
            lookupEventStatusRec(priMemFreezeFrameBuffer[i].eventId, &eventStatusRecPtr);
#endif
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE2)
            /* Check if this record has already been counted. */
            boolean alreadyCounted = FALSE;
            for( uint16 j = 0; j < i; j++ ) {
                if( DEM_EVENT_ID_NULL != priMemFreezeFrameBuffer[j].eventId ) {
                    EventStatusRecType *eventStatusRecPtr2 = NULL_PTR;
                    lookupEventStatusRec(priMemFreezeFrameBuffer[j].eventId, &eventStatusRecPtr2);
                    if(  NULL_PTR != eventStatusRecPtr2) {
                        if( (eventStatusRecPtr->eventParamRef->DTCClassRef == eventStatusRecPtr2->eventParamRef->DTCClassRef) &&
                                (priMemFreezeFrameBuffer[j].recordNumber == priMemFreezeFrameBuffer[i].recordNumber)) {
                            /* Same DTC and record found earlier in buffer -> already counted. */
                            alreadyCounted = TRUE;
                        }
                    }
                }
            }
#endif
            if( (NULL_PTR != eventStatusRecPtr) && (TRUE == eventHasDTCOnFormat(eventStatusRecPtr->eventParamRef, DTCFormat)) &&
                    (TRUE == DTCIsAvailable(eventStatusRecPtr->eventParamRef->DTCClassRef))
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE2)
                    && (FALSE == alreadyCounted)
#endif
                    ) {
                nofRecords++;
            }
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
            if( (NULL_PTR == eventStatusRecPtr) && IS_COMBINED_EVENT_ID(priMemFreezeFrameBuffer[i].eventId) ) {
                /* This is a combined event entry. */
                /* Get DTC config */
                const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(priMemFreezeFrameBuffer[i].eventId)];
                if( (TRUE == DTCISAvailableOnFormat(CombDTCCfg->DTCClassRef, DTCFormat)) &&
                        (TRUE == DTCIsAvailable(CombDTCCfg->DTCClassRef)) ) {
                    nofRecords++;
                }
            }
#endif
        }
    }
#endif
    *NumberOfFilteredRecords = nofRecords;
    /* @req DEM595 */
    ffRecordFilter.dtcFormat = DTCFormat;
    ffRecordFilter.ffIndex = 0;

    ret = DEM_FILTER_ACCEPTED;

    SchM_Exit_Dem_EA_0();
    return ret;
}

Dem_ReturnGetNextFilteredDTCType Dem_GetNextFilteredRecord(uint32 *DTC, uint8 *RecordNumber)
{
    /* No requirement on checking the pointers but do it anyway. */
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETNEXTFILTEREDRECORD_ID, DEM_E_UNINIT, DEM_FILTERED_NO_MATCHING_DTC);
    VALIDATE_RV(NULL != DTC, DEM_GETNEXTFILTEREDRECORD_ID, DEM_E_PARAM_POINTER, DEM_FILTERED_NO_MATCHING_DTC);
    VALIDATE_RV(NULL != RecordNumber, DEM_GETNEXTFILTEREDRECORD_ID, DEM_E_PARAM_POINTER, DEM_FILTERED_NO_MATCHING_DTC);
    Dem_ReturnGetNextFilteredDTCType ret = DEM_FILTERED_NO_MATCHING_DTC;
    SchM_Enter_Dem_EA_0();

#if ((DEM_USE_PRIMARY_MEMORY_SUPPORT == STD_ON) && DEM_FF_DATA_IN_PRI_MEM)
    /* Find the next record which has a DTC */
    EventStatusRecType *eventStatusRecPtr;
    boolean found = FALSE;
    for( uint16 i = ffRecordFilter.ffIndex; (i < DEM_MAX_NUMBER_FF_DATA_PRI_MEM) && (FALSE == found); i++  ) {
        if( DEM_EVENT_ID_NULL != priMemFreezeFrameBuffer[i].eventId ) {
            eventStatusRecPtr = NULL_PTR;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
            if( !IS_COMBINED_EVENT_ID(priMemFreezeFrameBuffer[i].eventId) ) {
                /* Only do this if the entry is NOT a combined event entry. This to avoid Det error. */
                lookupEventStatusRec(priMemFreezeFrameBuffer[i].eventId, &eventStatusRecPtr);
            }
#else
            lookupEventStatusRec(priMemFreezeFrameBuffer[i].eventId, &eventStatusRecPtr);
#endif
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE2)
            /* Check if this record has already been counted. */
            boolean alreadyCounted = FALSE;
            for( uint16 j = 0; j < i; j++ ) {
                if( DEM_EVENT_ID_NULL != priMemFreezeFrameBuffer[j].eventId ) {
                    EventStatusRecType *eventStatusRecPtr2 = NULL_PTR;
                    lookupEventStatusRec(priMemFreezeFrameBuffer[j].eventId, &eventStatusRecPtr2);
                    if(  NULL_PTR != eventStatusRecPtr2) {
                        if( (eventStatusRecPtr->eventParamRef->DTCClassRef == eventStatusRecPtr2->eventParamRef->DTCClassRef) &&
                                (priMemFreezeFrameBuffer[j].recordNumber == priMemFreezeFrameBuffer[i].recordNumber)) {
                            /* Same DTC and record found earlier in buffer -> already counted. */
                            alreadyCounted = TRUE;
                        }
                    }
                }
            }
#endif
            /* @req 4.2.2/SWS_Dem_01101 *//* @req DEM587 */
            if( (NULL != eventStatusRecPtr) && (TRUE == eventHasDTCOnFormat(eventStatusRecPtr->eventParamRef, ffRecordFilter.dtcFormat)) &&
                    (TRUE == DTCIsAvailable(eventStatusRecPtr->eventParamRef->DTCClassRef))
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE2)
                    && (FALSE == alreadyCounted)
#endif
            ) {
                /* Found one not already reported! */
                /* @req DEM225 */
                *RecordNumber = priMemFreezeFrameBuffer[i].recordNumber;
                *DTC = (DEM_DTC_FORMAT_UDS == ffRecordFilter.dtcFormat) ?
                        eventStatusRecPtr->eventParamRef->DTCClassRef->DTCRef->UDSDTC : TO_OBD_FORMAT(eventStatusRecPtr->eventParamRef->DTCClassRef->DTCRef->OBDDTC);
                /* @req DEM226 */
                ffRecordFilter.ffIndex = i + 1;
                found = TRUE;
                ret = DEM_FILTERED_OK;
            }
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
            if( (NULL_PTR == eventStatusRecPtr) && IS_COMBINED_EVENT_ID(priMemFreezeFrameBuffer[i].eventId) ) {
                /* This is a combined event entry. */
                /* Get DTC config */
                const Dem_CombinedDTCCfgType *CombDTCCfg = &configSet->CombinedDTCConfig[TO_COMBINED_EVENT_CFG_IDX(priMemFreezeFrameBuffer[i].eventId)];
                if( (TRUE == DTCISAvailableOnFormat(CombDTCCfg->DTCClassRef, ffRecordFilter.dtcFormat)) &&
                        (TRUE == DTCIsAvailable(CombDTCCfg->DTCClassRef)) ) {
                    *RecordNumber = priMemFreezeFrameBuffer[i].recordNumber;
                    *DTC = (DEM_DTC_FORMAT_UDS == ffRecordFilter.dtcFormat) ?
                            CombDTCCfg->DTCClassRef->DTCRef->UDSDTC : TO_OBD_FORMAT(CombDTCCfg->DTCClassRef->DTCRef->OBDDTC);
                    ffRecordFilter.ffIndex = i + 1;
                    found = TRUE;
                    ret = DEM_FILTERED_OK;
                }
            }
#endif
        }
    }
#endif

    SchM_Exit_Dem_EA_0();
    return ret;
    /*lint -e{818} *RecordNumber and *DTC these pointers are updated under DEM_USE_PRIMARY_MEMORY_SUPPORT condition   */
}

