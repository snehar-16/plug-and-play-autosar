#ifndef DEM_EVENT_LOGGER_H
#define DEM_EVENT_LOGGER_H

#include <stdint.h>
#include "../dcm/Dcm_Cfg.h" /* Use Master IDs from here */

/* Priority levels */
#define DEM_PRIO_LOW    (0U)
#define DEM_PRIO_MED    (1U)
#define DEM_PRIO_HIGH   (2U)
#define DEM_PRIO_CRIT   (3U)

/* Source types */
#define DEM_SRC_SYSTEM  (0U)
#define DEM_TYPE_FAIL   (0U)
#define DEM_TYPE_PASS   (1U)

typedef struct {
    uint16_t slno;
    uint32_t timestamp;
    uint32_t dtcNumber;
    uint16_t eventId;
    char     eventName[24];
    uint8_t  priority;
    uint8_t  type;
    uint8_t  udsStatus;
    uint8_t  occurrence;
} DEM_EventRecordA_t;

typedef struct {
    uint16_t eventId;
    uint16_t slno;
    uint32_t timestamp;
    uint32_t dtcNumber;
    uint8_t  priority;
    uint8_t  type;
    uint8_t  udsStatus;
    uint8_t  occurrence;
    char     eventName[24];
} DEM_EventRecordB_t;

void DEM_EventLogger_Init(void);
void DEM_EventLogger_Write(uint16_t eventId, uint8_t priority, uint8_t source, uint8_t type, uint32_t dtcNumber, uint8_t udsStatus);
uint16_t DEM_EventLogger_ReadAll(DEM_EventRecordA_t *out, uint16_t max);
#endif
