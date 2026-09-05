// bmp280.h
#ifndef BMP_280_H
#define BMP_280_H

#include "driver/i2c_master.h"
#include "esp_err.h"

// Attach the BMP280 device to the I2C bus
esp_err_t bmp280_init(i2c_master_bus_handle_t bus_handle, i2c_master_dev_handle_t *dev_handle);

// Write a single byte of data to a specific sensor register
esp_err_t bmp280_write_register(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t data);

// Read a specified number of bytes starting from a sensor register
esp_err_t bmp_280_read_register(i2c_master_dev_handle_t dev_handle , uint8_t reg, uint8_t *data, size_t len);

// Wake up the sensor from sleep mode and start measurements
esp_err_t bmp280_wake_up(i2c_master_dev_handle_t dev_handle);

// Read the uncompensated raw temperature data into the provided pointer
esp_err_t bmp280_read_raw_temp(i2c_master_dev_handle_t dev_handle, int32_t *raw_temp);

// Calculate and retrieve the final compensated temperature in Celsius
esp_err_t bmp280_get_temperature(i2c_master_dev_handle_t dev_handle, float *temperature);

#endif