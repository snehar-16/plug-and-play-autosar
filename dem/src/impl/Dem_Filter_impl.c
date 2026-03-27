Std_ReturnType Dem_GetDTCStatusAvailabilityMask(uint8 *dtcStatusMask) /** @req DEM014 */
{
    /** @req DEM060 */
    *dtcStatusMask =    DEM_DTC_STATUS_AVAILABILITY_MASK;       // User configuration mask
    return E_OK;
}


/*
 * Procedure:   Dem_SetDTCFilter
 * Reentrant:   No
 */
Dem_ReturnSetFilterType Dem_SetDTCFilter(uint8 dtcStatusMask,
        Dem_DTCKindType dtcKind,
        Dem_DTCFormatType dtcFormat,
        Dem_DTCOriginType dtcOrigin,
        Dem_FilterWithSeverityType filterWithSeverity,
        Dem_DTCSeverityType dtcSeverityMask,
        Dem_FilterForFDCType filterForFaultDetectionCounter)
{
    Dem_ReturnSetFilterType returnCode = DEM_FILTER_ACCEPTED;
    uint8 dtcStatusAvailabilityMask;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_SETDTCFILTER_ID, DEM_E_UNINIT, E_NOT_OK);

    // Check dtcKind parameter
    VALIDATE_RV((dtcKind == DEM_DTC_KIND_ALL_DTCS) || (dtcKind == DEM_DTC_KIND_EMISSION_REL_DTCS), DEM_SETDTCFILTER_ID, DEM_E_PARAM_DATA, DEM_WRONG_FILTER);

    // Check dtcOrigin parameter
    VALIDATE_RV((dtcOrigin == DEM_DTC_ORIGIN_SECONDARY_MEMORY) || (dtcOrigin == DEM_DTC_ORIGIN_PRIMARY_MEMORY)|| (dtcOrigin == DEM_DTC_ORIGIN_PERMANENT_MEMORY), DEM_SETDTCFILTER_ID, DEM_E_PARAM_DATA, DEM_WRONG_FILTER);

    // Check filterWithSeverity and dtcSeverityMask parameter
    VALIDATE_RV(((filterWithSeverity == DEM_FILTER_WITH_SEVERITY_NO)
                || ((filterWithSeverity == DEM_FILTER_WITH_SEVERITY_YES)
                    && (0 == (dtcSeverityMask & (Dem_DTCSeverityType)~(DEM_SEVERITY_MAINTENANCE_ONLY | DEM_SEVERITY_CHECK_AT_NEXT_HALT | DEM_SEVERITY_CHECK_IMMEDIATELY))))), DEM_SETDTCFILTER_ID, DEM_E_PARAM_DATA, DEM_WRONG_FILTER);

    // Check filterForFaultDetectionCounter parameter
    VALIDATE_RV((filterForFaultDetectionCounter == DEM_FILTER_FOR_FDC_YES) || (filterForFaultDetectionCounter ==  DEM_FILTER_FOR_FDC_NO), DEM_SETDTCFILTER_ID, DEM_E_PARAM_DATA, DEM_WRONG_FILTER);

    VALIDATE_RV( IS_VALID_DTC_FORMAT(dtcFormat), DEM_SETDTCFILTER_ID, DEM_E_PARAM_DATA, DEM_WRONG_FILTER);

    (void)Dem_GetDTCStatusAvailabilityMask(&dtcStatusAvailabilityMask);

    if( (0u == (dtcStatusMask & dtcStatusAvailabilityMask)) && (DEM_DTC_STATUS_MASK_ALL != dtcStatusMask) ) {
        /* No bit in the filter mask supported. */
        returnCode = DEM_WRONG_FILTER;
    } else {
        // Yes all parameters correct, set the new filters.  /** @req DEM057 */
        dtcFilter.dtcStatusMask = dtcStatusMask & dtcStatusAvailabilityMask;
        dtcFilter.dtcKind = dtcKind;
        dtcFilter.dtcOrigin = dtcOrigin;
        dtcFilter.filterWithSeverity = filterWithSeverity;
        dtcFilter.dtcSeverityMask = dtcSeverityMask;
        dtcFilter.filterForFaultDetectionCounter = filterForFaultDetectionCounter;
        dtcFilter.DTCIndex = 0u;
        dtcFilter.dtcFormat = dtcFormat;
    }

    return returnCode;
}


