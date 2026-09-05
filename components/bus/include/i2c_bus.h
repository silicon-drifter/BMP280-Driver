// i2c_bus.h
#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "driver/i2c_master.h"
#include "esp_err.h"

// Pass the bus handle as a pointer to receive the allocated bus instance
esp_err_t i2c_master_init(i2c_master_bus_handle_t *bus_handle);

#endif