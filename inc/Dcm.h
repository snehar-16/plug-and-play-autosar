/* ============================================================================
 * File: Dcm.h
 * Description: Header for the Diagnostic Communication Manager (DCM).
 * Exposes the initialization and main processing loop for incoming UDS requests.
 * ============================================================================ */

#ifndef BSW_DCM_DCM_H_
#define BSW_DCM_DCM_H_

#include <stdint.h>
#include "Dcm_Cfg.h"

/**
 * @brief Initializes the DCM module.
 */
void Dcm_Init(void);

/**
 * @brief Processes an incoming UDS request and generates a response.
 * * @param rxData Pointer to the incoming UDS request payload (from Ethernet).
 * @param rxLen  Length of the incoming request in bytes.
 * @param txData Pointer to the buffer where the UDS response will be written.
 * @param txLen  Pointer to a variable that will store the final response length.
 */
void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen);

#endif /* BSW_DCM_DCM_H_ */