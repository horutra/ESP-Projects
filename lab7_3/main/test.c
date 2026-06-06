
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "MORSE_RX";
void app_main(void)
{
    char message[64] = {0};
    int message_index = 0;
    bool word_gap_added = false;
    adc_oneshot_unit_handle_t adc_handle;
    int total_characters = 0;
TickType_t first_character_tick = 0;
bool timer_started = false;

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

    int threshold = 23; 
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
    if (strcmp(morse, "-----") == 0) return '0';
    if (strcmp(morse, ".----") == 0) return '1';
    if (strcmp(morse, "..---") == 0) return '2';
    if (strcmp(morse, "...--") == 0) return '3';
    if (strcmp(morse, "....-") == 0) return '4';
    if (strcmp(morse, ".....") == 0) return '5';
    if (strcmp(morse, "-....") == 0) return '6';
    if (strcmp(morse, "--...") == 0) return '7';
    if (strcmp(morse, "---..") == 0) return '8';
    if (strcmp(morse, "----.") == 0) return '9';

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
                    word_gap_added = false;
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
                    if (duration_ms < 20)
                    {
                        ESP_LOGI(TAG, "Ignoring noise pulse");
                        continue;
                    }
                    if (duration_ms < 150)
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
                if (!light_on &&
                    morse_index > 0 &&
                    ((now - last_symbol_tick) * portTICK_PERIOD_MS) > 400)
                {
                    char letter = decode_morse(morse_buffer);

                    if (!timer_started)
                    {
                        first_character_tick = xTaskGetTickCount();
                        timer_started = true;
                    }

                    message[message_index++] = letter;
                    message[message_index] = '\0';

                    total_characters++;

                    ESP_LOGI(TAG,
                            "MESSAGE = %s",
                            message);

                    if (timer_started && total_characters > 1)
                        {
                            float elapsed_seconds =
                                (xTaskGetTickCount() - first_character_tick)
                                * portTICK_PERIOD_MS / 1000.0f;

                            float chars_per_second = total_characters / elapsed_seconds;

                            ESP_LOGI(TAG,
                                    "Total Chars = %d | Speed = %.2f chars/sec",
                                    total_characters,
                                    chars_per_second);
                        }

                    ESP_LOGI(TAG,
                            "LETTER = %c",
                            letter);

                    ESP_LOGI(TAG,
                            "MESSAGE = %s",
                            message);

                    morse_index = 0;
                    morse_buffer[0] = '\0';
                }
                if (!light_on && message_index > 0 &&
                        !word_gap_added &&
                        ((now - last_symbol_tick) * portTICK_PERIOD_MS) > 1000)
                    {
                        if (message_index < sizeof(message) - 1)
                        {
                            message[message_index++] = ' ';
                            message[message_index] = '\0';

                            ESP_LOGI(TAG, "MESSAGE = %s", message);
                        }

                        word_gap_added = true;
                    }
                if (message_index > 0 &&
                    ((now - last_symbol_tick) * portTICK_PERIOD_MS) > 8000)
                {
                    ESP_LOGI(TAG, "FINAL MESSAGE = %s", message);

                    message_index = 0;
                    message[0] = '\0';

                    word_gap_added = false;
                    total_characters = 0;
                    timer_started = false;

                    ESP_LOGI(TAG, "Message buffer cleared");
                }
                

         vTaskDelay(pdMS_TO_TICKS(10));
    }
}

