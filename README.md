# ESP32 BMP280 I2C Driver using ESP-IDF

A lightweight, bare-metal I2C driver for the Bosch BMP280 Temperature and Pressure sensor, built from scratch using the native ESP-IDF I2C Master API. 

Unlike abstracted high-level libraries (e.g., Arduino wire), this driver is engineered at the register level to demonstrate absolute control over hardware communication, memory addressing, and bitwise data compensation.

## ⚙️ Core Architecture & Features

- **Strict Hardware Abstraction Layer (HAL):** Clean separation between the I2C bus initialization (`i2c_bus.c`) and the sensor-specific logic (`bmp280.c`).
- **Pointer-Based Memory Management:** Efficient data routing using strict pass-by-reference logic. No unnecessary global variables or buffer duplications.
- **Register-Level I2C Routing:** Direct manipulation of sensor registers (e.g., `0xD0` for Chip ID, `0xF4` for Power Control, `0xFA` for Raw Temperature).
- **Custom Bitwise Compensation:** Implements Bosch's highly precise bitwise compensation algorithms directly from the datasheet for accurate Celsius outputs.
- **ESP-IDF Native:** Utilizes ESP-IDF v5.x `i2c_master` drivers for robust, real-time operating system (FreeRTOS) compatible performance.

## 🗂️ Project Structure

```text
├── components/
│   ├── i2c_bus/
│   │   ├── i2c_bus.c         # I2C Master bus initialization and GPIO configuration
│   │   └── i2c_bus.h
│   └── bmp280/
│       ├── bmp280.c          # Register R/W logic, wake-up sequences, and math compensation
│       └── bmp280.h
└── main/
    └── app_main.c            # Application entry point and FreeRTOS task loop


Quick Start / Usage

The driver follows a strict 4-step initialization and execution pipeline:

-------------------------------------------------------------------------------------
#include "i2c_bus.h"
#include "bmp280.h"

void app_main(void) {
    // 1. Create memory handles
    i2c_master_bus_handle_t bus_handle;      
    i2c_master_dev_handle_t bmp280_handle;   

    // 2. Initialize the I2C Bus
    i2c_master_init(&bus_handle); 

    // 3. Attach the BMP280 device to the bus
    bmp280_init(bus_handle, &bmp280_handle);

    // 4. Wake up sensor and start reading
    bmp280_wake_up(bmp280_handle);

    float actual_temp = 0.0;
    while (1) {
        if (bmp280_get_temperature(bmp280_handle, &actual_temp) == ESP_OK) {
            ESP_LOGI("MAIN", "Temperature: %.2f °C", actual_temp);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
----------------------------------------------------------------------------------------

🧠 Under the Hood: The I2C Transaction Pipeline

This driver exploits the ESP-IDF transmission engine by precisely mapping variables to the hardware's expected memory slots. For example, during a read operation:

    The target register (0xD0) is passed by value and loaded into the transmit buffer.

    The destination variable's memory address is passed to the 4th positional argument (Read Buffer) of the i2c_master_transmit_receive API.

    The hardware engine executes the I2C read and drops the payload directly into the provided memory address, resulting in zero-overhead data retrieval.

Developed for deep-dive exploration into bare-metal C programming and embedded systems architecture.
