#define LOG_LOCAL_ESP_LOG_INFO
#include "esp_log.h"
static const char *TAG = "net_app";

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_eth.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/ip4_addr.h"

#include "driver/gpio.h"
#include "sdkconfig.h"


#define ETHERNET_ON 1




#if ETHERNET_ON == 0
int s_retry_num = 0;
static wifi_ap_record_t ap_info;

static esp_err_t event_handler(void *ctx, system_event_t *event) {
    /* Invoke Provisioning event handler first */
    //app_prov_event_handler(ctx, event);
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "WIFI STA start");
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "connected, got ip:%s",
                ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        s_retry_num = 0;
        mbSet(mbCONNECTED);
        esp_wifi_sta_get_ap_info(&ap_info);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        if (s_retry_num < 3) {
            esp_wifi_connect();
            mbClean(mbCONNECTED);
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            ESP_LOGI(TAG, "connect to the AP fail\n");
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}
#else /* ETHERNET_ON == 0 */


uint8_t mac_addr[6] = {0};
/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{

    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        mbClean(mbCONNECTED);
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const tcpip_adapter_ip_info_t *ip_info = &event->ip_info;
    mbSet(mbCONNECTED);
    ESP_LOGI(TAG, "connected, got ip: "IPSTR" mask: "IPSTR" gateway: "IPSTR, IP2STR(&ip_info->ip),IP2STR(&ip_info->netmask),IP2STR(&ip_info->gw));
}
//Defined in esp_eth_phy_lan8742a.c
esp_eth_phy_t *esp_eth_phy_new_lan8742a(const eth_phy_config_t *config);
#endif /* ETHERNET_ON == 0 */

int app_net_ini(app_ip_config_t* ipcfg){
	
	main_event_group = xEventGroupCreate();
    mbClean(mbCONNECTED);
	
	

#if ETHERNET_ON == 0

    tcpip_adapter_init();



    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "test",
            .password = "querty",
        },
    };

    ESP_LOGE(TAG,"WiFi used as netif SSID: %s Password: %s",wifi_config.sta.ssid,wifi_config.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );
#else /* ETHERNET_ON == 0 */

    tcpip_adapter_init();
	
    if(ipcfg->dhcp_on==0){

        int ret = tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH);
        if(ret!=ESP_OK){
           ESP_LOGE(TAG,"tcpip_adapter_dhcpc_stop() fail");
        }

        char* settings_str = archive_get_ip_string();
        ESP_LOGI(TAG,"%s",settings_str);
        free(settings_str);
        tcpip_adapter_ip_info_t ip_info;
        ip_info.ip=ipcfg->ip;
        ip_info.netmask=ipcfg->nm;
        ip_info.gw=ipcfg->gw;
        ret = tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &ip_info);
        if(ret!=ESP_OK){
           ESP_LOGE(TAG,"tcpip_adapter_set_ip_info() fail");
        }

        tcpip_adapter_dns_info_t dns;
        dns.ip.u_addr.ip4=ipcfg->dns1;
        dns.ip.type=IPADDR_TYPE_V4;
        ret = tcpip_adapter_set_dns_info(TCPIP_ADAPTER_IF_ETH,TCPIP_ADAPTER_DNS_MAIN,&dns);
        if(ret!=ESP_OK){
           ESP_LOGE(TAG,"tcpip_adapter_set_dns_info(main) fail");
        }
        dns.ip.u_addr.ip4=ipcfg->dns2;
        dns.ip.type=IPADDR_TYPE_V4;
        ret = tcpip_adapter_set_dns_info(TCPIP_ADAPTER_IF_ETH,TCPIP_ADAPTER_DNS_BACKUP,&dns);
        if(ret!=ESP_OK){
           ESP_LOGE(TAG,"tcpip_adapter_set_dns_info(backup) fail");
        }
        dns.ip.u_addr.ip4=ipcfg->dns3;
        dns.ip.type=IPADDR_TYPE_V4;
        ret = tcpip_adapter_set_dns_info(TCPIP_ADAPTER_IF_ETH,TCPIP_ADAPTER_DNS_FALLBACK,&dns);
        if(ret!=ESP_OK){
           ESP_LOGE(TAG,"tcpip_adapter_set_dns_info(fallback) fail");
        }
    }

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(tcpip_adapter_set_default_eth_handlers());
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = 0; //ETH_PHY_ADDR
    phy_config.reset_gpio_num = 5; //ETH_PHY_RST_GPIO

    mac_config.smi_mdc_gpio_num = 23; //ETH_MDC_GPIO
    mac_config.smi_mdio_gpio_num = 18; //ETH_MDIO_GPIO;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);

    esp_eth_phy_t *phy = esp_eth_phy_new_lan8742a(&phy_config);

    /* Other PHY variants */
    //esp_eth_phy_t *phy = esp_eth_phy_new_lan8720(&phy_config);
    //esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);
    //esp_eth_phy_t *phy = esp_eth_phy_new_rtl8201(&phy_config);
    //esp_eth_phy_t *phy = esp_eth_phy_new_dp83848(&phy_config);

    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

#endif /* ETHERNET_ON == 0 */

    return ESP_OK;
}



