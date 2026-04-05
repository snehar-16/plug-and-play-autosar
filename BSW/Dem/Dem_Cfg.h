#ifndef BSW_DEM_DEM_CFG_H_
#define BSW_DEM_DEM_CFG_H_

#include <stdint.h>

/* --- UDS Status Byte Bit Masks (ISO 14229-1)  --- */
#define DEM_UDS_STATUS_TF      0x01  /* Bit 0: TestFailed  */
#define DEM_UDS_STATUS_PDTC    0x04  /* Bit 2: PendingDTC  */
#define DEM_UDS_STATUS_CDTC    0x08  /* Bit 3: ConfirmedDTC  */
#define DEM_UDS_STATUS_TNCSLC  0x10  /* Bit 4: TestNotCompletedSinceLastClear  */
#define DEM_UDS_STATUS_WIR     0x80  /* Bit 7: WarningIndicatorRequested  */

/* --- Logic Thresholds  --- */
#define DEM_AGING_THRESHOLD    40    /* Mandatory for AUTOSAR compliance  */
#define DEM_HEALING_THRESHOLD  3     /* Consecutive passed cycles  */
#define TOTAL_DTC_SLOTS        20    /* Max slots in primary memory  */

/* --- EEPROM Physical Mapping (SMOP_SWRS_30) [cite: 61] --- */
#define EEPROM_PAGE_SIZE       64
#define DEM_S1_START_PAGE      2043   /* Pages 2043-2044 for Faults [cite: 62] */
#define DEM_S2_START_PAGE      2045   /* Pages 2045-2047 for Operational Events [cite: 62] */

/* --- Original DID Definitions --- */
#define DID_COMM_LINK_FAILURE       0xF100 
#define DID_COMM_LINK_RESTORED      0xF101 
#define DID_COMM_RETRY_EXHAUSTED    0xF105 
#define DID_MSG_LIFETIME_EXCEEDED   0xF106 
#define DID_CRC_FAILURE_VITAL       0xF107 
#define DID_STCAS_HEALTH_DEGRADED   0xF108 
#define DID_EEPROM_WRITE_FAILURE    0xF10B 
#define DID_SM_KEY_STATE_CHANGE     0xF10D 
#define DID_BUZZER_TIMEOUT_REACHED  0xF10E 

#define DID_LAST_STN_SOS_GEN        0xF200 
#define DID_LAST_STN_SOS_CANCEL     0xF201 
#define DID_LAST_STN_SOS_ACK        0xF202 
#define DID_LAST_LOCO_SOS_GEN       0xF203 
#define DID_LAST_LOCO_SOS_ACK       0xF204 
#define DID_LAST_LOCO_SOS_CANCEL    0xF205 

#endif /* BSW_DEM_DEM_CFG_H_ */