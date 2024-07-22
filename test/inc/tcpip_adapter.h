#ifndef _TCPIP_ADAPTER_H_
#define _TCPIP_ADAPTER_H_

#include "esp_err.h"
#include "lwip/ip4_addr.h"

/* @brief On-chip network interfaces */
typedef enum {
    TCPIP_ADAPTER_IF_STA = 0,     /**< Wi-Fi STA (station) interface */
    TCPIP_ADAPTER_IF_AP,          /**< Wi-Fi soft-AP interface */
    TCPIP_ADAPTER_IF_ETH,         /**< Ethernet interface */
    TCPIP_ADAPTER_IF_TEST,        /**< tcpip stack test interface */
    TCPIP_ADAPTER_IF_MAX
} tcpip_adapter_if_t;

/**
 *  @brief TCP-IP adapter IPV4 address information
 */
typedef struct {
    ip4_addr_t ip;              /**< TCP-IP adatpter IPV4 addresss */
    ip4_addr_t netmask;         /**< TCP-IP adatpter IPV4 netmask */
    ip4_addr_t gw;              /**< TCP-IP adatpter IPV4 gateway */
} tcpip_adapter_ip_info_t;

static inline esp_err_t tcpip_adapter_get_ip_info(tcpip_adapter_if_t tcpip_if, tcpip_adapter_ip_info_t *ip_info)
{
    (void)tcpip_if;
    (void)ip_info;
    return 0;
}

#endif // _TCPIP_ADAPTER_H_

