#include "nvs_cz.h"
#include "esp_wifi.h"
#include "AuthenticationServer.h"

static const char *NVS_TAG = "NVS";
#define STORAGE_NAMESPACE "storage"
#define WiFi_STORAGE_NAME "WiFi_CONFIG"

#define NVS_WIFI_SSID_NAME          "wifi_s"
#define NVS_WIFI_SSID_LEN_NAME      "wifi_s_len"
#define NVS_WIFI_PASSWORD_NAME      "wifi_p"
#define NVS_WIFI_PASSWORD_LEN_NAME  "wifi_p_len"

//NVS 写入WiFi名称和密码
esp_err_t nvs_write_wifi_msg(wifi_config_t wc)
{
    wc.sta.pmf_cfg.capable = true;
    wc.sta.pmf_cfg.required = false;
    esp_wifi_set_config(WIFI_IF_STA, &wc);
    nvs_handle_t nvs_handle;
    esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "NVS_WRITE: 存储空间打开失败");
        return err;
    }
    uint8_t ssid_len=0, password_len=0;
    ssid_len = ((uint8_t)(strlen((const char *)wc.sta.ssid)))+1;
    password_len = ((uint8_t)(strlen((const char *)wc.sta.password)))+1;
    ESP_LOGI(NVS_TAG, "更新WiFi配置信息: SSID:%s len=%d, PASSWORD:%s len=%d", (const char *)wc.sta.ssid, ssid_len, (const char *)wc.sta.password, password_len);
    err = nvs_set_u8(nvs_handle, NVS_WIFI_SSID_LEN_NAME, ssid_len);
    vTaskDelay(10 / portTICK_RATE_MS);
    err = nvs_set_u8(nvs_handle, NVS_WIFI_PASSWORD_LEN_NAME, password_len);
    vTaskDelay(10 / portTICK_RATE_MS);
    err = nvs_set_str(nvs_handle, NVS_WIFI_SSID_NAME, (const char *)wc.sta.ssid);
    vTaskDelay(10 / portTICK_RATE_MS);
    err = nvs_set_str(nvs_handle, NVS_WIFI_PASSWORD_NAME, (const char *)wc.sta.password);
    if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "NVS_WRITE: 存储空间写入失败");
    } else {
        err = nvs_commit(nvs_handle);
    }
    ESP_LOGI(NVS_TAG, "存储空间关闭");
    nvs_close(nvs_handle);
    ESP_LOGI(NVS_TAG, "WiFi名称和密码写入成功");
    return err;
}

//NVS 读取WiFi名称和密码
esp_err_t nvs_read_wifi_msg(wifi_config_t *wc)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "NVS_READ: 存储空间打开失败");
        return err;
    }
    uint8_t ssid_len=0, password_len=0;
    nvs_get_u8(nvs_handle, NVS_WIFI_SSID_LEN_NAME, &ssid_len);
    vTaskDelay(10 / portTICK_RATE_MS);
    nvs_get_u8(nvs_handle, NVS_WIFI_PASSWORD_LEN_NAME, &password_len);
    vTaskDelay(10 / portTICK_RATE_MS);
    if(ssid_len == 0 || password_len == 0) {
        ESP_LOGE(NVS_TAG, "NVS_READ: WiFi长度读取失败");
        printf("ssid_len=%d, password_len=%d\n", ssid_len, password_len);
        return ESP_FAIL;
    }
    size_t temp = (size_t)ssid_len;
    nvs_get_str(nvs_handle, NVS_WIFI_SSID_NAME, (char *)wc->sta.ssid, &temp);
    vTaskDelay(10 / portTICK_RATE_MS);
    temp = (size_t)password_len;
    nvs_get_str(nvs_handle, NVS_WIFI_PASSWORD_NAME, (char *)wc->sta.password, &temp);

    ESP_LOGI(NVS_TAG, "获取WiFi配置 \r\n\twifi_ssid = %s\tlen=%d\r\n\twifi_password = %s\tlen=%d", wc->sta.ssid, ssid_len, wc->sta.password, password_len);

    ESP_LOGI(NVS_TAG, "存储空间关闭");
    nvs_close(nvs_handle);
    ESP_LOGI(NVS_TAG, "WiFi名称和密码读取成功");
    return err;
}
