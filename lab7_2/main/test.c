
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"


#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "MORSE_RX";
void app_main(void)
{
    adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };

    ESP_ERROR_CHECK(
        adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    // CHANGE THIS IF YOUR SENSOR IS ON A DIFFERENT PIN
    ESP_ERROR_CHECK(
        adc_oneshot_config_channel(
            adc_handle,
            ADC_CHANNEL_1,
            &config));

    int threshold = 50; 
    bool light_on = false; 

    int64_t start_time = 0;
    
    while(1) {
        int adc_raw = 0; 
        ESP_ERROR_CHECK(
            adc_oneshot_read(adc_handle, ADC_CHANNEL_1, &adc_raw));
        bool new_light_on = (adc_raw > threshold);

        if (new_light_on != light_on)
            {
                light_on = new_light_on;

                if (light_on)
                {
                    start_time = esp_timer_get_time();

                    ESP_LOGI(TAG, "LIGHT ON");
                }
                else
                {
                    int64_t duration =
                        (esp_timer_get_time() - start_time) / 1000;

                    ESP_LOGI(TAG, "LIGHT OFF, duration = %lld ms", duration);
                }
            }
         vTaskDelay(pdMS_TO_TICKS(1000));
    }
    }

