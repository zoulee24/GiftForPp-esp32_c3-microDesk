#include "http_api.h"
#include "lvgl_task.h"
#include "esp_netif.h"

static char http_res_buf[MAX_HTTP_OUTPUT_BUFFER] = {0};   //用于接收通过http协议返回的数据
TaskHandle_t update_time_task_handle = NULL;
esp_err_t cJSON_deal_weather(cJSON * root, WeatherInfo *info);
void print_weather(WeatherInfo info);

esp_err_t http_time_api(sysTime *systime)
{
	esp_err_t err = ESP_FAIL;
    esp_http_client_config_t config ;
    memset(&config,0,sizeof(config));
	config.timeout_ms = 2000;
    config.url = TIME_URL;
    esp_http_client_handle_t client = esp_http_client_init(&config);	//初始化http连接

    //设置发送请求 
    err = esp_http_client_set_method(client, HTTP_METHOD_GET);
	if (err != ESP_OK) {
        ESP_LOGE(HTTPTAG, "Failed to set HTTP method: %s", esp_err_to_name(err));
		return err;
	}
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(HTTPTAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
		if (client != NULL) {
			esp_err_t close_err = esp_http_client_cleanup(client);
			if (close_err != ESP_OK) {
				ESP_LOGE(HTTPTAG, "esp http client close error: %s", esp_err_to_name(close_err));
			}
		}
		return err;
    }
	//如果协议头长度小于0，说明没有成功读取到
	if (esp_http_client_fetch_headers(client) < 0) {
		ESP_LOGE(HTTPTAG, "HTTP client fetch headers failed");
		return ESP_FAIL;
	} 
	//读取目标主机通过http的响应内容
	int data_read = esp_http_client_read_response(client, http_res_buf, MAX_HTTP_OUTPUT_BUFFER);
	if (data_read >= 0 && esp_http_client_get_status_code(client) == 200) {
		if (strlen(http_res_buf) == 41){
			http_res_buf[39] = '\0';
			char temp[20];
			strcpy(temp, http_res_buf+20);
			systime->second = atoi(temp+17);
			temp[16] = '\0';
			systime->min = atoi(temp+14);
			//测试待会改
			extern int last_update_hour;
			last_update_hour = systime->min;
			temp[13] = '\0';
			systime->hour = atoi(temp+10);
			// extern int last_update_hour;
			// last_update_hour = systime->hour;
			temp[10] = '\0';
			systime->date = atoi(temp+8);
			temp[7] = '\0';
			systime->mounth = atoi(temp+5);
			temp[4] = '\0';
			systime->year = atoi(temp);
			print_time(*systime);
		} else {
			ESP_LOGE(HTTPTAG, "获取时间失败");
			err = ESP_FAIL;
		}
	} else {
		ESP_LOGE(HTTPTAG, "Failed to read response");
		err = ESP_FAIL;
	}
	memset(&http_res_buf,0,MAX_HTTP_OUTPUT_BUFFER);
    //关闭连接
	if (client != NULL) {
    	esp_err_t close_err = esp_http_client_cleanup(client);
		if (err != ESP_OK) {
			return err;
		} else if (close_err != ESP_OK) {
			ESP_LOGE(HTTPTAG, "esp http client close error: %s", esp_err_to_name(close_err));
		}
	}
	ESP_LOGI(HTTPTAG, "esp http client close");
	return ESP_OK;
}

esp_err_t http_weather_api(WeatherInfo *info)
{
    esp_http_client_config_t config ;
	esp_err_t err = ESP_FAIL;
    memset(&config,0,sizeof(config));
    config.url = WEATHER_URL;
	config.timeout_ms = 2000;
	config.user_agent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/60.0.3100.0 Safari/537.36";
    esp_http_client_handle_t client = esp_http_client_init(&config);	//初始化http连接
	if (client == NULL) {
		return ESP_FAIL;
	}
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(HTTPTAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
		if (client != NULL) {
			esp_err_t close_err = esp_http_client_cleanup(client);
			if (close_err != ESP_OK) {
				ESP_LOGE(HTTPTAG, "esp http client close error: %s", esp_err_to_name(close_err));
			}
		}
		return err;
    } else {
        //如果协议头长度小于0，说明没有成功读取到
        if (esp_http_client_fetch_headers(client) < 0) {
            ESP_LOGE(HTTPTAG, "HTTP client fetch headers failed");
			err = ESP_FAIL;
        } else{
            if (esp_http_client_read_response(client, http_res_buf, MAX_HTTP_OUTPUT_BUFFER) >= 0 && esp_http_client_get_status_code(client) == 200) {
				int len_http_res_buf = esp_http_client_get_content_length(client);
                // printf("data:%s\n", http_res_buf);
				cJSON* root = cJSON_Parse(http_res_buf);
				cJSON_deal_weather(root, info);
            } else {
                ESP_LOGE(HTTPTAG, "Failed to read response");
				err = ESP_FAIL;
            }
        }
    }
	memset(&http_res_buf,0,MAX_HTTP_OUTPUT_BUFFER);
    //关闭连接
    esp_err_t close_err = esp_http_client_cleanup(client);
	if (err != ESP_OK) {
		return err;
	} else if (close_err != ESP_OK) {
		ESP_LOGE(HTTPTAG, "esp http client close error");
	}
	ESP_LOGI(HTTPTAG, "esp http client close");
	return ESP_OK;
}

