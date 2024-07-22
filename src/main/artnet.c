/*
 * Based on the implementation of Frans-Willem 
 * https://github.com/Frans-Willem/EspLightNode/tree/master/user/input_protocols
 */

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>    // memset

#include "esp_log.h"

#include "esp_wifi.h"
#include "tcpip_adapter.h"
#include "lwip/inet.h"      // INADDR_ANY
#include "lwip/sockets.h"
#include "lwip/def.h"

#include "FreeRTOS.h"
#include "task.h"

#include "mxp.h"


#define ARTNET_Port 0x1936

uint8_t artnet_enabled = 1;
char artnet_shortname[18] = "LHC";
char artnet_longname[64] = "LHC Painel de LED";
uint8_t artnet_net = 0;
uint8_t artnet_subnet = 0;
uint8_t artnet_universe = 0;

static const char *TAG = "Artnet";
led_callback callback;


static void artnet_recv_opoutput(unsigned char *packet, ssize_t packetlen) {
    if (packetlen >= 8) {
        uint16_t ProtVer=((uint16_t)packet[0] << 8) | packet[1];
        if (ProtVer == 14) {
            //uint8_t Sequence = packet[2];
            //uint8_t Physical = packet[3];
            uint8_t SubUni = packet[4];
            uint8_t Net = packet[5];

            if (Net == artnet_net && (SubUni >> 4) == artnet_subnet && (SubUni & 0xF) == artnet_universe) {
                uint16_t Length = ((uint16_t)packet[6] << 8) | packet[7];
                if ((packetlen - 8)/3 >= Length){
                    callback(&packet[8], Length, 0);
                }
            } else {
                //Not intended for us...
            }
        }
    } else {
        //Invalid length
    }
}

#define ARTNET_invalid      0xFFFF
#define ARTNET_OpPoll       0x2000
#define ARTNET_OpPollReply  0x2100
#define ARTNET_OpOutput     0x5000

#define TTM_REPLY_MASK    0

struct ArtNetPollReply {
    uint8_t ID[8];
    uint8_t OpCode[2]; //low-first
    uint8_t IP[4];
    uint8_t Port[2]; //Low-first
    uint8_t VersInfo[2]; //High-first
    uint8_t NetSwitch;
    uint8_t SubSwitch;
    uint8_t Oem[2]; //High-first
    uint8_t Ubea_Version;
    uint8_t Status1;
    uint8_t EstaMan[2]; //Low-first
    uint8_t ShortName[18];
    uint8_t LongName[64];
    uint8_t NodeReport[64];
    uint8_t NumPorts[2]; //High-first
    uint8_t PortTypes[4];
    uint8_t GoodInput[4];
    uint8_t GoodOutput[4];
    uint8_t SwIn[4];
    uint8_t SwOut[4];
    uint8_t SwVideo;
    uint8_t SwMacro;
    uint8_t SwRemote;
    uint8_t Spare[3];
    uint8_t Style;
    uint8_t MAC[6]; //High-byte first
    uint8_t BindIp[4];
    uint8_t BindIndex;
    uint8_t Status2;
    uint8_t Filler[26];
};

#define ARTNET_SET_SHORT_LOFIRST(target,value) (target)[0] = (value) & 0xFF; (target)[1] = (value) >> 8;
#define ARTNET_SET_SHORT_HIFIRST(target,value) (target)[0] = (value) >> 8; (target)[1] = (value) & 0xFF;

