#include "MemAl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MEMAL_FILE_MAX_BLOCKS  16U
#define MEMAL_FILE_BLOCK_SIZE  512U
#define MEMAL_FILE_PATH        "dem_nvram.bin"
#define MEMAL_FILE_MAGIC       0xA5A5A5A5UL

typedef struct {
    uint32_t magic;
    uint8_t  valid[MEMAL_FILE_MAX_BLOCKS];
    uint16_t len  [MEMAL_FILE_MAX_BLOCKS];
    uint8_t  data [MEMAL_FILE_MAX_BLOCKS][MEMAL_FILE_BLOCK_SIZE];
} MemAl_FileStore_t;

static MemAl_FileStore_t FileStore;
static uint8_t           FileStore_Loaded = 0U;

static void File_Load(void)
{
    if (FileStore_Loaded != 0U) { return; }
    FILE* f = fopen(MEMAL_FILE_PATH, "rb");
    if (f == NULL) {
        printf("[MemAl_File] No NvM file — starting fresh\n");
        (void)memset(&FileStore, 0, sizeof(FileStore));
        FileStore_Loaded = 1U;
        return;
    }
    size_t r = fread(&FileStore, sizeof(FileStore), 1U, f);
    fclose(f);
    if (r != 1U || FileStore.magic != MEMAL_FILE_MAGIC) {
        printf("[MemAl_File] NvM corrupt — starting fresh\n");
        (void)memset(&FileStore, 0, sizeof(FileStore));
    } else {
        printf("[MemAl_File] NvM loaded from %s\n", MEMAL_FILE_PATH);
    }
    FileStore_Loaded = 1U;
}

static void File_Save(void)
{
    FileStore.magic = MEMAL_FILE_MAGIC;
    FILE* f = fopen(MEMAL_FILE_PATH, "wb");
    if (f == NULL) { printf("[MemAl_File] ERROR: cannot write NvM\n"); return; }
    fwrite(&FileStore, sizeof(FileStore), 1U, f);
    fclose(f);
}

static void* File_Alloc(uint32_t size)
{
    return malloc((size_t)size);
}

static void File_Free(void* ptr)
{
    free(ptr);
}

static Std_ReturnType File_Store(uint16_t blockId,
                                 const uint8_t* data,
                                 uint16_t len)
{
    File_Load();
    if (blockId >= MEMAL_FILE_MAX_BLOCKS) { return E_NOT_OK; }
    if (len > MEMAL_FILE_BLOCK_SIZE)      { return E_NOT_OK; }
    (void)memcpy(FileStore.data[blockId], data, len);
    FileStore.len  [blockId] = len;
    FileStore.valid[blockId] = 1U;
    File_Save();
    printf("[MemAl_File] Block %u stored (%u bytes)\n", blockId, len);
    return E_OK;
}

static Std_ReturnType File_Restore(uint16_t blockId,
                                   uint8_t* data,
                                   uint16_t len)
{
    File_Load();
    if (blockId >= MEMAL_FILE_MAX_BLOCKS)  { return E_NOT_OK; }
    if (FileStore.valid[blockId] == 0U)    { return E_NOT_OK; }
    if (len > FileStore.len[blockId])      { return E_NOT_OK; }
    (void)memcpy(data, FileStore.data[blockId], len);
    printf("[MemAl_File] Block %u restored (%u bytes)\n", blockId, len);
    return E_OK;
}

extern "C" const MemAl_DriverType MemAl_File_Driver = {
    File_Alloc,
    File_Free,
    File_Store,
    File_Restore,
    "FILE"
};
