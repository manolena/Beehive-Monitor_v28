#include "safe_freertos.h"

BaseType_t safeSemaphoreTake(SemaphoreHandle_t sem, TickType_t ticksToWait, const char *tag) {
  if (!sem) {
    if (tag) Serial.printf("[SAFE-FR] Semaphore TAKE NULL - %s\n", tag);
    else Serial.println("[SAFE-FR] Semaphore TAKE NULL");
    return pdFALSE;
  }
  return xSemaphoreTake(sem, ticksToWait);
}

BaseType_t safeSemaphoreGive(SemaphoreHandle_t sem, const char *tag) {
  if (!sem) {
    if (tag) Serial.printf("[SAFE-FR] Semaphore GIVE NULL - %s\n", tag);
    else Serial.println("[SAFE-FR] Semaphore GIVE NULL");
    return pdFALSE;
  }
  return xSemaphoreGive(sem);
}

BaseType_t safeQueueSend(QueueHandle_t q, const void *item, TickType_t ticksToWait, const char *tag) {
  if (!q) {
    if (tag) Serial.printf("[SAFE-FR] Queue SEND NULL - %s\n", tag);
    else Serial.println("[SAFE-FR] Queue SEND NULL");
    return pdFALSE;
  }
  return xQueueSend(q, item, ticksToWait);
}

BaseType_t safeQueueReceive(QueueHandle_t q, void *buf, TickType_t ticksToWait, const char *tag) {
  if (!q) {
    if (tag) Serial.printf("[SAFE-FR] Queue RECV NULL - %s\n", tag);
    else Serial.println("[SAFE-FR] Queue RECV NULL");
    return pdFALSE;
  }
  return xQueueReceive(q, buf, ticksToWait);
}