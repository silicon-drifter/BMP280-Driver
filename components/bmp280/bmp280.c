#include "bmp280.h"
#include "esp_log.h"

static const char *TAG = "BMP280";

// Standard I2C address for BMP280
#define BMP280_I2C_ADDRESS 0X76

// Initialize the sensor
esp_err_t bmp280_init(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t *dev_handle) {
    
    // Sensor configuration parameters
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = BMP280_I2C_ADDRESS,
        .scl_speed_hz = 100000,
    };

    // Attach the sensor to the I2C bus
    esp_err_t err = i2c_master_bus_add_device(bus_handle,&dev_cfg, dev_handle);

    if( err == ESP_OK) {
        ESP_LOGI(TAG, "BMP280 Succesfully Attached To The I2C Bus");
    } else {
        ESP_LOGE(TAG, "FAILED TO ATTACH BMP280 TO THE BUS");
    }
    return err;
}

esp_err_t bmp280_write_register(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t data) {
    uint8_t buffer[2] = {reg,data};

    return i2c_master_transmit(dev_handle,buffer,sizeof(buffer),-1);
}

esp_err_t bmp_280_read_register(i2c_master_dev_handle_t dev_handle, uint8_t reg , uint8_t *data, size_t len ) {
    return i2c_master_transmit_receive(
                                    dev_handle, 
                                    &reg,
                                    1,
                                    data,
                                    len,
                                    -1
                                );
                    }

// Wake up the sensor
esp_err_t bmp280_wake_up(i2c_master_dev_handle_t dev_handle) {
    // Register 0xF4 controls power and measurement modes.
    // Writing 0x27 sets the sensor to normal mode and starts temperature/pressure measurements.
    return bmp280_write_register(dev_handle, 0xF4, 0x27);
}

// Read raw temperature data
esp_err_t bmp280_read_raw_temp(i2c_master_dev_handle_t dev_handle, int32_t *raw_temp) {
    uint8_t data[3]; // Buffer to store the 3 bytes of temperature data
    
    // Temperature data is stored across registers 0xFA, 0xFB, and 0xFC.
    // Reading 3 consecutive bytes starting from 0xFA (auto-increment).
    esp_err_t err = bmp_280_read_register(dev_handle, 0xFA, data, 3);
    
    if (err == ESP_OK) {
        // Combine the 3 bytes into a single 20-bit raw temperature value
        *raw_temp = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    }
    return err;
}

esp_err_t bmp280_get_temperature(i2c_master_dev_handle_t dev_handle, float *temperature) {
    uint8_t calib_data[6];
    
    // 1. Read factory calibration data (6 bytes starting from register 0x88)
    esp_err_t err = bmp_280_read_register(dev_handle, 0x88, calib_data, 6);
    if (err != ESP_OK) return err;

    // Parse calibration data according to the Bosch datasheet
    uint16_t dig_T1 = (calib_data[1] << 8) | calib_data[0];
    int16_t  dig_T2 = (calib_data[3] << 8) | calib_data[2];
    int16_t  dig_T3 = (calib_data[5] << 8) | calib_data[4];

    // 2. Read the raw temperature value
    int32_t raw_temp;
    err = bmp280_read_raw_temp(dev_handle, &raw_temp);
    if (err != ESP_OK) return err;

    // 3. Apply Bosch compensation formula (from datasheet)
    int32_t var1, var2, t_fine;
    
    var1 = ((((raw_temp >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    var2 = (((((raw_temp >> 4) - ((int32_t)dig_T1)) * ((raw_temp >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    
    t_fine = var1 + var2;
    
    // Calculate final temperature in Celsius
    float final_temp = ((t_fine * 5 + 128) >> 8) / 100.0f;
    
    // Store the calculated temperature in the provided pointer
    *temperature = final_temp;
    
    return ESP_OK;
}