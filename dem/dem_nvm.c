#include "platform_api.h"
#include <stdint.h>

uint8_t NvM_Write_Verified(uint16_t blockId, const uint8_t* data, uint16_t size) {
    /* Direct call to mock. If mock returns 0, this returns 0. */
    return Platform_NvmWrite_Final(blockId, data, size);
}