/*
 * Procedure:   Dem_GetStatusOfDTC
 * Reentrant:   No
 */
Dem_ReturnGetStatusOfDTCType Dem_GetStatusOfDTC(uint32 dtc, Dem_DTCOriginType dtcOrigin, Dem_EventStatusExtendedType* status) {
    /* NOTE: dtc is in UDS format according to DEM212 */
    Dem_ReturnGetStatusOfDTCType returnCode = DEM_STATUS_FAILED;
    EventStatusRecType *eventRec;
    const Dem_DTCClassType *DTCClass;

    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETSTATUSOFDTC_ID, DEM_E_UNINIT, DEM_STATUS_FAILED);
    VALIDATE_RV(IS_SUPPORTED_ORIGIN(dtcOrigin), DEM_GETSTATUSOFDTC_ID, DEM_E_PARAM_DATA, DEM_STATUS_WRONG_DTCORIGIN);/** @req DEM171 */

    SchM_Enter_Dem_EA_0();

    Dem_EventStatusExtendedType temp = 0u;
    if ( TRUE == LookupUdsDTC(dtc, &DTCClass)) {
        returnCode = DEM_STATUS_OK;
        for(uint16 i = 0; (i < DTCClass->NofEvents) && (DEM_STATUS_OK == returnCode); i++) {
            returnCode = DEM_STATUS_OK;
            eventRec = NULL_PTR;
            lookupEventStatusRec(DTCClass->Events[i], &eventRec);
            if( NULL_PTR != eventRec ) {
                /* Event found for this DTC */
                if (checkDtcOrigin(dtcOrigin,eventRec->eventParamRef, FALSE) == TRUE) {
                    /* NOTE: Should the availability mask be used here? */
                    /* @req DEM059 */
                    /* @req DEM441 */
                    if( TRUE == eventRec->isAvailable ) {
                        temp |= eventRec->eventStatusExtended;
                    }

                } else {
                    /* Here we know that dtcOrigin is a supported one */
                    returnCode = DEM_STATUS_WRONG_DTC; /** @req DEM172 */
                }
            }
            else {
                returnCode = DEM_STATUS_FAILED;
            }
        }
        uint8 mask = 0xFFU;
        if( (DEM_STATUS_OK == returnCode) && (DTCClass->NofEvents > 1u) ) {
            /* This is a combined DTC. Bits have already been OR-ed above. Now we should and bits. */
            /* @req DEM441 */
            mask = ((temp & (1u << 5u)) >> 1u) | ((temp & (1u << 1u)) << 5u);
            mask = (uint8)((~mask) & 0xFFu);
        }
        *status = (temp & mask);
    } else {
        /* Event has no DTC or DTC is suppressed */
        /* @req 4.2.2/SWS_Dem_01100 *//* @req DEM587 */
        returnCode = DEM_STATUS_WRONG_DTC;
    }

    SchM_Exit_Dem_EA_0();

    return returnCode;
}


/*
 * Procedure:   Dem_GetNumberOfFilteredDtc
 * Reentrant:   No
 */
