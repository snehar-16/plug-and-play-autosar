#ifndef DCM_H
#define DCM_H

#include <stdint.h>
#include "Dcm_Cfg.h"

/**
 * @brief Initializes the DCM module.
 */
void Dcm_Init(void);

/**
 * @brief Processes an incoming UDS request and generates a response.
 * @param rxData Pointer to the incoming Ethernet packet payload.
 * @param rxLen  Length of the incoming request.
 * @param txData Pointer to the buffer where the response will be written.
 * @param txLen  Pointer to a variable that will store the response length.
 */
void Dcm_MainFunction(uint8_t *rxData, uint16_t rxLen, uint8_t *txData, uint16_t *txLen);

#endif /* DCM_H */
