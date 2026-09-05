// i2c_bus.c
#include "i2c_bus.h"
#include "esp_log.h"

// Initialize the I2C master bus and pass the handle back via pointer
esp_err_t i2c_master_init(i2c_master_bus_handle_t *bus_handle) {
    
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .scl_io_num = 22,
        .sda_io_num = 21,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    // bus_handle is already a pointer, so it is passed directly without '&'
    return i2c_new_master_bus(&i2c_bus_config, bus_handle);
}