#include "DFR_display.h" 
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "esp_log.h"
#include <string.h>

#define SHTC3_ADD 0x70
#define SHTC3_SLEEP 0xB098 
#define SHTC3_WAKE 0x3517 
#define SHTC3_MEASURE 0x7CA2 
#define SHTC3_TIMEOUT_MS 1000

static uint8_t crc8(const uint8_t *data, size_t len) {
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? ((crc << 1) ^ 0x31) : (crc << 1);
        }
    }
    return crc;
}

extern "C" void app_main(void){ 
	DFR_display lcd(0x2D, 16, 2); 
	lcd.init();
	lcd.dispRGB(255,0,0);

	i2c_master_dev_handle_t sensorDev; 
	i2c_device_config_t sensorConf = {};
	sensorConf.dev_addr_length = I2C_ADDR_BIT_LEN_7;
	sensorConf.device_address = SHTC3_ADD;
	sensorConf.scl_speed_hz = 100000; 
	i2c_master_bus_add_device(lcd.getBus(), &sensorConf, &sensorDev);

    uint8_t cmd[2];        // add this
    uint8_t data[6];
	
    while (true) {
        // Wake up sensor
        cmd[0] = (uint8_t)(SHTC3_WAKE >> 8);
        cmd[1] = (uint8_t)(SHTC3_WAKE & 0xFF);
        i2c_master_transmit(sensorDev, cmd, 2, SHTC3_TIMEOUT_MS);
        vTaskDelay(pdMS_TO_TICKS(200));

        // Send measure command
        cmd[0] = (uint8_t)(SHTC3_MEASURE >> 8);
        cmd[1] = (uint8_t)(SHTC3_MEASURE & 0xFF);
        i2c_master_transmit(sensorDev, cmd, 2, SHTC3_TIMEOUT_MS);
        vTaskDelay(pdMS_TO_TICKS(200));

        // Read 6 bytes
        memset(data, 0, sizeof(data));
        i2c_master_receive(sensorDev, data, 6, SHTC3_TIMEOUT_MS);

        // CRC checks
        if (crc8(data, 2) != data[2] || crc8(&data[3], 2) != data[5]) {
            lcd.setCursor(0, 0);
            lcd.printstr("Sensor Error!   ");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // Convert raw values
        int temp_raw = (data[0] << 8) | data[1];
        int hum_raw  = (data[3] << 8) | data[4];
        float temp   = -45.0f + 175.0f * (temp_raw / 65535.0f);
        float humid  = 100.0f * (hum_raw  / 65535.0f);

        int tf = (int)roundf(temp * 9.0f/ 5.0f + 32.0f);
        int rh = (int)roundf(humid);

        // Format strings
        char line1[17];
        char line2[17];
        snprintf(line1, sizeof(line1), "Temp: %dF       ", tf);
        snprintf(line2, sizeof(line2), "Hum : %d%%       ", rh);

        // Update display
        lcd.setCursor(0, 0);
        lcd.printstr(line1);
        lcd.setCursor(0, 1);
        lcd.printstr(line2);

        // Sleep sensor
        cmd[0] = (uint8_t)(SHTC3_SLEEP >> 8);
        cmd[1] = (uint8_t)(SHTC3_SLEEP & 0xFF);
        i2c_master_transmit(sensorDev, cmd, 2, SHTC3_TIMEOUT_MS);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }


}
