#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"


void https_get_request(void)
{
    esp_tls_cfg_t cfg = {
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    char buf[512];
    int ret, len;

    struct esp_tls *tls = esp_tls_conn_http_new(TIME_URL, &cfg);

    if (tls != NULL) {
        ESP_LOGI(TIME_URL, "Connection established...");
    } else {
        ESP_LOGE(TIME_URL, "Connection failed...");
        fail_count++;
        goto exite;
    }

    size_t written_bytes = 0;
    do {
        ret = esp_tls_conn_write(tls,
                                 REQUEST + written_bytes,
                                 sizeof(REQUEST) - written_bytes);
        if (ret >= 0) {
            // ESP_LOGI(TIME_URL, "%d bytes written", ret);
            written_bytes += ret;
        } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(TIME_URL, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
            goto exite;
        }
    } while (written_bytes < sizeof(REQUEST));

    // ESP_LOGI(TIME_URL, "Reading HTTP response...");

    do {
        len = sizeof(buf) - 1;
        bzero(buf, sizeof(buf));
        ret = esp_tls_conn_read(tls, (char *)buf, len);

        if (ret == ESP_TLS_ERR_SSL_WANT_WRITE  || ret == ESP_TLS_ERR_SSL_WANT_READ) {
            continue;
        }

        if (ret < 0) {
            ESP_LOGE(TIME_URL, "esp_tls_conn_read  returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));
            break;
        }

        if (ret == 0) {
            ESP_LOGI(TIME_URL, "connection closed");
            break;
        }

        len = ret;
        char *res = strchr(buf, '{');
        if(res){
            int len = strlen(res);
            for(int i=0;i<len;i++){
                if(res[i] == '}'){
                    res[i+1] = '\0';
                    break;
                }
            }
            printf("rx:\t%s\r\n", res);
        }
    } while (1);

exite:
    esp_tls_conn_delete(tls);
    for (int countdown = 5; countdown >= 0; countdown--) {
        printf("[INFO]\tClosing connection in %d seconds...\r", countdown);
        // ESP_LOGI(TIME_URL, "%d...", countdown);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("\r\n");
}
