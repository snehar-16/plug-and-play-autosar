void Dem_MainFunction(void)/** @req DEM125 */
{
    VALIDATE_NO_RV(DEM_UNINITIALIZED != demState, DEM_MAINFUNCTION_ID, DEM_E_UNINIT);

#ifdef DEM_USE_MEMORY_FUNCTIONS
    Dem_NvM_MainFunction();
#endif /* DEM_USE_MEMORY_FUNCTIONS */
#if defined(USE_DEM_EXTENSION)
    Dem_Extension_MainFunction();
#endif

#if defined(DEM_USE_TIME_BASE_PREDEBOUNCE)
    /* Handle time based predebounce */
    TimeBasedDebounceMainFunction();
#endif
}

/**
 * Gets the indicator status derived from the event status
 * @param IndicatorId
 * @param IndicatorStatus
 * @return E_OK: Operation was successful, E_NOT_OK: Operation failed or is not supported
 */
/*lint -efunc(818,Dem_GetIndicatorStatus) Dem_IndicatorStatusType cannot be declared as pointing to const as API defined by AUTOSAR  */
Std_ReturnType Dem_GetIndicatorStatus( uint8 IndicatorId, Dem_IndicatorStatusType* IndicatorStatus )
{
    /* @req DEM046 */
    /* @req DEM508 */
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETINDICATORSTATUS_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV(NULL != IndicatorStatus, DEM_GETINDICATORSTATUS_ID, DEM_E_PARAM_POINTER, E_NOT_OK);
#if defined(DEM_USE_INDICATORS)
    VALIDATE_RV(DEM_NOF_INDICATORS > IndicatorId, DEM_GETINDICATORSTATUS_ID, DEM_E_PARAM_CONFIG, E_NOT_OK);
    Std_ReturnType ret = E_NOT_OK;
    const Dem_IndicatorType *indConfig;
    const Dem_EventParameterType *eventParam;
    uint8 currPrio = 0xff;
    if( IndicatorId < DEM_NOF_INDICATORS  ) {
        indConfig = &configSet->Indicators[IndicatorId];
        *IndicatorStatus = DEM_INDICATOR_OFF;
        for( uint8 indx = 0; indx < indConfig->EventListSize; indx++ ) {
            eventParam = NULL;
            lookupEventIdParameter(indConfig->EventList[indx], &eventParam);
            if( (NULL != eventParam) && (NULL != eventParam->EventClass->IndicatorAttribute) && (TRUE == (boolean)(*eventParam->EventClass->IndicatorAttribute->IndicatorValid)) ) {
                const Dem_IndicatorAttributeType *indAttrPtr = eventParam->EventClass->IndicatorAttribute;
                while( FALSE == indAttrPtr->Arc_EOL ) {
                    if( indAttrPtr->IndicatorId == IndicatorId ) {
                        /* Found a match */
                        ret = E_OK;
                        if( TRUE == indicatorFailFulfilled(eventParam, indAttrPtr) ) {
                            if( eventParam->EventClass->EventPriority < currPrio ) {
                                *IndicatorStatus = indAttrPtr->IndicatorBehaviour;
                                currPrio = eventParam->EventClass->EventPriority;
                            }
                        }
                    }
                    indAttrPtr++;
                }
            }
        }
    }
    return ret;
#else
    (void)IndicatorId;
    DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_GETINDICATORSTATUS_ID, DEM_E_PARAM_CONFIG);
    return E_NOT_OK;
#endif
}

/***************************************************
 * Interface SW-Components via RTE <-> DEM (8.3.3) *
 ***************************************************/

/*
 * Procedure:   Dem_SetEventStatus
 * Reentrant:   Yes
 */
