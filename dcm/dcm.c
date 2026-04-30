#include "Dcm_Cfg.h"
#include "../dem/dem_cfg.h"
#include <string.h>

/* --- DoIP / Railway Ethernet Constants --- */
#define DOIP_HEADER_SIZE                8U
#define DOIP_PROTOCOL_VERSION           0x02U
#define DOIP_PAYLOAD_TYPE_DIAG          0x8001U
#define DOIP_PAYLOAD_TYPE_ROUTING_REQ   0x0005U
#define DOIP_PAYLOAD_TYPE_ROUTING_RES   0x0006U

/* MISRA Note: Externs should ideally be in a shared header. */
extern uint8_t Dcm_ExecuteRoutine(uint16_t routineId, uint8_t subFunction, uint8_t currentSession, uint8_t securityLevel);
extern void Dem_EventLogger_Clear(void);

/* --- Global State --- */
static uint8_t CurrentSession = 0x01U; 
static uint8_t SecurityLevel = 0x00U;  /* 0x00U: Locked, 0x01U: Unlocked */
static uint32_t S3_Timer = 0U;         /* ISO 14229-1 S3 Server Timer Tracker */
static uint8_t DoIP_RoutingActive = 0U; /* Ethernet Routing State */

/* ==========================================================================
 * INTERNAL UTILITIES
 * ========================================================================== */

/**
 * @brief Sends a Negative Response Code (NRC)
 * BUG FIXED: txLen is now correctly dereferenced using '*' to update the value.
 */
static void Dcm_SendNRC(uint8_t sid, uint8_t nrc, uint8_t *txData, uint16_t *txLen) {
    if ((txData != NULL) && (txLen != NULL)) {
        txData[0] = 0x7FU; 
        txData[1] = sid; 
        txData[2] = nrc; 
        *txLen = 3U; /* Corrected: Using pointer dereference */
    }
}

void Dcm_ManageSessionTimer(uint32_t elapsed_ms) {
    if (CurrentSession != 0x01U) {
        S3_Timer += elapsed_ms;
        if (S3_Timer >= DCM_TIMING_S3_SERVER) {
            CurrentSession = 0x01U; 
            SecurityLevel = 0x00U;  
            S3_Timer = 0U;          
        }
    }
}

/* ==========================================================================
 * SERVICE HANDLERS (Operate on UDS Payload)
 * ========================================================================== */

static void Dcm_Handle_0x10(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 2U) { 
        Dcm_SendNRC(0x10U, 0x13U, tx, txLen); 
    } else {
        uint8_t reqSession = rx[1];
        if ((reqSession == 0x01U) || (reqSession == 0x02U) || (reqSession == 0x03U)) {
            CurrentSession = reqSession;
            SecurityLevel = 0x00U; 
            tx[0] = 0x50U; 
            tx[1] = CurrentSession; 
            *txLen = 2U;
        } else { 
            Dcm_SendNRC(0x10U, 0x31U, tx, txLen); 
        }
    }
}

static void Dcm_Handle_0x14(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 4U) { 
        Dcm_SendNRC(0x14U, 0x13U, tx, txLen); 
    } else {
        Dem_EventLogger_Clear();
        tx[0] = 0x54U; 
        *txLen = 1U;
    }
}

static void Dcm_Handle_0x22(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 3U) { 
        Dcm_SendNRC(0x22U, 0x13U, tx, txLen); 
    } else {
        uint16_t did = (uint16_t)(((uint16_t)rx[1] << 8U) | (uint16_t)rx[2]);
        uint8_t isValid = 0U;
        uint8_t nrcToReturn = 0x31U; /* Default to Request Out Of Range */

        /* --- CATEGORY 1: Default Session DIDs (F1xx) --- */
        if ((did == 0xF100U) || (did == 0xF101U) || ((did >= 0xF105U) && (did <= 0xF108U)) || 
            (did == 0xF10BU) || (did == 0xF10DU) || (did == 0xF10EU) || 
            ((did >= 0xF110U) && (did <= 0xF11DU))) {
            isValid = 1U;
        } 
        /* --- CATEGORY 2: Extended Session DIDs (F2xx) --- */
        else if ((did >= 0xF200U) && (did <= 0xF207U)) {
            if (CurrentSession == 0x03U) {
                isValid = 1U;
            } else {
                isValid = 0U;
                nrcToReturn = 0x7FU; /* Mandatory Security: Service Not Supported In Active Session */
            }
        }
        /* --- CATEGORY 3: Programming Session DIDs (F3xx) --- */
        else if ((did == 0xF300U) || (did == 0xF301U)) {
            if (CurrentSession == 0x02U) {
                isValid = 1U;
            } else {
                isValid = 0U;
                nrcToReturn = 0x7FU; /* Mandatory Security: Service Not Supported In Active Session */
            }
        }

        if (isValid == 1U) {
            tx[0] = 0x62U; tx[1] = rx[1]; tx[2] = rx[2];
            /* Placeholder for 8-byte timestamp payload from DEM */
            (void)memset(&tx[3], 0x00, 8U); 
            *txLen = 11U;
        } else {
            Dcm_SendNRC(0x22U, nrcToReturn, tx, txLen);
        }
    }
}

static void Dcm_Handle_0x27(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rx[1] == 0x01U) { 
        tx[0] = 0x67U; tx[1] = 0x01U; tx[2] = 0xAAU; *txLen = 3U; 
    } else if (rx[1] == 0x02U) { 
        SecurityLevel = 0x01U; tx[0] = 0x67U; tx[1] = 0x02U; *txLen = 2U;
    } else { 
        Dcm_SendNRC(0x27U, 0x12U, tx, txLen); 
    }
}

