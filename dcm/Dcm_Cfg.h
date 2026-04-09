/* ============================================================================
 * File: Dcm_Cfg.h
 * Description: Configuration header for the Diagnostic Communication Manager.
 * Defines standard UDS Services, NRCs, TCP Port, and Unified DIDs.
 * Compliance: ISO 14229-1 (UDS) Standard
 * ============================================================================ */

#ifndef BSW_DCM_DCM_CFG_H_
#define BSW_DCM_DCM_CFG_H_

#include <stdint.h>

/* --- Diagnostic Network Configuration --- */
#define DCM_TCP_PORT                13400  /* ISO 13400 Standard DoIP Port */

/* --------------------------------------------------------------------------
 * UDS SESSION DEFINITIONS (ISO 14229-1 Compliant)
 * -------------------------------------------------------------------------- */
#define DCM_SESSION_DEFAULT         0x01
#define DCM_SESSION_PROGRAMMING     0x02  /* Write Privileges */
#define DCM_SESSION_EXTENDED        0x03  /* Audit Privileges */

/* --------------------------------------------------------------------------
 * UDS SERVICE IDENTIFIERS (SIDs)
 * -------------------------------------------------------------------------- */
#define SID_SESSION_CONTROL         0x10
#define SID_HARD_RESET              0x11
#define SID_CLEAR_DIAG_INFO         0x14
#define SID_READ_DTC_INFO           0x19
#define SID_READ_DATA_BY_ID         0x22
#define SID_SECURITY_ACCESS         0x27
#define SID_WRITE_DATA_BY_ID        0x2E
#define SID_ROUTINE_CONTROL         0x31

/* --- NEW SIDs ADDED FOR SMOCIP FULL FUNCTIONALITY --- */
#define SID_IO_CONTROL              0x2F  /* LED/Buzzer Hardware Override */
#define SID_REQUEST_DOWNLOAD        0x34  /* Flash Upload Init */
#define SID_TRANSFER_DATA           0x36  /* Flash Data Transfer */
#define SID_REQUEST_TRANSFER_EXIT   0x37  /* Flash Transfer Complete */
#define SID_TESTER_PRESENT          0x3E  /* Keep-Alive */
#define SID_CONTROL_DTC_SETTING     0x85  /* Disable Fault Logging */

/* --------------------------------------------------------------------------
 * NEGATIVE RESPONSE CODES (NRCs)
 * -------------------------------------------------------------------------- */
#define NRC_SERVICE_NOT_SUPPORTED   0x11
#define NRC_INCORRECT_LENGTH        0x13
#define NRC_CONDITIONS_NOT_CORRECT  0x22
#define NRC_ROOR                    0x31  /* Request Out Of Range */
#define NRC_SECURITY_ACCESS_DENIED  0x33
#define NRC_SERVICE_NOT_IN_SESSION  0x7F  /* Service Not Supported in Active Session */

/* --------------------------------------------------------------------------
 * UNIFIED MASTER DID & EVENT TABLE
 * -------------------------------------------------------------------------- */

/* --- Session 1: System Faults (0xF1xx) [Default Session Access] --- */
#define DEM_EVT_COMM_LINK_FAIL       0xF100 
#define DEM_EVT_COMM_LINK_RESTORED   0xF101 
#define DEM_EVT_COMM_RETRY_EXHAUST   0xF105 
#define DEM_EVT_MSG_STALE            0xF106 
#define DEM_EVT_CRC_ERROR_VITAL      0xF107 
#define DEM_EVT_HEALTH_FAIL          0xF108 
#define DEM_EVT_EEPROM_WRITE_FAIL    0xF10B 
#define DEM_EVT_SM_KEY_STATE         0xF10D 
#define DEM_EVT_BUZZER_TIMEOUT       0xF10E 
#define DEM_EVT_RS485_UP             0xF110 
#define DEM_EVT_RS485_DOWN           0xF111 
#define DEM_EVT_ETH_UP               0xF112 
#define DEM_EVT_ETH_DOWN             0xF113 
#define DEM_EVT_HARDWIRE_FAIL        0xF114 
#define DEM_EVT_GPS1_FAULT           0xF115 
#define DEM_EVT_GPS2_FAULT           0xF116 
#define DEM_EVT_RADIO_FAULT          0xF117 
#define DEM_EVT_GSM_FAULT            0xF118 
#define DEM_EVT_POWER_ON             0xF119 
#define DEM_EVT_COMM_FALLBACK        0xF11A 
#define DEM_EVT_HEALTH_OK            0xF11B 
#define DEM_EVT_POWER_OFF_RESET      0xF11C 
#define DEM_EVT_CHECKSUM_DISPLAY     0xF11D 

/* --- Session 3: Operational Audit / SOS (0xF2xx) [Extended Session Access] --- */
#define DEM_EVT_STN_SOS_GEN          0xF200 
#define DEM_EVT_STN_SOS_CANCEL       0xF201 
#define DEM_EVT_STN_SOS_ACK          0xF202 
#define DEM_EVT_LOCO_SOS_RECV        0xF203 
#define DEM_EVT_LOCO_SOS_ACK         0xF204 
#define DEM_EVT_LOCO_SOS_CANCEL      0xF205 
#define DEM_EVT_STN_SOS_CANCEL_ACK   0xF206 
#define DEM_EVT_TSR_ACK              0xF207 

/* --- Session 2: Configuration & Write (0xF3xx) [Programming Session Access] --- */
#define DEM_EVT_WRITE_SOS_COUNTER    0xF300 
#define DEM_EVT_WRITE_VIN_ID         0xF301 

#endif /* BSW_DCM_DCM_CFG_H_ */