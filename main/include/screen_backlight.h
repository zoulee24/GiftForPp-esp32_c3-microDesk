#ifndef __SCREEN_BACKLIGHT_H__
#define __SCREEN_BACKLIGHT_H__

#include "driver/ledc.h"
#include "esp_log.h"

#define FREQHZ 5000

void BacklightInit();
void Setbacklight(uint32_t target_duty);

#endif // __SCREEN_BACKLIGHT_H__
