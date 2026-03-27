#include <stdint.h>
#include <stdio.h>

typedef uint8_t Std_ReturnType;
#ifndef E_OK
#define E_OK 0U
#endif

Std_ReturnType Det_ReportError(uint16_t ModuleId,
                               uint8_t  InstanceId,
                               uint8_t  ApiId,
                               uint8_t  ErrorId)
{
    (void)InstanceId;
    printf("[Det] ModuleId=%u ApiId=%u ErrorId=%u\n",
           (unsigned)ModuleId,
           (unsigned)ApiId,
           (unsigned)ErrorId);
    return E_OK;
}
