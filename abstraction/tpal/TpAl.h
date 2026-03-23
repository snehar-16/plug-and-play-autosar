#ifndef TPAL_H
#define TPAL_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void           (*Init)        (void);
    Std_ReturnType (*Transmit)    (const uint8_t* data, uint16_t len);
    Std_ReturnType (*Receive)     (uint8_t* buf, uint16_t* len);
    void           (*MainFunction)(void);
    const char*    name;
} TpAl_DriverType;

void           TpAl_Register    (const TpAl_DriverType* driver);
Std_ReturnType TpAl_Transmit    (const uint8_t* data, uint16_t len);
Std_ReturnType TpAl_Receive     (uint8_t* buf, uint16_t* len);
void           TpAl_MainFunction(void);
const char*    TpAl_GetName     (void);

/* ── Driver declarations ─────────────────────────────────────── */
extern const TpAl_DriverType TpAl_UDP_Driver;

#ifdef __cplusplus
}
#endif

#endif /* TPAL_H */
