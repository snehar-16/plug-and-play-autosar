#include "Dcm.h"
#include "Dcm_Cfg.h"
#include "NvM_Interface.h"
#include <stdint.h>
#include <string.h>

/* Global State Variables for Session and Security */
static uint8_t CurrentSession = SESSION_DEFAULT;
static uint8_t SecurityUnlocked = 0;

void Dcm_Init(void) {
    CurrentSession = SESSION_DEFAULT;
    SecurityUnlocked = 0;
}

/**
 * @brief Helper to determine data size based on DID category
 * Session 1 (Faults): 10 bytes | Session 2 (Events): 8 bytes
 */
static uint8_t Dcm_GetExpectedLength(uint16_t did) {
    if (did >= 0xF100 && did <= 0xF1FF) return 10; [cite: 52, 62]
    if (did >= 0xF200 && did <= 0xF2FF) return 8;  [cite: 52, 62]
    return 0; 
}

/**
 * @brief Standardized Negative Response Code (NRC) Generator
 */
static void Dcm_SendNRC(uint8_t sid, uint8_t nrc, uint8_t *txData, uint16_t *txLen) {
    txData[0] = 0x7F; /* Negative Response SID */
    txData[1] = sid; 
    txData[2] = nrc; 
    *txLen = 3; [cite: 40]
}

/**
 * @brief Logic for Service 0x22 Multi-DID Read
 * Packs multiple DID responses into a single adaptive buffer.
 */
static void Dcm_Handle_MultiRead(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen) {
    uint16_t rxIdx = 1; // Skip SID 0x22
    uint16_t txIdx = 1; // Start packing data after SID 0x62
    
    txData[0] = 0x62; // Positive Response SID

    while (rxIdx + 2 <= rxLen) {
        uint16_t did = (rxData[rxIdx] << 8) | rxData[rxIdx+1];
        uint8_t size = Dcm_GetExpectedLength(did); [cite: 52]

        if (size > 0) {
            /* Adaptive Response: Pack DID followed by its data */
            txData[txIdx++] = rxData[rxIdx];     // DID High Byte
            txData[txIdx++] = rxData[rxIdx+1];   // DID Low Byte
            
            NvM_ReadDemRecord(did, &txData[txIdx], size);
            
            txIdx += size;
            rxIdx += 2;
        } else {
            /* If any DID in the list is invalid, return RequestOutOfRange */
            Dcm_SendNRC(0x22, 0x31, txData, txLen); [cite: 40]
            return;
        }
    }
    *txLen = txIdx;
}

/**
 * @brief Main Entry Point for UDS Request Processing
 */
void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen) {
    if (rxLen < 1 || rxData == NULL) return;

    uint8_t sid = rxData[0];

    switch (sid) {
        /* Service 0x10: Diagnostic Session Control */
        case 0x10: 
            if (rxLen != 2) {
                Dcm_SendNRC(sid, 0x13, txData, txLen); 
            } else {
                CurrentSession = rxData[1];
                txData[0] = sid + 0x40; 
                txData[1] = CurrentSession;
                *txLen = 2;
            }
            break;

        /* Service 0x27: Security Access (Seed-Key) */
        case 0x27:
            SecurityUnlocked = 1; 
            txData[0] = sid + 0x40;
            txData[1] = rxData[1];
            *txLen = 2;
            break;

        /* Service 0x22: Read Data By Identifier (Multi-DID Integrated) */
        case 0x22: 
            if (rxLen < 3 || (rxLen % 2 == 0)) {
                Dcm_SendNRC(sid, 0x13, txData, txLen); // Length check
            } else {
                Dcm_Handle_MultiRead(rxData, rxLen, txData, txLen);
            }
            break;

        /* Service 0x31: Routine Control */
        case 0x31:
            if (CurrentSession != SESSION_EXTENDED) {
                Dcm_SendNRC(sid, 0x7E, txData, txLen); 
            } else if (rxLen < 4) {
                Dcm_SendNRC(sid, 0x13, txData, txLen);
            } else {
                txData[0] = sid + 0x40; 
                txData[1] = rxData[1]; 
                txData[2] = rxData[2]; 
                txData[3] = rxData[3]; 
                *txLen = 4;
            }
            break;

        /* Service 0x11: ECU Reset */
        case 0x11: 
            if (rxLen != 2) {
                Dcm_SendNRC(sid, 0x13, txData, txLen);
            } else {
                txData[0] = sid + 0x40; 
                txData[1] = rxData[1]; 
                *txLen = 2; [cite: 34]
            }
            break;

        default:
            Dcm_SendNRC(sid, 0x11, txData, txLen); [cite: 40]
            break;
    }
}