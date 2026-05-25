idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES esp_wifi nvs_flash
                    REQUIRES esp_http_client
                    INCLUDE_DIRS ".")
