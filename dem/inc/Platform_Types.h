#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

/*
 * Platform_Types.h
 * ArcCore/openAUTOSAR platform type aliases
 * Maps ArcCore short types to standard C99 stdint types
 */

#include <stdint.h>
#include <stdbool.h>

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

typedef uint8_t   boolean;

#ifndef TRUE
#define TRUE  1U
#endif
#ifndef FALSE
#define FALSE 0U
#endif

/* AUTOSAR return type */
typedef uint8_t Std_ReturnType;
#ifndef E_OK
#define E_OK        0U
#endif
#ifndef E_NOT_OK
#define E_NOT_OK    1U
#endif

/* STD_ON / STD_OFF */
#ifndef STD_ON
#define STD_ON  1U
#endif
#ifndef STD_OFF
#define STD_OFF 0U
#endif

/* Operation cycle ID type */
typedef uint8_t Dem_OperationCycleIdType;

/* Ratio ID type */
typedef uint16_t Dem_RatioIdType;
typedef uint16_t Dem_RatioType;

/* Clear DTC return type */
typedef uint8_t Dem_ReturnClearDTCType;
#define DEM_CLEAR_OK                0x00U
#define DEM_CLEAR_WRONG_DTC         0x01U
#define DEM_CLEAR_WRONG_DTCORIGIN   0x02U
#define DEM_CLEAR_FAILED            0x03U
#define DEM_CLEAR_PENDING           0x04U
#define DEM_CLEAR_BUSY              0x05U

/* Extended event status type */
typedef uint8_t Dem_EventStatusExtendedType;

#endif /* PLATFORM_TYPES_H */

/* Additional types needed by Dem_Lcfg.h */
typedef uint8_t  Dem_OperationCycleIdType;
typedef uint16_t Dem_RatioIdType;
typedef uint8_t  Dem_ReturnClearDTCType;
#define DEM_CLEAR_OK                0x00U
#define DEM_CLEAR_WRONG_DTC         0x01U
#define DEM_CLEAR_FAILED            0x03U
#define DEM_CLEAR_PENDING           0x04U

/* Additional types needed by Dem_Lcfg.h */
typedef uint8_t  Dem_OperationCycleIdType;
typedef uint16_t Dem_RatioIdType;
typedef uint8_t  Dem_ReturnClearDTCType;
#define DEM_CLEAR_OK                0x00U
#define DEM_CLEAR_WRONG_DTC         0x01U
#define DEM_CLEAR_FAILED            0x03U
#define DEM_CLEAR_PENDING           0x04U
