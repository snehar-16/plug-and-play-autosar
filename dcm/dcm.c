#include "Dcm_Cfg.h"
#include "../dem/dem_cfg.h"
#include <string.h>

/* MISRA Note: Externs should ideally be in a shared header (e.g., dcm_routine_table.h). */
extern uint8_t Dcm_ExecuteRoutine(uint16_t routineId, uint8_t subFunction, uint8_t currentSession, uint8_t securityLevel);

/* --- Global State --- */
static uint8_t CurrentSession = 0x01U; 
static uint8_t SecurityLevel = 0x00U;  /* 0x00U: Locked, 0x01U: Unlocked */
static uint32_t S3_Timer = 0U;         /* ISO 14229-1 S3 Server Timer Tracker */


static void Dcm_SendNRC(uint8_t sid, uint8_t nrc, uint8_t *txData, uint16_t *txLen) {
    if ((txData != NULL) && (txLen != NULL)) {
        txData[0] = 0x7FU; 
        txData[1] = sid; 
        txData[2] = nrc; 
        *txLen = 3U;
    }
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
    if (CurrentSession != 0x01U) {
        S3_Timer += elapsed_ms;
        
        if (S3_Timer >= DCM_TIMING_S3_SERVER) {
            /* [Hardware UART Log]: ALARM - S3 Server Timer Expired! */
            /* [Hardware UART Log]: ALARM - Security Locked. Reverting to Default Session. */
            
            CurrentSession = 0x01U;  /* Revert to Default Session */
            SecurityLevel = 0x00U;   /* Revoke Security Access */
            S3_Timer = 0U;           /* Reset the clock */
        }
    }
}

/* ==========================================================================
 * INDIVIDUAL SERVICE HANDLERS
 * ========================================================================== */

static void Dcm_Handle_0x10(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 2U) { 
        Dcm_SendNRC(0x10U, 0x13U, tx, txLen); 
    } else {
        uint8_t reqSession = rx[1];

        if ((reqSession == 0x01U) || (reqSession == 0x02U) || (reqSession == 0x03U)) {
            CurrentSession = reqSession;
            SecurityLevel = 0x00U; /* Lock security on session transition */
            tx[0] = 0x50U; 
            tx[1] = CurrentSession; 
            *txLen = 2U;
            /* [Hardware UART Log]: Transitioned to new Session */
        } else { 
            Dcm_SendNRC(0x10U, 0x31U, tx, txLen); 
        }
    }
}

static void Dcm_Handle_0x11(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    (void)rxLen; /* MISRA: Explicitly ignore unused parameter */
    tx[0] = 0x51U; 
    tx[1] = rx[1]; 
    *txLen = 2U;
    /* [Hardware UART Log]: Hard Reset Initialized. */
}

static void Dcm_Handle_0x19(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    (void)rxLen;
    tx[0] = 0x59U; 
    tx[1] = rx[1]; 
    tx[2] = rx[2]; 
    *txLen = 3U;
}

/**
 * @brief Service 0x22: Read Data By Identifier (Strict Zero-Trust Whitelist)
 */
