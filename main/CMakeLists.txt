idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES driver bmp280 bus nvs_flash esp_wifi esp_netif esp_http_client mbedtls esp-tls)