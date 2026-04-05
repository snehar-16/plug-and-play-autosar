/* ============================================================================
 * File: Dcm_Cfg.h
 * Description: Configuration header for the Diagnostic Communication Manager.
 * Defines standard UDS Services, NRCs, and all Kavach SM-OCIP DIDs.
 * ============================================================================ */

#ifndef BSW_DCM_DCM_CFG_H_
#define BSW_DCM_DCM_CFG_H_

#include <stdint.h>

/* --------------------------------------------------------------------------
 * UDS SERVICE IDENTIFIERS (SIDs)
 * -------------------------------------------------------------------------- */
#define SID_READ_DATA_BY_ID         0x22
#define SID_HARD_RESET              0x11

/* --------------------------------------------------------------------------
 * NEGATIVE RESPONSE CODES (NRCs)
 * -------------------------------------------------------------------------- */
#define NRC_SERVICE_NOT_SUPPORTED   0x11
#define NRC_INCORRECT_LENGTH        0x13
#define NRC_CONDITIONS_NOT_CORRECT  0x22
#define NRC_ROOR                    0x31  /* Request Out Of Range */

/* --------------------------------------------------------------------------
 * KAVACH SM-OCIP DATA IDENTIFIERS (DIDs)
 * -------------------------------------------------------------------------- */

/* --- SESSION 1: SYSTEM FAULTS (10 Bytes: DID + Epoch Timestamp) --- */
#define DID_COMM_LINK_FAILURE       0xF100
#define DID_COMM_LINK_RESTORED      0xF101
#define DID_COMM_RETRY_EXHAUSTED    0xF105
#define DID_MSG_LIFETIME_EXCEEDED   0xF106
#define DID_CRC_FAILURE_VITAL       0xF107
#define DID_STCAS_HEALTH_DEGRADED   0xF108
#define DID_EEPROM_WRITE_FAILURE    0xF10B
#define DID_SM_KEY_STATE_CHANGE     0xF10D
#define DID_BUZZER_TIMEOUT_REACHED  0xF10E

/* --- SESSION 2: OPERATIONAL LAST-OCCURRENCE (8 Bytes: Epoch Only) --- */
#define DID_LAST_STN_SOS_GEN        0xF200
#define DID_LAST_STN_SOS_CANCEL     0xF201
#define DID_LAST_STN_SOS_ACK        0xF202
#define DID_LAST_LOCO_SOS_GEN       0xF203
#define DID_LAST_LOCO_SOS_ACK       0xF204
#define DID_LAST_LOCO_SOS_CANCEL    0xF205

#endif /* BSW_DCM_DCM_CFG_H_ */