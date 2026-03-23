#include "MemAl.h"
#include <string.h>
#include <stdio.h>

#define MEMAL_POOL_SIZE        8192U
#define MEMAL_NVM_MAX_BLOCKS   16U
#define MEMAL_NVM_BLOCK_SIZE   512U

static uint8_t  MemAl_Pool[MEMAL_POOL_SIZE];
static uint32_t MemAl_PoolOffset = 0U;

static uint8_t  MemAl_NvmData [MEMAL_NVM_MAX_BLOCKS][MEMAL_NVM_BLOCK_SIZE];
static uint16_t MemAl_NvmLen  [MEMAL_NVM_MAX_BLOCKS];
static uint8_t  MemAl_NvmValid[MEMAL_NVM_MAX_BLOCKS];

static void* Static_Alloc(uint32_t size)
{
    if ((MemAl_PoolOffset + size) > MEMAL_POOL_SIZE) {
        printf("[MemAl_Static] Pool full\n");
        return NULL;
    }
    void* ptr = &MemAl_Pool[MemAl_PoolOffset];
    MemAl_PoolOffset += size;
    return ptr;
}

static void Static_Free(void* ptr)
{
    (void)ptr;
}

static Std_ReturnType Static_Store(uint16_t blockId,
                                   const uint8_t* data,
                                   uint16_t len)
{
    if (blockId >= MEMAL_NVM_MAX_BLOCKS) { return E_NOT_OK; }
    if (len > MEMAL_NVM_BLOCK_SIZE)      { return E_NOT_OK; }
    (void)memcpy(MemAl_NvmData[blockId], data, len);
    MemAl_NvmLen  [blockId] = len;
    MemAl_NvmValid[blockId] = 1U;
    return E_OK;
}

static Std_ReturnType Static_Restore(uint16_t blockId,
                                     uint8_t* data,
                                     uint16_t len)
{
    if (blockId >= MEMAL_NVM_MAX_BLOCKS)  { return E_NOT_OK; }
    if (MemAl_NvmValid[blockId] == 0U)    { return E_NOT_OK; }
    if (len > MemAl_NvmLen[blockId])      { return E_NOT_OK; }
    (void)memcpy(data, MemAl_NvmData[blockId], len);
    return E_OK;
}

extern "C" const MemAl_DriverType MemAl_Static_Driver = {
    Static_Alloc,
    Static_Free,
    Static_Store,
    Static_Restore,
    "STATIC"
};