void print_time(sysTime st)
{
	printf("时间:\t%d年%d月%d日\t%d:%d:%d\r\n", st.year, st.mounth, st.date, st.hour, st.min, st.second);
}

esp_err_t cJSON_deal_weather(cJSON * root, WeatherInfo *info) {
	esp_err_t err;
	if (root != NULL){
		cJSON* results =cJSON_GetObjectItem(root,"results");
		int arraySize = cJSON_GetArraySize(results);
		if (arraySize > 0 ) {
			cJSON * result = cJSON_GetArrayItem(results, 0);
			cJSON * location = cJSON_GetObjectItem(result, "location");
			cJSON* city =cJSON_GetObjectItem(location,"name");
			strcpy(info->city, city->valuestring);
			cJSON* weather_result =cJSON_GetObjectItem(result,"now");
			//天气状况
			cJSON* temp =cJSON_GetObjectItem(weather_result,"text");
			strcpy(info->weather, temp->valuestring);
			//温度
			temp = cJSON_GetObjectItem(weather_result,"temperature");
			strcpy(info->temperature, temp->valuestring);
			//湿度
			temp = cJSON_GetObjectItem(weather_result,"humidity");
			strcpy(info->humidity, temp->valuestring);
			//风向
			temp = cJSON_GetObjectItem(weather_result,"wind_direction");
			strcpy(info->wind, temp->valuestring);
			//天气代号
			temp = cJSON_GetObjectItem(weather_result,"code");
			info->weather_code = atoi(temp->valuestring);
			//天气更新日期
			cJSON* last_update_data =cJSON_GetObjectItem(result,"last_update");
			strcpy(info->date, last_update_data->valuestring);
		}
		cJSON_Delete(root);
		print_weather(*info);
		err = ESP_OK;
	} else {
		ESP_LOGE(HTTPTAG, "cJSON NULL");
		err = ESP_FAIL;
	}
	return err;
}

void print_weather(WeatherInfo info) {
	printf("[INFO] 日期信息打印\r\n\t日期:%s\r\n\t城市:%s\r\n\t天气更新日期:%s\r\n\t温度:%s℃\t\n\t湿度:%s %%\r\n\t风向:%s\r\n\t天气代号:%d\r\n", 
	info.date, info.city, info.weather, info.temperature, info.humidity, info.wind, info.weather_code);
}

void updateTime_task(void *Parmar)
{
	esp_err_t err = ESP_FAIL;
	extern sysTime realtime;
	static uint8_t count = 0;
	static uint8_t toll_count = 0;
	do{
		vTaskDelay(3000 / portTICK_PERIOD_MS);
		if (wifi_ap_state == WIFI_AP_OK){
			ESP_LOGE("TIME", "时间更新任务 无网络状态");
			update_time_task_handle = NULL;
			vTaskDelete(update_time_task_handle);
		} else if (wifi_sta_state == WIFI_STA_GOT_IP){
			break;
		}else if(wifi_sta_state == WIFI_STA_CONNECTING || wifi_sta_state == WIFI_STA_CONNECTED){
			ESP_LOGW("TIME", "时间更新任务 等待WiFi获取IP %d", wifi_sta_state);
			continue;
		} else if (wifi_sta_state == WIFI_STOP) {
			ESP_LOGE("TIME", "时间更新任务 WiFi状态异常 %d 设置WiFi为sta", wifi_sta_state);
			wifi_sta();
		} 
	}while(wifi_sta_state != WIFI_STA_GOT_IP);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	do{
		err = http_time_api(&realtime);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}while(err != ESP_OK && count++ < 3);
	if (count == 3){
		toll_count++;
	}
	count = 0;
	extern WeatherInfo weather_info;
	err = ESP_FAIL;
	do{
		err = http_weather_api(&weather_info);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}while (err != ESP_OK && count++ < 3);
	if (count == 3){
		toll_count++;
	}
	count = 0;
	ESP_LOGI("TIME", "总更新失败次数%d", toll_count);
	lv_update_weather(weather_info);
	wifi_sta_state = WIFI_STA_DISCONNECTING;
	esp_wifi_stop();
	update_time_task_handle = NULL;
	vTaskDelete(update_time_task_handle);
}