Dem_ReturnGetNumberOfFilteredDTCType Dem_GetNumberOfFilteredDtc(uint16 *numberOfFilteredDTC) {

    uint16 numberOfFaults = 0;
    Dem_ReturnGetNumberOfFilteredDTCType returnCode = DEM_NUMBER_OK;

    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETNUMBEROFFILTEREDDTC_ID, DEM_E_UNINIT, DEM_NUMBER_FAILED);
    VALIDATE_RV(NULL != numberOfFilteredDTC, DEM_GETNUMBEROFFILTEREDDTC_ID, DEM_E_PARAM_POINTER, DEM_NUMBER_FAILED);
    SchM_Enter_Dem_EA_0();
    const Dem_DTCClassType *DTCClass = configSet->DTCClass;
    Dem_EventStatusExtendedType DTCStatus;
    /* Find all DTCs matching filter. Ignore suppressed DTCs *//* @req DEM587 *//* @req 4.2.2/SWS_Dem_01101 */
    while( FALSE == DTCClass->Arc_EOL ) {
        if( TRUE == matchDTCWithDtcFilter(DTCClass, &DTCStatus) ) {
            numberOfFaults++;
        }
        DTCClass++;
    }

    *numberOfFilteredDTC = numberOfFaults; /** @req DEM061 */

    SchM_Exit_Dem_EA_0();
    return returnCode;
}


/*
 * Procedure:   Dem_GetNextFilteredDTC
 * Reentrant:   No
 */
Dem_ReturnGetNextFilteredDTCType Dem_GetNextFilteredDTC(uint32 *dtc, Dem_EventStatusExtendedType *dtcStatus)
{
    Dem_ReturnGetNextFilteredDTCType returnCode = DEM_FILTERED_OK;
    boolean dtcFound = FALSE;
    Dem_EventStatusExtendedType DTCStatus;

    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETNEXTFILTEREDDTC_ID, DEM_E_UNINIT, DEM_FILTERED_NO_MATCHING_DTC);
    VALIDATE_RV(NULL != dtc, DEM_GETNEXTFILTEREDDTC_ID, DEM_E_PARAM_POINTER, DEM_FILTERED_NO_MATCHING_DTC);
    VALIDATE_RV(NULL != dtcStatus, DEM_GETNEXTFILTEREDDTC_ID, DEM_E_PARAM_POINTER, DEM_FILTERED_NO_MATCHING_DTC);

    SchM_Enter_Dem_EA_0();

    /* Find the next DTC matching filter. Ignore suppressed DTCs *//* @req DEM587 *//* @req 4.2.2/SWS_Dem_01101 */
    /* @req DEM217 */
    const Dem_DTCClassType *DTCClass = &configSet->DTCClass[dtcFilter.DTCIndex];
    while( (dtcFound == FALSE) && (FALSE == DTCClass->Arc_EOL) ) {
        if( TRUE == matchDTCWithDtcFilter(DTCClass, &DTCStatus) ) {
            if( DEM_DTC_FORMAT_UDS == dtcFilter.dtcFormat ) {
                *dtc = DTCClass->DTCRef->UDSDTC; /** @req DEM216 */
            }
            else {
                *dtc = TO_OBD_FORMAT(DTCClass->DTCRef->OBDDTC);
            }
            *dtcStatus = DTCStatus;
            dtcFound = TRUE;
        }
        dtcFilter.DTCIndex++;
        DTCClass++;
    }

    if( FALSE == dtcFound ) {
        dtcFilter.DTCIndex = 0u;
        returnCode = DEM_FILTERED_NO_MATCHING_DTC;
    }

    SchM_Exit_Dem_EA_0();
    return returnCode;
}


/*
 * Procedure:   Dem_GetTranslationType
 * Reentrant:   No
 */
Dem_DTCTranslationFormatType Dem_GetTranslationType(void)
{
    return DEM_TYPE_OF_DTC_SUPPORTED; /** @req DEM231 */
}

/*
 * Procedure:   Dem_ClearDTC
 * Comment:     Stating the dtcOrigin makes no since when reading the reqiurements in the specification.
 * Reentrant:   No
 */
