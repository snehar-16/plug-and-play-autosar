#ifndef MEMAL_H
#define MEMAL_H

#include "Std_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void*          (*Alloc)  (uint32_t size);
    void           (*Free)   (void* ptr);
    Std_ReturnType (*Store)  (uint16_t blockId,
                              const uint8_t* data,
                              uint16_t len);
    Std_ReturnType (*Restore)(uint16_t blockId,
                              uint8_t* data,
                              uint16_t len);
    const char*    name;
} MemAl_DriverType;

void           MemAl_Register(const MemAl_DriverType* driver);
void*          MemAl_Alloc   (uint32_t size);
void           MemAl_Free    (void* ptr);
Std_ReturnType MemAl_Store   (uint16_t blockId,
                              const uint8_t* data,
                              uint16_t len);
Std_ReturnType MemAl_Restore (uint16_t blockId,
                              uint8_t* data,
                              uint16_t len);
const char*    MemAl_GetName (void);

#define MEMAL_BLOCK_DEM_PRIMARY    0x0000U
#define MEMAL_BLOCK_DEM_PERMANENT  0x0001U
#define MEMAL_BLOCK_CALIBRATION_ID 0x0002U
#define MEMAL_BLOCK_VEHICLE_CONFIG 0x0003U
#define MEMAL_BLOCK_APP_SW_ID      0x0004U
#define MEMAL_BLOCK_DMI_CONFIG     0x0005U

extern const MemAl_DriverType MemAl_Static_Driver;
extern const MemAl_DriverType MemAl_File_Driver;

#ifdef __cplusplus
}
#endif

#endif
