#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c_bus.h"   // I2C bus interface
#include "bmp280.h"    // BMP280 sensor interface

static const char *TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "Starting ESP32 Bare-Metal I2C Master...");

    // ---------------------------------------------------------
    // STEP 1: CREATE HANDLES
    // ---------------------------------------------------------
    // Declare handles for the I2C bus and the BMP280 device.
    // These are currently uninitialized memory spaces.
    i2c_master_bus_handle_t bus_handle;      
    i2c_master_dev_handle_t bmp280_handle;   

    // ---------------------------------------------------------
    // STEP 2: INITIALIZE I2C BUS
    // ---------------------------------------------------------
    ESP_LOGI(TAG, "Initializing I2C Master Bus...");
    // Call the initialization function and pass the address of bus_handle.
    // This configures the GPIO pins and sets up the I2C master.
    esp_err_t err = i2c_master_init(&bus_handle); 
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus!");
        return; 
    }

    // ---------------------------------------------------------
    // STEP 3: ATTACH BMP280 TO THE BUS
    // ---------------------------------------------------------
    ESP_LOGI(TAG, "Adding BMP280 to the I2C Bus...");
    // Pass the initialized bus_handle (by value) and the address of 
    // bmp280_handle (by reference) to attach the device.
    err = bmp280_init(bus_handle, &bmp280_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register sensor!");
        return;
    }

    // ---------------------------------------------------------
    // STEP 4: COMMUNICATION TEST (Read Chip ID)
    // ---------------------------------------------------------
    uint8_t chip_id = 0; // Buffer to store the read ID
    
    // Read register 0xD0 using the device handle to verify communication.
    // The result will be stored in the chip_id variable.
    err = bmp_280_read_register(bmp280_handle, 0xD0, &chip_id, 1);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "SUCCESS! BMP280 Chip ID: 0x%02X", chip_id);
    } else {
        ESP_LOGE(TAG, "Failed to read Chip ID!");
    }

    if (chip_id == 0x58) {
        ESP_LOGI(TAG, "BMP280 verified (0x58) 🎉");
        
        // Wake up the sensor to start measurements
        bmp280_wake_up(bmp280_handle);
        ESP_LOGI(TAG, "Sensor is awake and measuring...");
    } else {
        // Handle incorrect chip ID scenario
    }

    // Continuous measurement loop
    int32_t raw_temperature = 0; // Buffer for raw temperature
    float actual_temp = 0.0;     // Final calculated temperature

    while (1) {
        esp_err_t status = bmp280_get_temperature(bmp280_handle, &actual_temp);
        
        if (status == ESP_OK) {
            ESP_LOGI(TAG, "🌡️ Actual Temperature: %.2f °C", actual_temp);
        } else {
            ESP_LOGE(TAG, "Failed to read data!");
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}