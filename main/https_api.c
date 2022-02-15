#include "https_api.h"
#include "math.h"
#include "esp_sleep.h"

#define TIME_SERVER     "api.uukit.com"
#define TIME_URL        "https://api.uukit.com/time"
#define WEATHER_SERVER  "api.seniverse.com"
#define WEATHER_URL     "https://api.seniverse.com/v3/weather/now.json?key=SA7qWQTq6xHv4SaXG&location=%s"
#define LOCATION_SERVER	"api.asilu.com"
#define LOCATION_URL	"https://api.asilu.com/geoip/"
#define DATE_SERVER     "hn216.api.yesapi.cn"
#define DATE_URL    	"https://hn216.api.yesapi.cn/?s=App.Common_Date.GetDaysFromNow&return_data=0&yearStart=2021&monthStart=12&dayStart=26&app_key=19789DF5CE26549E5EB0956A290BDBFE&sign=13434A79E5CF0BB62DEA1852EBEEFD56"

#define UserAgent "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.99 Safari/537.36 Edg/97.0.1072.69"
#define REQUEST "GET %s HTTP/1.1\r\n"\
                "Host: %s\r\n"\
                UserAgent "\r\n\r\n"

static const char *HTTPSTAG = "https";
uint32_t fail_count = 0;

TaskHandle_t https_task_handle = NULL;
static char *now_city = NULL;

struct esp_tls *tls = NULL;

void print_weather(WeatherInfo info);
void print_time(sysTime st);
void lv_update_weather();
void HexToStr(char *Hex, char **Str);
esp_err_t deal_time(char *http_res_buf);
esp_err_t deal_weather(char *http_res_buf);
esp_err_t deal_date(char *http_res_buf);
esp_err_t deal_location(char *http_res_buf);
void SetWiFiIconVisible(bool enable);

void https_task(void *param)
{
    extern bool wifi_ok;
    if(!wifi_ok) {
        ex_wifi_sta_connect();
        vTaskSuspend(NULL);
    }
    goto updateStart;
updateReStart:
    SetWiFiIconVisible(false);
    ESP_LOGI("HTTPS", "更新失败 准备断开WiFi重新尝试");
    vTaskDelay(25000 / portTICK_PERIOD_MS);
    ex_wifi_sta_disconnect();
    ESP_LOGI("HTTPS", "准备连接WiFi重新尝试");
    vTaskDelay(25000 / portTICK_PERIOD_MS);
    ex_wifi_sta_connect();
updateStart:
    if(https_get_update(HTTPS_TIME) != ESP_OK){
        goto updateReStart;
    }
    if(https_get_update(HTTPS_LOCATION) != ESP_OK){
        goto updateReStart;
    }
    if(https_get_update(HTTPS_WEATHER) != ESP_OK){
        goto updateReStart;
    }
    if(https_get_update(HTTPS_DATE) != ESP_OK){
        goto updateReStart;
    }
    lv_update_weather();
    ex_wifi_sta_disconnect();
    https_task_handle = NULL;
    // esp_light_sleep_start();
    vTaskDelete(https_task_handle);
}

void start_https_task(void)
{
    if(https_task_handle == NULL)
    {
        xTaskCreate(https_task, "https_task", 4096*2, NULL, 4, &https_task_handle);
    }
}

