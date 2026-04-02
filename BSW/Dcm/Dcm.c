#include "Dcm.h"
#include <string.h>

/* External prototype to bridge DCM to your 1Mbit EEPROM Logic */
extern void NvM_ReadDemRecord(uint16_t did, uint8_t* data, uint16_t size);

void Dcm_Init(void) {
    // Ready for future security states or session timers
}

/* -------------------------------------------------------------------------- */
/* INTERNAL HELPERS                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Automatically maps a DID to its expected physical memory length.
 */
static uint8_t Dcm_GetExpectedLength(uint16_t did) {
    if (did >= 0xF100 && did <= 0xF10E) return 10; // Session 1: ID + Time
    if (did >= 0xF200 && did <= 0xF205) return 8;  // Session 2: Time Only
    return 0; // Unconfigured DID
}

/**
 * @brief Formats a UDS Negative Response Code (NRC)
 */
static void Dcm_SendNRC(uint8_t sid, uint8_t nrc, uint8_t *txData, uint16_t *txLen) {
    txData[0] = UDS_NRC_NEGATIVE_RESPONSE; // 0x7F
    txData[1] = sid;                       // Original Service
    txData[2] = nrc;                       // Error Reason
    *txLen = 3;
}

/* -------------------------------------------------------------------------- */
/* MAIN PROCESSOR                                                             */
/* -------------------------------------------------------------------------- */

void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen) {
    // 1. Minimum Length Check (Must have at least a Service ID)
    if (rxLen < 1) return;

    uint8_t sid = rxData[0];

    // 2. Service Dispatcher
    switch (sid) {
        
        /* --- SERVICE 0x22: READ DATA BY IDENTIFIER --- */
        case UDS_SID_READ_DATA_BY_ID:
            if (rxLen != 3) {
                Dcm_SendNRC(sid, NRC_INCORRECT_MESSAGE_LENGTH, txData, txLen);
            } else {
                uint16_t did = (rxData[1] << 8) | rxData[2];
                uint8_t dataSize = Dcm_GetExpectedLength(did);

                if (dataSize == 0) {
                    Dcm_SendNRC(sid, NRC_REQUEST_OUT_OF_RANGE, txData, txLen);
                } else {
                    uint8_t eepromBuffer[10] = {0}; // Max size we might need
                    
                    /* Fetch from the NvM Layer */
                    NvM_ReadDemRecord(did, eepromBuffer, dataSize);

                    /* Build Positive Response: 0x62 + DID + Data */
                    txData[0] = sid + UDS_SID_POSITIVE_RESPONSE; // 0x62
                    txData[1] = rxData[1]; // DID High
                    txData[2] = rxData[2]; // DID Low
                    memcpy(&txData[3], eepromBuffer, dataSize);
                    
                    *txLen = 3 + dataSize;
                }
            }
            break;

        /* --- SERVICE 0x11: ECU RESET --- */
        case UDS_SID_ECU_RESET:
            if (rxLen != 2) {
                Dcm_SendNRC(sid, NRC_INCORRECT_MESSAGE_LENGTH, txData, txLen);
            } else {
                /* Build Positive Response: 0x51 + SubFunction */
                txData[0] = sid + UDS_SID_POSITIVE_RESPONSE; // 0x51
                txData[1] = rxData[1]; 
                *txLen = 2;
                // Note: The actual NVIC_SystemReset() should trigger AFTER sending this message
            }
            break;

        /* --- UNKNOWN SERVICE --- */
        default:
            Dcm_SendNRC(sid, NRC_SERVICE_NOT_SUPPORTED, txData, txLen);
            break;
    }
}
