#ifndef DCM_CFG_H
#define DCM_CFG_H

#include <stdint.h>

/* --- UDS SERVICE IDENTIFIERS (SIDs) --- */
#define UDS_SID_READ_DATA_BY_ID       0x22
#define UDS_SID_ECU_RESET             0x11
#define UDS_SID_POSITIVE_RESPONSE     0x40
#define UDS_NRC_NEGATIVE_RESPONSE     0x7F

/* --- NEGATIVE RESPONSE CODES (NRCs) --- */
#define NRC_SERVICE_NOT_SUPPORTED     0x11
#define NRC_INCORRECT_MESSAGE_LENGTH  0x13
#define NRC_CONDITIONS_NOT_CORRECT    0x22
#define NRC_REQUEST_OUT_OF_RANGE      0x31

/* --- DATA IDENTIFIERS (DIDs) --- */
/* Session 1: System Faults (10 Bytes: ID + Epoch) */
#define DID_COMM_FAIL                 0xF100
#define DID_SYS_REBOOT                0xF101
#define DID_SENSOR_ERR                0xF10E 

/* Session 2: Last Occurrence (8 Bytes: Epoch Only) */
#define DID_LOCO_SOS                  0xF203
#define DID_MAINT_REQ                 0xF204

#endif /* DCM_CFG_H */
