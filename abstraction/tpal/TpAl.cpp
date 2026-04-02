#include "TpAl.h"
#include <stddef.h>

static const TpAl_DriverType* TpAl_ActiveDriver = NULL;

void TpAl_Register(const TpAl_DriverType* driver)
{
    TpAl_ActiveDriver = driver;
    if (TpAl_ActiveDriver != NULL &&
        TpAl_ActiveDriver->Init != NULL) {
        TpAl_ActiveDriver->Init();
    }
}

Std_ReturnType TpAl_Transmit(const uint8_t* data, uint16_t len)
{
    if (TpAl_ActiveDriver == NULL ||
        TpAl_ActiveDriver->Transmit == NULL) {
        return E_NOT_OK;
    }
    return TpAl_ActiveDriver->Transmit(data, len);
}

Std_ReturnType TpAl_Receive(uint8_t* buf, uint16_t* len)
{
    if (TpAl_ActiveDriver == NULL ||
        TpAl_ActiveDriver->Receive == NULL) {
        return E_NOT_OK;
    }
    return TpAl_ActiveDriver->Receive(buf, len);
}

void TpAl_MainFunction(void)
{
    if (TpAl_ActiveDriver != NULL &&
        TpAl_ActiveDriver->MainFunction != NULL) {
        TpAl_ActiveDriver->MainFunction();
    }
}

const char* TpAl_GetName(void)
{
    if (TpAl_ActiveDriver == NULL) { return "NONE"; }
    return TpAl_ActiveDriver->name;
}
