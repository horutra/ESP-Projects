/*
 * Ultrasonic Range Finder - US100/SR04 Mode
 * ESP32-C3
 */

#include <stdio.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "driver/gpio.h"
#include "driver/temperature_sensor.h"

#include "rom/ets_sys.h"

static const char *TAG = "Ultrasonic";

/* GPIO PINS */
#define TRIG_PIN GPIO_NUM_8
#define ECHO_PIN GPIO_NUM_7

/*
 * Speed of sound:
 * v = 331.3 + (0.606 * tempC)
 */
#define SPEED_SOUND(TempC) (331.3f + (0.606f * TempC))

temperature_sensor_handle_t temp_handle = NULL;

static void temperature_sensor_init(void)
{
    temperature_sensor_config_t temp_config =
        TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);

    ESP_ERROR_CHECK(
        temperature_sensor_install(
            &temp_config,
            &temp_handle
        )
    );

    ESP_ERROR_CHECK(
        temperature_sensor_enable(temp_handle)
    );
}

static float read_temperature_c(void)
{
    float temperature_c = 0;

    ESP_ERROR_CHECK(
        temperature_sensor_get_celsius(
            temp_handle,
            &temperature_c
        )
    );

    return temperature_c;
}


static void ultrasonic_gpio_init(void)
{
    gpio_config_t trig_config = {
        .pin_bit_mask = (1ULL << TRIG_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };

    gpio_config_t echo_config = {
        .pin_bit_mask = (1ULL << ECHO_PIN),
        .mode = GPIO_MODE_INPUT,
    };

    gpio_config(&trig_config);
    gpio_config(&echo_config);

    gpio_set_level(TRIG_PIN, 0);
}


static void ultrasonic_trigger(void)
{
    gpio_set_level(TRIG_PIN, 0);
    ets_delay_us(2);

    gpio_set_level(TRIG_PIN, 1);
    ets_delay_us(10);

    gpio_set_level(TRIG_PIN, 0);
}


static int64_t measure_echo_time(void)
{
    int64_t start_time = 0;
    int64_t end_time = 0;

    int64_t timeout_start =
        esp_timer_get_time();

    /* Wait for ECHO to go HIGH */
    while (gpio_get_level(ECHO_PIN) == 0) {

        if (
            esp_timer_get_time() - timeout_start
            > 30000
        ) {

            return -1;
        }
    }

    start_time = esp_timer_get_time();

    /* Wait for ECHO to go LOW */
    while (gpio_get_level(ECHO_PIN) == 1) {

        if (
            esp_timer_get_time() - start_time
            > 30000
        ) {

            return -1;
        }
    }

    end_time = esp_timer_get_time();

    return end_time - start_time;
}


static float calculate_distance_cm(
    int64_t echo_time_us,
    float temperature_c
)
{
    float speed_sound =
        SPEED_SOUND(temperature_c);

    /*
     * divide by 2 because sound
     * travels to object and back
     */

    float distance_cm =
        (speed_sound * echo_time_us)
        / 20000.0f;

    return distance_cm;
}

void app_main(void)
{
    ultrasonic_gpio_init();

    temperature_sensor_init();

    while (1) {

        ultrasonic_trigger();

        int64_t echo_time =
            measure_echo_time();

        if (echo_time < 0) {

            ESP_LOGE(TAG, "Echo timeout");

            vTaskDelay(pdMS_TO_TICKS(1000));

            continue;
        }

        float temperature_c =
            read_temperature_c();

        float distance_cm =
            calculate_distance_cm(
                echo_time,
                temperature_c
            );

        ESP_LOGI(
            TAG,
            "Distance: %.2f cm | Temp: %.2f C",
            distance_cm,
            temperature_c
        );

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}