/* ============================================================================
 * File: Dcm_Cfg.h
 * Description: Configuration header for the Diagnostic Communication Manager.
 * Defines standard UDS Services, NRCs, TCP Port, and Unified DIDs.
 * Compliance: ISO 14229-1 (UDS) Standard, MISRA C:2012
 * ============================================================================ */

#ifndef BSW_DCM_DCM_CFG_H_
#define BSW_DCM_DCM_CFG_H_

#include <stdint.h>

/* --- Diagnostic Network Configuration --- */
#define DCM_TCP_PORT                13400U  /* ISO 13400 Standard DoIP Port */

/* --------------------------------------------------------------------------
 * ISO 14229-1 TIMING PARAMETERS (in milliseconds)
 * -------------------------------------------------------------------------- */
#define DCM_TIMING_P2_MAX           50U     /* Max time to reply */
#define DCM_TIMING_P2_STAR_MAX      5000U   /* Max time to reply after NRC 78 */
#define DCM_TIMING_S3_SERVER        5000U   /* Session expiration timeout */

/* Expose the timer management function to the OS/Main */
void Dcm_ManageSessionTimer(uint32_t elapsed_ms);

/* --------------------------------------------------------------------------
 * UDS SESSION DEFINITIONS (ISO 14229-1 Compliant)
 * -------------------------------------------------------------------------- */
#define DCM_SESSION_DEFAULT         0x01U
#define DCM_SESSION_PROGRAMMING     0x02U  /* Write Privileges */
#define DCM_SESSION_EXTENDED        0x03U  /* Audit Privileges */

/* --------------------------------------------------------------------------
 * UDS SERVICE IDENTIFIERS (SIDs)
 * -------------------------------------------------------------------------- */
#define SID_SESSION_CONTROL         0x10U
#define SID_HARD_RESET              0x11U
#define SID_CLEAR_DIAG_INFO         0x14U
#define SID_READ_DTC_INFO           0x19U
#define SID_READ_DATA_BY_ID         0x22U
#define SID_SECURITY_ACCESS         0x27U
#define SID_WRITE_DATA_BY_ID        0x2EU
#define SID_ROUTINE_CONTROL         0x31U

/* --- NEW SIDs ADDED FOR SMOCIP FULL FUNCTIONALITY --- */
#define SID_IO_CONTROL              0x2FU  /* LED/Buzzer Hardware Override */
#define SID_REQUEST_DOWNLOAD        0x34U  /* Flash Upload Init */
#define SID_TRANSFER_DATA           0x36U  /* Flash Data Transfer */
#define SID_REQUEST_TRANSFER_EXIT   0x37U  /* Flash Transfer Complete */
#define SID_TESTER_PRESENT          0x3EU  /* Keep-Alive */
#define SID_CONTROL_DTC_SETTING     0x85U  /* Disable Fault Logging */

/* --------------------------------------------------------------------------
 * NEGATIVE RESPONSE CODES (NRCs)
 * -------------------------------------------------------------------------- */
#define NRC_SERVICE_NOT_SUPPORTED   0x11U
#define NRC_INCORRECT_LENGTH        0x13U
#define NRC_CONDITIONS_NOT_CORRECT  0x22U
#define NRC_ROOR                    0x31U  /* Request Out Of Range */
#define NRC_SECURITY_ACCESS_DENIED  0x33U
#define NRC_SERVICE_NOT_IN_SESSION  0x7FU  /* Service Not Supported in Active Session */

/* --------------------------------------------------------------------------
 * UNIFIED MASTER DID & EVENT TABLE
 * -------------------------------------------------------------------------- */

/* --- Session 1: System Faults (0xF1xx) [Default Session Access] --- */
#define DEM_EVT_COMM_LINK_FAIL       0xF100U 
#define DEM_EVT_COMM_LINK_RESTORED   0xF101U 
#define DEM_EVT_COMM_RETRY_EXHAUST   0xF105U 
#define DEM_EVT_MSG_STALE            0xF106U 
#define DEM_EVT_CRC_ERROR_VITAL      0xF107U 
#define DEM_EVT_HEALTH_FAIL          0xF108U 
#define DEM_EVT_EEPROM_WRITE_FAIL    0xF10BU 
#define DEM_EVT_SM_KEY_STATE         0xF10DU 
#define DEM_EVT_BUZZER_TIMEOUT       0xF10EU 
#define DEM_EVT_RS485_UP             0xF110U 
#define DEM_EVT_RS485_DOWN           0xF111U 
#define DEM_EVT_ETH_UP               0xF112U 
#define DEM_EVT_ETH_DOWN             0xF113U 
#define DEM_EVT_HARDWIRE_FAIL        0xF114U 
#define DEM_EVT_GPS1_FAULT           0xF115U 
#define DEM_EVT_GPS2_FAULT           0xF116U 
#define DEM_EVT_RADIO_FAULT          0xF117U 
#define DEM_EVT_GSM_FAULT            0xF118U 
#define DEM_EVT_POWER_ON             0xF119U 
#define DEM_EVT_COMM_FALLBACK        0xF11AU 
#define DEM_EVT_HEALTH_OK            0xF11BU 
#define DEM_EVT_POWER_OFF_RESET      0xF11CU 
#define DEM_EVT_CHECKSUM_DISPLAY     0xF11DU 

/* --- Session 3: Operational Audit / SOS (0xF2xx) [Extended Session Access] --- */
#define DEM_EVT_STN_SOS_GEN          0xF200U 
#define DEM_EVT_STN_SOS_CANCEL       0xF201U 
#define DEM_EVT_STN_SOS_ACK          0xF202U 
#define DEM_EVT_LOCO_SOS_RECV        0xF203U 
#define DEM_EVT_LOCO_SOS_ACK         0xF204U 
#define DEM_EVT_LOCO_SOS_CANCEL      0xF205U 
#define DEM_EVT_STN_SOS_CANCEL_ACK   0xF206U 
#define DEM_EVT_TSR_ACK              0xF207U 

/* --- Session 2: Configuration & Write (0xF3xx) [Programming Session Access] --- */
#define DEM_EVT_WRITE_SOS_COUNTER    0xF300U 
#define DEM_EVT_WRITE_VIN_ID         0xF301U 

#endif /* BSW_DCM_DCM_CFG_H_ */