Dem_ReturnClearDTCType Dem_ClearDTC(uint32 dtc, Dem_DTCFormatType dtcFormat, Dem_DTCOriginType dtcOrigin) /** @req DEM009 *//** @req DEM241 */
{
    Dem_ReturnClearDTCType returnCode = DEM_CLEAR_WRONG_DTCORIGIN;
    const Dem_EventParameterType *eventParam;
    Dem_EventStatusExtendedType oldStatus;
    boolean dataDeleted;
#ifdef DEM_USE_MEMORY_FUNCTIONS
    boolean allClearOK = TRUE;
#endif
    (void)dtcOrigin;
#if defined(DEM_USE_INDICATORS) && defined(DEM_USE_MEMORY_FUNCTIONS)
    boolean indicatorsChanged = FALSE;
#endif
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_CLEARDTC_ID, DEM_E_UNINIT, DEM_CLEAR_FAILED);
    VALIDATE_RV(IS_VALID_DTC_FORMAT(dtcFormat), DEM_CLEARDTC_ID, DEM_E_PARAM_DATA, DEM_CLEAR_FAILED);


    for (uint16 i = 0; i < DEM_MAX_NUMBER_EVENT; i++) {
       SchM_Enter_Dem_EA_0();
        dataDeleted = FALSE;
        if ((DEM_EVENT_ID_NULL != eventStatusBuffer[i].eventId) && (NULL != eventStatusBuffer[i].eventParamRef)) {
            eventParam = eventStatusBuffer[i].eventParamRef;
            if ((DEM_CLEAR_ALL_EVENTS == STD_ON) || (eventParam->DTCClassRef != NULL)) {/*lint !e506 !e774*/
                if (checkDtcGroup(dtc, eventParam, dtcFormat) == TRUE) {
                    if( eventParam->EventClass->EventDestination == dtcOrigin ) {
                        if(FALSE == eventDTCRecordDataUpdateDisabled(eventParam)) {
                            if( clearEventAllowed(eventParam) == TRUE) {
                                boolean dtcOriginFound = FALSE;
                                oldStatus = eventStatusBuffer[i].eventStatusExtended;
#if defined(DEM_EVENT_COMBINATION_DEM_EVCOMB_TYPE1)
                                dataDeleted = DeleteDTCData(eventParam, TRUE, &dtcOriginFound, (DEM_COMBINED_EVENT_NO_DTC_ID != eventParam->CombinedDTCCID));/* @req DEM343 */
#else
                                dataDeleted = DeleteDTCData(eventParam, TRUE, &dtcOriginFound, FALSE);/* @req DEM343 */
#endif
                                if (dtcOriginFound == FALSE) {
                                    returnCode = DEM_CLEAR_WRONG_DTCORIGIN;
                                    DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_CLEARDTC_ID, DEM_E_NOT_IMPLEMENTED_YET);
                                } else {
#if defined(DEM_USE_INDICATORS)
                                    if( TRUE == resetIndicatorCounters(eventParam) ) {
#ifdef DEM_USE_MEMORY_FUNCTIONS
                                        indicatorsChanged = TRUE;
#endif
                                    }
#endif
#if defined(USE_DEM_EXTENSION)
                                    Dem_Extension_ClearEvent(eventParam);
#endif

                                    if( dataDeleted == TRUE) {
                                        /* @req DEM475 */
                                        notifyEventDataChanged(eventParam);
                                    }
                                    if( oldStatus != eventStatusBuffer[i].eventStatusExtended ) {
                                        /* @req DEM016 */
                                        notifyEventStatusChange(eventParam, oldStatus, eventStatusBuffer[i].eventStatusExtended);
                                    }
                                    if( NULL != eventParam->CallbackInitMforE ) {
                                        /* @req DEM376 */
                                        (void)eventParam->CallbackInitMforE(DEM_INIT_MONITOR_CLEAR);
                                    }
                                    /* Have cleared at least one, OK */
                                    returnCode = DEM_CLEAR_OK;
                                }
                            } else {
                                returnCode = DEM_CLEAR_FAILED; /* CallbackClearEventAllowed returned not allowed to clear */
#ifdef DEM_USE_MEMORY_FUNCTIONS
                                /* Clear was not allowed */
                                allClearOK = FALSE;
#endif
                            }
                        }
                    }
                } else {
                    if( (((DEM_DTC_FORMAT_UDS == dtcFormat) && (dtc == eventParam->DTCClassRef->DTCRef->UDSDTC)) ||
                            ((DEM_DTC_FORMAT_OBD == dtcFormat) && (dtc == TO_OBD_FORMAT(eventParam->DTCClassRef->DTCRef->OBDDTC))))
                            && (DTCIsAvailable(eventParam->DTCClassRef) == FALSE) ) {
                        /* This DTC is suppressed *//* @req 4.2.2/SWS_Dem_01101 */
                        returnCode = DEM_CLEAR_WRONG_DTC;
                    }
                }
            }
        } else {
            // Fatal error, no event parameters found for the event!
            DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_CLEARDTC_ID, DEM_E_UNEXPECTED_EXECUTION);
        }
    SchM_Exit_Dem_EA_0();
    }


  SchM_Enter_Dem_EA_0();
