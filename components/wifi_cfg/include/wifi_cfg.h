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
 * @brief  Start WiFi AP + HTTP config server.
 *
 * Creates a SoftAP "4G-Tracker" and an HTTP server on port 80.
 * When the user submits the config form, @p on_configured is called.
 *
 * @param on_configured  callback (may be NULL)
 * @return ESP_OK on success
 */
esp_err_t wifi_cfg_start(wifi_cfg_on_configured_t on_configured);

/** Stop the AP and HTTP server. */
void wifi_cfg_stop(void);

/** Returns true once configuration has been submitted. */
bool wifi_cfg_is_done(void);

/** Returns true if WiFi STA is connected and has IP. */
bool wifi_cfg_is_sta_connected(void);

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
