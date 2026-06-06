#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Callback invoked after user submits WiFi credentials via web page.
 *  @param ssid      WiFi SSID (or NULL if skipped)
 *  @param password  WiFi password
 *  @param server_ip target TCP server IP
 *  @param server_port target TCP server port
 */
typedef void (*wifi_cfg_on_configured_t)(const char *ssid,
					 const char *password,
					 const char *server_ip,
					 uint16_t server_port);

/**
 * @brief  Start WiFi AP + HTTP config server (full AP+STA mode).
 */
esp_err_t wifi_cfg_start(wifi_cfg_on_configured_t on_configured);

/**
 * @brief  Start WiFi STA-only (no AP). Tries to connect to saved network.
 *         Call wifi_cfg_stop() to turn off.
 * @return ESP_OK if STA connected, ESP_FAIL if failed.
 */
esp_err_t wifi_cfg_start_sta_only(void);

/**
 * @brief  Start WiFi AP-only (no STA). HTTP config server on :80.
 *         Call wifi_cfg_stop() to turn off.
 */
esp_err_t wifi_cfg_start_ap_only(wifi_cfg_on_configured_t on_configured);

/**
 * @brief  Pre-allocate DMA memory for WiFi BEFORE BLE fragments the heap.
 */
void wifi_cfg_reserve_dma(void);

/** Stop WiFi (AP + STA) and HTTP server. */
void wifi_cfg_stop(void);

/** Full deinit — calls esp_wifi_deinit() + esp_netif_deinit() to release all memory. */
void wifi_cfg_deinit(void);

/** Returns true once configuration has been submitted. */
bool wifi_cfg_is_done(void);

/** Returns true if WiFi STA is connected and has IP. */
bool wifi_cfg_is_sta_connected(void);

/** Returns true if any station is connected to the SoftAP. */
bool wifi_cfg_is_ap_sta_connected(void);

/** Get the configured server IP. */
const char *wifi_cfg_get_server_ip(void);

/** Get the configured server port. */
uint16_t wifi_cfg_get_server_port(void);

/**
 * @brief  Send payload via WiFi TCP connection.
 *
 * Opens a socket to the configured server, sends @p data, and closes.
 * Returns bytes sent on success, -1 on failure.
 */
int wifi_cfg_tcp_send(const char *data);

#ifdef __cplusplus
}
#endif
