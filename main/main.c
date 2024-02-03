#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "nvs_flash.h"

#include "ttn.h"
#include "nmea_parser.h"

// EUI Key for synchronize data to TTN
const char *appEui = "B827EBFFFE5F4077";
const char *devEui = "70B3D57ED00641C0";
const char *appKey = "D30B8B0E797F129D134044E5447C9574";

// Pins and other resources
#define TTN_SPI_HOST      SPI2_HOST
#define TTN_SPI_DMA_CHAN  SPI_DMA_DISABLED
#define TTN_PIN_SPI_SCLK  5
#define TTN_PIN_SPI_MOSI  27
#define TTN_PIN_SPI_MISO  19
#define TTN_PIN_NSS       18
#define TTN_PIN_RXTX      TTN_NOT_CONNECTED
#define TTN_PIN_RST       14
#define TTN_PIN_DIO0      26
#define TTN_PIN_DIO1      35

static const char *TAG = "GNSS";

#define TX_INTERVAL 2       // default 5
#define TIME_ZONE (+7)
#define YEAR_BASE (2000)
#define EARTH_RADIUS 6371

// Harversine important variable
double a, c, distance;

gps_t *gps = NULL;

QueueHandle_t myqueue;

//Haversine Equations
double hav(double lat1, double lon1, double lat2, double lon2)
{
    // Convert the Geographic Location from degree to Radian.
    lon1 = (lon1 * M_PI) / 180;
    lon2 = (lon2 * M_PI) / 180;
    lat1 = (lat1 * M_PI) / 180;
    lat2 = (lat2 * M_PI) / 180;

    // Geographical Location Differences
    double diff_lon = lon2 - lon1;
    double diff_lat = lat2 - lat1;

    double sin2_lat = sin(diff_lat/2) * sin(diff_lat/2);
    double sin2_lon = sin(diff_lon/2) * sin(diff_lon/2);

    a = sin2_lat + cos(lat1)*cos(lat2) * sin2_lon;

    c = 2*atan2(sqrt(a), sqrt(1-a));

    distance = c * EARTH_RADIUS;

    return distance;
}

static void gps_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
        case GPS_UPDATE:
            gps = (gps_t *)event_data;

            double longitude = gps->longitude;
            double latitude = gps->latitude;
            double coordinate[2] = {latitude, longitude};

            xQueueSend(myqueue, &coordinate, portMAX_DELAY);

            ESP_LOGI(TAG, "Data is logging");
            ESP_LOGI(TAG, " latitude = %.5f °N \r\n \t\tlongitude = %.5f °E \r\n", gps->latitude, gps->longitude);
            ESP_LOGI( TAG, "%.2f km", hav(11.571006, 104.897023,gps->latitude, gps->longitude) );
            break;
        case GPS_UNKNOWN:
            /* print unknown statements */
            ESP_LOGW(TAG, "Unknown statement:%s", (char *)event_data);
            break;
        default:
            break;
    }
}

// Function to send Payload to TTN Applications
void sendMessages(void* pvParameter)
{
    double coordinate[2];

    nmea_parser_config_t config = NMEA_PARSER_CONFIG_DEFAULT();
    nmea_parser_handle_t nmea_hdl = nmea_parser_init(&config);
    nmea_parser_add_handler(nmea_hdl, gps_event_handler, NULL);

    vTaskDelay(10000 / portTICK_PERIOD_MS);
    
    while (1)
     {
        printf("Sending message...\n");

        if (gps == NULL)            // For Condition, if the gps do not receive the event_data of the handler.
        {
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        if(xQueueReceive(myqueue, &coordinate, portMAX_DELAY))
        {
            double latitude2 = coordinate[0];
            double longitude2 = coordinate[1];    

            printf("longitude2 = %.5f | latitude2 = %.5f \r\n", longitude2, latitude2);

            int32_t longi = (int32_t)(longitude2 * 10000);              // use 32 bits because the data is 24 bits
            int32_t lati = (int32_t)(latitude2 * 10000);

            uint8_t payload[6];

            payload[0] = (int8_t)lati;
            payload[1] = (int8_t)(lati >> 8);
            payload[2] = (int8_t)(lati >> 16);

            payload[3] = (int8_t) longi;
            payload[4] = (int8_t)(longi >> 8);
            payload[5] = (int8_t)(longi >> 16);
            
            ttn_response_code_t res = ttn_transmit_message(payload, sizeof(payload), 1, false);
            printf(res == TTN_SUCCESSFUL_TRANSMISSION ? "Message sent.\n" : "Transmission failed.\n");

            nmea_parser_remove_handler(nmea_hdl, gps_event_handler);    //
            nmea_parser_deinit(nmea_hdl);                               //

            vTaskDelay(TX_INTERVAL * pdMS_TO_TICKS(1000)); 
        }
    }
}

//Function to received status of Payload
void messageReceived(const uint8_t* message, size_t length, ttn_port_t port)
{
    printf("Message of %d bytes received on port %d:\n", length, port);
    for (int i = 0; i < length; i++)
        printf(" %02x", message[i]);
    printf("\n");
}

void app_main(void)
{
    myqueue = xQueueCreate(2, sizeof(double[2]));

    esp_err_t err;
    err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    ESP_ERROR_CHECK(err);
    
    err = nvs_flash_init();
    ESP_ERROR_CHECK(err);

    //Initiate SPI configurations
    spi_bus_config_t spi_bus_config = {
        .miso_io_num = TTN_PIN_SPI_MISO,
        .mosi_io_num = TTN_PIN_SPI_MOSI,
        .sclk_io_num = TTN_PIN_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    }; 

    err = spi_bus_initialize(TTN_SPI_HOST, &spi_bus_config, TTN_SPI_DMA_CHAN);
    ESP_ERROR_CHECK(err);

    //Initiate TTN
    ttn_init();
    ttn_configure_pins(TTN_SPI_HOST, TTN_PIN_NSS, TTN_PIN_RXTX, TTN_PIN_RST, TTN_PIN_DIO0, TTN_PIN_DIO1);
    ttn_provision(devEui, appEui, appKey);
    ttn_on_message(messageReceived);

    printf("Joining...\n");
    if (ttn_join())
    {
        printf("Joined.\n");
        xTaskCreate(sendMessages, "send_messages", 1024 * 4, (void* )0, 3, NULL);
    }
    else
    {
        printf("Join failed. Goodbye\n");
    }
}
