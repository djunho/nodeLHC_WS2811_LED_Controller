// System
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>

#include "esp_libc.h"
#include "esp_log.h"
#include "lwip/ip_addr.h"
#include "lwip/sockets.h"

// Project
#include "mxp.h"

// Configuration: UDP port and maximum number of LEDs in payload
#define MXP_UDP_PORT 2711
#define MXP_MAXLEN 300

static const char *TAG = "MXP";
led_callback callback;

void onMxpRecv(uint8_t *dat, uint16_t len) {
    uint16_t length = dat[3] * 0xff + dat[4];
    uint16_t offset = dat[1] * 0xff + dat[2];
    //uart0_tx_buffer(dat,len);

    /*** Copy buffer data into static memory to pass a pointer to callback function ***/
    uint16_t i;
    static uint8_t buf[MXP_MAXLEN*3];
    for (i = 0; i < (length * 3); i++)
    {
        buf[((i / 3)*3) + (i % 3)] = dat[5 + i];
    }

    callback(buf, length, offset);
}

static void udp_server_task(void *pvParameters)
{
    uint8_t buffer_size = (uint32_t)pvParameters;
    uint8_t rx_buffer[buffer_size];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while(1) {
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(MXP_UDP_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket binded");

        while (1) {
            ESP_LOGD(TAG, "Waiting for data");
            struct sockaddr_in sourceAddr;
            socklen_t socklen = sizeof(sourceAddr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&sourceAddr, &socklen);

            // Error occured during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                if (rx_buffer[0] != 0x55){
                    ESP_LOGE(TAG, "Wrong start of frame");
                    continue;
                }

                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);

                //ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                //ESP_LOGI(TAG, "%02X %04X %04X", rx_buffer[0], rx_buffer[1]*0xff + rx_buffer[2], rx_buffer[3]*0xff + rx_buffer[4]);
                //for (int i = 0; i < 8*8; i++){
                //    for (int j = 0; j < 8*3; j += 3) {
                //        printf("%02X%02X%02X ", rx_buffer[5 + 8*i + j], rx_buffer[5 + 8*i + j + 1], rx_buffer[5 + 8*i + j + 2]);
                //    }
                //    printf("\n");
                //}
                onMxpRecv(rx_buffer, len);

                //int err = sendto(sock, rx_buffer, len, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
                //if (err < 0) {
                //    ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                //    break;
                //}
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void mxp_init(led_callback led_cb, uint32_t num_leds) {
    uint32_t buffer_size = 5 + num_leds*3;
    callback = led_cb;
    ESP_LOGI(TAG, "Initializing MXP");
    xTaskCreate(udp_server_task, "MXP UDP port", 4*1024, (void*)buffer_size, 2, NULL);
}
