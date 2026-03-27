#ifndef DET_H
#define DET_H

#include <stdint.h>

typedef uint8_t Std_ReturnType;

#ifndef E_OK
#define E_OK 0U
#endif

#define DEM_MODULE_ID 54U

Std_ReturnType Det_ReportError(uint16_t ModuleId,
                               uint8_t  InstanceId,
                               uint8_t  ApiId,
                               uint8_t  ErrorId);

#endif /* DET_H */