/* @req DEM183 */
Std_ReturnType Dem_SetEventStatus(Dem_EventIdType eventId, Dem_EventStatusType eventStatus) /** @req DEM330 */
{
    /* @req DEM330 */
	Std_ReturnType returnCode = E_NOT_OK;
	VALIDATE_RV(((DEM_INITIALIZED == demState) || (DEM_SHUTDOWN == demState)), DEM_SETEVENTSTATUS_ID, DEM_E_UNINIT, E_NOT_OK);
	VALIDATE_RV(IS_VALID_EVENT_STATUS(eventStatus), DEM_SETEVENTSTATUS_ID, DEM_E_PARAM_DATA, E_NOT_OK);
	// Ignore this API call after Dem_Shutdown()
	if (DEM_SHUTDOWN != demState) {
		SchM_Enter_Dem_EA_0();

		returnCode = handleEvent(eventId, eventStatus);

		SchM_Exit_Dem_EA_0();
	}
    return returnCode;
}


/*
 * Procedure:   Dem_ResetEventStatus
 * Reentrant:   Yes
 */
/* @req DEM185 */
Std_ReturnType Dem_ResetEventStatus(Dem_EventIdType eventId) /** @req DEM331 */
{
    /* @req DEM331 */
    Std_ReturnType returnCode;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_RESETEVENTSTATUS_ID, DEM_E_UNINIT, E_NOT_OK);

    SchM_Enter_Dem_EA_0();

    /* Function resetEventStatus will notify application if there is a change in the status bits */
    returnCode = resetEventStatus(eventId);


    SchM_Exit_Dem_EA_0();
    return returnCode;
}


/*
 * Procedure:   Dem_GetEventStatus
 * Reentrant:   Yes
 */
Std_ReturnType Dem_GetEventStatus(Dem_EventIdType eventId, Dem_EventStatusExtendedType *eventStatusExtended)
{
    Std_ReturnType returnCode;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETEVENTSTATUS_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV(NULL != eventStatusExtended, DEM_GETEVENTSTATUS_ID, DEM_E_PARAM_POINTER, E_NOT_OK);
    SchM_Enter_Dem_EA_0();

    returnCode = getEventStatus(eventId, eventStatusExtended);

    SchM_Exit_Dem_EA_0();

    return returnCode;
}


/*
 * Procedure:   Dem_GetEventFailed
 * Reentrant:   Yes
 */
Std_ReturnType Dem_GetEventFailed(Dem_EventIdType eventId, boolean *eventFailed) /** @req DEM333 */
{
    /* @req DEM333 */
    Std_ReturnType returnCode;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETEVENTFAILED_ID, DEM_E_UNINIT, E_NOT_OK);

    SchM_Enter_Dem_EA_0();

    returnCode = getEventFailed(eventId, eventFailed);

    SchM_Exit_Dem_EA_0();

    return returnCode;
}


/*
 * Procedure:   Dem_GetEventTested
 * Reentrant:   Yes
 */
Std_ReturnType Dem_GetEventTested(Dem_EventIdType eventId, boolean *eventTested)
{
    /* @req DEM333 */
    Std_ReturnType returnCode;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETEVENTTESTED_ID, DEM_E_UNINIT, E_NOT_OK);

    SchM_Enter_Dem_EA_0();

    returnCode = getEventTested(eventId, eventTested);

    SchM_Exit_Dem_EA_0();

    return returnCode;
}


/*
 * Procedure:   Dem_GetFaultDetectionCounter
 * Reentrant:   No
 */
Std_ReturnType Dem_GetFaultDetectionCounter(Dem_EventIdType eventId, sint8 *counter)
{
    /* @req DEM204 */
    Std_ReturnType returnCode;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETFAULTDETECTIONCOUNTER_ID, DEM_E_UNINIT, E_NOT_OK);
    SchM_Enter_Dem_EA_0();

    returnCode = getFaultDetectionCounter(eventId, counter);

    SchM_Exit_Dem_EA_0();
    return returnCode;
}


/*
 * Procedure:   Dem_SetOperationCycleState
 * Reentrant:   No
 */
