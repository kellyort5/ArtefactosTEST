#pragma once
#include "FreeRTOS.h"
struct FakeQueue;
using QueueHandle_t = FakeQueue *;
QueueHandle_t xQueueCreate(unsigned, unsigned);
int xQueueSend(QueueHandle_t, const void *, unsigned);
int xQueueReceive(QueueHandle_t, void *, unsigned);
void xQueueReset(QueueHandle_t);
void vQueueDelete(QueueHandle_t);
