#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t Std_ReturnType;

#define E_OK        ((Std_ReturnType)0x00U)
#define E_NOT_OK    ((Std_ReturnType)0x01U)

typedef uint8_t boolean;

#ifndef TRUE
#define TRUE    ((boolean)1U)
#endif

#ifndef FALSE
#define FALSE   ((boolean)0U)
#endif

#ifndef NULL_PTR
#define NULL_PTR    ((void*)0)
#endif

#if defined(AUTOSAR_CLASSIC)
    #define STD_PROFILE_CLASSIC     1U
    #define STD_PROFILE_ADAPTIVE    0U
#elif defined(AUTOSAR_ADAPTIVE)
    #define STD_PROFILE_CLASSIC     0U
    #define STD_PROFILE_ADAPTIVE    1U
#else
    #define STD_PROFILE_CLASSIC     1U
    #define STD_PROFILE_ADAPTIVE    0U
#endif

#define STD_UNUSED(x)   ((void)(x))

#endif /* STD_TYPES_H */
