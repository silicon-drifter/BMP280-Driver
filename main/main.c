#include <stdio.h>
#include <string.h>
#include <math.h> 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c_bus.h"
#include "bmp280.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h" 
#include "esp_crt_bundle.h"
#include "esp_netif_sntp.h"

static const char *TAG = "MAIN";

// Global variables for Deadband
float last_published_temp = -100.0;
const float DEADBAND_THRESHOLD = 0.2; 

// WIFI CONFIGURATION
#define WIFI_SSID      "Airtel_Arjun_2.4_ESP"
#define WIFI_PASS      "Kirandj@78"
#define FIREBASE_URL   "https://iot-dashboard-15f19-default-rtdb.asia-southeast1.firebasedatabase.app/sensor.json"

void wifi_init_and_connect(void) {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_t *netif = esp_netif_create_default_wifi_sta(); 

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "WiFi Hardware ON. Starting Connection...");

  esp_netif_ip_info_t ip_info;
    while (1) {
        esp_wifi_connect(); // WiFi connection trigger
        vTaskDelay(pdMS_TO_TICKS(3000));

        esp_netif_get_ip_info(netif, &ip_info);
        if (ip_info.ip.addr != 0) {
            ESP_LOGI(TAG, "Got IP Address! Internet is ready.");
            break;
        }
        ESP_LOGW(TAG, "Waiting for Router to assign IP...");
    }
}

void sync_time(void) {
    ESP_LOGI(TAG, "Syncing real time from internet for SSL verification...");
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_netif_sntp_init(&config);

    // Wait up to 15 seconds for time synchronization
    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(15000)) == ESP_OK) {
        ESP_LOGI(TAG, "Time synced successfully! SSL will work now.");
    } else {
        ESP_LOGE(TAG, "Time sync failed! Internet might be down.");
    }
}

void post_data_to_firebase(float temp) {
    char post_data[100];
    sprintf(post_data, "{\"temperature\": %.2f}", temp);

    esp_http_client_config_t config = {
        .url = FIREBASE_URL,
        .method = HTTP_METHOD_PATCH,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Firebase Update Success! Data: %s", post_data);
    } else {
        ESP_LOGE(TAG, "Firebase Update Failed!");
    }
    esp_http_client_cleanup(client);
}

// Ye ek naya helper function banaya hai FORCED MODE ke liye
// Tere bmp280.h/c mein shayad iska direct function nahi hoga, isliye manually register write kar rahe hain
void bmp280_trigger_forced_measurement(i2c_master_dev_handle_t handle) {
    // BMP280 ke 'ctrl_meas' register (0xF4) par 0x25 likhne se Forced Mode (1x oversampling) trigger hota hai
    uint8_t forced_mode_cmd[2] = {0xF4, 0x25}; 
    i2c_master_transmit(handle, forced_mode_cmd, sizeof(forced_mode_cmd), -1);
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting ESP32 Bare-Metal I2C Master (Optimized)...");

    // 1. Connect to WiFi and Ensure IP is assigned
    wifi_init_and_connect();

    // 2. Get Real Date/Time for SSL Validation
    sync_time();

    // 3. Initialize I2C and BMP280 Sensor
    i2c_master_bus_handle_t bus_handle;      
    i2c_master_dev_handle_t bmp280_handle;   

    ESP_LOGI(TAG, "Initializing I2C Master Bus...");
    esp_err_t err = i2c_master_init(&bus_handle); 
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C bus!");
        return; 
    }

    ESP_LOGI(TAG, "Adding BMP280 to the I2C Bus...");
    err = bmp280_init(bus_handle, &bmp280_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register sensor!");
        return;
    }

    uint8_t chip_id = 0; 
    err = bmp_280_read_register(bmp280_handle, 0xD0, &chip_id, 1);
    if (chip_id == 0x58) {
        ESP_LOGI(TAG, "BMP280 verified (0x58) 🎉");
        // HATA DIYA: bmp280_wake_up() ko hata diya kyunki ab hum Normal (continuous) mode use nahi karenge
        ESP_LOGI(TAG, "Sensor is ready in SLEEP mode...");
    }

    // 4. Infinite Loop: Trigger Read -> Check Deadband -> Post to Firebase -> Wait 5s
    float actual_temp = 0.0;     
    while (1) {
        
        // STEP 1: Sensor ko jaga kar sirf ek measurement lene bolo (Forced Mode)
        bmp280_trigger_forced_measurement(bmp280_handle);
        
        // Sensor ko reading lene ke liye thoda time do (50ms bahut hai)
        vTaskDelay(pdMS_TO_TICKS(50)); 
        
        // Is point ke baad sensor waise bhi wapas sleep mein chala jayega (No self-heating!)

        // STEP 2: Ab data read karo
        esp_err_t status = bmp280_get_temperature(bmp280_handle, &actual_temp);
        
        if (status == ESP_OK) {
            ESP_LOGI(TAG, "Actual Temperature: %.2f °C", actual_temp);
            
            // STEP 3: DEADBAND CHECK
            // Agar pichli bheji hui value aur is nayi value mein 0.2 ya usse zyada ka difference hai, tabhi Firebase bhejo
            if (fabs(actual_temp - last_published_temp) >= DEADBAND_THRESHOLD) {
                ESP_LOGW(TAG, "Significant Temp Change! Publishing to Firebase...");
                post_data_to_firebase(actual_temp);
                last_published_temp = actual_temp; // Nayi value save kar lo
            } else {
                ESP_LOGI(TAG, "Temp is stable. Skipping Firebase Write.");
            }
            
        } else {
            ESP_LOGE(TAG, "Failed to read data from sensor!");
        }

        // STEP 4: So jao 5 second ke liye
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}