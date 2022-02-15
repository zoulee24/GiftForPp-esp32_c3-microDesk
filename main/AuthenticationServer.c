#include "AuthenticationServer.h"

static const char *SERVERTAG = "SERVER";
static const char *POSTTAG = "POST";
static const char *GETTAG = "GET";
httpd_handle_t server = NULL;
bool isGetWifiCfg = false;

esp_err_t post_handler(httpd_req_t *req)
{
    ESP_LOGI(POSTTAG, "处理接受数据中");
    int total_len = req->content_len;
    int cur_len = 0;
    int received = 0;
    char *buf = ((struct file_server_data *)req->user_ctx)->scratch;

    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }

    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }

    buf[total_len] = '\0';
    ESP_LOGI(POSTTAG, "接收到数据大小为: %d", total_len);

    cJSON *root = cJSON_Parse(buf);
    char *tempSsid = cJSON_GetObjectItem(root, "wifi_name")->valuestring;
    char *tempCode = cJSON_GetObjectItem(root, "wifi_code")->valuestring;
    
    uint8_t len_user_ssid = 0, len_user_password = 0;
    wifi_config_t wifi_config;
    for(uint8_t i = 0; i < 32; i++) {
        if (tempSsid[i] == '*' && tempSsid[i+1] == '$') {
            tempSsid[i] = '\0';
            wifi_config.sta.ssid[i] = '\0';
            len_user_ssid = i + 1;
            break;
        }
        wifi_config.sta.ssid[i] = tempSsid[i];
    }
    
    for(uint8_t i = 0; i < 32; i++) {
        if (tempCode[i] == '*' && tempCode[i+1] == '$') {
            tempCode[i] = '\0';
            wifi_config.sta.password[i] = '\0';
            len_user_password = i + 1;
            break;
        }
        wifi_config.sta.password[i] = tempCode[i];
    }

    ESP_LOGI(POSTTAG, "从服务器接收到新 wifi 配置\r\n\tssid:%s\r\n\tpassword:%s", (const char *)wifi_config.sta.ssid, (const char *)wifi_config.sta.password);
    nvs_write_wifi_msg(wifi_config);
    cJSON_Delete(root);

    SetWiFiCfgUpdate(true);
    /* 发送简单的响应数据包 */
    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    extern TaskHandle_t ipv4_task_handle;
    if (ipv4_task_handle != NULL) {
        vTaskResume(ipv4_task_handle);
    }
    return ESP_OK;
}

esp_err_t Start_webserver(void)
{
    if(server != NULL) {
        ESP_LOGW(SERVERTAG, "服务器已经启动 请勿再次启动");
        return ESP_OK;
    }
    esp_err_t err;
    SetWiFiCfgUpdate(false);
    struct file_server_data *my_server_data = NULL;
    /* Allocate memory for server data */
    my_server_data = calloc(1, sizeof(struct file_server_data));
    if (!my_server_data) {
        ESP_LOGE(SERVERTAG, "无法分配服务器数据缓存");
        return ESP_FAIL;
    }
    ESP_LOGI(SERVERTAG, "服务器数据缓存 创建成功");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 13;
    config.lru_purge_enable = true;

    httpd_uri_t uri_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = get_handler,
        .user_ctx = NULL
    };

    /* POST /uri 的 URI 处理结构 */
    httpd_uri_t uri_post = {
        .uri      = "/wifi_data",
        .method   = HTTP_POST,
        .handler  = post_handler,
        .user_ctx = my_server_data
    };
    
    // Start the httpd server
    ESP_LOGI(SERVERTAG, "开始运行服务器 端口:'%d'", config.server_port);


    err = httpd_start(&server, &config);
    if (err != ESP_OK){
        // Set URI handlers
        ESP_LOGE(SERVERTAG, "http服务器开启失败");
        return err;
    }
        
    httpd_register_uri_handler(server, &uri_get);
    httpd_register_uri_handler(server, &uri_post);
    // httpd_register_uri_handler(server, &wifi_data);
    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    return ESP_OK;
}

esp_err_t End_web(void)
{
    esp_err_t err = ESP_FAIL;
    if(server != NULL) {
        ESP_LOGW(SERVERTAG, "关闭服务器");
        err = httpd_stop(server);
        server = NULL;
    }
    return err;
}

// HTTP GET Handler
esp_err_t get_handler(httpd_req_t *req)
{
    const uint32_t html_len = html_end - html_start;

    ESP_LOGI(GETTAG, "Serve html");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_start, html_len);

    return ESP_OK;
}

// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(SERVERTAG, "Redirecting to root");
    return ESP_OK;
}

bool GetWiFiCfgUpdate(void)
{
    return isGetWifiCfg;
}

void SetWiFiCfgUpdate(bool res)
{
    isGetWifiCfg = res;
}
