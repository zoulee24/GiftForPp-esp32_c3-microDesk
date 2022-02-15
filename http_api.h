#ifndef _HTTP_API_H__
#define _HTTP_API_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_http_client.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "cJSON.h"

#include "wifi_cz.h"

#define HTTPTAG					"HTTP_API"
// #define TIME_URL				"https://api.uukit.com/time"
// #define TIME_URL				"http://quan.suning.com/getSysTime.do"
// #define TIME_URL                "http://api.k780.com/?app=life.time&appkey=10003&sign=b59bc3ef6191eb9f747dd4e83c99f2a4&format=json"
#define TIME_URL                "https://apps.game.qq.com/CommArticle/app/reg/gdate.php"
// #define WEATHER_URL				"http://query.asilu.com/weather/baidu/"
#define WEATHER_URL			    "http://api.seniverse.com/v3/weather/now.json?key=SA7qWQTq6xHv4SaXG&location=%E5%B9%BF%E5%B7%9E"
#define MAX_HTTP_OUTPUT_BUFFER	2048


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

esp_err_t http_time_api(sysTime *systime);
esp_err_t http_weather_api(WeatherInfo *info);

void update_time(void);

void print_time(sysTime st);

#endif // _HTTP_API_H__