#include "../dem/dem_cfg.h"
#include <stdio.h>
#include <stdint.h>

/* Forward declaration for external DEM event logger clear */
extern void Dem_EventLogger_Clear(void);

/* -------------------------------------------------------------------------
 * Routine 0x0100: RunSelfDiag
 * Required: Session 03 (Extended), No Security
 * ------------------------------------------------------------------------- */
static uint8_t Routine_RunSelfDiag(uint8_t currentSession) {
    /* ISO Standard: Extended Session (0x03) used for hardware diagnostics */
    if (currentSession != 0x03) return 0x7F; 
    
    printf("[Routine] Executing RAM & EEPROM Stuck-bit Tests...\n");
    return 0x00; 
}

/* -------------------------------------------------------------------------
 * Routine 0x0101: ClearEventLog
 * Required: Session 03 (Extended), Security Level 1
 * ------------------------------------------------------------------------- */
static uint8_t Routine_ClearEventLog(uint8_t currentSession, uint8_t securityLevel) {
    /* ISO Standard: Extended Session (0x03) used for diagnostic management */
    if (currentSession != 0x03) return 0x7F; 
    if (securityLevel != 0x01) return 0x33;  
    
    Dem_EventLogger_Clear();
    printf("[Routine] Event Log Cleared successfully.\n");
    return 0x00; 
}

/* -------------------------------------------------------------------------
 * Routine 0x0102: TestBuzzer
 * Required: Session 03 (Extended), No Security
 * ------------------------------------------------------------------------- */
static uint8_t Routine_TestBuzzer(uint8_t currentSession, uint8_t subFunction) {
    if (currentSession != 0x03) return 0x7F; 
    
    if (subFunction == 0x01) {
        printf("[Routine] Buzzer HW activated.\n");
    } else if (subFunction == 0x02) {
        printf("[Routine] Buzzer HW deactivated.\n");
    } else {
        return 0x12; /* NRC: SubFunction Not Supported */
    }
    return 0x00; 
}

/* -------------------------------------------------------------------------
 * Routine 0xFF01 (Example): Memory Erase / Flash 
 * Required: Session 02 (Programming), Security Level 1
 * ------------------------------------------------------------------------- */
static uint8_t Routine_MemoryErase(uint8_t currentSession, uint8_t securityLevel) {
    /* ISO Standard: Programming Session (0x02) required for memory modifications */
    if (currentSession != 0x02) return 0x7F;
    if (securityLevel != 0x01) return 0x33;
    
    printf("[Routine] Memory Erase initialized.\n");
    return 0x00;
}

/* -------------------------------------------------------------------------
 * ROUTINE DISPATCHER
 * ------------------------------------------------------------------------- */
uint8_t Dcm_ExecuteRoutine(uint16_t routineId, uint8_t subFunction, uint8_t currentSession, uint8_t securityLevel) {
    switch (routineId) {
        case 0x0100: 
        case 0xFF00: /* Map both to Self-Diag */
            return Routine_RunSelfDiag(currentSession);
            
        case 0x0101: 
            return Routine_ClearEventLog(currentSession, securityLevel);
            
        case 0x0102: 
            return Routine_TestBuzzer(currentSession, subFunction);
            
        case 0xFF01: /* Memory Modification */
            return Routine_MemoryErase(currentSession, securityLevel);
            
        case 0x0103: /* Reset Comms Stats */
            /* Extended session for stats reset */
            if (currentSession != 0x03) return 0x7F;
            if (securityLevel != 0x01) return 0x33;
            printf("[Routine] Diagnostic counters reset.\n");
            return 0x00;

        default:     
            return 0x31; /* NRC: Request Out Of Range */
    }
}