#include "MemAl.h"
#include <stddef.h>
#include <stdio.h>

static const MemAl_DriverType* MemAl_ActiveDriver = NULL;

void MemAl_Register(const MemAl_DriverType* driver)
{
    MemAl_ActiveDriver = driver;
    if (driver != NULL) {
        printf("[MemAl] Backend: %s\n", driver->name);
    }
}

void* MemAl_Alloc(uint32_t size)
{
    if (MemAl_ActiveDriver == NULL ||
        MemAl_ActiveDriver->Alloc == NULL) { return NULL; }
    return MemAl_ActiveDriver->Alloc(size);
}

void MemAl_Free(void* ptr)
{
    if (MemAl_ActiveDriver != NULL &&
        MemAl_ActiveDriver->Free != NULL) {
        MemAl_ActiveDriver->Free(ptr);
    }
}

Std_ReturnType MemAl_Store(uint16_t blockId,
                           const uint8_t* data,
                           uint16_t len)
{
    if (MemAl_ActiveDriver == NULL ||
        MemAl_ActiveDriver->Store == NULL) { return E_NOT_OK; }
    return MemAl_ActiveDriver->Store(blockId, data, len);
}

Std_ReturnType MemAl_Restore(uint16_t blockId,
                             uint8_t* data,
                             uint16_t len)
{
    if (MemAl_ActiveDriver == NULL ||
        MemAl_ActiveDriver->Restore == NULL) { return E_NOT_OK; }
    return MemAl_ActiveDriver->Restore(blockId, data, len);
}

const char* MemAl_GetName(void)
{
    if (MemAl_ActiveDriver == NULL) { return "NONE"; }
    return MemAl_ActiveDriver->name;
}
