/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <stdio.h>
#include <math.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "example";

#define I2C_MASTER_SCL_IO           8
#define I2C_MASTER_SDA_IO           7
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          CONFIG_I2C_MASTER_FREQUENCY
#define I2C_MASTER_TIMEOUT_MS       1000

#define SHTC3_SENSOR_ADDR           0x70
#define SHTC3_Sleep_Mode            0xB098
#define SHTC3_Power_Up_Mode         0x3517
#define SHTC3_measurement           0x7CA2

/* CRC-8: polynomial 0x31, init 0xFF */
static uint8_t crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
        }
    }
    return crc;
}

static esp_err_t SHTC3_register_read(i2c_master_dev_handle_t dev_handle, uint8_t *data, size_t len)
{
    return i2c_master_receive(dev_handle, data, len, I2C_MASTER_TIMEOUT_MS);
}

static esp_err_t SHTC3_register_write(i2c_master_dev_handle_t dev_handle, uint16_t cmd)
{
    uint8_t write_buf[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS);
}

static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port               = I2C_MASTER_NUM,
        .sda_io_num             = I2C_MASTER_SDA_IO,
        .scl_io_num             = I2C_MASTER_SCL_IO,
        .clk_source             = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt      = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = SHTC3_SENSOR_ADDR,
        .scl_speed_hz    = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));
}

void app_main(void)
{
    uint8_t data[6];
    uint8_t cmd[2] = { (uint8_t)(SHTC3_measurement >> 8),
                       (uint8_t)(SHTC3_measurement & 0xFF) };

    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    i2c_master_init(&bus_handle, &dev_handle);

    while (1) {
        // Power up
        SHTC3_register_write(dev_handle, SHTC3_Power_Up_Mode);
        vTaskDelay(pdMS_TO_TICKS(200));

        // Send measure command and read 6 bytes in one transaction
        i2c_master_transmit(dev_handle, cmd, 2, I2C_MASTER_TIMEOUT_MS);

	vTaskDelay(pdMS_TO_TICKS(200)); 

	memset(data, 0, sizeof(data));

	i2c_master_receive(dev_handle, data, 6, I2C_MASTER_TIMEOUT_MS); 
       

	ESP_LOGI(TAG, "Raw: %02X %02X %02X %02X %02X %02X",
           data[0], data[1], data[2],
           data[3], data[4], data[5]);


        // Verify CRC for temperature (bytes 0,1 -> CRC byte 2)
        if (crc8(data, 2) != data[2]) {
            ESP_LOGE(TAG, "Temperature CRC mismatch");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        // Verify CRC for humidity (bytes 3,4 -> CRC byte 5)
        if (crc8(&data[3], 2) != data[5]) {
            ESP_LOGE(TAG, "Humidity CRC mismatch");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        int temp_raw = (data[0] << 8) | data[1];
        int hum_raw  = (data[3] << 8) | data[4];
        float temp  = -45.0f + 175.0f * (temp_raw / 65535.0f);
        float humid = 100.0f * (hum_raw  / 65535.0f);

        int tc = (int)roundf(temp);
        int tf = (int)roundf(temp * 9.0f / 5.0f + 32.0f);
        int rh = (int)roundf(humid);

        ESP_LOGI(TAG, "Temperature is %dC (or %dF) with a %d%% humidity", tc, tf, rh);
	
	SHTC3_register_write(dev_handle, SHTC3_Sleep_Mode);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
