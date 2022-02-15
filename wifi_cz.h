#ifndef __WIFI_CZ_H__
#define __WIFI_CZ_H__

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_cz.h"

#define TEST_MAXIMUM_RETRY 5

typedef enum wifi_task_flag
{
    WIFI_NOT_INIT = 0x00,
    WIFI_INIT,
    WIFI_STA_CONNECTING,
    WIFI_STA_RECONNECTING,
    WIFI_STA_CONNECTED,
    WIFI_STA_GOT_IP,
    WIFI_STA_DISCONNECTING,
    WIFI_STA_DISCONNECTED,
    WIFI_AP_NOT_OK,
    WIFI_AP_OK,
    WIFI_SLEEP,
    WIFI_STOP
}WTF;

WTF wifi_sta_state;
WTF wifi_ap_state;

void wifi_init(void);
void wifi_sta(void);
void wifi_ap(void);
void wifi_both(void);
void start_wifi(void);

void wifi_sta_my(void);


#endif //!__WIFI_CZ_H__
