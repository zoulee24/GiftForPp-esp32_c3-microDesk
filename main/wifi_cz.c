#include "wifi_cz.h"

static uint32_t fail_connect = 0;
static uint8_t ap_connect = 0;
TaskHandle_t ipv4_task_handle = NULL, sta_disconnect_task_handle = NULL, Dns_Handle_task = NULL;

static const char *TAG = "WIFI_MANAGER";
static const char *CALLBACK_TAG = "WIFI_CALLBACK";
bool wifi_ok = false;

#define Second(x) (x*1000) / portTICK_PERIOD_MS

void SetWiFiIconVisible(bool enable);

static bool is_our_netif(const char *prefix, esp_netif_t *netif)
{
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

esp_netif_t *get_netif_from_desc(const char *desc)
{
    esp_netif_t *netif = NULL;
    char *expected_desc;
    asprintf(&expected_desc, "%s: %s", TAG, desc);
    while ((netif = esp_netif_next(netif)) != NULL) {
        if (strcmp(esp_netif_get_desc(netif), expected_desc) == 0) {
            free(expected_desc);
            return netif;
        }
    }
    free(expected_desc);
    return netif;
}

void apsta_ip_task(void *param)
{
    ESP_LOGI("IPv4_TASK", "等待获取IP事件");
    vTaskSuspend(NULL);
    ex_wifi_apsta_disconnect();
    if(Dns_Handle_task != NULL) {
        End_web();
        vTaskDelete(Dns_Handle_task);
        Dns_Handle_task = NULL;
        ESP_LOGW("DNS", "DNS服务器已关闭");
    }
    ex_wifi_sta_connect();
    ipv4_task_handle = NULL;
    vTaskDelete(ipv4_task_handle);
}

void sta_disconnect_task(void *param)
{
    ESP_LOGI("STA_DISCONNECT_TASK", "等待断开连接事件");
    vTaskSuspend(NULL);
    ex_wifi_sta_disconnect();
    ex_wifi_apsta_connect();
    sta_disconnect_task_handle = NULL;
    vTaskDelete(sta_disconnect_task_handle);
}

static esp_ip4_addr_t s_ip_addr;

static void on_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    fail_connect=0;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    if (!is_our_netif(TAG, event->esp_netif)) {
        ESP_LOGW(CALLBACK_TAG, "从其他任务 \"%s\"获取到IPv4地址: 忽略", esp_netif_get_desc(event->esp_netif));
        return;
    }
    ESP_LOGI(CALLBACK_TAG, "从任务 \"%s\" 中获取IPv4地址: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
    wifi_ok = true;
    start_https_task();
    extern TaskHandle_t ScreenUpdateFunc_task_handle, https_task_handle;
    if(ScreenUpdateFunc_task_handle != NULL){
        vTaskResume(ScreenUpdateFunc_task_handle);
    }
    if (ipv4_task_handle != NULL) {
        vTaskResume(ipv4_task_handle);
    }
    if (https_task_handle != NULL) {
        vTaskResume(https_task_handle);
    }
    SetWiFiIconVisible(false);
}

static void on_wifi_event(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if(event_base != WIFI_EVENT){
        ESP_LOGW(CALLBACK_TAG, "不是WiFi事件");
        return;
    }

    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    esp_err_t err;
    switch (event_id) {
        // STA事件
        case WIFI_EVENT_STA_START:
            ESP_LOGI(CALLBACK_TAG, "WiFi STA 启动");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            SetWiFiIconVisible(true);
            wifi_ok = false;
            fail_connect++;
            if( mode == WIFI_MODE_STA && fail_connect > STA_FAIL_CONNECT_TIMES){
                ESP_LOGI(CALLBACK_TAG, "WiFi STA 尝试连接次数达到上限 %d次", STA_FAIL_CONNECT_TIMES);
                if (sta_disconnect_task_handle != NULL) {
                    vTaskResume(sta_disconnect_task_handle);
                }
            } else if (mode == WIFI_MODE_APSTA) {
                vTaskDelay(Second(15));
            }
            ESP_LOGI(CALLBACK_TAG, "WiFi断开第 %d 次, 尝试重新连接...", fail_connect);
            if(fail_connect > 40){
                esp_restart();
            }
            err = esp_wifi_connect();
            if (err == ESP_ERR_WIFI_NOT_STARTED) {
                ESP_LOGE(CALLBACK_TAG, "WiFi未启动");
                return;
            }
            ESP_ERROR_CHECK(err);
            SetWiFiIconVisible(true);
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(CALLBACK_TAG, "WiFi连接成功 未获取IP");
            break;
        case WIFI_EVENT_STA_STOP:
            ESP_LOGI(CALLBACK_TAG, "WiFi STA 停止");
            break;
        // AP事件
        case WIFI_EVENT_AP_START:
            ESP_LOGI(CALLBACK_TAG, "WiFi AP 启动");
            Start_webserver();
            xTaskCreate(dns_server_task, "dns_server_task", 4096, NULL, 3, &Dns_Handle_task);
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ap_connect++;
            ESP_LOGI(CALLBACK_TAG, "WiFi AP 连接成功, 已连接数量 %d", ap_connect);
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ap_connect--;
            ESP_LOGI(CALLBACK_TAG, "WiFi AP 断开连接, 已连接数量 %d", ap_connect);
            break;
        case WIFI_EVENT_AP_STOP:
            ESP_LOGI(CALLBACK_TAG, "WiFi AP 停止");
            if(Dns_Handle_task != NULL) {
                End_web();
                vTaskDelete(Dns_Handle_task);
                Dns_Handle_task = NULL;
                ESP_LOGW("DNS", "DNS服务器已关闭");
            }
            break;
        default:
            break;
    }
}

static void wifi_sta_stop(void)
{
    esp_netif_t *wifi_netif = get_netif_from_desc("sta");
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(wifi_netif));
    esp_netif_destroy(wifi_netif);
}