esp_err_t get_tls(HTTPS_TYPE type, struct esp_tls** tls)
{
    esp_tls_cfg_t tls_cfg = {
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    if (type == HTTPS_TIME) {
        ESP_LOGI(HTTPSTAG, "准备更新时间");
        *tls = esp_tls_conn_http_new(TIME_URL, &tls_cfg);
        return ESP_OK;
    }
    else if (type == HTTPS_WEATHER) {
        ESP_LOGI(HTTPSTAG, "准备更新天气");
        if(now_city == NULL) {
            ESP_LOGE(HTTPSTAG, "now_city为NULL");
            return ESP_FAIL;
        }
        char *hex_str = NULL;
        HexToStr(now_city, &hex_str);
        size_t len = strlen(hex_str);
        char *url = (char *)malloc(sizeof(WEATHER_URL) + len - 2);
        sprintf(url, WEATHER_URL, hex_str);
        if(hex_str != NULL) {
            free(hex_str);
            hex_str = NULL;
        }
        *tls = esp_tls_conn_http_new(url, &tls_cfg);
        free(url);
        url = NULL;
        return ESP_OK;
    } else if (type == HTTPS_LOCATION) {
        ESP_LOGI(HTTPSTAG, "准备更新地址");
        *tls = esp_tls_conn_http_new(LOCATION_URL, &tls_cfg);
        return ESP_OK;
    } else if (type == HTTPS_DATE) {
        ESP_LOGI(HTTPSTAG, "准备更新纪念日");
        *tls = esp_tls_conn_http_new(DATE_URL, &tls_cfg);
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
    return ESP_FAIL;
}

esp_err_t getRequestCharAndLen(HTTPS_TYPE type, char **request, size_t *len)
{
    if (*request != NULL){
        ESP_LOGE(HTTPSTAG, "request不为NULL free(request)");
        free(*request);
        *request = NULL;
    }
    size_t request_len = sizeof(REQUEST);
    switch (type)
    {
    case HTTPS_TIME:
        *len = request_len + sizeof(TIME_URL) + sizeof(TIME_SERVER) - 6;
        *request = (char *)malloc(*len);
        sprintf(*request, REQUEST, TIME_URL, TIME_SERVER);
        *len = strlen(*request) + 1;
        return ESP_OK;
    case HTTPS_LOCATION:
        *len = request_len + sizeof(LOCATION_URL) + sizeof(LOCATION_SERVER) - 6;
        *request = (char *)malloc(*len);
        sprintf(*request, REQUEST, LOCATION_URL, LOCATION_SERVER);
        *len = strlen(*request) + 1;
        return ESP_OK;
    case HTTPS_WEATHER:
        if(now_city == NULL) {
            ESP_LOGE(HTTPSTAG, "now_city为NULL");
            return ESP_FAIL;
        }
        char *hex_str = NULL;
        HexToStr(now_city, &hex_str);
        *len = strlen(hex_str);
        char *url = (char *)malloc(sizeof(WEATHER_URL) + *len - 2);
        sprintf(url, WEATHER_URL, hex_str);
        if(hex_str != NULL) {
            free(hex_str);
            hex_str = NULL;
        }
        
        *len = request_len + strlen(url) + sizeof(WEATHER_SERVER) - 5;
        *request = (char *)malloc(*len);
        sprintf(*request, REQUEST, url, WEATHER_SERVER);
        *len = strlen(*request) + 1;
        free(url);
        url = NULL;
        return ESP_OK;
    case HTTPS_DATE:
        *len = request_len + sizeof(DATE_URL) + sizeof(DATE_SERVER) - 6;
        *request = (char *)malloc(*len);
        sprintf(*request, REQUEST, DATE_URL, DATE_SERVER);
        *len = strlen(*request) + 1;
        return ESP_OK;
    default:
        return ESP_FAIL;
    }
    return ESP_FAIL;
}

// 更新主函数
esp_err_t https_get_update(HTTPS_TYPE type)
{
    esp_err_t err = ESP_FAIL;
    char *rq = NULL;
    char buf[512];
    int ret, len, connect_times = 0;
    if(tls != NULL) {
        esp_tls_conn_delete(tls);
        tls = NULL;
    }
    if(get_tls(type, &tls) != ESP_OK) {
        ESP_LOGE(HTTPSTAG, "get_tls failed...");
        goto exit;
    }

    if (tls != NULL) {
        ESP_LOGI(HTTPSTAG, "服务器连接成功...");
    } else {
        ESP_LOGE(HTTPSTAG, "服务器连接失败...");
        fail_count++;
        goto exit;
    }
    size_t written_bytes = 0, rq_len;
    if(getRequestCharAndLen(type, &rq, &rq_len) != ESP_OK)
    {
        ESP_LOGE(HTTPSTAG, "函数 getRequestCharAndLen 出错");
        goto exit;
    }
    if(rq == NULL) {
        ESP_LOGE(HTTPSTAG, "rq == NULL");
        goto exit;
    }
    do {
        if(connect_times++ > 8){
            ESP_LOGE("HTTPS", "连接次数超时");
            goto exit;
        }
        ESP_LOGI("HTTPS", "esp_tls_conn_write 第%d次连接", connect_times);
        ret = esp_tls_conn_write(tls, rq + written_bytes, rq_len - written_bytes);
        if (ret >= 0) {
            // ESP_LOGI(HTTPSTAG, "%d bytes written", ret);
            written_bytes += ret;
        } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(HTTPSTAG, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
            goto exit;
        }
    } while (written_bytes < rq_len);
    connect_times = 0;
    do {
        if(connect_times++ > 8){
            ESP_LOGE("HTTPS", "连接次数超时");
            goto exit;
        }
        ESP_LOGI("HTTPS", "esp_tls_conn_read 第%d次连接", connect_times);
        len = sizeof(buf) - 1;
        bzero(buf, sizeof(buf));
        ret = esp_tls_conn_read(tls, (char *)buf, len);
        if (ret == ESP_TLS_ERR_SSL_WANT_WRITE  || ret == ESP_TLS_ERR_SSL_WANT_READ) {
            continue;
        }
        if (ret < 0) {
            ESP_LOGE(HTTPSTAG, "esp_tls_conn_read  returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));
            break;
        }
        if (ret == 0) {
            ESP_LOGI(HTTPSTAG, "connection closed");
            break;
        }

        len = ret;
        switch(type)
        {
            case HTTPS_TIME:
                if(deal_time(buf) == ESP_OK){
                    err = ESP_OK;
                    goto exit;
                }
                break;
            case HTTPS_WEATHER:
                if(deal_weather(buf) == ESP_OK){
                    err = ESP_OK;
                    goto exit;
                }
                break;
            case HTTPS_LOCATION:
            {
                if(deal_location(buf) == ESP_OK){
                    err = ESP_OK;
                    goto exit;
                }
                break;
            }
            case HTTPS_DATE:
            {
                if(deal_date(buf) == ESP_OK){
                    err = ESP_OK;
                    goto exit;
                }
                break;
            }
            default:
                break;
        }
    } while (1);

exit:
    if(rq != NULL) {
        free(rq);
        rq = NULL;
    }
    connect_times = 0;
    ESP_LOGI(HTTPSTAG, "连接关闭");
    esp_tls_conn_delete(tls);
    tls = NULL;
    return err;
}

char * find_json(char *http_res_buf){
    char *res = strchr(http_res_buf, '{');
    if(res){
        int len = strlen(res), end = 0;
        for(int i=0;i<len;i++){
            if(res[i] == '}'){
                end = i+1;
            }
        }
        if (end > 0 && end < len+1){
            res[end] = '\0';
        } else {
            ESP_LOGE(HTTPSTAG, "json格式错误 找到{ 未找到}");
            return NULL;
        }
    } else {
        return NULL;
    }
    return res;
}

esp_err_t deal_weather(char *http_res_buf){
    char *json = find_json(http_res_buf);
    if (json) {
        extern WeatherInfo weather_info;
        cJSON* root = cJSON_Parse(json);
        if (root != NULL){
            cJSON* results =cJSON_GetObjectItem(root,"results");
            int arraySize = cJSON_GetArraySize(results);
            if (arraySize > 0 ) {
                cJSON * result = cJSON_GetArrayItem(results, 0);
                cJSON * location = cJSON_GetObjectItem(result, "location");
                cJSON* city =cJSON_GetObjectItem(location,"name");
                strcpy(weather_info.city, city->valuestring);
                cJSON* weather_result =cJSON_GetObjectItem(result,"now");
                //天气状况
                cJSON* temp =cJSON_GetObjectItem(weather_result,"text");
                strcpy(weather_info.weather, temp->valuestring);
                //温度
                temp = cJSON_GetObjectItem(weather_result,"temperature");
                strcpy(weather_info.temperature, temp->valuestring);
                //湿度
                temp = cJSON_GetObjectItem(weather_result,"humidity");
                strcpy(weather_info.humidity, temp->valuestring);
                //风向
                temp = cJSON_GetObjectItem(weather_result,"wind_direction");
                strcpy(weather_info.wind, temp->valuestring);
                //天气代号
                temp = cJSON_GetObjectItem(weather_result,"code");
                weather_info.weather_code = atoi(temp->valuestring);
                //天气更新日期
                cJSON* last_update_data =cJSON_GetObjectItem(result,"last_update");
                strcpy(weather_info.date, last_update_data->valuestring);

                if(now_city != NULL){
                    free(now_city);
                    now_city = NULL;
                }
            } else {
                cJSON_Delete(root);
                return ESP_FAIL;
            }
            cJSON_Delete(root);
            print_weather(weather_info);
        } else {
            ESP_LOGE(HTTPSTAG, "cJSON NULL");
            return ESP_FAIL;
        }
    } else {
        ESP_LOGW(HTTPSTAG, "无法找到JSON");
        return ESP_FAIL;
    }
	return ESP_OK;
}

esp_err_t deal_date(char *http_res_buf){
    char *json = find_json(http_res_buf);
    if (json) {
        cJSON* root = cJSON_Parse(json);
        if (root != NULL){
            cJSON* data =cJSON_GetObjectItem(root,"data");
            if(data == NULL){
                cJSON_Delete(root);
                return ESP_FAIL;
            }
            cJSON* diff =cJSON_GetObjectItem(data,"diff");
            if(diff == NULL){
                cJSON_Delete(root);
                return ESP_FAIL;
            }
            extern int Countdown_days;
            Countdown_days = diff->valueint;
            printf("更新: 在一起已经%d天\n", Countdown_days);
            cJSON_Delete(root);
        } else {
            ESP_LOGE(HTTPSTAG, "cJSON NULL");
            return ESP_FAIL;
        }
    } else {
        ESP_LOGE(HTTPSTAG, "无法找到JSON");
        return ESP_FAIL;
    }
	return ESP_OK;
}

esp_err_t deal_time(char *http_res_buf){
    char *json = find_json(http_res_buf);
    if (json) {
        cJSON* root = cJSON_Parse(json);
        if(root == NULL){
            ESP_LOGE(HTTPSTAG, "cJSON_Parse NULL");
            cJSON_Delete(root);
            return ESP_FAIL;
        }
        cJSON* time = cJSON_GetObjectItem(root,"data");
        if(time == NULL){
            ESP_LOGE(HTTPSTAG, "result error");
            cJSON_Delete(root);
            return ESP_FAIL;
        }
        cJSON* gmt = cJSON_GetObjectItem(time,"gmt");
        char temp[19];
        int temp_time = 0;
        strcpy(temp, gmt->valuestring);
        extern sysTime realtime;
        realtime.second = atoi(temp+17);
        temp[16] = '\0';
        realtime.min = atoi(temp+14);
        temp[13] = '\0';
        temp_time = atoi(temp+11);
        if(temp_time+8>23){
            realtime.hour = temp_time - 16;
            temp_time = 1;
        } else{
            realtime.hour = temp_time + 8;
            temp_time = 0;
        }
        extern int last_update_hour;
        last_update_hour = realtime.hour;
        temp[10] = '\0';
        realtime.date = atoi(temp+9)+temp_time;
        if(realtime.date > 31){
            realtime.date = 1;
            temp_time = 1;
        } else {
            temp_time = 0;
        }
        temp[8] = '\0';
        realtime.mounth = atoi(temp+5)+temp_time;
        if(realtime.mounth > 12){
            realtime.mounth = 1;
            temp_time = 1;
        } else {
            temp_time = 0;
        }
        temp[4] = '\0';
        realtime.year = atoi(temp)+temp_time;
        print_time(realtime);
        cJSON_Delete(root);
    } else {
        ESP_LOGW(HTTPSTAG, "无法找到JSON");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t deal_location(char *http_res_buf){
    if (http_res_buf[0] != '{') {
        return ESP_FAIL;
    }
    char *json = find_json(http_res_buf);
    if (json) {
        cJSON* root = cJSON_Parse(json);
        if(root == NULL){
            ESP_LOGE(HTTPSTAG, "cJSON_Parse NULL");
            cJSON_Delete(root);
            return ESP_FAIL;
        }
        cJSON* city = cJSON_GetObjectItem(root,"city");
        if(city == NULL){
            ESP_LOGE(HTTPSTAG, "错误\t找不到JSON dz");
            cJSON_Delete(root);
            return ESP_FAIL;
        }
        if(now_city != NULL){
            ESP_LOGI(HTTPSTAG, "now_city != NULL 释放内存");
            free(now_city);
            now_city = NULL;
        }
        now_city = (char*)malloc(strlen(city->valuestring)+1);
        strcpy(now_city, city->valuestring);
        cJSON_Delete(root);
    } else {
        ESP_LOGW(HTTPSTAG, "无法找到JSON");
        return ESP_FAIL;
    }
    return ESP_OK;
}

// Func Conver uint8_t Hex to Hex char* input uint8_t char** return void
void HexToStr(char *Hex, char **Str){
    int Hex_len = strlen(Hex);
    if(*Str != NULL){
        free(*Str);
        *Str = NULL;
    }
    char *temp = (char *)malloc(Hex_len*2+1);
    *Str = (char *)malloc(Hex_len*3+1);
    for(int i = 0; i<Hex_len; i++) {
        temp[2*i] = Hex[i] >> 4;
        temp[2*i+1] = Hex[i] & 0xF;
    }
    for(int i = 0, j = 0; i<Hex_len*2;) {
        (*Str)[j++] = '%';
        sprintf(&((*Str)[j++]),"%X",temp[i++]);
        sprintf(&((*Str)[j++]),"%X",temp[i++]);
    }
    (*Str)[Hex_len*3] = '\0';
    free(temp);
    temp = NULL;
}

