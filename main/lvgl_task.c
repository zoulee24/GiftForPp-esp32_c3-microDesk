#include "lvgl_task.h"
#include "images.h"

// static void set_temperature_bar(void *bar, int32_t temp);
static void humidity_bar_event_cb(lv_event_t * e);
static void set_humidity_bar(void *bar, int32_t v);
void lv_tick_task(void *arg)
{
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

lv_obj_t *waitScreen_obj = NULL, *mainScreen_obj = NULL;
lv_obj_t *temperature_label = NULL, *weather_label = NULL, *img_obj = NULL, *city_label = NULL, *humidity_bar = NULL;
lv_obj_t *time_min_label = NULL, *time_hour_label = NULL, *wifi_img = NULL;
lv_obj_t *Countdown_days_label = NULL, *msg_label = NULL;

TaskHandle_t ScreenUpdateFunc_task_handle = NULL;

SemaphoreHandle_t xGuiSemaphore;

#define CHINESE_NORMAL HanBiaoCuYuan35
#define CHINESE_SPECIAL comic28
#define CHINESE_NUM comic60
#define CHINESE_MSG HuaWenNewWei50
#define CHINESE_TIME HuaWenNewWei90
#define SET_CHINESE(obj, style) lv_obj_add_style(obj, &style, LV_STATE_DEFAULT)
LV_FONT_DECLARE(CHINESE_NORMAL);
LV_FONT_DECLARE(CHINESE_SPECIAL);
LV_FONT_DECLARE(CHINESE_NUM);
LV_FONT_DECLARE(CHINESE_MSG);
LV_FONT_DECLARE(CHINESE_TIME);

void MainScreen(){
    // 设置字体大小 66
    static lv_style_t font_style_cn_66_up;
    lv_style_init(&font_style_cn_66_up);
    //橙色
    lv_style_set_text_color(&font_style_cn_66_up, lv_color_make(0xFF, 0x99, 0x00));
    lv_style_set_text_font(&font_style_cn_66_up, &CHINESE_TIME);

    static lv_style_t font_style_cn_66_down;
    lv_style_init(&font_style_cn_66_down);
    //蓝色
    lv_style_set_text_color(&font_style_cn_66_down, lv_color_make(0x33, 0x66, 0xCC));
    lv_style_set_text_font(&font_style_cn_66_down, &CHINESE_TIME);

    static lv_style_t font_style_cn_34;
    lv_style_init(&font_style_cn_34);
    lv_style_set_text_font(&font_style_cn_34, &CHINESE_NORMAL);

    static lv_style_t font_style_cn_49;
    lv_style_init(&font_style_cn_49);
    lv_style_set_text_color(&font_style_cn_49, lv_color_make(0xE6, 0xE6, 0xE6));
    lv_style_set_text_font(&font_style_cn_49, &CHINESE_NUM);
    
    static lv_style_t font_style_cn_22;
    lv_style_init(&font_style_cn_22);
    lv_style_set_text_color(&font_style_cn_22, lv_color_make(0xE6, 0xE6, 0xE6));
    lv_style_set_text_font(&font_style_cn_22, &CHINESE_SPECIAL);

    mainScreen_obj = lv_obj_create(NULL); //创建scr
    lv_obj_set_pos(mainScreen_obj, 0, 0);
    
    time_hour_label = lv_label_create(mainScreen_obj);
    lv_obj_set_pos(time_hour_label, 10, 10);
    lv_label_set_text_fmt(time_hour_label, "%02d", 0);
    time_min_label = lv_label_create(mainScreen_obj);
    lv_obj_set_pos(time_min_label, 15, 80);
    lv_label_set_text_fmt(time_min_label, "%02d", 0);

    static lv_style_t humidity_bar_style;
    lv_style_init(&humidity_bar_style);
    lv_style_set_bg_opa(&humidity_bar_style, LV_OPA_COVER);
    lv_style_set_bg_color(&humidity_bar_style, lv_color_make(0x00, 0xE5, 0xE5));
    lv_style_set_bg_grad_color(&humidity_bar_style, lv_color_make(0xFF, 0x33, 0x33));
    lv_style_set_bg_grad_dir(&humidity_bar_style, LV_GRAD_DIR_HOR);

    // 湿度进度条
    humidity_bar = lv_bar_create(mainScreen_obj);
    lv_obj_add_event_cb(humidity_bar, humidity_bar_event_cb, LV_EVENT_DRAW_PART_END, NULL);
    lv_obj_set_pos(humidity_bar, 12, 165);
    lv_obj_set_size(humidity_bar, 95, 15);
    lv_bar_set_range(humidity_bar, 0, 100);
    lv_bar_set_start_value(humidity_bar, 0, LV_ANIM_ON);
    lv_bar_set_value(humidity_bar, 20, LV_ANIM_ON);
    lv_obj_add_style(humidity_bar, &humidity_bar_style, LV_PART_INDICATOR);

    // 天气标签
    weather_label = lv_label_create(mainScreen_obj);
    lv_label_set_text(weather_label, "未知");
    lv_obj_set_pos(weather_label, 12, 200);
    
    // 天气图标
    extern const struct LV_WEATHER lv_weather[40];
    img_obj = lv_img_create(mainScreen_obj);
    lv_img_set_src(img_obj, lv_weather[39].img_dsc);
    lv_obj_set_pos(img_obj, 150, 20);

    // 温度标签
    temperature_label = lv_label_create(mainScreen_obj);
    lv_label_set_text(temperature_label, "NA");
    lv_obj_set_pos(temperature_label, 140, 85);

    // ℃
    lv_obj_t *du_label = lv_label_create(mainScreen_obj);
    lv_label_set_text(du_label, "℃");
    lv_obj_set_pos(du_label, 210, 85);

    //城市标签
    city_label = lv_label_create(mainScreen_obj);
    lv_label_set_text(city_label, "未知");
    lv_obj_set_pos(city_label, 147, 150);

    // 纪念日标签
    Countdown_days_label = lv_label_create(mainScreen_obj);
    lv_obj_set_pos(Countdown_days_label, 105, 200);
    extern int Countdown_days;
    lv_label_set_text_fmt(Countdown_days_label, "第%d天", Countdown_days);

    // WiFi问题图标
    LV_IMG_DECLARE(WiFi_icon);
    wifi_img = lv_img_create(mainScreen_obj);
    lv_img_set_src(wifi_img, &WiFi_icon);
    lv_obj_set_pos(wifi_img, 120, 10);
    SetWiFiIconVisible(false);

    SET_CHINESE(time_hour_label, font_style_cn_66_up);
    SET_CHINESE(time_min_label, font_style_cn_66_down);
    SET_CHINESE(city_label, font_style_cn_34);
    SET_CHINESE(weather_label, font_style_cn_34);
    SET_CHINESE(temperature_label, font_style_cn_49);
    SET_CHINESE(du_label, font_style_cn_22);
    SET_CHINESE(Countdown_days_label, font_style_cn_34);
    lv_scr_load_anim(mainScreen_obj, LV_SCR_LOAD_ANIM_FADE_ON, 750, 500, true);
}

void waitScreenUpdate(void *param){
    // extern bool wifi_ok;
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    // while(!wifi_ok){
    //     if(mode == WIFI_MODE_STA){
    lv_label_set_text(msg_label, "等待连接\nWiFi...");
    //         vTaskDelay(50 / portTICK_PERIOD_MS);
    //     } else if (mode == WIFI_MODE_APSTA){
    //         lv_label_set_text(msg_label, "请连接WiFi\nWiFi名称: 屁屁的礼物");
    //         vTaskDelay(100 / portTICK_PERIOD_MS);
    //     }
    // }
    vTaskSuspend(ScreenUpdateFunc_task_handle);
    MainScreen();
    ScreenUpdateFunc_task_handle = NULL;
    vTaskDelete(ScreenUpdateFunc_task_handle);
}

void WaitScreen(void){
    waitScreen_obj = lv_scr_act(); //创建scr
    lv_obj_set_pos(waitScreen_obj, 0, 0);

    static lv_style_t font_HuaWenXinWei_cn_46;
    lv_style_init(&font_HuaWenXinWei_cn_46);
    lv_style_set_text_font(&font_HuaWenXinWei_cn_46, &CHINESE_MSG);

    lv_scr_load(waitScreen_obj);
    msg_label = lv_label_create(waitScreen_obj);
    SET_CHINESE(msg_label, font_HuaWenXinWei_cn_46);
    lv_obj_set_pos(msg_label, 5, 5);

    
    LV_IMG_DECLARE(man_pic);
    lv_obj_t *man_img = lv_img_create(waitScreen_obj);
    lv_img_set_src(man_img, &man_pic);
    lv_obj_set_pos(man_img, 5, 100);
    LV_IMG_DECLARE(gl_pic);
    lv_obj_t *gl_img = lv_img_create(waitScreen_obj);
    lv_img_set_src(gl_img, &gl_pic);
    lv_obj_set_pos(gl_img, 120, 110);

    xTaskCreate(waitScreenUpdate, "waitScreenUpdate", 2048, NULL, 9, &ScreenUpdateFunc_task_handle);
}

void mUpdateweather(void *pvParameter){
    if (img_obj != NULL)
    {
        extern WeatherInfo weather_info;
        int code = weather_info.weather_code;
        if (code > 38 || code < 0) {
            code = 39;
        }
        lv_img_set_src(img_obj, lv_weather[code].img_dsc);
        int temp = strlen(weather_info.temperature);
        if (temp == 2) {
            lv_obj_set_pos(temperature_label, 145, 85);
        } else {
            lv_obj_set_pos(temperature_label, 155, 85);
        }
        lv_label_set_text_fmt(temperature_label, "%s", weather_info.temperature);

        temp = strlen(weather_info.city);
        if(temp == 6) {
            lv_obj_set_pos(city_label, 147, 150);
        } else {
            lv_obj_set_pos(city_label, 130, 150);
        }
        lv_label_set_text_fmt(city_label, "%s", weather_info.city);

        temp = strlen(weather_info.weather);
        printf("天气状况长度%d\n", temp);
        if(temp == 6){
            lv_obj_set_pos(weather_label, 15, 200);
        } else if (temp > 6) {
            lv_obj_set_pos(weather_label, 2, 200);
        } else {
            lv_obj_set_pos(weather_label, 25, 200);
        }
        lv_label_set_text_fmt(weather_label, "%s", weather_info.weather);
        temp = atoi(weather_info.humidity);
        set_humidity_bar(humidity_bar, (int32_t)temp);
        extern int Countdown_days;
        if(Countdown_days > 99){
            lv_obj_set_pos(Countdown_days_label, 105, 200);
        } else {
            lv_obj_set_pos(Countdown_days_label, 118, 200);
        }
        lv_label_set_text_fmt(Countdown_days_label, "第%d天", Countdown_days);
    }
    vTaskDelete(NULL);
}

void lv_update_time()
{
    if (time_hour_label != NULL)
    {
        extern sysTime realtime;
        lv_label_set_text_fmt(time_hour_label, "%02d", realtime.hour);
        lv_label_set_text_fmt(time_min_label, "%02d", realtime.min);
    }
}

void lv_update_weather(){
    xTaskCreate(mUpdateweather, "mUpdateweather", 2048, NULL, 8, NULL);
}

void guiTask1(void *pvParameter)
{
    (void)pvParameter;
    xGuiSemaphore = xSemaphoreCreateMutex();

    lv_init();
    /* Initialize SPI or I2C bus used by the drivers */
    lvgl_driver_init();

    lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    // memset(buf1,0x00ff,DISP_BUF_SIZE * sizeof(lv_color_t));
    assert(buf1 != NULL);

    /* Use double buffered when not working with monochrome displays */
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    lv_color_t *buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    //memset(buf2,0x00ff,DISP_BUF_SIZE * sizeof(lv_color_t));
    assert(buf2 != NULL);
#else
    static lv_color_t *buf2 = NULL;
#endif

    static lv_disp_draw_buf_t disp_buf;

    uint32_t size_in_px = DISP_BUF_SIZE;

    /* Initialize the working buffer depending on the selected display.
     * NOTE: buf2 == NULL when using monochrome displays. */
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.sw_rotate = 1;
    disp_drv.rotated = LV_DISP_ROT_270;
    lv_disp_drv_register(&disp_drv);

    // lv_theme_t *th = lv_theme_default_init(&disp_drv, lv_color_make(0xff, 0xff, 0xff), lv_color_make(0xff, 0xff, 0xff), true, &lv_font_montserrat_36);
    // lv_disp_set_theme(&disp_drv, th);
    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    // // MainScreen();
    // bool test = lv_theme_default_is_inited();
    // ESP_LOGI("THEME", "是否创建了主题 %d", test);
    // lv_theme_t *th = lv_theme_default_init(&disp_drv, lv_color_make(0xff, 0xff, 0xff), lv_color_make(0xff, 0xff, 0xff), true, &lv_font_montserrat_36);
    // lv_disp_set_theme(&disp_drv, th);
    WaitScreen();
    
    
    while (1)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(30));

        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }
    /* A task should NEVER return */
    free(buf1);
