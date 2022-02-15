#include "wifi_cz.h"
#include "dns_server.h"
#include "AuthenticationServer.h"

static const char *WiFi_TAG = "WiFi";
TaskHandle_t wifi_task_handle = NULL, Dns_Handle_task = NULL;
esp_netif_t *wifi_sta_netif = NULL, *wifi_ap_netif = NULL;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  static int s_retry_num = 0;
  static wifi_mode_t wifi_mode;
  static uint32_t retry_num = 0;
  static int ap_connect_num = 0;
  esp_wifi_get_mode(&wifi_mode);
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    ESP_LOGI(WiFi_TAG, "WiFi STA 被启动");
    s_retry_num = 0;
    if(wifi_mode == WIFI_MODE_STA) {
      wifi_sta_state = WIFI_STA_CONNECTING;
    } else if(wifi_mode == WIFI_MODE_APSTA) {
      wifi_ap_state = WIFI_AP_OK;
      wifi_sta_state = WIFI_STA_CONNECTING;
    }
    vTaskDelay(100 / portTICK_RATE_MS);
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (wifi_mode == WIFI_MODE_STA){
      if(wifi_sta_state == WIFI_STA_CONNECTING){
        if (s_retry_num < TEST_MAXIMUM_RETRY) {
          ESP_LOGI(WiFi_TAG, "尝试重连WiFi 第%d次", ++s_retry_num);
          esp_wifi_connect();
          vTaskDelay(6000 / portTICK_RATE_MS);
        } else {
          s_retry_num = 0;
          ESP_LOGW(WiFi_TAG, "WiFi连接失败 尝试重连次数达到最大值 %d", TEST_MAXIMUM_RETRY);
          wifi_sta_state = WIFI_STA_DISCONNECTED;
          wifi_both();
          // wifi_ap();
        }
      } else if (wifi_sta_state == WIFI_STA_RECONNECTING) {
        if (s_retry_num < 2 * TEST_MAXIMUM_RETRY) {
          ESP_LOGI(WiFi_TAG, "断连尝试重连WiFi 第%d次", ++s_retry_num);
          esp_wifi_connect();
          vTaskDelay(6000 / portTICK_RATE_MS);
        } else {
          s_retry_num = 0;
          ESP_LOGW(WiFi_TAG, "WiFi连接失败 尝试重连次数达到最大值 %d", 2 * TEST_MAXIMUM_RETRY);
          wifi_sta_state = WIFI_STA_DISCONNECTED;
          wifi_both();
          // wifi_ap();
        }
      } else if (wifi_sta_state == WIFI_STA_DISCONNECTING){
        ESP_LOGI(WiFi_TAG, "WiFi断开连接");
        wifi_sta_state = WIFI_STA_DISCONNECTED;
      } else {
        if(wifi_sta_state != WIFI_STOP){
          ESP_LOGI(WiFi_TAG, "WiFi断开连接");
          wifi_sta_state = WIFI_STA_RECONNECTING;
          esp_wifi_connect();
          vTaskDelay(4000 / portTICK_RATE_MS);
        }
      }
    } else if( wifi_mode == WIFI_MODE_APSTA){
      vTaskDelay(10000 / portTICK_RATE_MS);
      ESP_LOGI(WiFi_TAG, "尝试重连WiFi 第%ld次", ++retry_num);
      esp_wifi_connect();
    }
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
    s_retry_num = 0;
    wifi_sta_state = WIFI_STA_CONNECTED;
    ESP_LOGI(WiFi_TAG, "WiFi连接成功");
    vTaskDelay(1000 / portTICK_RATE_MS);
    if(wifi_mode == WIFI_MODE_APSTA) {
      wifi_sta();
    }
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {
    wifi_sta_state = WIFI_STOP;
    ESP_LOGI(WiFi_TAG, "WiFi STA 已停止");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ESP_LOGI(WiFi_TAG, "获取到IP地址");
    wifi_sta_state = WIFI_STA_GOT_IP;
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    ESP_LOGI(WiFi_TAG, "ESP32-C3 ip: " IPSTR, IP2STR(&event->ip_info.ip));
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START){
    ESP_LOGI(WiFi_TAG, "ESP32-C3 WiFi 热点已打开.");
    wifi_ap_state = WIFI_AP_OK;
    if(Dns_Handle_task == NULL) {
      Start_webserver();
      ESP_LOGW("DNS", "创建DNS服务器");
      xTaskCreate(dns_server_task, "dns_server_task", 4096, NULL, 3, &Dns_Handle_task);
    }
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
    ap_connect_num++;
    wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
    ESP_LOGI(WiFi_TAG, "检测到连接 " MACSTR " , AID=%d\t数量 %d", MAC2STR(event->mac), event->aid, ap_connect_num);
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED ) {
    ap_connect_num--;
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGW(WiFi_TAG, "检测到断开 " MACSTR " , AID=%d\t数量 %d", MAC2STR(event->mac), event->aid, ap_connect_num);
    // if(ap_connect_num == 0 && Dns_Handle_task != NULL) {
    //   End_web();
    //   vTaskDelete(Dns_Handle_task);
    //   Dns_Handle_task = NULL;
    //   ESP_LOGW("DNS", "DNS服务器已关闭");
    // }
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STOP) {
    ESP_LOGI(WiFi_TAG, "WiFi AP 已停止");
    wifi_ap_state = WIFI_AP_NOT_OK;
    if(Dns_Handle_task != NULL) {
      End_web();
      vTaskDelete(Dns_Handle_task);
      Dns_Handle_task = NULL;
      ESP_LOGW("DNS", "DNS服务器已关闭");
    }
  }
}

