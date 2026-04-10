#include "Dcm_Cfg.h"
#include "../dem/dem_cfg.h"
#include <string.h>
#include <stdio.h>

/* --- Global State --- */
static uint8_t CurrentSession = 0x01; 
static uint8_t SecurityLevel = 0x00;  /* 0x00: Locked, 0x01: Unlocked */
static uint32_t S3_Timer = 0;         /* ISO 14229-1 S3 Server Timer Tracker */

extern uint8_t Dcm_ExecuteRoutine(uint16_t routineId, uint8_t subFunction, uint8_t currentSession, uint8_t securityLevel);

static void Dcm_SendNRC(uint8_t sid, uint8_t nrc, uint8_t *txData, uint16_t *txLen) {
    txData[0] = 0x7F; txData[1] = sid; txData[2] = nrc; *txLen = 3;
}

/* ==========================================================================
 * ISO 14229-1 TIMING & BACKGROUND TASKS
 * ========================================================================== */

/**
 * @brief Manages the S3 Server Timer for non-default sessions.
 * Should be called periodically by the OS or Main Loop.
 */
void Dcm_ManageSessionTimer(uint32_t elapsed_ms) {
    /* Only track time if we are in Programming (02) or Extended (03) sessions */
    if (CurrentSession != 0x01) {
        S3_Timer += elapsed_ms;
        
        if (S3_Timer >= DCM_TIMING_S3_SERVER) {
            printf("\n\n[ALARM] S3 Server Timer Expired (%d ms)!\n", DCM_TIMING_S3_SERVER);
            printf("[ALARM] Security Locked. Reverting to Default Session.\n");
            
            CurrentSession = 0x01;  /* Revert to Default Session */
            SecurityLevel = 0x00;   /* Revoke Security Access */
            S3_Timer = 0;           /* Reset the clock */
            
            printf("\nScanner Request > ");
            fflush(stdout);
        }
    }
}

/* ==========================================================================
 * INDIVIDUAL SERVICE HANDLERS
 * ========================================================================== */

static void Dcm_Handle_0x10(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 2) { Dcm_SendNRC(0x10, 0x13, tx, txLen); return; }
    uint8_t reqSession = rx[1];

    if (reqSession == 0x01 || reqSession == 0x02 || reqSession == 0x03) {
        CurrentSession = reqSession;
        SecurityLevel = 0x00; /* Lock security on session transition */
        tx[0] = 0x50; tx[1] = CurrentSession; *txLen = 2;
        printf("[DCM] Transitioned to Session 0x%02X\n", CurrentSession);
    } else { Dcm_SendNRC(0x10, 0x31, tx, txLen); }
}

static void Dcm_Handle_0x11(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    tx[0] = 0x51; tx[1] = rx[1]; *txLen = 2;
    printf("[System] Hard Reset Initialized.\n");
}

static void Dcm_Handle_0x19(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    tx[0] = 0x59; tx[1] = rx[1]; tx[2] = rx[2]; *txLen = 3;
}

/**
 * @brief Service 0x22: Read Data By Identifier (Strict Zero-Trust Whitelist)
 */
static void Dcm_Handle_0x22(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 3) { Dcm_SendNRC(0x22, 0x13, tx, txLen); return; }
    uint16_t did = (uint16_t)((rx[1] << 8) | rx[2]);
    uint8_t isValid = 0;

    /* 1. Session 0x01 (Default): STRICTLY Fault DIDs (F100 - F11D) */
    if (did == 0xF100 || did == 0xF101 || (did >= 0xF105 && did <= 0xF108) || 
        did == 0xF10B || did == 0xF10D || did == 0xF10E || 
        (did >= 0xF110 && did <= 0xF11D)) {
        isValid = 1;
    } 
    /* 2. Session 0x03 (Extended): STRICTLY Audit DIDs (F200 - F207) */
    else if (did >= 0xF200 && did <= 0xF207) {
        if (CurrentSession != 0x03) { 
            Dcm_SendNRC(0x22, 0x7F, tx, txLen); 
            return; 
        }
        isValid = 1;
    }
    /* 3. Session 0x02 (Programming): STRICTLY Config DIDs (F300 - F301) */
    else if (did == 0xF300 || did == 0xF301) {
        if (CurrentSession != 0x02) { 
            Dcm_SendNRC(0x22, 0x7F, tx, txLen); 
            return; 
        }
        isValid = 1;
    }

    if (!isValid) { 
        Dcm_SendNRC(0x22, 0x31, tx, txLen); 
        printf("[DCM] Read Denied: DID 0x%04X not in table or out of session.\n", did);
        return; 
    }

    /* Positive Response */
    tx[0] = 0x62; tx[1] = rx[1]; tx[2] = rx[2];
    memset(&tx[3], 0x00, 8); *txLen = 11;
}

static void Dcm_Handle_0x27(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rx[1] == 0x01) { 
        tx[0] = 0x67; tx[1] = 0x01; tx[2] = 0xAA; *txLen = 3; 
    } else if (rx[1] == 0x02) { 
        SecurityLevel = 0x01; tx[0] = 0x67; tx[1] = 0x02; *txLen = 2;
        printf("[Security] Memory Unlocked.\n");
    } else { Dcm_SendNRC(0x27, 0x12, tx, txLen); }
}

