#include "main.h"
#include "screen_backlight.h"

void system_init();
void time_task(void *pvParameters);
void print_weather(WeatherInfo info);
void print_time(sysTime st);

sysTime realtime = {0};
int last_update_hour = -1;
WeatherInfo weather_info = {0};
int Countdown_days = 999;

void app_main(void)
{
  system_init();
  BacklightInit();
  Setbacklight(6000);
  ex_wifi_sta_connect();
  xTaskCreate(time_task, "time_task", 4096, NULL, 2, NULL);
  xTaskCreate(guiTask1, "guiTask1", 4096 * 2, NULL, 1, NULL);
}

void time_task(void *pvParameters)
{
  TickType_t m = xTaskGetTickCount(); //获取当前时间
  while(1)
  {
    if(realtime.second<59){
      lv_update_time();
      realtime.second++;
      vTaskDelayUntil(&m, pdMS_TO_TICKS(1000));
    } else {
      realtime.second = 0;
      if(realtime.min<59){
        realtime.min++;
      } else {
        realtime.min = 0;
        if(realtime.hour<23){
          realtime.hour++;
        } else {
          realtime.hour = 0;
          realtime.date++;
        }
        if (realtime.hour - last_update_hour > 4 || last_update_hour - realtime.hour > 20) {
          extern bool wifi_ok;
          if(wifi_ok){
            start_https_task();
          }
        }
      }
      vTaskDelayUntil(&m, pdMS_TO_TICKS(999));
    }
  }
  vTaskDelete(NULL);
}
void system_init() 
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void print_time(sysTime st)
{
	printf("时间:\t%d年%d月%d日\t%d:%d:%d\r\n", st.year, st.mounth, st.date, st.hour, st.min, st.second);
}
void print_weather(WeatherInfo info) {
	printf("[INFO] 日期信息打印\r\n\t日期:%s\r\n\t城市:%s\r\n\t天气更新日期:%s\r\n\t温度:%s℃\t\n\t湿度:%s %%\r\n\t风向:%s\r\n\t天气代号:%d\r\n", 
	info.date, info.city, info.weather, info.temperature, info.humidity, info.wind, info.weather_code);
}

