#include "../dem/dem_cfg.h"
#include "../dem/dem_event_logger.h" /* MISRA: Externs must be included via header */
#include <stdint.h>

/* -------------------------------------------------------------------------
 * Routine 0x0100: RunSelfDiag
 * Required: Session 03 (Extended), No Security
 * ------------------------------------------------------------------------- */
static uint8_t Routine_RunSelfDiag(uint8_t currentSession) {
    uint8_t status = 0x00U;

    /* ISO Standard: Extended Session (0x03) used for hardware diagnostics */
    if (currentSession != 0x03U) {
        status = 0x7FU; 
    } else {
        /* [Hardware UART Log]: Executing RAM & EEPROM Stuck-bit Tests... */
    }

    return status; 
}

/* -------------------------------------------------------------------------
 * Routine 0x0101: ClearEventLog
 * Required: Session 03 (Extended), Security Level 1
 * ------------------------------------------------------------------------- */
static uint8_t Routine_ClearEventLog(uint8_t currentSession, uint8_t securityLevel) {
    uint8_t status = 0x00U;

    /* ISO Standard: Extended Session (0x03) used for diagnostic management */
    if (currentSession != 0x03U) {
        status = 0x7FU; 
    } else if (securityLevel != 0x01U) {
        status = 0x33U;  
    } else {
        Dem_EventLogger_Clear();
        /* [Hardware UART Log]: Event Log Cleared successfully. */
    }

    return status; 
}

/* -------------------------------------------------------------------------
 * Routine 0x0102: TestBuzzer
 * Required: Session 03 (Extended), No Security
 * ------------------------------------------------------------------------- */
static uint8_t Routine_TestBuzzer(uint8_t currentSession, uint8_t subFunction) {
    uint8_t status = 0x00U;

    if (currentSession != 0x03U) {
        status = 0x7FU; 
    } else {
        if (subFunction == 0x01U) {
            /* [Hardware UART Log]: Buzzer HW activated. */
        } else if (subFunction == 0x02U) {
            /* [Hardware UART Log]: Buzzer HW deactivated. */
        } else {
            status = 0x12U; /* NRC: SubFunction Not Supported */
        }
    }

    return status; 
}

/* -------------------------------------------------------------------------
 * Routine 0xFF01 (Example): Memory Erase / Flash 
 * Required: Session 02 (Programming), Security Level 1
 * ------------------------------------------------------------------------- */
static uint8_t Routine_MemoryErase(uint8_t currentSession, uint8_t securityLevel) {
    uint8_t status = 0x00U;

    /* ISO Standard: Programming Session (0x02) required for memory modifications */
    if (currentSession != 0x02U) {
        status = 0x7FU;
    } else if (securityLevel != 0x01U) {
        status = 0x33U;
    } else {
        /* [Hardware UART Log]: Memory Erase initialized. */
    }

    return status;
}

/* -------------------------------------------------------------------------
 * ROUTINE DISPATCHER
 * ------------------------------------------------------------------------- */
uint8_t Dcm_ExecuteRoutine(uint16_t routineId, uint8_t subFunction, uint8_t currentSession, uint8_t securityLevel) {
    uint8_t status;

    switch (routineId) {
        case 0x0100U: 
        case 0xFF00U: /* Map both to Self-Diag */
            status = Routine_RunSelfDiag(currentSession);
            break;
            
        case 0x0101U: 
            status = Routine_ClearEventLog(currentSession, securityLevel);
            break;
            
        case 0x0102U: 
            status = Routine_TestBuzzer(currentSession, subFunction);
            break;
            
        case 0xFF01U: /* Memory Modification */
            status = Routine_MemoryErase(currentSession, securityLevel);
            break;
            
        case 0x0103U: /* Reset Comms Stats */
            /* Extended session for stats reset */
            if (currentSession != 0x03U) {
                status = 0x7FU;
            } else if (securityLevel != 0x01U) {
                status = 0x33U;
            } else {
                /* [Hardware UART Log]: Diagnostic counters reset. */
                status = 0x00U;
            }
            break;

        default:     
            status = 0x31U; /* NRC: Request Out Of Range */
            break;
    }

    return status;
}