static void Dcm_Handle_0x2E(const uint8_t *rx, uint16_t rxLen, uint8_t *tx, uint16_t *txLen) {
    if (rxLen < 4U) { 
        Dcm_SendNRC(0x2EU, 0x13U, tx, txLen); 
    } else if (SecurityLevel == 0x00U) { 
        Dcm_SendNRC(0x2EU, 0x33U, tx, txLen); 
    } else if (CurrentSession != 0x02U) {
        /* Service 2E (Write) must only happen in Programming Session 0x02 */
        Dcm_SendNRC(0x2EU, 0x7FU, tx, txLen);
    } else {
        uint16_t did = (uint16_t)(((uint16_t)rx[1] << 8U) | (uint16_t)rx[2]);
        if ((did == 0xF300U) || (did == 0xF301U)) {
            tx[0] = 0x6EU; tx[1] = rx[1]; tx[2] = rx[2];
            *txLen = 3U;
        } else {
            Dcm_SendNRC(0x2EU, 0x31U, tx, txLen);
        }
    }
}

/* ==========================================================================
 * CENTRAL GATEKEEPER & ETHERNET WRAPPER
 * ========================================================================== */

void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen) {
    
    if ((rxData != NULL) && (txData != NULL) && (txLen != NULL) && (rxLen >= DOIP_HEADER_SIZE)) {
        
        uint8_t  ver        = rxData[0];
        uint16_t payloadTyp = (uint16_t)(((uint16_t)rxData[2] << 8U) | (uint16_t)rxData[3]);
        
        if (ver != DOIP_PROTOCOL_VERSION) {
            return; 
        }

        /* 1. HANDLE ROUTING ACTIVATION (0x0005) */
        if (payloadTyp == DOIP_PAYLOAD_TYPE_ROUTING_REQ) {
            DoIP_RoutingActive = 1U; 
            
            txData[0] = DOIP_PROTOCOL_VERSION;
            txData[1] = (uint8_t)(~DOIP_PROTOCOL_VERSION);
            txData[2] = 0x00U; txData[3] = 0x06U; 
            txData[4] = 0x00U; txData[5] = 0x00U; 
            txData[6] = 0x00U; txData[7] = 0x09U; 
            
            txData[8] = rxData[8]; txData[9] = rxData[9]; 
            txData[10] = 0x10U; txData[11] = 0x00U;       
            txData[12] = 0x10U;                           
            (void)memset(&txData[13], 0x00, 4U);          
            
            *txLen = DOIP_HEADER_SIZE + 9U;
            return; 
        }

        /* 2. HANDLE UDS DIAGNOSTICS (0x8001) */
        if (payloadTyp == DOIP_PAYLOAD_TYPE_DIAG) {
            
            if (DoIP_RoutingActive == 0U) {
                return; 
            }

            uint8_t *udsData    = &rxData[DOIP_HEADER_SIZE];
            uint16_t udsLen     = rxLen - DOIP_HEADER_SIZE;
            uint8_t  sid        = udsData[0];
            uint8_t  isAllowed  = 0U;

            S3_Timer = 0U; 

            /* Global Session Guard for Services based on your requirements */
            switch (CurrentSession) {
                case 0x01U: /* Default Session */
                    if ((sid == 0x10U) || (sid == 0x14U) || (sid == 0x22U)) { isAllowed = 1U; }
                    break;
                case 0x02U: /* Programming Session */
                    if ((sid == 0x10U) || (sid == 0x14U) || (sid == 0x27U) || (sid == 0x2EU)) { isAllowed = 1U; }
                    break;
                case 0x03U: /* Extended Session */
                    if ((sid == 0x10U) || (sid == 0x14U) || (sid == 0x22U) || (sid == 0x27U) || 
                        (sid == 0x2EU) || (sid == 0x2FU) || (sid == 0x31U) || (sid == 0x85U) || (sid == 0x3EU)) {
                        isAllowed = 1U;
                    }
                    break;
                default: break;
            }

            if (isAllowed == 0U) {
                Dcm_SendNRC(sid, 0x7FU, &txData[DOIP_HEADER_SIZE], txLen);
            } else {
                switch (sid) {
                    case 0x10U: Dcm_Handle_0x10(udsData, udsLen, &txData[DOIP_HEADER_SIZE], txLen); break;
                    case 0x14U: Dcm_Handle_0x14(udsData, udsLen, &txData[DOIP_HEADER_SIZE], txLen); break;
                    case 0x22U: Dcm_Handle_0x22(udsData, udsLen, &txData[DOIP_HEADER_SIZE], txLen); break;
                    case 0x27U: Dcm_Handle_0x27(udsData, udsLen, &txData[DOIP_HEADER_SIZE], txLen); break;
                    case 0x2EU: Dcm_Handle_0x2E(udsData, udsLen, &txData[DOIP_HEADER_SIZE], txLen); break;
                    default:    Dcm_SendNRC(sid, 0x11U, &txData[DOIP_HEADER_SIZE], txLen); break;
                }
            }

            /* Wrap UDS payload in DoIP Header for Response */
            txData[0] = DOIP_PROTOCOL_VERSION;
            txData[1] = (uint8_t)(~DOIP_PROTOCOL_VERSION); 
            txData[2] = 0x80U; txData[3] = 0x01U;          
            txData[4] = 0x00U; txData[5] = 0x00U;          
            txData[6] = (uint8_t)((*txLen >> 8U) & 0xFFU); 
            txData[7] = (uint8_t)(*txLen & 0xFFU);         
            
            *txLen += DOIP_HEADER_SIZE; 
        }
    }
}

void Dcm_Init(void) { 
    CurrentSession = 0x01U; 
    SecurityLevel = 0x00U; 
    S3_Timer = 0U; 
    DoIP_RoutingActive = 0U; 
}