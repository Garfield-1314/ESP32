#include "driver/inc/espnow.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "esp_crc.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"

static const char *TAG = "app_espnow";

static void app_espnow_send_cb(const esp_now_send_info_t *info,
                               esp_now_send_status_t status) {
  if (info == NULL || info->des_addr == NULL) {
    ESP_LOGE(TAG, "Send cb arg error");
    return;
  }

  ESP_LOGD(TAG, "Send to " MACSTR " status: %d", MAC2STR(info->des_addr),
           status);
}

static void app_espnow_recv_cb(const esp_now_recv_info_t *recv_info,
                               const uint8_t *data, int len) {
  uint8_t *mac_addr = recv_info->src_addr;

  if (mac_addr == NULL || data == NULL || len <= 0) {
    ESP_LOGE(TAG, "Receive cb arg error");
    return;
  }

  ESP_LOGI(TAG, "Receive len: %d from " MACSTR, len, MAC2STR(mac_addr));
  // ESP_LOG_BUFFER_HEXDUMP(TAG, data, len, ESP_LOG_INFO);

  // TODO: Process received data here or send to a queue
}

void app_espnow_init(void) {
  /* Initialize NVS */
  /* NVS should be initialized in main or before this function.
     If not, uncomment the following lines:
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  { ESP_ERROR_CHECK( nvs_flash_erase() ); ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK( ret );
  */

  /* Initialize WiFi */
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(
      esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));  // Default channel 1

  /* Initialize ESPNOW */
  ESP_ERROR_CHECK(esp_now_init());
  ESP_ERROR_CHECK(esp_now_register_send_cb(app_espnow_send_cb));
  ESP_ERROR_CHECK(esp_now_register_recv_cb(app_espnow_recv_cb));

  ESP_LOGI(TAG, "ESPNow Initialized");
}

esp_err_t app_espnow_send(const uint8_t *dest_addr, const uint8_t *data,
                          size_t len) {
  return esp_now_send(dest_addr, data, len);
}

esp_err_t app_espnow_add_peer(const uint8_t *peer_addr) {
  esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
  if (peer == NULL) {
    return ESP_ERR_NO_MEM;
  }
  memset(peer, 0, sizeof(esp_now_peer_info_t));
  peer->channel = 1;  // Default channel
  peer->ifidx = WIFI_IF_STA;
  peer->encrypt = false;
  memcpy(peer->peer_addr, peer_addr, ESP_NOW_ETH_ALEN);

  esp_err_t err = esp_now_add_peer(peer);
  free(peer);
  return err;
}
