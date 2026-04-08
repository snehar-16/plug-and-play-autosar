#include "dem_event_logger.h"
#include "../platform/platform_api.h" /* Added for EEPROM & Timestamps */
#include <string.h>
#include <stdio.h>

/* EEPROM partition: pages 365-511, 16 bytes/record, 4 records/page */
#define LOG_START_PAGE   (365U)
#define LOG_END_PAGE     (511U)
#define LOG_NUM_PAGES    (147U)
#define REC_SIZE         (16U)
#define RECS_PER_PAGE    (4U)
#define MAX_RECORDS      (588U)   /* 147 * 4 */

/* Internal compact record stored in EEPROM */
typedef struct {
    uint32_t timestamp;
    uint32_t dtcNumber;
    uint16_t eventId;
    uint16_t slno;
    uint8_t  priority;
    uint8_t  type;
    uint8_t  udsStatus;
    uint8_t  occurrence;
} __attribute__((packed)) RawRec_t;  /* exactly 16 bytes */

/* Event name lookup */
static const struct { uint16_t id; const char *name; } s_names[] = {
    {0x0101U,"STN_SOS_GEN"},   {0x0102U,"STN_SOS_ACK"},
    {0x0103U,"STN_SOS_CANCEL"},{0x0104U,"LOCO_SOS_RECV"},
    {0x0105U,"LOCO_SOS_ACK"},  {0x0106U,"LOCO_SOS_CANCEL"},
    {0x0201U,"RS485_UP"},      {0x0202U,"RS485_DOWN"},
    {0x0203U,"ETH_UP"},        {0x0204U,"ETH_DOWN"},
    {0x0205U,"HARDWIRE_FAIL"}, {0x0206U,"CRC_ERROR"},
    {0x0207U,"HEALTH_FAIL"},   {0x0208U,"GPS1_FAULT"},
    {0x0209U,"GPS2_FAULT"},    {0x020AU,"RADIO_FAULT"},
    {0x020BU,"GSM_FAULT"},     {0x0301U,"POWER_ON"},
    {0x0302U,"COMM_FALLBACK"}, {0x0303U,"HEALTH_OK"},
    {0x0304U,"TSR_ACK"},       {0x0401U,"POWER_OFF_RESET"},
    {0x0402U,"MSG_STALE"},     {0x0403U,"BUZZER_TIMEOUT"},
    {0x0404U,"CHECKSUM_DISP"}, {0x0000U,"UNKNOWN"}
};

static const char *GetName(uint16_t id)
{
    uint8_t i;
    for (i = 0U; s_names[i].id != 0x0000U; i++)
        if (s_names[i].id == id) return s_names[i].name;
    return "UNKNOWN";
}

/* In-RAM circular buffer mirror */
static RawRec_t  s_log[MAX_RECORDS];
static uint16_t  s_count    = 0U;
static uint16_t  s_writeIdx = 0U;
static uint16_t  s_slno     = 0U;
static uint8_t   s_ready    = 0U;

void DEM_EventLogger_Init(void)
{
    memset(s_log, 0, sizeof(s_log));
    s_count = s_writeIdx = s_slno = 0U;
    s_ready = 1U;
    printf("[Logger] Black Box initialized on EEPROM Pages %d-%d\n", LOG_START_PAGE, LOG_END_PAGE);
}

void DEM_EventLogger_Write(uint16_t eventId, uint8_t priority,
                            uint8_t source,   uint8_t type,
                            uint32_t dtcNumber, uint8_t udsStatus)
{
    RawRec_t *r;
    (void)source;
    if (!s_ready) { return; }

    r = &s_log[s_writeIdx];
    r->eventId    = eventId;
    r->slno       = ++s_slno;
    r->timestamp  = Platform_GetTick_ms(); /* Replaced 0U with actual Hardware Tick */
    r->dtcNumber  = dtcNumber;
    r->priority   = priority;
    r->type       = type;
    r->udsStatus  = udsStatus;
    r->occurrence = (r->occurrence < 255U) ? r->occurrence + 1U : 255U;

    /* --- INTEGRATION FIX: Write through to physical EEPROM --- */
    uint16_t pageNumber = LOG_START_PAGE + (s_writeIdx / RECS_PER_PAGE);
    uint8_t pageOffset = (s_writeIdx % RECS_PER_PAGE) * REC_SIZE;
    
    uint8_t pageBuffer[64] = {0};
    
    /* Read-Modify-Write: Fetch the 64-byte page, inject the 16-byte record, save back */
    Platform_EnterCritical();
    if (Platform_NvmRead(pageNumber, pageBuffer, 64) == PLATFORM_OK) {
        memcpy(&pageBuffer[pageOffset], r, REC_SIZE);
        Platform_NvmWrite(pageNumber, pageBuffer, 64);
    }
    Platform_ExitCritical();
    /* --------------------------------------------------------- */

    s_writeIdx = (uint16_t)((s_writeIdx + 1U) % MAX_RECORDS);
    if (s_count < MAX_RECORDS) s_count++;
}

/* Clear Function required by UDS Routine 0x0101 */
void Dem_EventLogger_Clear(void)
{
    uint8_t emptyPage[64] = {0};
    
    /* Erase physical EEPROM pages */
    for (uint16_t page = LOG_START_PAGE; page <= LOG_END_PAGE; page++) {
        Platform_NvmWrite(page, emptyPage, 64);
    }
    
    /* Erase RAM mirror */
    memset(s_log, 0, sizeof(s_log));
    s_count = s_writeIdx = s_slno = 0U;
    
    printf("[Logger] Security routine: All %d black box records completely erased.\n", MAX_RECORDS);
}

/* Format A — return all records */
uint16_t DEM_EventLogger_ReadAll(DEM_EventRecordA_t *out, uint16_t max)
{
    uint16_t i, n = (s_count < max) ? s_count : max;
    for (i = 0U; i < n; i++)
    {
        RawRec_t *r = &s_log[i];
        out[i].slno      = r->slno;
        out[i].timestamp = r->timestamp;
        out[i].dtcNumber = r->dtcNumber;
        out[i].eventId   = r->eventId;
        out[i].priority  = r->priority;
        out[i].type      = r->type;
        out[i].udsStatus = r->udsStatus;
        out[i].occurrence= r->occurrence;
        strncpy(out[i].eventName, GetName(r->eventId), 23U);
        out[i].eventName[23] = '\0';
    }
    return n;
}

/* Format B — query by eventId (plug and play) */
uint8_t DEM_EventLogger_QueryById(uint16_t eventId,
                                   DEM_EventRecordB_t *out)
{
    uint16_t i;
    if (out == NULL) { return 0U; }
    /* Return most recent match */
    for (i = s_count; i > 0U; i--)
    {
        RawRec_t *r = &s_log[i - 1U];
        if (r->eventId == eventId)
        {
            out->eventId   = r->eventId;
            out->slno      = r->slno;
            out->timestamp = r->timestamp;
            out->dtcNumber = r->dtcNumber;
            out->priority  = r->priority;
            out->type      = r->type;
            out->udsStatus = r->udsStatus;
            out->occurrence= r->occurrence;
            strncpy(out->eventName, GetName(r->eventId), 23U);
            out->eventName[23] = '\0';
            return 1U;
        }
    }
    return 0U;
}