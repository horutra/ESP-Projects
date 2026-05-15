#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define THRESHOLD 100 

static const char *TAG = "Lab4_1";

static void direct_location(i2c_master_dev_handle_t dev_handle)
{
    uint8_t X0_DATA = 0x0C; 
    uint8_t X1_DATA = 0x0B; 
    uint8_t Y0_DATA = 0x0E;
    uint8_t Y1_DATA = 0x0D;

    //Low and hiugh bits for both X and Y coordinates
    uint8_t high_x = 0; 
    uint8_t low_x = 0;
    
    uint8_t high_y = 0;
    uint8_t low_y = 0;

    i2c_master_transmit_receive(dev_handle, &X0_DATA, 1, &low_x, 1, -1);
    i2c_master_transmit_receive(dev_handle, &X1_DATA, 1, &high_x, 1, -1);
    i2c_master_transmit_receive(dev_handle, &Y0_DATA, 1, &low_y, 1, -1);
    i2c_master_transmit_receive(dev_handle, &Y1_DATA, 1, &high_y, 1, -1);

    int16_t X_ACCEL = (int16_t)((high_x << 8 )| low_x);
    int16_t Y_ACCEL = (int16_t)((high_y << 8) | low_y);

  

// Diagonal directions first
    if (X_ACCEL > THRESHOLD && Y_ACCEL > THRESHOLD)
    {
        ESP_LOGI(TAG, "UP LEFT");
    }
    else if (X_ACCEL < -THRESHOLD && Y_ACCEL > THRESHOLD)
    {
        ESP_LOGI(TAG, "UP RIGHT");
    }
    else if (X_ACCEL > THRESHOLD && Y_ACCEL < -THRESHOLD)
    {
        ESP_LOGI(TAG, "DOWN LEFT");
    }
    else if (X_ACCEL < -THRESHOLD && Y_ACCEL < -THRESHOLD)
    {
        ESP_LOGI(TAG, "DOWN RIGHT");
    }
    // Single directions
    else if (Y_ACCEL > THRESHOLD)
    {
        ESP_LOGI(TAG, "UP y %d", Y_ACCEL);
    }
    else if (Y_ACCEL < -THRESHOLD)
    {
        ESP_LOGI(TAG, "DOWN y %d", Y_ACCEL);
    }
    else if (X_ACCEL > THRESHOLD)
    {
        ESP_LOGI(TAG, "LEFT x %d", X_ACCEL);
    }
    else if (X_ACCEL < -THRESHOLD)
    {
        ESP_LOGI(TAG, "RIGHT x %d", X_ACCEL);
    }
    else
    {
        ESP_LOGI(TAG, "CENTER");
    }
}

void app_main(void)
{   
    uint8_t buffer[2];
    buffer[0] = 0x1F; // PWR_MGMT0 register
    buffer[1] = 0x03; // Set to 0 to wake up



    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = GPIO_NUM_8,
        .sda_io_num = GPIO_NUM_7,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t ICM_DEV = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x68,
        .scl_speed_hz = 100000,
    };

    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &ICM_DEV, &dev_handle));

    i2c_master_transmit(dev_handle, buffer, 2, -1);

    while (1) {
        direct_location(dev_handle);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}