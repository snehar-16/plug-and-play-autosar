#include <stdint.h>
#include <string.h>

/* MISRA: Enforce unsigned 32-bit type during macro expansion */
#define pdMS_TO_TICKS(x) ((uint32_t)(x))

/* MISRA: Replace basic 'int' with sized, unsigned type. 
 * Renamed parameter to 'ticks' for clearer self-documentation. */
void vTaskDelay(uint32_t ticks);