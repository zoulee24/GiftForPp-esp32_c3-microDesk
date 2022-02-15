#ifndef USER_HTTP_S_
#define USER_HTTP_S_

#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "cJSON.h"
#include "wifi_cz.h"

typedef struct system_time{
    int year;
    int mounth;
    int date;
    int hour;
    int min;
    int second;
}sysTime;

typedef struct weather_info{
    char city[24];
    char weather[24];
    char temperature[12];
    char humidity[4];
    char wind[32];
    char date[32];
    int weather_code;
}WeatherInfo;

typedef enum {
    HTTPS_TIME = 0x0,
    HTTPS_WEATHER,
    HTTPS_LOCATION,
    HTTPS_DATE
}HTTPS_TYPE;

esp_err_t https_get_update(HTTPS_TYPE type);
void start_https_task(void);
void https_get_request(void);


#endif/* __USER_HTTPS_H__ */