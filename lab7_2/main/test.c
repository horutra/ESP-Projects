
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include <string.h>

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

    int threshold = 20; 
    bool light_on = false; 

    TickType_t start_tick = 0;
    char morse_buffer[16] = {0};
        int morse_index = 0;

        TickType_t last_symbol_tick = 0;


char decode_morse(const char *morse)
{
    if (strcmp(morse, ".-") == 0) return 'A';
    if (strcmp(morse, "-...") == 0) return 'B';
    if (strcmp(morse, "-.-.") == 0) return 'C';
    if (strcmp(morse, "-..") == 0) return 'D';
    if (strcmp(morse, ".") == 0) return 'E';
    if (strcmp(morse, "..-.") == 0) return 'F';
    if (strcmp(morse, "--.") == 0) return 'G';
    if (strcmp(morse, "....") == 0) return 'H';
    if (strcmp(morse, "..") == 0) return 'I';
    if (strcmp(morse, ".---") == 0) return 'J';
    if (strcmp(morse, "-.-") == 0) return 'K';
    if (strcmp(morse, ".-..") == 0) return 'L';
    if (strcmp(morse, "--") == 0) return 'M';
    if (strcmp(morse, "-.") == 0) return 'N';
    if (strcmp(morse, "---") == 0) return 'O';
    if (strcmp(morse, ".--.") == 0) return 'P';
    if (strcmp(morse, "--.-") == 0) return 'Q';
    if (strcmp(morse, ".-.") == 0) return 'R';
    if (strcmp(morse, "...") == 0) return 'S';
    if (strcmp(morse, "-") == 0) return 'T';
    if (strcmp(morse, "..-") == 0) return 'U';
    if (strcmp(morse, "...-") == 0) return 'V';
    if (strcmp(morse, ".--") == 0) return 'W';
    if (strcmp(morse, "-..-") == 0) return 'X';
    if (strcmp(morse, "-.--") == 0) return 'Y';
    if (strcmp(morse, "--..") == 0) return 'Z';

    return '?';
}

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
                    start_tick = xTaskGetTickCount();

                    ESP_LOGI(TAG, "LIGHT ON");
                }
                else
                {
                TickType_t end_tick = xTaskGetTickCount();

                    ESP_LOGI(TAG, "start=%lu end=%lu",
                            (unsigned long)start_tick,
                            (unsigned long)end_tick);

                    uint32_t duration_ms =
                        (uint32_t)((end_tick - start_tick) * portTICK_PERIOD_MS);

                    ESP_LOGI(TAG, "duration=%lu ms",
                            (unsigned long)duration_ms);
                    if (duration_ms < 200)
                    {
                        ESP_LOGI(TAG, "Ignoring noise pulse");
                        continue;
                    }
                    if (duration_ms < 1500)
                        {
                            morse_buffer[morse_index++] = '.';
                            last_symbol_tick = xTaskGetTickCount();

                            ESP_LOGI(TAG, "DOT (.)");
                        }
                        else
                        {
                            morse_buffer[morse_index++] = '-';
                            last_symbol_tick = xTaskGetTickCount();

                            ESP_LOGI(TAG, "DASH (-)");
                        }
                        morse_buffer[morse_index] = '\0';
                        ESP_LOGI(TAG, "Current Morse: %s", morse_buffer);
                }



            }
                TickType_t now = xTaskGetTickCount();

                if (morse_index > 0 &&
                    ((now - last_symbol_tick) * portTICK_PERIOD_MS) > 1940)
                {
                    char letter = decode_morse(morse_buffer);

                    ESP_LOGI(TAG,
                            "LETTER = %c (Morse = %s)",
                            letter,
                            morse_buffer);

                    morse_index = 0;
                    morse_buffer[0] = '\0';
                }

         vTaskDelay(pdMS_TO_TICKS(50));
    }
}

