#ifndef PDUR_DCM_H
#define PDUR_DCM_H

/*
 * PduR_Dcm.h — PDU Router DCM interface stub
 * In our stack TpAl handles transport — PduR is not used.
 */

#include "PduR.h"

Std_ReturnType PduR_DcmTransmit(PduIdType TxPduId,
                                const PduInfoType* PduInfoPtr);

#endif /* PDUR_DCM_H */
