#ifndef SCHM_DEM_H
#define SCHM_DEM_H

/*
 * SchM_Dem.h
 * Schedule Manager stub for openAUTOSAR DEM
 *
 * The full AUTOSAR RTE generates this file.
 * We stub it out — DEM uses it for exclusive area
 * protection which is handled by OsAl_EnterCritical
 * in our portable stack.
 */

#include "Platform_Types.h"

/* Exclusive area enter/exit — mapped to no-ops in sim */
/* On real hardware these map to OsAl_EnterCritical    */
static inline void SchM_Enter_Dem_EA0(void) {}
static inline void SchM_Exit_Dem_EA0(void)  {}
static inline void SchM_Enter_Dem_EA1(void) {}
static inline void SchM_Exit_Dem_EA1(void)  {}
static inline void SchM_Enter_Dem_EA2(void) {}
static inline void SchM_Exit_Dem_EA2(void)  {}
static inline void SchM_Enter_Dem_EA3(void) {}
static inline void SchM_Exit_Dem_EA3(void)  {}

#define SchM_Enter_Dem_EA_0() SchM_Enter_Dem_EA0()
#define SchM_Exit_Dem_EA_0()  SchM_Exit_Dem_EA0()
#define SchM_Enter_Dem_EA_1() SchM_Enter_Dem_EA1()
#define SchM_Exit_Dem_EA_1()  SchM_Exit_Dem_EA1()
#define SchM_Enter_Dem_EA_2() SchM_Enter_Dem_EA2()
#define SchM_Exit_Dem_EA_2()  SchM_Exit_Dem_EA2()
#define SchM_Enter_Dem_EA_3() SchM_Enter_Dem_EA3()
#define SchM_Exit_Dem_EA_3()  SchM_Exit_Dem_EA3()

#endif /* SCHM_DEM_H */
