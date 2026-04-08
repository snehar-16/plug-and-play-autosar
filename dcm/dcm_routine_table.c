#include "../dem/dem_cfg.h"
#include <stdio.h>

/* Forward declaration for external DEM event logger clear */
extern void Dem_EventLogger_Clear(void);

/* -------------------------------------------------------------------------
 * Routine 0x0100 (Merged with 0x0201): RunSelfDiag & Stuck-Bit Test
 * Required: Session 02 (Extended), No Security
 * ------------------------------------------------------------------------- */
uint8_t Routine_RunSelfDiag(uint8_t currentSession) {
    if (currentSession != 0x02) return 0x7F; /* NRC: Service Not Supported in Active Session */
    
    printf("[Routine] Executing RAM & EEPROM Stuck-bit Tests...\n");
    return 0x00; /* Positive Response */
}

/* -------------------------------------------------------------------------
 * Routine 0x0101 (Merged with 0xFF00): ClearEventLog
 * Required: Session 02 (Extended), Security Level 1
 * ------------------------------------------------------------------------- */
uint8_t Routine_ClearEventLog(uint8_t currentSession, uint8_t securityLevel) {
    if (currentSession != 0x02) return 0x7F; /* NRC: Service Not Supported in Active Session */
    if (securityLevel != 0x01) return 0x33;  /* NRC: Security Access Denied */
    
    Dem_EventLogger_Clear();
    return 0x00; /* Positive Response */
}

/* -------------------------------------------------------------------------
 * Routine 0x0102: TestBuzzer
 * Required: Session 02 (Extended), No Security
 * ------------------------------------------------------------------------- */
uint8_t Routine_TestBuzzer(uint8_t currentSession, uint8_t subFunction) {
    if (currentSession != 0x02) return 0x7F; /* NRC: Service Not Supported in Active Session */
    
    if (subFunction == 0x01) {
        printf("[Routine] Buzzer HW activated.\n");
    } else if (subFunction == 0x02) {
        printf("[Routine] Buzzer HW deactivated.\n");
    } else {
        return 0x12; /* NRC: SubFunction Not Supported */
    }
    return 0x00; /* Positive Response */
}

/* -------------------------------------------------------------------------
 * Routine 0x0103: ResetCommsStats
 * Required: Session 03 (Programming), Security Level 1
 * ------------------------------------------------------------------------- */
uint8_t Routine_ResetCommsStats(uint8_t currentSession, uint8_t securityLevel) {
    if (currentSession != 0x03) return 0x7F; /* NRC: Service Not Supported in Active Session */
    if (securityLevel != 0x01) return 0x33;  /* NRC: Security Access Denied */
    
    printf("[Routine] TCP and RS485 diagnostic counters reset to 0.\n");
    return 0x00; /* Positive Response */
}

/* -------------------------------------------------------------------------
 * ROUTINE DISPATCHER
 * ------------------------------------------------------------------------- */
uint8_t Dcm_ExecuteRoutine(uint16_t routineId, uint8_t subFunction, uint8_t currentSession, uint8_t securityLevel) {
    switch (routineId) {
        case 0x0100: 
            return Routine_RunSelfDiag(currentSession);
        case 0x0101: 
            return Routine_ClearEventLog(currentSession, securityLevel);
        case 0x0102: 
            return Routine_TestBuzzer(currentSession, subFunction);
        case 0x0103: 
            return Routine_ResetCommsStats(currentSession, securityLevel);
        default:     
            return 0x31; /* NRC: Request Out Of Range */
    }
}  