#ifndef CONFIG_LV_TFT_DISPLAY_MONOCHROME
    free(buf2);
#endif
    vTaskDelete(NULL);
}

static void set_humidity_bar(void *bar, int32_t v)
{
    lv_bar_set_value(bar, v, LV_ANIM_OFF);
}

static void humidity_bar_event_cb(lv_event_t * e)
{
    lv_obj_draw_part_dsc_t * dsc = lv_event_get_param(e);
    if(dsc->part != LV_PART_INDICATOR) return;

    lv_obj_t * obj= lv_event_get_target(e);

    lv_draw_label_dsc_t label_dsc;
    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.font = LV_FONT_DEFAULT;

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d", (int)lv_bar_get_value(obj));

    lv_point_t txt_size;
    lv_txt_get_size(&txt_size, buf, label_dsc.font, label_dsc.letter_space, label_dsc.line_space, LV_COORD_MAX, label_dsc.flag);

    lv_area_t txt_area;
    /*If the indicator is long enough put the text inside on the right*/
    if(lv_area_get_width(dsc->draw_area) > txt_size.x + 20) {
        txt_area.x2 = dsc->draw_area->x2 - 5;
        txt_area.x1 = txt_area.x2 - txt_size.x + 1;
        label_dsc.color = lv_color_make(0xE1, 0xE1, 0xE1);
    }
    /*If the indicator is still short put the text out of it on the right*/
    else {
        txt_area.x1 = dsc->draw_area->x2 + 5;
        txt_area.x2 = txt_area.x1 + txt_size.x - 1;
        label_dsc.color = lv_color_white();
    }

    txt_area.y1 = dsc->draw_area->y1 + (lv_area_get_height(dsc->draw_area) - txt_size.y) / 2;
    txt_area.y2 = txt_area.y1 + txt_size.y - 1;

    lv_draw_label(dsc->draw_ctx, &label_dsc, &txt_area, buf, NULL);
}

void SetWiFiIconVisible(bool enable) {
    if(wifi_img == NULL){
        ESP_LOGW("LVGL", "wifi_img == NULL");
        return;
    }
    if(enable){
        lv_obj_clear_flag(wifi_img, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(wifi_img, LV_OBJ_FLAG_HIDDEN);
    }
}

void SetMsgAP(){
    if(msg_label!=NULL){
        lv_label_set_text(msg_label, "连接热点\n屁屁的礼物\n配置WiFi");
    }
}
