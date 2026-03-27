#ifndef STD_TYPES_H
#define STD_TYPES_H

/*
 * Std_Types.h — DEM-local version
 * Provides all ArcCore/openAUTOSAR base types
 * This file is found first because dem/inc is in the include path
 */

#include <stdint.h>
#include <stddef.h>

/* ── Standard return type ────────────────────────────────────── */
typedef uint8_t Std_ReturnType;
#ifndef E_OK
#define E_OK        0U
#endif
#ifndef E_NOT_OK
#define E_NOT_OK    1U
#endif

/* ── Boolean ─────────────────────────────────────────────────── */
typedef uint8_t boolean;
#ifndef TRUE
#define TRUE  1U
#endif
#ifndef FALSE
#define FALSE 0U
#endif

/* ── STD_ON / STD_OFF ────────────────────────────────────────── */
#ifndef STD_ON
#define STD_ON  1U
#endif
#ifndef STD_OFF
#define STD_OFF 0U
#endif

/* ── ArcCore short type aliases ──────────────────────────────── */
typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;
typedef uint64_t  uint64;
typedef int8_t    sint8;
typedef int16_t   sint16;
typedef int32_t   sint32;
typedef int64_t   sint64;
typedef float     float32;
typedef double    float64;

/* ── NULL pointer ────────────────────────────────────────────── */
#ifndef NULL_PTR
#define NULL_PTR ((void*)0)
#endif

/* ── DEM-specific base types ─────────────────────────────────── */
typedef uint16_t Dem_EventIdType;
typedef uint8_t  Dem_EventStatusExtendedType;
typedef uint8_t  Dem_OperationCycleIdType;
typedef uint16_t Dem_RatioIdType;
typedef uint16_t Dem_RatioType;

/* Clear DTC return */
typedef uint8_t Dem_ReturnClearDTCType;
#define DEM_CLEAR_OK                0x00U
#define DEM_CLEAR_WRONG_DTC         0x01U
#define DEM_CLEAR_WRONG_DTCORIGIN   0x02U
#define DEM_CLEAR_FAILED            0x03U
#define DEM_CLEAR_PENDING           0x04U
#define DEM_CLEAR_BUSY              0x05U


/* Std_VersionInfoType */
typedef struct {
    uint16_t vendorID;
    uint16_t moduleID;
    uint8_t  sw_major_version;
    uint8_t  sw_minor_version;
    uint8_t  sw_patch_version;
} Std_VersionInfoType;

#endif /* STD_TYPES_H */

/* MAX / MIN macros used by openAUTOSAR DEM internals */
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
