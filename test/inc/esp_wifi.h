#ifndef _ESP_WIFI_H_
#define _ESP_WIFI_H_

#include <stdint.h>

typedef int esp_err_t;

typedef enum {
    WIFI_IF_STA = 0,
    WIFI_IF_AP  = 1,
} wifi_interface_t;

static inline esp_err_t esp_wifi_get_mac(wifi_interface_t ifx, uint8_t mac[6])
{
    (void)ifx;
    (void)mac;
    return 0;
}

#endif // _ESP_WIFI_H_
