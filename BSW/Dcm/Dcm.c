#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* --- UDS Diagnostic Communication Settings --- */
#define DCM_TCP_PORT           8888  
#define SESSION_DEFAULT        0x01
#define SESSION_EXTENDED       0x03

/* --- Session 1: Fault DID Definitions (0xF1xx) [cite: 12] --- */
#define DID_COMM_LINK_FAILURE       0xF100 
#define DID_EEPROM_WRITE_FAILURE    0xF10B 
#define DID_BUZZER_TIMEOUT_REACHED  0xF10E 
/* ... include others from your header as needed ... */

/* --- Session 2: Operational Event DID Definitions (0xF2xx) [cite: 17] --- */
#define DID_LAST_STN_SOS_GEN        0xF200 
#define DID_LAST_LOCO_SOS_GEN       0xF203 
/* ... include others from your header as needed ... */

/* Global State Variables */
static uint8_t CurrentSession = SESSION_DEFAULT;

/**
 * @brief Enforces Security Lockout and validates DIDs per SMOP_SWRS v1.1.
 * Returns payload size if allowed, or 0 if locked/invalid.
 */
static uint8_t Dcm_CheckAccessRights(uint16_t did) {
    /* Session 1: Fault DIDs (0xF100 - 0xF10E) [cite: 12] */
    /* System-detectable: Accessible in both Default and Extended Sessions  */
    if (did >= 0xF100 && did <= 0xF10E) {
        return 10; // Fault record size
    }

    /* Session 2: Operational Event DIDs (0xF200 - 0xF205) [cite: 17] */
    /* Read-only: Extended Diagnostic Session ONLY  */
    if (did >= 0xF200 && did <= 0xF205) {
        if (CurrentSession == SESSION_EXTENDED) {
            return 8; // Operational record size
        } else {
            /* Security Lockout active for Session 2  */
            return 0; 
        }
    }
    return 0; // Unsupported DID
}

/**
 * @brief Generates UDS Negative Response Code (NRC) 
 */
static void Dcm_SendNRC(uint8_t sid, uint8_t nrc, uint8_t *txData, uint16_t *txLen) {
    txData[0] = 0x7F; // Negative Response SID
    txData[1] = sid; 
    txData[2] = nrc; 
    *txLen = 3;
}

/**
 * @brief Service 0x22: Read Data By Identifier 
 */
static void Dcm_Handle_ReadData(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen) {
    uint16_t did = (rxData[1] << 8) | rxData[2];
    uint8_t size = Dcm_CheckAccessRights(did);

    if (size > 0) {
        txData[0] = 0x62; // Positive Response SID (0x22 + 0x40)
        txData[1] = rxData[1]; // DID High Byte
        txData[2] = rxData[2]; // DID Low Byte
        
        /* Mocking NvM Data Retrieval (e.g., from 1Mbit EEPROM ) */
        memset(&txData[3], 0xAA, size); 
        *txLen = 3 + size;
    } else {
        /* NRC 0x31: Request Out Of Range (DID invalid or Session Locked)  */
        Dcm_SendNRC(0x22, 0x31, txData, txLen);
    }
}

/**
 * @brief Main Entry Point for UDS Diagnostic Requests
 */
void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen) {
    if (rxLen < 1) return;
    uint8_t sid = rxData[0];

    switch (sid) {
        case 0x10: /* Diagnostic Session Control  */
            if (rxLen == 2 && (rxData[1] == 0x01 || rxData[1] == 0x03)) {
                CurrentSession = rxData[1];
                txData[0] = 0x50; // Positive Response
                txData[1] = CurrentSession;
                *txLen = 2;
            } else {
                Dcm_SendNRC(sid, 0x13, txData, txLen); // Invalid Format
            }
            break;

        case 0x22: /* Read Data By Identifier  */
            Dcm_Handle_ReadData(rxData, rxLen, txData, txLen);
            break;

        default:
            Dcm_SendNRC(sid, 0x11, txData, txLen); // Service Not Supported 
            break;
    }
}

/**
 * @brief Initialization of the SM-OCIP DCM Stack
 */
void Dcm_Init(void) {
    CurrentSession = SESSION_DEFAULT;
    printf("[DCM] Kavach SM-OCIP Diagnostic Stack Initialized.\n");
    printf("[DCM] Listening on Ethernet Port: %d\n", DCM_TCP_PORT);
}