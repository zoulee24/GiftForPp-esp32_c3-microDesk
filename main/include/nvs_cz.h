#ifndef __NVS_CZ_H__
#define __NVS_CZ_H__

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"


esp_err_t nvs_write_wifi_msg(wifi_config_t wc);
esp_err_t nvs_read_wifi_msg(wifi_config_t *wc);

#endif //!__NVS_CZ_H__