static void Dcm_Handle_0x22(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 3U) { 
        Dcm_SendNRC(0x22U, 0x13U, tx, txLen); 
    } else {
        /* MISRA: Explicit casting required before shifting 8-bit values */
        uint16_t did = (uint16_t)(((uint16_t)rx[1] << 8U) | (uint16_t)rx[2]);
        uint8_t isValid = 0U;

        /* 1. Session 0x01 (Default): STRICTLY Fault DIDs (F100 - F11D) */
        if ((did == 0xF100U) || (did == 0xF101U) || ((did >= 0xF105U) && (did <= 0xF108U)) || 
            (did == 0xF10BU) || (did == 0xF10DU) || (did == 0xF10EU) || 
            ((did >= 0xF110U) && (did <= 0xF11DU))) {
            isValid = 1U;
        } 
        /* 2. Session 0x03 (Extended): STRICTLY Audit DIDs (F200 - F207) */
        else if ((did >= 0xF200U) && (did <= 0xF207U)) {
            if (CurrentSession == 0x03U) { 
                isValid = 1U;
            } else {
                Dcm_SendNRC(0x22U, 0x7FU, tx, txLen); 
            }
        }
        /* 3. Session 0x02 (Programming): STRICTLY Config DIDs (F300 - F301) */
        else if ((did == 0xF300U) || (did == 0xF301U)) {
            if (CurrentSession == 0x02U) { 
                isValid = 1U;
            } else {
                Dcm_SendNRC(0x22U, 0x7FU, tx, txLen); 
            }
        } else {
            /* MISRA demands all if/else chains terminate securely */
        }

        if (isValid == 0U) { 
            /* Only send NRC if we haven't already sent 0x7F above */
            if (tx[0] != 0x7FU) {
                Dcm_SendNRC(0x22U, 0x31U, tx, txLen); 
            }
            /* [Hardware UART Log]: Read Denied. */
        } else {
            /* Positive Response */
            tx[0] = 0x62U; 
            tx[1] = rx[1]; 
            tx[2] = rx[2];
            (void)memset(&tx[3], 0x00, 8U); 
            *txLen = 11U;
        }
    }
}

static void Dcm_Handle_0x27(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    (void)rxLen;
    if (rx[1] == 0x01U) { 
        tx[0] = 0x67U; tx[1] = 0x01U; tx[2] = 0xAAU; *txLen = 3U; 
    } else if (rx[1] == 0x02U) { 
        SecurityLevel = 0x01U; tx[0] = 0x67U; tx[1] = 0x02U; *txLen = 2U;
        /* [Hardware UART Log]: Security Memory Unlocked. */
    } else { 
        Dcm_SendNRC(0x27U, 0x12U, tx, txLen); 
    }
}

/**
 * @brief Service 0x2E: Write Data By Identifier (Strict Zero-Trust Whitelist)
 */
static void Dcm_Handle_0x2E(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 3U) { 
        Dcm_SendNRC(0x2EU, 0x13U, tx, txLen); 
    } else if (SecurityLevel == 0x00U) { 
        /* Rule: Security must be unlocked */
        Dcm_SendNRC(0x2EU, 0x33U, tx, txLen); 
    } else {
        uint16_t did = (uint16_t)(((uint16_t)rx[1] << 8U) | (uint16_t)rx[2]);

        /* ONLY F300 and F301 are allowed to be written to, and ONLY in Session 02 */
        if ((did == 0xF300U) || (did == 0xF301U)) {
            if (CurrentSession != 0x02U) { 
                Dcm_SendNRC(0x2EU, 0x7FU, tx, txLen); 
            } else {
                tx[0] = 0x6EU; tx[1] = rx[1]; tx[2] = rx[2]; *txLen = 3U;
                /* [Hardware UART Log]: Write Successful to Config DID */
            }
        } else {
            Dcm_SendNRC(0x2EU, 0x31U, tx, txLen);
            /* [Hardware UART Log]: Write Denied - Read Only or Invalid */
        }
    }
}

static void Dcm_Handle_0x2F(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    (void)rxLen;
    if (SecurityLevel == 0x00U) { 
        Dcm_SendNRC(0x2FU, 0x33U, tx, txLen); 
    } else {
        tx[0] = 0x6FU; tx[1] = rx[1]; tx[2] = rx[2]; *txLen = 3U;
        /* [Hardware UART Log]: IO Control override engaged. */
    }
}

static void Dcm_Handle_0x31(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    uint16_t rid = (uint16_t)(((uint16_t)rx[1] << 8U) | (uint16_t)rx[2]);
    uint8_t sub = (rxLen > 3U) ? rx[3] : 0x01U;
    
    uint8_t nrc = Dcm_ExecuteRoutine(rid, sub, CurrentSession, SecurityLevel);
    
    if (nrc == 0x00U) { 
        tx[0] = 0x71U; tx[1] = rx[1]; tx[2] = rx[2]; *txLen = 3U; 
    } else { 
        Dcm_SendNRC(0x31U, nrc, tx, txLen); 
    }
}