void update_time(void)
{
	if(update_time_task_handle == NULL) {
		xTaskCreate(updateTime_task, "update_time_task", 4096, NULL, 3, &update_time_task_handle);
	} else {
		ESP_LOGW(HTTPTAG, "更新时间程序正在运行!");
	}
}

/*
bool http_location_task()
{
	bool res = false;
	int content_length = 0;  //http协议头的长度
    

    //02-2 配置http结构体
   
   //定义http配置结构体，并且进行清零
    esp_http_client_config_t config ;
    memset(&config,0,sizeof(config));

    //向配置结构体内部写入url
	#define WEATHER_URL4	"http://api.asilu.com/ip/"
    static const char *URL = WEATHER_URL4;
    config.url = URL;

    //初始化结构体
    esp_http_client_handle_t client = esp_http_client_init(&config);	//初始化http连接
	if (client == NULL) {
		return false;
	}
    //设置发送请求 
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    //02-3 循环通讯
    // 与目标主机创建连接，并且声明写入内容长度为0
    esp_err_t err = esp_http_client_open(client, 0);

    //如果连接失败
    if (err != ESP_OK) {
        ESP_LOGE(HTTPTAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    } 
    //如果连接成功
    else {

        //读取目标主机的返回内容的协议头
        content_length = esp_http_client_fetch_headers(client);

        //如果协议头长度小于0，说明没有成功读取到
        if (content_length < 0) {
            ESP_LOGE(HTTPTAG, "HTTP client fetch headers failed");
        } 

        //如果成功读取到了协议头
        else {

            //读取目标主机通过http的响应内容
            int data_read = esp_http_client_read_response(client, http_res_buf, MAX_HTTP_OUTPUT_BUFFER);
            if (data_read >= 0 && esp_http_client_get_status_code(client) == 200) {

                //打印响应内容，包括响应状态，响应体长度及其内容
				int len_http_res_buf = esp_http_client_get_content_length(client);
                ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),				//获取响应状态信息
                len_http_res_buf);			//获取响应信息长度
				// http_res_buf[len_http_res_buf] = '\0';
                printf("data:%s\n", http_res_buf);
				res = true;
                cJSON* root = NULL;
                root = cJSON_Parse(http_res_buf);
				if (root != NULL){
                	// cJSON* time =cJSON_GetObjectItem(root,"sysTime2");
                	cJSON* lc =cJSON_GetObjectItem(root,"dz");
					if (lc != NULL){
						ESP_LOGW(TAG, "cJSON lc is not NULL");
                        printf("lc->valuestring:%s\r\n", lc->valuestring);
                        char * tt = strstr(lc->valuestring, "省");
                        if (tt != NULL){
                            printf("tt not NULL\tlc->valuestring:%s\r\n", tt);
                            printf("WiFi地区:%s\r\n", tt+4);
                            strcpy(location_char, tt+4);
                            printf("复制WiFi地区:%s\r\n", location_char);
                        } else {
                            printf("tt is NULL\r\n");
                        }
						res = true;
					} else {
						ESP_LOGE(TAG, "cJSON lc is NULL");
					}
				} else {
					ESP_LOGE(TAG, "cJSON root is NULL");
				}
				cJSON_Delete(root);
            } 
            //如果不成功
            else {
    			memset(&http_res_buf,0,MAX_HTTP_OUTPUT_BUFFER);
                ESP_LOGE(TAG, "Failed to read response");
            }
        }
    }

	ESP_LOGI(TAG, "Failed to read response");
    //关闭连接
    esp_http_client_close(client);
	ESP_LOGI(TAG, "esp http client close");
	return res;
}
*/