Std_ReturnType Dem_SetOperationCycleState(Dem_OperationCycleIdType operationCycleId, Dem_OperationCycleStateType cycleState)
{
    Std_ReturnType returnCode = E_OK;
    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_SETOPERATIONCYCLESTATE_ID, DEM_E_UNINIT, E_NOT_OK);
    SchM_Enter_Dem_EA_0();

    if( DEM_ACTIVE == operationCycleId ) {
        /* Handled internally */
        DET_REPORTERROR(DEM_MODULE_ID, 0, DEM_SETOPERATIONCYCLESTATE_ID, DEM_E_UNEXPECTED_EXECUTION);
        returnCode = E_NOT_OK;
    } else {
        returnCode = setOperationCycleState(operationCycleId, cycleState);
    }

    SchM_Exit_Dem_EA_0();
    return returnCode;
}


/*
 * Procedure:   Dem_GetDTCOfEvent
 * Reentrant:   Yes
 */
Std_ReturnType Dem_GetDTCOfEvent(Dem_EventIdType eventId, Dem_DTCFormatType dtcFormat, uint32* dtcOfEvent)
{
    Std_ReturnType returnCode = E_NO_DTC_AVAILABLE;
    const Dem_EventParameterType *eventParam;
    EventStatusRecType * eventStatusRec;

    VALIDATE_RV(DEM_INITIALIZED == demState, DEM_GETDTCOFEVENT_ID, DEM_E_UNINIT, E_NOT_OK);
    VALIDATE_RV(IS_VALID_DTC_FORMAT(dtcFormat), DEM_GETDTCOFEVENT_ID, DEM_E_PARAM_DATA, E_NOT_OK);

    SchM_Enter_Dem_EA_0();

    lookupEventIdParameter(eventId, &eventParam);
    lookupEventStatusRec(eventId, &eventStatusRec);
    if ( (eventParam != NULL) && (NULL != eventStatusRec) && (TRUE == eventStatusRec->isAvailable)) {
        if ((eventParam->DTCClassRef != NULL) && (TRUE == eventParam->DTCClassRef->DTCRef->DTCUsed)) {
            if( TRUE == eventHasDTCOnFormat(eventParam, dtcFormat) ) {
                *dtcOfEvent = (DEM_DTC_FORMAT_UDS == dtcFormat) ? eventParam->DTCClassRef->DTCRef->UDSDTC : TO_OBD_FORMAT(eventParam->DTCClassRef->DTCRef->OBDDTC);/** @req DEM269 */
                returnCode = E_OK;
            }
        }
    } else {
        // Event Id not found
        returnCode = E_NOT_OK;
    }

    SchM_Exit_Dem_EA_0();
    return returnCode;
}


/********************************************
 * Interface BSW-Components <-> DEM (8.3.4) *
 ********************************************/

/*
 * Procedure:   Dem_ReportErrorStatus
 * Reentrant:   Yes
 */
void Dem_ReportErrorStatus( Dem_EventIdType eventId, Dem_EventStatusType eventStatus ) /** @req DEM206 */
{
    /* @req DEM330 */
    /* @req DEM107 */
    VALIDATE_NO_RV((DEM_UNINITIALIZED != demState), DEM_REPORTERRORSTATUS_ID, DEM_E_UNINIT);
    VALIDATE_NO_RV(IS_VALID_EVENT_STATUS(eventStatus), DEM_REPORTERRORSTATUS_ID, DEM_E_PARAM_DATA);

    SchM_Enter_Dem_EA_0();

    switch (demState) {
        case DEM_PREINITIALIZED:
            // Update status and check if is to be stored
            if ((eventStatus == DEM_EVENT_STATUS_PASSED) || (eventStatus == DEM_EVENT_STATUS_FAILED)) {
                handlePreInitEvent(eventId, eventStatus); /** @req DEM167 */
            }
            break;

        case DEM_INITIALIZED:
            (void)handleEvent(eventId, eventStatus);
            break;

        case DEM_SHUTDOWN:
        default:
            // Ignore api call
            break;

    } // switch (demState)

    SchM_Exit_Dem_EA_0();
}

/*********************************
 * Interface DCM <-> DEM (8.3.5) *
 *********************************/
/*
 * Procedure:   Dem_GetDTCStatusAvailabilityMask
 * Reentrant:   No
 */
/*lint -esym(793, Dem_GetDTCStatusAvailabilityMask) Function name defined by AUTOSAR. */
