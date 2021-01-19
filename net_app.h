#ifndef APP_NET_H_
#define APP_NET_H_

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/ip4_addr.h"

enum{
    mbCONNECTED=BIT0,     //Net connected
};

typedef struct{
    ip4_addr_t ip;
    ip4_addr_t nm;
    ip4_addr_t gw;
    ip4_addr_t dns1;
    ip4_addr_t dns2;
    ip4_addr_t dns3;
    uint8_t dhcp_on;
}app_ip_config_t;


EventGroupHandle_t main_event_group;

#define mbCheck(bit) ((xEventGroupWaitBits(main_event_group,bit,pdFALSE,pdTRUE,0)&bit)==bit)
#define mbSet(bit)   xEventGroupSetBits(main_event_group,bit)
#define mbClean(bit) xEventGroupClearBits(main_event_group,bit)


void app_net_ini(app_ip_config_t* ipconfig);



#endif /* APP_NET_H_ */