static void artnet_recv_oppoll(unsigned char *packet, ssize_t packetlen, struct ArtNetPollReply *response) {
    if (packetlen >= 3) {
        //uint16_t ProtVer=((uint16_t)packet[0] << 8) | packet[1];
        uint8_t TalkToMe=packet[2];
        //TODO
        if (TalkToMe & TTM_REPLY_MASK) {

        } else {

        }
        uint8_t eth_mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, eth_mac);

        tcpip_adapter_ip_info_t ip_info;
        tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);

        memset(response, 0, sizeof(struct ArtNetPollReply));
        strcpy((char*)response->ID, "Art-Net");
        ARTNET_SET_SHORT_LOFIRST(response->OpCode, ARTNET_OpPollReply);
        memcpy(response->IP, &ip_info.ip.addr, 4);
        ARTNET_SET_SHORT_LOFIRST(response->Port, ARTNET_Port);
        ARTNET_SET_SHORT_HIFIRST(response->VersInfo, 0);
        response->NetSwitch = artnet_net;
        response->SubSwitch = artnet_subnet;
        ARTNET_SET_SHORT_HIFIRST(response->Oem,0);
        response->Ubea_Version = 0;
        response->Status1 = 0;
        ARTNET_SET_SHORT_LOFIRST(response->EstaMan,0);
        memcpy(response->ShortName,artnet_shortname, sizeof(response->ShortName));
        memcpy(response->LongName,artnet_longname, sizeof(response->LongName));
        strcpy((char *)response->NodeReport,"");
        ARTNET_SET_SHORT_HIFIRST(response->NumPorts,1);
        response->PortTypes[0]=0x80;
        //Not set is set to 0
        //response->GoodInput = 0;
        //response->GoodOutput = 0;
        response->SwOut[0] = artnet_universe;
        //response->SwVideo
        //response->SwMacro
        //response->SwRemote
        //response->Style
        memcpy(response->MAC, eth_mac, 6);
        //response->BindIp
        //response->BindIndex
        //response->Status2
    } else {
        //Invalid length
    }
}

static uint16_t get_op_artnet(uint8_t *data, ssize_t length){
    if (data && length>=10) {
        if (data[0] == 'A' && data[1] == 'r' && data[2] == 't' && data[3] == '-' &&
            data[4] == 'N' && data[5] == 'e' && data[6] == 't' && data[7] == '\0'   ) {
            uint16_t OpCode = data[8] | ((uint16_t)data[9] << 8);
            return OpCode;
        } else {
            //Header invalid
            ESP_LOGE(TAG, "Wrong header input");
        }
    } else {
        //Package too small.
        ESP_LOGE(TAG, "Pckt too small");
    }
    return ARTNET_invalid;
}

static void udp_server_task(void *pvParameters)
{
    size_t buffer_size = (size_t)pvParameters;
    uint8_t rx_buffer[buffer_size];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while(1) {
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(ARTNET_Port);
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
            ssize_t len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&sourceAddr, &socklen);

            // Error occured during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                // Get the sender's ip address as string
                inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                //
                //ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                //ESP_LOGI(TAG, "%s", rx_buffer);
                //ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                //ESP_LOGI(TAG, "%02X %04X %04X", rx_buffer[0], rx_buffer[1]*0xff + rx_buffer[2], rx_buffer[3]*0xff + rx_buffer[4]);
                //for (int i = 0; i < 8*8; i++){
                //    for (int j = 0; j < 8*3; j += 3) {
                //        printf("%02X%02X%02X ", rx_buffer[5 + 8*i + j], rx_buffer[5 + 8*i + j + 1], rx_buffer[5 + 8*i + j + 2]);
                //    }
                //    printf("\n");
                //}

                uint16_t opcode = get_op_artnet(rx_buffer, len);
                switch (opcode) {
                    case ARTNET_OpOutput:
                        artnet_recv_opoutput(&rx_buffer[10], len-10);
                        ESP_LOGE(TAG, "Received msg");
                        break;
                    case ARTNET_OpPoll:
                    {
                        struct ArtNetPollReply response;
                        artnet_recv_oppoll(&rx_buffer[10], len-10, &response);
                        ssize_t send_len = sendto(sock, rx_buffer, (size_t)len, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
                        if (send_len < 0) {
                            ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                        }
                        break;
                    }
                    default:
                        ESP_LOGE(TAG, "Invalid opcode/Not implemented");
                }
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

void artnet_init(led_callback led_cb, uint32_t num_leds) {
    size_t buffer_size = 18 + num_leds*3;
    callback = led_cb;
    ESP_LOGI(TAG, "Initializing Artenet");
    xTaskCreate(udp_server_task, "Artnet UDP port", 4*1024, (void*)buffer_size, 2, NULL);
}
