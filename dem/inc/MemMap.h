#ifndef MEMMAP_H
#define MEMMAP_H

/*
 * MemMap.h
 * Memory mapping stub for openAUTOSAR DEM
 *
 * Full AUTOSAR uses this for linker section placement.
 * We stub it out — all sections map to default memory.
 */

/* All MemMap pragmas are no-ops in our portable stack */
#define DEM_START_SEC_CODE
#define DEM_STOP_SEC_CODE
#define DEM_START_SEC_VAR_INIT_UNSPECIFIED
#define DEM_STOP_SEC_VAR_INIT_UNSPECIFIED
#define DEM_START_SEC_VAR_NOINIT_UNSPECIFIED
#define DEM_STOP_SEC_VAR_NOINIT_UNSPECIFIED
#define DEM_START_SEC_VAR_NOINIT_8
#define DEM_STOP_SEC_VAR_NOINIT_8
#define DEM_START_SEC_VAR_NOINIT_16
#define DEM_STOP_SEC_VAR_NOINIT_16
#define DEM_START_SEC_VAR_NOINIT_32
#define DEM_STOP_SEC_VAR_NOINIT_32
#define DEM_START_SEC_CONST_UNSPECIFIED
#define DEM_STOP_SEC_CONST_UNSPECIFIED
#define DEM_START_SEC_CONST_8
#define DEM_STOP_SEC_CONST_8
#define DEM_START_SEC_CONST_16
#define DEM_STOP_SEC_CONST_16
#define DEM_START_SEC_CONST_32
#define DEM_STOP_SEC_CONST_32

#define MEMMAP_ERROR_DEM

#endif /* MEMMAP_H */
