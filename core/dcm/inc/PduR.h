#ifndef PDUR_H
#define PDUR_H

/*
 * PduR.h — PDU Router stub
 * DCM uses PduR to send/receive PDUs.
 * In our portable stack TpAl replaces PduR.
 * This stub provides the minimum types DCM needs.
 */

#include "Std_Types.h"
#include "ComStack_Types.h"

typedef uint8_t  PduR_StateType;
typedef uint16_t PduIdType;
typedef uint16_t PduLengthType;

typedef struct {
    uint8_t*      SduDataPtr;
    PduLengthType SduLength;
} PduInfoType;

#define PDUR_UNINIT     0x00U
#define PDUR_ONLINE     0x01U

#endif /* PDUR_H */
