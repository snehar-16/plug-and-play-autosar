#ifndef DEM_CORE_H
#define DEM_CORE_H

#include <stdint.h>

/* Function Prototypes */
void Dem_Init(void);
void Dem_MainFunction(void);
void Dem_SetEventStatus(uint16_t did, uint8_t isFailed);
void Dem_ReportError(uint16_t did, uint8_t rawSignal);

#endif /* DEM_CORE_H */
