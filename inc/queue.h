#ifndef QUEUE_H
#define QUEUE_H
#include "FreeRTOS.h"
/* Mock Queue Types for SM-OCIP Logic Testing */
typedef void* QueueHandle_t;
#define xQueueSend(a, b, c) (0)
#define xQueueReceive(a, b, c) (0)
#endif
