#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "nvs_flash.h"

#include "driver/gpio.h"
#include "driver/uart.h"

#if defined(CONFIG_Mxp)
    #warning Protocol used MXP
    #include "mxp.h"
#endif
#if defined(CONFIG_Artnet)
    #warning Protocol used Artnet
    #include "artnet.h"
#endif

#include "ws2811dma.h"

/**
 * Caso a placa fique piscando loucamente após gravar e não volte a funcionar nem por ação de reza brava. Use o seguinte comando
 *   esptool.py --port /dev/ttyUSB0 write_flash 0x3fc000 ~/.platformio/packages/framework-esp8266-rtos-sdk/bin/esp_init_data_default.bin
 * 
 * Para instalar o esptool.py, use
 *   pip install esptool
 * 
 * Para dar erase 
 *   esptool.py --port /dev/ttyUSB0 erase_flash
 * 
 */

#if defined(CONFIG_Artnet) && defined(CONFIG_Mxp)
    #error "Choose only one protocol!"
#endif

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
/* User AP config is defined in wifi_config.h, make sure to create this file and associate USER_SSID and USER_PASS*. This file will not be versioned.*/
#ifdef USER_SSID
    #define ESP_WIFI_SSID       USER_SSID
#else
    #define ESP_WIFI_SSID       CONFIG_USER_SSID
#endif
#ifdef USER_PASS
    #define ESP_WIFI_PASS       USER_PASS
#else
    #define ESP_WIFI_PASS       CONFIG_USER_PASS
#endif
#define ESP_MAXIMUM_RETRY  10

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "main";
static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (WIFI_EVENT == event_base) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                if (s_retry_num < ESP_MAXIMUM_RETRY) {
                    esp_wifi_connect();
                    s_retry_num++;
                    ESP_LOGI(TAG, "retry to connect to the AP");
                } else {
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                }
                ESP_LOGI(TAG,"connect to the AP fail");
                break;
            default:
                break;
        }
    } else if (IP_EVENT == event_base) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
            {
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI(TAG, "got ip:%s",
                         ip4addr_ntoa(&event->ip_info.ip));
                s_retry_num = 0;
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

                #if defined(CONFIG_Artnet)
                    printf("Running Artnet!\n");
                    artnet_init(ws2811dma_put, CONFIG_MAXPIXELS);
                #endif
                #if defined(CONFIG_Mxp)
                    printf("Running MXP!\n");
                    mxp_init(ws2811dma_put, CONFIG_MAXPIXELS);
                #endif
                break;
            }
            default:
                break;
        }
    }
}

void task_blink(void* ignore)
{
    #define PIN 2
    gpio_config_t gpio = {
        .pin_bit_mask = BIT(PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&gpio);

    while(true) {
        ret = gpio_set_level(PIN, 0);
        vTaskDelay(1000/portTICK_RATE_MS);
        ret = gpio_set_level(PIN, 1);
        vTaskDelay(1000/portTICK_RATE_MS);
        ESP_LOGI(TAG, "LED panel is alive!");
    }

    vTaskDelete(NULL);
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS
        },
    };

    /* Setting a password implies station will connect to all security modes including WEP/WPA.
        * However these modes are deprecated and not advisable to be used. Incase your Access point
        * doesn't support WPA2, these mode can be enabled by commenting below line */

    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    //uart_init_new();
    //UART_SetBaudrate(UART0, 115200);
    //UART_SetBaudrate(UART1, 115200);
    xTaskCreate(&task_blink, "startup", 2*1024, NULL, 5, NULL);

    //espconn_init();
    ESP_LOGI(TAG, "SDK version:%s\r\n", esp_get_idf_version());

    ESP_LOGI(TAG, "Initializing WS2811...\n\n");
    ws2811dma_init(COLOR_GRB);
}

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");

    user_init();
    wifi_init_sta();
}

