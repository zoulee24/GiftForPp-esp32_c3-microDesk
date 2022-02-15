#include "screen_backlight.h"

ledc_channel_config_t ledc_channel = {
        .channel    = LEDC_CHANNEL_0,
        .duty       = 0,
        .gpio_num   = 4,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_1,
        .flags.output_invert = 0
};

void BacklightInit() {
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = FREQHZ,                      // frequency of PWM signal
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    
    ledc_timer_config(&ledc_timer);
    ledc_channel_config(&ledc_channel);
}

void Setbacklight(uint32_t target_duty) {
    ESP_LOGI("Backlight", "设置背光: %.1f%%", (1.0f - (float)target_duty / FREQHZ) * 100.0f);
    ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, target_duty);
    ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}
