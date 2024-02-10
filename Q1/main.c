// Including the required libraries

#include <stdio.h>

// Integer types
#include <stdint.h>
#include <stddef.h>

// String manipulation
#include <string.h>

// WiFi
#include "esp_wifi.h"

// System
#include "esp_system.h"

// Non-volatile storage
#include "nvs_flash.h"

// Event system
#include "esp_event.h"

// Network interface
#include "esp_netif.h"
#include "my_data.h"

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

// Socket interface
#include "lwip/sockets.h"

// Domain name resolution (DNS)
#include "lwip/dns.h"

// Network information retrieval
#include "lwip/netdb.h"

// Logging
#include "esp_log.h"

// MQTT client
#include "mqtt_client.h"


static const char *TAG = "MQTT_TCP";

// Handling WiFi events
static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {

    // WiFi station mode started and initializing
    case WIFI_EVENT_STA_START:
        printf("WiFi connecting ... \n");
        break;

    // WiFi station connected to an AP (Access Point)
    case WIFI_EVENT_STA_CONNECTED:
        printf("WiFi connected ... \n");
        break;
    
    // WiFi station connection lost
    case WIFI_EVENT_STA_DISCONNECTED:
        printf("WiFi lost connection ... \n");
        break;

    // WiFi station received an IP address and established a network connection
    case IP_EVENT_STA_GOT_IP:
        printf("WiFi got IP ... \n\n");
        break;

    // Unexpected WiFi event
    default:
        break;

    }
}

// Connecting to WiFi
void wifi_connection()
{
    /// 1 - Wi-Fi/LwIP Init
    // TCP/IP stack (LwIP) initialization
    esp_netif_init();                    
    // Creating the default event loop instance for handling WiFi events
    esp_event_loop_create_default();    
    // Creating the default WiFi station interface
    esp_netif_create_default_wifi_sta(); 
    // Initializing the WiFi module with default settings
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT(); 
    esp_wifi_init(&wifi_initiation); 

    /// 2 - Wi-Fi Configuration 
    // Registering the WiFi event handler to receive notifications about WiFi events
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    // Registering the IP event handler to receive notifications about IP acquisition
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    // Specifying the WiFi network configuration parameters
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = SSID,
            .password = PASS}};
    // Applying the WiFi configuration to the ESP32
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);

    /// 3 - Wi-Fi Start 
    // Beginning the WiFi station mode and scanning for available networks
    esp_wifi_start();

    /// 4- Wi-Fi Connect 
    // Initiating the connection to the specified WiFi network
    esp_wifi_connect();
}

// Handling MQTT event callbacks
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{   
    // Pointer to the MQTT client instance
    esp_mqtt_client_handle_t client = event->client;

    // Handling different MQTT events
    switch (event->event_id)
    {

    // Connected to the MQTT broker
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // Subscribing to the 'SIoT' topic
        esp_mqtt_client_subscribe(client, "SIoT", 0);
        // Publishing a message to the 'SIoT' topic
        esp_mqtt_client_publish(client, "SIoT", "Hello! This is Melika with 99101608, Enjoying HW3 of SIOT course :))", 0, 1, 0);
        // QoS 0, retain flag set
        break;
    
    // Disconnected from the MQTT broker
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    
    // Successfully subscribed to a topic
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    // Successfully unsubscribed from a topic
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    // Successfully published a message to a topic
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    // Received data from a subscribed topic
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        // Printing the received topic and data
        printf("\nTOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;

    // An error occurred in the MQTT connection
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;

    // Handle any other MQTT events
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;

    }

    // Returning ESP_OK to indicate success
    return ESP_OK;
}

// Handling MQTT event
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // Logging a debug message indicating the event source and ID
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    // Forwarding the MQTT event to the MQTT event callback function for processing
    mqtt_event_handler_cb(event_data);
}

// Initializing and starting the MQTT client
static void mqtt_app_start(void)
{
    // Creating an MQTT client configuration struct
    esp_mqtt_client_config_t mqtt_cfg = {
        // Setting the MQTT broker URI
        .broker.address.uri = "mqtt://192.168.43.16:1883",
    };
    // Initializing the MQTT client using the configuration
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    // Registering the MQTT event handler to receive notifications about MQTT events
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    // Starting the MQTT client and connecting to the broker
    esp_mqtt_client_start(client);
}

// Main entry point for the application
void app_main(void)
{
    // Initializing the non-volatile storage (NVS) for storing configuration data
    nvs_flash_init();

    // Establishing a WiFi connection
    wifi_connection();

    // Delaying for 2 seconds to allow WiFi connection to complete
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    printf("WIFI was initiated ...........\n");

    // Starting the MQTT application
    mqtt_app_start();
}