/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "driver/i2c_master.h" //just added for sensor

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"
#include "esp_mac.h"

#define I2C_MASTER_SCL_IO           8
#define I2C_MASTER_SDA_IO           7
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TIMEOUT_MS       1000

#define SHTC3_SENSOR_ADDR           0x70
#define SHTC3_Sleep_Mode            0xB098
#define SHTC3_Power_Up_Mode         0x3517
#define SHTC3_measurement           0x7CA2

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;


/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "10.0.0.45"
#define WEB_PORT "8000"
#define WEB_PATH "http://wttr.in/?format=3"

static const char *TAG = "example";




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
    uint8_t write_buf[2] = {
        (uint8_t)(cmd >> 8),
        (uint8_t)(cmd & 0xFF)
    };
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS);
}

static void i2c_master_init(i2c_master_bus_handle_t *bus_handle,
                            i2c_master_dev_handle_t *dev_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHTC3_SENSOR_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));
}

float read_temperature(void)
{
    uint8_t data[6];
    uint8_t cmd[2] = {
        (uint8_t)(SHTC3_measurement >> 8),
        (uint8_t)(SHTC3_measurement & 0xFF)
    };

    SHTC3_register_write(dev_handle, SHTC3_Power_Up_Mode);
    vTaskDelay(pdMS_TO_TICKS(20));

    i2c_master_transmit(dev_handle, cmd, 2, I2C_MASTER_TIMEOUT_MS);

    vTaskDelay(pdMS_TO_TICKS(20));

    i2c_master_receive(dev_handle, data, 6, I2C_MASTER_TIMEOUT_MS);

    if (crc8(data, 2) != data[2]) {
        ESP_LOGE(TAG, "Temperature CRC mismatch");
        return -999.0f;
    }

    int temp_raw = (data[0] << 8) | data[1];

    float temp =
        -45.0f + 175.0f * (temp_raw / 65535.0f);

    SHTC3_register_write(dev_handle, SHTC3_Sleep_Mode);

    return temp;
}

static void http_get_task(void *pvParameters)
{

    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    while(1) {

        float temp = read_temperature();
        char post_data[64];
        snprintf(post_data, sizeof(post_data), "Temperature: %.2f C", temp);
        ESP_LOGI(TAG, "Posting temperature: %s", post_data);

        char request[256];
            snprintf(request, sizeof(request),
                    "POST " WEB_PATH " HTTP/1.0\r\n"
                    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
                    "User-Agent: esp-idf/1.0 esp32 curl\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: %d\r\n"
                    "\r\n"
                    "%s",
                    strlen(post_data), post_data);

        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, request, strlen(request)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}


void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    i2c_master_init(&bus_handle, &dev_handle);

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    uint8_t mac[6];

    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    ESP_LOGI(TAG, "ESP32 MAC: %02X:%02X:%02X:%02X:%02X:%02X",
         mac[0], mac[1], mac[2],
         mac[3], mac[4], mac[5]);

    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}
