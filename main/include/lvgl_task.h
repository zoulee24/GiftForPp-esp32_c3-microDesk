#ifndef __LVGL_TASK_H__
#define __LVGL_TASK_H__

#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lvgl.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"

/* Littlevgl specific */
#include "lvgl.h"


// #include "lv_sjpg.h"

#include "lvgl_helpers.h"
#include "lvgl/src/hal/lv_hal_disp.h"

#include "wifi_cz.h"
#include "https_api.h"

#define LV_TICK_PERIOD_MS 1
#define BUF_W 20
#define BUF_H 10

void lv_tick_task(void *arg);
void guiTask1(void *pvParameter);
void lv_update_time();
void lv_update_weather();
void SetWiFiIconVisible(bool enable);

#endif // __LVGL_TASK_H__