/**
 * @brief Service 0x2E: Write Data By Identifier (Strict Zero-Trust Whitelist)
 */
static void Dcm_Handle_0x2E(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 3) { Dcm_SendNRC(0x2E, 0x13, tx, txLen); return; }
    uint16_t did = (uint16_t)((rx[1] << 8) | rx[2]);

    /* Rule: Security must be unlocked */
    if (SecurityLevel == 0x00) { Dcm_SendNRC(0x2E, 0x33, tx, txLen); return; }

    /* ONLY F300 and F301 are allowed to be written to, and ONLY in Session 02 */
    if (did == 0xF300 || did == 0xF301) {
        if (CurrentSession != 0x02) { 
            Dcm_SendNRC(0x2E, 0x7F, tx, txLen); 
            return; 
        }
        tx[0] = 0x6E; tx[1] = rx[1]; tx[2] = rx[2]; *txLen = 3;
        printf("[DCM] Write Successful to Config DID %04X\n", did);
    } else {
        Dcm_SendNRC(0x2E, 0x31, tx, txLen);
        printf("[DCM] Write Denied: DID %04X is Read-Only or Invalid.\n", did);
    }
}

static void Dcm_Handle_0x2F(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (SecurityLevel == 0x00) { Dcm_SendNRC(0x2F, 0x33, tx, txLen); return; }
    tx[0] = 0x6F; tx[1] = rx[1]; tx[2] = rx[2]; *txLen = 3;
    printf("[IO Control] Hardware override engaged.\n");
}

static void Dcm_Handle_0x31(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    uint16_t rid = (uint16_t)((rx[1] << 8) | rx[2]);
    uint8_t sub = (rxLen > 3) ? rx[3] : 0x01;
    uint8_t nrc = Dcm_ExecuteRoutine(rid, sub, CurrentSession, SecurityLevel);
    if (nrc == 0x00) { tx[0] = 0x71; tx[1] = rx[1]; tx[2] = rx[2]; *txLen = 3; } 
    else { Dcm_SendNRC(0x31, nrc, tx, txLen); }
}

static void Dcm_Handle_Flash(uint8_t sid, uint8_t *rx, uint8_t *tx, uint16_t *txLen) {
    if (SecurityLevel == 0x00) { Dcm_SendNRC(sid, 0x33, tx, txLen); return; }
    tx[0] = sid + 0x40; tx[1] = rx[1]; *txLen = 2;
    printf("[Flash] Service %02X Processed.\n", sid);
}

/* ==========================================================================
 * CENTRAL GATEKEEPER
 * ========================================================================== */
void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen) {
    if (rxLen < 1) return;
    uint8_t sid = rxData[0];
    uint8_t isAllowed = 0;

    /* ISO 14229-1: Any valid diagnostic request resets the S3 Server Timer */
    S3_Timer = 0;

    switch (CurrentSession) {
        case 0x01: /* Default */
            if (sid == 0x10 || sid == 0x11 || sid == 0x19 || sid == 0x22 || sid == 0x3E) isAllowed = 1;
            break;
        case 0x02: /* Programming */
            if (sid == 0x10 || sid == 0x11 || sid == 0x27 || sid == 0x2E ||  sid == 0x31 || 
                sid == 0x34 || sid == 0x36 || sid == 0x37 || sid == 0x3E) isAllowed = 1;
            break;
        case 0x03: /* Extended */
            if (sid == 0x10 || sid == 0x22 || sid == 0x27 || sid == 0x2E || 
                sid == 0x2F || sid == 0x31 || sid == 0x85 || sid == 0x3E) isAllowed = 1;
            break;
    }

    if (!isAllowed) {
        Dcm_SendNRC(sid, 0x7F, txData, txLen);
        printf("[Gatekeeper] NRC 0x7F: SID %02X blocked in Session %02X\n", sid, CurrentSession);
        return;
    }

    switch (sid) {
        case 0x10: Dcm_Handle_0x10(rxData, rxLen, txData, txLen); break;
        case 0x11: Dcm_Handle_0x11(rxData, rxLen, txData, txLen); break;
        case 0x19: Dcm_Handle_0x19(rxData, rxLen, txData, txLen); break;
        case 0x22: Dcm_Handle_0x22(rxData, rxLen, txData, txLen); break;
        case 0x27: Dcm_Handle_0x27(rxData, rxLen, txData, txLen); break;
        case 0x2E: Dcm_Handle_0x2E(rxData, rxLen, txData, txLen); break;
        case 0x2F: Dcm_Handle_0x2F(rxData, rxLen, txData, txLen); break;
        case 0x31: Dcm_Handle_0x31(rxData, rxLen, txData, txLen); break;
        case 0x34: case 0x36: case 0x37: 
                   Dcm_Handle_Flash(sid, rxData, txData, txLen); break;
        case 0x3E: txData[0] = 0x7E; txData[1] = 0x00; *txLen = 2; break; 
        case 0x85: txData[0] = 0xC5; txData[1] = rxData[1]; *txLen = 2; break; 
        default:   Dcm_SendNRC(sid, 0x11, txData, txLen); break; 
    }
}

void Dcm_Init(void) { 
    CurrentSession = 0x01; 
    SecurityLevel = 0x00; 
    S3_Timer = 0; 
}