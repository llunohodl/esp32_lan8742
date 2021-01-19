#ifndef APP_NET_H_
#define APP_NET_H_

enum{
    mbCONNECTED=BIT0,     //Net connected
};


EventGroupHandle_t main_event_group;

#define mbCheck(bit) ((xEventGroupWaitBits(main_event_group,bit,pdFALSE,pdTRUE,0)&bit)==bit)
#define mbSet(bit)   xEventGroupSetBits(main_event_group,bit)
#define mbClean(bit) xEventGroupClearBits(main_event_group,bit)


void app_net_ini();



#endif /* APP_NET_H_ */
