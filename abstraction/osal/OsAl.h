#ifndef OSAL_H
#define OSAL_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

void     OsAl_Init(void);
uint32_t OsAl_GetTickMs(void);
void     OsAl_EnterCritical(void);
void     OsAl_ExitCritical(void);
void     OsAl_Delay(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* OSAL_H */
