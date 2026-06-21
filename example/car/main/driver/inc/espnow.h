#pragma once

#include "esp_err.h"
#include "esp_now.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize ESPNOW
 */
void app_espnow_init(void);

/**
 * @brief Send ESPNOW data
 *
 * @param[in] dest_addr Destination MAC address (NULL for broadcast)
 * @param[in] data      Data to send
 * @param[in] len       Length of data
 * @return esp_err_t    ESP_OK on success
 */
esp_err_t app_espnow_send(const uint8_t *dest_addr, const uint8_t *data,
                          size_t len);

/**
 * @brief Add a peer to the peer list
 *
 * @param[in] peer_addr Peer MAC address
 * @param[in] channel   WiFi channel
 * @param[in] ifidx     WiFi interface (e.g., WIFI_IF_STA)
 * @param[in] encrypt   Enable encryption
 * @return esp_err_t    ESP_OK on success
 */
esp_err_t app_espnow_add_peer(const uint8_t *peer_addr);

#ifdef __cplusplus
}
#endif