#ifdef DEM_USE_MEMORY_FUNCTIONS
#if defined(DEM_USE_INDICATORS)
    if( indicatorsChanged == TRUE ) {
        /* IMPROVEMENT: Immediate storage when deleting? */
        Dem_NvM_SetIndicatorBlockChanged(FALSE);
    }
#endif
    if( (DEM_DTC_GROUP_ALL_DTCS == dtc) && (allClearOK == TRUE)) {
        /* @req DEM399 */
        setOverflowIndication(dtcOrigin, FALSE);
    }
#endif

    SchM_Exit_Dem_EA_0();

    return returnCode;
}


/*
 * Procedure:   Dem_DisableDTCStorage
 * Reentrant:   No
 */
Dem_ReturnControlDTCStorageType Dem_DisableDTCSetting(Dem_DTCGroupType dtcGroup, Dem_DTCKindType dtcKind) /** @req DEM035 */
{
    Dem_ReturnControlDTCStorageType returnCode = DEM_CONTROL_DTC_STORAGE_OK;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_DISABLEDTCSETTING_ID, DEM_E_UNINIT, DEM_CONTROL_DTC_STORAGE_N_OK);
    // Check dtcGroup parameter
    uint32 DTCGroupLower;
    uint32 DTCGroupUpper;
    if ( (dtcGroup == DEM_DTC_GROUP_ALL_DTCS) || (DEM_DTC_GROUP_EMISSION_REL_DTCS == dtcGroup) || (TRUE == dtcIsGroup(dtcGroup, DEM_DTC_FORMAT_UDS, &DTCGroupLower, &DTCGroupUpper))) {
        // Check dtcKind parameter
        if ((dtcKind == DEM_DTC_KIND_ALL_DTCS) || (dtcKind ==  DEM_DTC_KIND_EMISSION_REL_DTCS)) {
            /** @req DEM079 */
            disableDtcSetting.dtcGroup = dtcGroup;
            disableDtcSetting.dtcKind = dtcKind;
            disableDtcSetting.settingDisabled = TRUE;
        } else {
            returnCode = DEM_CONTROL_DTC_STORAGE_N_OK;
        }
    } else {
        returnCode = DEM_CONTROL_DTC_WRONG_DTCGROUP;
    }

    return returnCode;
}


/*
 * Procedure:   Dem_EnableDTCStorage
 * Reentrant:   No
 */
Dem_ReturnControlDTCStorageType Dem_EnableDTCSetting(Dem_DTCGroupType dtcGroup, Dem_DTCKindType dtcKind)
{
    Dem_ReturnControlDTCStorageType returnCode = DEM_CONTROL_DTC_STORAGE_OK;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_ENABLEDTCSETTING_ID, DEM_E_UNINIT, DEM_CONTROL_DTC_STORAGE_N_OK);

    // NOTE: Behavior is not defined if group or kind do not match active settings, therefore the filter is just switched off.
    (void)dtcGroup; (void)dtcKind;  // Just to make get rid of PC-Lint warnings
    disableDtcSetting.settingDisabled = FALSE; /** @req DEM080 */

    return returnCode;
}



/*
 * Procedure:   Dem_GetExtendedDataRecordByDTC
 * Reentrant:   No
 */