void wifi_init(void)
{
  wifi_sta_state = WIFI_NOT_INIT;
  wifi_ap_state = WIFI_NOT_INIT;
  esp_netif_init();
  esp_event_loop_create_default();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  wifi_sta_state = WIFI_INIT;
  wifi_ap_state = WIFI_INIT;
  // wifi_ap_netif = esp_netif_create_default_wifi_ap();
  // wifi_sta_netif = esp_netif_create_default_wifi_sta();
  esp_wifi_set_storage(WIFI_STORAGE_FLASH);
}

void wifi_sta(void)
{
  if( wifi_sta_state == WIFI_STA_GOT_IP){
	  wifi_sta_state = WIFI_STA_DISCONNECTING;
    esp_wifi_stop();
  } else if (wifi_sta_state != WIFI_STOP || wifi_ap_state != WIFI_STOP){
    esp_wifi_stop();
  }
  vTaskDelay(2000 / portTICK_RATE_MS);
  ESP_LOGI(WiFi_TAG, "设置WiFi模式STA");
  wifi_config_t sta = {0};
  nvs_read_wifi_msg(&sta);
  sta.sta.pmf_cfg.capable = true;
  sta.sta.pmf_cfg.required = false;
  // read.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  static bool first = true;
  if (first){
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip);
    first = false;
  }
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &sta);
  esp_wifi_start();
}
/*
void wifi_sta_my(void)
{
  esp_netif_init();
  char *desc;
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
  // Prefix the interface description with the module TAG
  // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
  asprintf(&desc, "%s: %s", WiFi_TAG, esp_netif_config.if_desc);
  esp_netif_config.if_desc = desc;
  esp_netif_config.route_prio = 128;
  wifi_sta_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
  free(desc);
  esp_wifi_set_default_wifi_sta_handlers();
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));

  ESP_LOGI(WiFi_TAG, "设置WiFi模式STA");
  wifi_config_t sta = {
    .sta = {
      .ssid = "esp8266",
      .password = "987654321",
      .bssid_set = false,
      .pmf_cfg = {
        .capable = true,
        .required = false,
      },
    },
  };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
  ESP_ERROR_CHECK(esp_wifi_start());
  esp_wifi_connect();
  s_active_interfaces++;

  s_semph_get_ip_addrs = xSemaphoreCreateCounting(NR_OF_IP_ADDRESSES_TO_WAIT_FOR, 0);

  
  ESP_ERROR_CHECK(esp_register_shutdown_handler(&stop));
  ESP_LOGI(WiFi_TAG, "Waiting for IP(s)");
  for (int i = 0; i < NR_OF_IP_ADDRESSES_TO_WAIT_FOR; ++i) {
    xSemaphoreTake(s_semph_get_ip_addrs, portMAX_DELAY);
  }

  esp_netif_ip_info_t ip;
  ESP_LOGI(WiFi_TAG, "Connected to %s", esp_netif_get_desc(wifi_sta_netif));
  ESP_ERROR_CHECK(esp_netif_get_ip_info(wifi_sta_netif, &ip));
  ESP_LOGI(WiFi_TAG, "- IPv4 address: " IPSTR, IP2STR(&ip.ip));


  // esp_event_handler_instance_t instance_any_id;
  // esp_event_handler_instance_t instance_got_ip;
  // esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id);
  // esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip);
  // esp_wifi_set_mode(WIFI_MODE_STA);
  // esp_wifi_set_config(WIFI_IF_STA, &sta);
  // esp_wifi_start();
}
*/
void wifi_both(void)
{
  if( wifi_sta_state == WIFI_STA_GOT_IP){
	  wifi_sta_state = WIFI_STA_DISCONNECTING;
    esp_wifi_stop();
  } else if (wifi_sta_state != WIFI_STOP || wifi_ap_state != WIFI_STOP){
    esp_wifi_stop();
  }
  vTaskDelay(2000 / portTICK_RATE_MS);
  ESP_LOGI(WiFi_TAG, "设置WiFi模式AP+STA");
  wifi_ap_state = WIFI_AP_NOT_OK;
  wifi_config_t sta;
  nvs_read_wifi_msg(&sta);
  sta.sta.pmf_cfg.capable = true;
  sta.sta.pmf_cfg.required = false;
  #define WIFI_AP_SSID "ESP32-C3"
  wifi_config_t ap = {
    .ap = {
      .ssid = WIFI_AP_SSID,
      .ssid_len = strlen(WIFI_AP_SSID),
      .max_connection = 5,
      .authmode = WIFI_AUTH_OPEN,
    }
  };
  #undef WIFI_AP_SSID
  
  esp_wifi_set_mode(WIFI_MODE_APSTA);
  esp_wifi_set_config(WIFI_IF_STA, &sta);
  esp_wifi_set_config(WIFI_IF_AP, &ap);
  esp_wifi_start();
}

void wifi_ap(void)
{
  if( wifi_sta_state == WIFI_STA_GOT_IP){
	  wifi_sta_state = WIFI_STA_DISCONNECTING;
    esp_wifi_stop();
  } else if (wifi_sta_state != WIFI_STOP || wifi_ap_state != WIFI_STOP){
    esp_wifi_stop();
  }
  ESP_LOGI(WiFi_TAG, "设置WiFi模式AP");
  wifi_ap_state = WIFI_AP_NOT_OK;
  wifi_sta_state = WIFI_STOP;
  
  #define WIFI_AP_SSID "ESP32-C3"
  wifi_config_t ap = {
    .ap = {
      .ssid = WIFI_AP_SSID,
      .ssid_len = strlen(WIFI_AP_SSID),
      .max_connection = 5,
      .authmode = WIFI_AUTH_OPEN,
      .channel = 3,
    }
  };
  #undef WIFI_AP_SSID

  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &ap);
  esp_wifi_start();
}

void start_wifi(void)
{
  wifi_init();
  wifi_sta();
}
