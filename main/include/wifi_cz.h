#ifndef __WIFI_CZ_H__
#define __WIFI_CZ_H__

#include <string.h>
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "nvs_cz.h"
#include "dns_server.h"
#include "AuthenticationServer.h"
#include "https_api.h"

#define STA_FAIL_CONNECT_TIMES  15
#define WiFi_AP_NAME "屁屁的礼物"

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

esp_err_t ex_wifi_sta_connect(void);
esp_err_t ex_wifi_sta_disconnect(void);
esp_err_t ex_wifi_apsta_connect(void);
esp_err_t ex_wifi_apsta_disconnect(void);

#endif // __WIFI_CZ_H__