static void Dcm_Handle_Flash(uint8_t sid, const uint8_t *rx, uint8_t *tx, uint16_t *txLen) {
    if (SecurityLevel == 0x00U) { 
        Dcm_SendNRC(sid, 0x33U, tx, txLen); 
    } else {
        tx[0] = sid + 0x40U; tx[1] = rx[1]; *txLen = 2U;
        /* [Hardware UART Log]: Flash Service Processed. */
    }
}

/* ==========================================================================
 * CENTRAL GATEKEEPER
 * ========================================================================== */
void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen) {
    
    /* MISRA: Check bounds and pointer validity */
    if ((rxData != NULL) && (txData != NULL) && (txLen != NULL) && (rxLen >= 1U)) {
        uint8_t sid = rxData[0];
        uint8_t isAllowed = 0U;

        /* ISO 14229-1: Any valid diagnostic request resets the S3 Server Timer */
        S3_Timer = 0U;

        switch (CurrentSession) {
            case 0x01U: /* Default */
                if ((sid == 0x10U) || (sid == 0x11U) || (sid == 0x19U) || (sid == 0x22U) || (sid == 0x3EU)) {
                    isAllowed = 1U;
                }
                break;
            case 0x02U: /* Programming */
                if ((sid == 0x10U) || (sid == 0x11U) || (sid == 0x27U) || (sid == 0x2EU) || (sid == 0x31U) || 
                    (sid == 0x34U) || (sid == 0x36U) || (sid == 0x37U) || (sid == 0x3EU)) {
                    isAllowed = 1U;
                }
                break;
            case 0x03U: /* Extended */
                if ((sid == 0x10U) || (sid == 0x22U) || (sid == 0x27U) || (sid == 0x2EU) || 
                    (sid == 0x2FU) || (sid == 0x31U) || (sid == 0x85U) || (sid == 0x3EU)) {
                    isAllowed = 1U;
                }
                break;
            default:
                /* MISRA Requires default case */
                isAllowed = 0U;
                break;
        }

        if (isAllowed == 0U) {
            Dcm_SendNRC(sid, 0x7FU, txData, txLen);
            /* [Hardware UART Log]: Gatekeeper NRC 0x7F - SID blocked in Current Session */
        } else {
            switch (sid) {
                case 0x10U: Dcm_Handle_0x10(rxData, rxLen, txData, txLen); break;
                case 0x11U: Dcm_Handle_0x11(rxData, rxLen, txData, txLen); break;
                case 0x19U: Dcm_Handle_0x19(rxData, rxLen, txData, txLen); break;
                case 0x22U: Dcm_Handle_0x22(rxData, rxLen, txData, txLen); break;
                case 0x27U: Dcm_Handle_0x27(rxData, rxLen, txData, txLen); break;
                case 0x2EU: Dcm_Handle_0x2E(rxData, rxLen, txData, txLen); break;
                case 0x2FU: Dcm_Handle_0x2F(rxData, rxLen, txData, txLen); break;
                case 0x31U: Dcm_Handle_0x31(rxData, rxLen, txData, txLen); break;
                case 0x34U: 
                case 0x36U: 
                case 0x37U: 
                           Dcm_Handle_Flash(sid, rxData, txData, txLen); break;
                case 0x3EU: txData[0] = 0x7EU; txData[1] = 0x00U; *txLen = 2U; break; 
                case 0x85U: txData[0] = 0xC5U; txData[1] = rxData[1]; *txLen = 2U; break; 
                default:   Dcm_SendNRC(sid, 0x11U, txData, txLen); break; 
            }
        }
    }
}

void Dcm_Init(void) { 
    CurrentSession = 0x01U; 
    SecurityLevel = 0x00U; 
    S3_Timer = 0U; 
}