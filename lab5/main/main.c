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


#include "rom/ets_sys.h"
#include "driver/i2c_master.h"

#define I2C_MASTER_SCL_IO       8
#define I2C_MASTER_SDA_IO       7
#define I2C_MASTER_NUM          I2C_NUM_0
#define I2C_MASTER_TIMEOUT_MS   1000

#define SHTC3_SENSOR_ADDR       0x70
#define SHTC3_Power_Up_Mode     0x3517
#define SHTC3_measurement       0x7CA2


static const char *TAG = "Ultrasonic";

/* GPIO PINS */
#define TRIG_PIN GPIO_NUM_4
#define ECHO_PIN GPIO_NUM_3

/*
 * Speed of sound:
 * v = 331.3 + (0.606 * tempC)
 */
#define SPEED_SOUND(TempC) (331.3f + (0.606f * TempC))

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
    ESP_LOGI(TAG, "Sending trigger pulse");
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


static uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;

    for (size_t i = 0; i < len; i++) {

        crc ^= data[i];

        for (int bit = 0; bit < 8; bit++) {

            crc =
                (crc & 0x80)
                ? ((crc << 1) ^ 0x31)
                : (crc << 1);
        }
    }

    return crc;
}

static void shtc3_init(
    i2c_master_bus_handle_t *bus_handle,
    i2c_master_dev_handle_t *dev_handle
)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(
        i2c_new_master_bus(
            &bus_config,
            bus_handle
        )
    );

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHTC3_SENSOR_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            *bus_handle,
            &dev_config,
            dev_handle
        )
    );
}

static float read_temperature_c(
    i2c_master_dev_handle_t dev_handle
)
{
    uint8_t data[6];

    uint8_t cmd[2] = {
        (uint8_t)(SHTC3_measurement >> 8),
        (uint8_t)(SHTC3_measurement & 0xFF)
    };

    uint8_t wakeup[2] = {
        (uint8_t)(SHTC3_Power_Up_Mode >> 8),
        (uint8_t)(SHTC3_Power_Up_Mode & 0xFF)
    };

    i2c_master_transmit(
        dev_handle,
        wakeup,
        2,
        I2C_MASTER_TIMEOUT_MS
    );

    vTaskDelay(pdMS_TO_TICKS(100));

    i2c_master_transmit(
        dev_handle,
        cmd,
        2,
        I2C_MASTER_TIMEOUT_MS
    );

    vTaskDelay(pdMS_TO_TICKS(20));

    i2c_master_receive(
        dev_handle,
        data,
        6,
        I2C_MASTER_TIMEOUT_MS
    );

    if (crc8(data, 2) != data[2]) {

        ESP_LOGE(TAG, "Temperature CRC mismatch");

        return 25.0f;
    }

    int temp_raw =
        (data[0] << 8) | data[1];

    float temp =
        -45.0f +
        175.0f *
        (temp_raw / 65535.0f);

    return temp;
}


void app_main(void)
{
    ultrasonic_gpio_init();

    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;

    shtc3_init(
        &bus_handle,
        &dev_handle
    );

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
            read_temperature_c(dev_handle);

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