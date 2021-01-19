# esp32_lan8742

This is minimal implementation of ethernet PHY lan8742 driver for ESP32

## Usage

in main.c:
```
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "lwip/ip4_addr.h"

#include "net_app.h"

void app_main(void){
    printf("\n\n+++ REBOOT +++\n\n");

    app_ip_config_t* ipcfg = (app_ip_config_t*)malloc(sizeof(app_ip_config_t));
    ip4addr_aton("192.168.1.100",&ipcfg->ip);
    ip4addr_aton("192.168.1.255",&ipcfg->nm);
    ip4addr_aton("192.168.1.1",&ipcfg->gw);
    ip4addr_aton("8.8.8.8",&ipcfg->dns1);
    ip4addr_aton("8.8.4.4",&ipcfg->dns2);
    ip4addr_aton("195.239.36.27",&ipcfg->dns3);
    ipcfg->dhcp_on=1;

    app_net_ini(ipcfg);
    uint8_t last_state = mbCheck(mbCONNECTED);
    while(1){
       uint8_t cur_state = mbCheck(mbCONNECTED);
       if(last_state!=cur_state){
          ESP_LOGI("main","Net is %s", (cur_state ? "up" : "down" ));
       }
       last_state=cur_state;
       vTaskDelay(1000 / portTICK_PERIOD_MS); //Wait 1 second
    }
    free(ipcfg);
}
```

