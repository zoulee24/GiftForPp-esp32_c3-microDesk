#ifndef _AUTHENTICATIONSERVER_H__
#define _AUTHENTICATIONSERVER_H__

#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cJSON.h"

#include "esp_http_server.h"
#include "esp_vfs.h"

#include "dns_server.h"
#include "nvs_cz.h"
#include "wifi_cz.h"

#define SCRATCH_BUFSIZE  8192

extern const char html_start[] asm("_binary_uploadscript_html_start");
extern const char html_end[] asm("_binary_uploadscript_html_end");

struct file_server_data {
    /* Base path of file storage */
    char base_path[ESP_VFS_PATH_MAX + 1];

    /* Scratch buffer for temporary storage during file transfer */
    char scratch[SCRATCH_BUFSIZE];
};

// void RunAuthenticationServer(void * param);

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
esp_err_t get_handler(httpd_req_t *req);
esp_err_t post_handler(httpd_req_t *req);
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err);
esp_err_t Start_webserver(void);
esp_err_t End_web(void);
bool GetWiFiCfgUpdate(void);
void SetWiFiCfgUpdate(bool res);

#endif //!_AUTHENTICATIONSERVER_H__