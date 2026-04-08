#include "Dcm_Cfg.h"
#include "../dem/dem_cfg.h"
#include <string.h>
#include <stdio.h>

static uint8_t CurrentSession = 0x01; 
static uint8_t SecurityLevel = 0x00;  

extern uint8_t Dcm_ExecuteRoutine(uint16_t routineId, uint8_t subFunction, uint8_t currentSession, uint8_t securityLevel);

static void Dcm_SendNRC(uint8_t sid, uint8_t nrc, uint8_t *txData, uint16_t *txLen) {
    txData[0] = 0x7F;   
    txData[1] = sid;    
    txData[2] = nrc;    
    *txLen = 3;
}

static void Dcm_Handle_0x10(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 2) { Dcm_SendNRC(0x10, 0x13, tx, txLen); return; }
    uint8_t reqSession = rx[1];
    if (reqSession == 0x01 || reqSession == 0x02 || reqSession == 0x03) {
        CurrentSession = reqSession;
        SecurityLevel = 0x00; 
        tx[0] = 0x50; tx[1] = CurrentSession; *txLen = 2;
    } else {
        Dcm_SendNRC(0x10, 0x31, tx, txLen);
    }
}

static void Dcm_Handle_0x11(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 2) { Dcm_SendNRC(0x11, 0x13, tx, txLen); return; }
    tx[0] = 0x51; tx[1] = rx[1]; *txLen = 2;
}

static void Dcm_Handle_0x14(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    tx[0] = 0x54; *txLen = 1;
}

static void Dcm_Handle_0x19(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 3) { Dcm_SendNRC(0x19, 0x13, tx, txLen); return; }
    tx[0] = 0x59; tx[1] = rx[1]; tx[2] = rx[2]; *txLen = 3;
}

/* --- THE STRICT WHITELIST LOGIC --- */
static void Dcm_Handle_0x22(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 3) { Dcm_SendNRC(0x22, 0x13, tx, txLen); return; }

    uint16_t did = (uint16_t)((rx[1] << 8) | rx[2]);
    uint8_t isValid = 0;

    /* 1. Session 1 Fault DIDs (Matches your table exactly) */
    if ((did >= 0xF100 && did <= 0xF101) || 
        (did >= 0xF105 && did <= 0xF108) || 
        (did == 0xF10B) || (did == 0xF10D) || (did == 0xF10E) ||
        (did >= 0xF110 && did <= 0xF11D)) {
        isValid = 1;
    }
    /* 2. Session 2 Audit DIDs */
    else if (did >= 0xF200 && did <= 0xF205) {
        isValid = 1;
        if (CurrentSession != 0x02) {
            Dcm_SendNRC(0x22, 0x31, tx, txLen); 
            return;
        }
    }
    /* 3. Status DIDs */
    else if (did == 0xF186 || did == 0xF188 || did == 0xF18C || did == 0xF190 || did == 0xF1A0 || (did >= 0x0300 && did <= 0x0302)) {
        isValid = 1;
    }

    /* REJECT UNLISTED DIDs (Like F102, F103, F104) */
    if (!isValid) {
        Dcm_SendNRC(0x22, 0x31, tx, txLen);
        printf("[DCM] Access Denied: DID 0x%04X is not defined in SWRS v1.1\n", did);
        return;
    }

    /* EXECUTE VALID DIDs */
    tx[0] = 0x62; tx[1] = rx[1]; tx[2] = rx[2];
    memset(&tx[3], 0x00, 8); 
    *txLen = 11;
}

static void Dcm_Handle_0x27(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 2) { Dcm_SendNRC(0x27, 0x13, tx, txLen); return; }
    if (rx[1] == 0x01) { 
        tx[0] = 0x67; tx[1] = 0x01; tx[2] = 0xAA; *txLen = 3;
    } else if (rx[1] == 0x02) { 
        SecurityLevel = 0x01; tx[0] = 0x67; tx[1] = 0x02; *txLen = 2;
    } else {
        Dcm_SendNRC(0x27, 0x12, tx, txLen); 
    }
}

static void Dcm_Handle_0x2E(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 3) { Dcm_SendNRC(0x2E, 0x13, tx, txLen); return; }
    if (CurrentSession != 0x03) { Dcm_SendNRC(0x2E, 0x7F, tx, txLen); return; }
    if (SecurityLevel == 0x00) { Dcm_SendNRC(0x2E, 0x33, tx, txLen); return; }
    tx[0] = 0x6E; tx[1] = rx[1]; tx[2] = rx[2]; *txLen = 3;
}

static void Dcm_Handle_0x31(uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 3) { Dcm_SendNRC(0x31, 0x13, tx, txLen); return; }
    uint16_t routineId = (uint16_t)((rx[1] << 8) | rx[2]);
    uint8_t subFunction = (rxLen > 3) ? rx[3] : 0x01; 
    uint8_t nrc = Dcm_ExecuteRoutine(routineId, subFunction, CurrentSession, SecurityLevel);
    if (nrc == 0x00) {
        tx[0] = 0x71; tx[1] = rx[1]; tx[2] = rx[2]; *txLen = 3;
    } else {
        Dcm_SendNRC(0x31, nrc, tx, txLen);
    }
}

void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen) {
    if (rxLen < 1) return;
    switch (rxData[0]) {
        case 0x10: Dcm_Handle_0x10(rxData, rxLen, txData, txLen); break;
        case 0x11: Dcm_Handle_0x11(rxData, rxLen, txData, txLen); break;
        case 0x14: Dcm_Handle_0x14(rxData, rxLen, txData, txLen); break;
        case 0x19: Dcm_Handle_0x19(rxData, rxLen, txData, txLen); break;
        case 0x22: Dcm_Handle_0x22(rxData, rxLen, txData, txLen); break;
        case 0x27: Dcm_Handle_0x27(rxData, rxLen, txData, txLen); break;
        case 0x2E: Dcm_Handle_0x2E(rxData, rxLen, txData, txLen); break;
        case 0x31: Dcm_Handle_0x31(rxData, rxLen, txData, txLen); break;
        default:   Dcm_SendNRC(rxData[0], 0x11, txData, txLen); break;
    }
}

void Dcm_Init(void) {
    CurrentSession = 0x01; 
    SecurityLevel = 0x00;  
}