static void wifi_sta_start(void)
{
    char *desc;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    // Prefix the interface description with the module TAG
    // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    asprintf(&desc, "%s: %s", TAG, esp_netif_config.if_desc);
    esp_netif_config.if_desc = desc;
    esp_netif_config.route_prio = 128;
    esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    free(desc);
    esp_wifi_set_default_wifi_sta_handlers();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };
    nvs_read_wifi_msg(&wifi_config);
    if(strlen((char *)wifi_config.sta.password) == 0){
        ESP_LOGW(TAG, "WiFi密码为空");
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
        if(strlen((char *)wifi_config.sta.ssid) == 0){
            ESP_LOGE(TAG, "WiFi名字为空");
            strcpy((char *)wifi_config.sta.ssid, "unkonw-wifiname");
        }
    }
    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
}

esp_err_t ex_wifi_sta_connect(void)
{
    wifi_sta_start();
    xTaskCreate(sta_disconnect_task, "sta_disconnect_task", 4096, NULL, 7, &sta_disconnect_task_handle);
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&wifi_sta_stop));
    return ESP_OK;
}

esp_err_t ex_wifi_sta_disconnect(void)
{
    wifi_sta_stop();
    ESP_ERROR_CHECK(esp_unregister_shutdown_handler(&wifi_sta_stop));
    ESP_LOGI(TAG, "WiFi STA 已断开");
    return ESP_OK;
}

static void wifi_apsta_start(void)
{
    char *desc;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_netif_inherent_config_t esp_netif_sta_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    esp_netif_inherent_config_t esp_netif_ap_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_AP();
    // Prefix the interface description with the module TAG
    // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    asprintf(&desc, "%s: %s", TAG, esp_netif_sta_config.if_desc);
    esp_netif_sta_config.if_desc = desc;
    esp_netif_sta_config.route_prio = 128;
    asprintf(&desc, "%s: %s", TAG, esp_netif_ap_config.if_desc);
    esp_netif_ap_config.if_desc = desc;
    esp_netif_ap_config.route_prio = 128;
    esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_sta_config);
    esp_netif_create_wifi(WIFI_IF_AP, &esp_netif_ap_config);
    free(desc);
    esp_wifi_set_default_wifi_sta_handlers();
    esp_wifi_set_default_wifi_ap_handlers();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,  ESP_EVENT_ANY_ID,       &on_wifi_event, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,    IP_EVENT_STA_GOT_IP,    &on_got_ip,     NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    wifi_config_t wifi_sta_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };
    nvs_read_wifi_msg(&wifi_sta_config);
    if(strlen((char *)wifi_sta_config.sta.password) == 0){
        ESP_LOGW(TAG, "WiFi密码为空");
        wifi_sta_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = WiFi_AP_NAME,
            .ssid_len = strlen(WiFi_AP_NAME),
            .channel = 1,
            .ssid_hidden = 0,
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 4,
            .beacon_interval = 100,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_connect();
}

static void wifi_apsta_stop(void)
{
    esp_netif_t *wifi_sta_netif = get_netif_from_desc("sta");
    esp_netif_t *wifi_ap_netif = get_netif_from_desc("ap");
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(wifi_sta_netif));
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(wifi_ap_netif));
    esp_netif_destroy(wifi_sta_netif);
    esp_netif_destroy(wifi_ap_netif);
}

esp_err_t ex_wifi_apsta_connect(void)
{
    wifi_apsta_start();
    xTaskCreate(apsta_ip_task, "apsta_ip_task", 4096, NULL, 8, &ipv4_task_handle);
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&wifi_apsta_stop));
    return ESP_OK;
}

esp_err_t ex_wifi_apsta_disconnect(void)
{
    wifi_apsta_stop();
    ESP_ERROR_CHECK(esp_unregister_shutdown_handler(&wifi_apsta_stop));
    ESP_LOGI(TAG, "WiFi APSTA 已断开");
    return ESP_OK;
}
