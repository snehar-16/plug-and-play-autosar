#ifndef HAL_H
#define HAL_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

void     Hal_Init        (void);
uint32_t Hal_GetTickMs   (void);
void     Hal_GpioWrite   (uint8_t pin, uint8_t value);
uint8_t  Hal_GpioRead    (uint8_t pin);
void     Hal_UartWrite   (uint8_t byte);
uint8_t  Hal_UartRead    (void);
void     Hal_WatchdogKick(void);

#ifdef __cplusplus
}
#endif

#endif /* HAL_H */
