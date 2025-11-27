#ifndef SAFE_FREERTOS_H
#define SAFE_FREERTOS_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

// Safe wrappers to avoid asserts when a NULL semaphore/queue is used.
// tag is an optional short string identifying the caller location (for Serial).
BaseType_t safeSemaphoreTake(SemaphoreHandle_t sem, TickType_t ticksToWait, const char *tag = nullptr);
BaseType_t safeSemaphoreGive(SemaphoreHandle_t sem, const char *tag = nullptr);

BaseType_t safeQueueSend(QueueHandle_t q, const void *item, TickType_t ticksToWait, const char *tag = nullptr);
BaseType_t safeQueueReceive(QueueHandle_t q, void *buf, TickType_t ticksToWait, const char *tag = nullptr);

#endif // SAFE_FREERTOS_H