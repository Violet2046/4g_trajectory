#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CT511N_RAT_MAX_LEN 16
#define CT511N_CELLINFO_MAX_NEIGHBORS 12

/* Retry delay presets (ms) */
#define CT511N_RETRY_DELAY_SHORT_MS 200
#define CT511N_RETRY_DELAY_MID_MS 500
#define CT511N_RETRY_DELAY_LONG_MS 1000

/* Basic commands */
#define CT511N_AT_CMD_AT "AT"
#define CT511N_AT_CMD_ATE_OFF "ATE0"
#define CT511N_AT_CMD_ATI "ATI"
#define CT511N_AT_CMD_IPR "AT+IPR"
#define CT511N_AT_CMD_ICCID "AT+ICCID"
#define CT511N_AT_CMD_RESET "AT+RESET"

/* Network service commands */
#define CT511N_AT_CMD_CEREG "AT+CEREG?"
#define CT511N_AT_CMD_CGREG_START "AT+CGREG=0"
#define CT511N_AT_CMD_CSQ "AT+CSQ"
#define CT511N_AT_CMD_QICSGP "AT+QICSGP"
#define CT511N_AT_CMD_NETOPEN "AT+NETOPEN"
#define CT511N_AT_CMD_NETCLOSE "AT+NETCLOSE"
#define CT511N_AT_CMD_QNTP "AT+QNTP=1,\"tms.dynamicode.com.cn\",123,1"
#define CT511N_AT_CMD_CCLK_GET "AT+CCLK?"
#define CT511N_AT_CMD_MDNSGIP "AT+MDNSGIP"
#define CT511N_AT_CMD_MPING "AT+MPING"

/* Power commands */
#define CT511N_AT_CMD_SYSSLEEP_ON "AT+SYSSLEEP=0"
#define CT511N_AT_CMD_SYSSLEEP_OFF "AT+SYSSLEEP=1"
#define CT511N_AT_CMD_CSCLK "AT+CSCLK"

/* TCP/UDP commands */
#define CT511N_AT_CMD_CIPMODE_ON "AT+CIPMODE=1"
#define CT511N_AT_CMD_CIPMODE_OFF "AT+CIPMODE=0"
#define CT511N_AT_CMD_MCIPCFG "AT+MCIPCFG"
#define CT511N_AT_CMD_CIPOPEN "AT+CIPOPEN"
#define CT511N_AT_CMD_CIPSEND "AT+CIPSEND"
#define CT511N_AT_CMD_ATO "ATO"
#define CT511N_AT_CMD_TRANSPARENT_EXIT "+++"
#define CT511N_AT_CMD_CIPCLOSE "AT+CIPCLOSE"

/* MQTT commands */
#define CT511N_AT_CMD_MCONFIG "AT+MCONFIG"
#define CT511N_AT_CMD_MIPSTART "AT+MIPSTART"
#define CT511N_AT_CMD_MCONNECT "AT+MCONNECT"
#define CT511N_AT_CMD_MPUB "AT+MPUB"
#define CT511N_AT_CMD_MPUBEX "AT+MPUBEX"
#define CT511N_AT_CMD_MSUB "AT+MSUB"
#define CT511N_AT_CMD_MUNSUB "AT+MUNSUB"
#define CT511N_AT_CMD_MQTTSTATU "AT+MQTTSTATU"
#define CT511N_AT_CMD_MDISCONNECT "AT+MDISCONNECT"
#define CT511N_AT_CMD_MIPCLOSE "AT+MIPCLOSE"

/* HTTP commands */
#define CT511N_AT_CMD_HTTPOPEN "AT$HTTPOPEN"
#define CT511N_AT_CMD_HTTPCLOSE "AT$HTTPCLOSE"
#define CT511N_AT_CMD_HTTPPARA "AT$HTTPPARA"
#define CT511N_AT_CMD_HTTPACTION "AT$HTTPACTION"
#define CT511N_AT_CMD_HTTPRQH "AT$HTTPRQH"
#define CT511N_AT_CMD_HTTPDATAEX "AT$HTTPDATAEX"
#define CT511N_AT_CMD_HTTPDATA "AT$HTTPDATA"
#define CT511N_AT_CMD_HTTPSEND "AT$HTTPSEND"

/* GPS commands (DX-CT511N only) */
#define CT511N_AT_CMD_MGPSC "AT+MGPSC=1"
#define CT511N_AT_CMD_GPSMODE_HOT "AT+GPSMODE=1"
#define CT511N_AT_CMD_GPSMODE_WARM "AT+GPSMODE=2"
#define CT511N_AT_CMD_GPSMODE_COLD "AT+GPSMODE=3"
#define CT511N_AT_CMD_MGPSGET "AT+MGPSGET=ALL,0"
#define CT511N_AT_CMD_GPSST "AT+GPSST"
#define CT511N_AT_CMD_AGNSSGET_SET "AT+AGNSSGET=pos.asrmicro.com"
#define CT511N_AT_CMD_AGNSSSET "AT+AGNSSSET"


typedef struct ct511n_handle_t ct511n_handle_t;

typedef struct {
	uart_port_t uart_port;
	uart_config_t uart_config;
	uart_port_t uart_num;
	int tx_pin;
	int rx_pin;
	int rts_pin;
	int cts_pin;
	int dtr_pin;  /* GPIO connected to module DTR, GPIO_NUM_NC to disable */
} ct511n_uart_config_t;

typedef struct {
	int earfcn;
	int cell_id;
	int rsrp_dbm;
} ct511n_serving_cell_t;

typedef struct {
	int earfcn;
	int cell_id;
	int pci;
	int rsrp_dbm;
} ct511n_neighbor_cell_t;


esp_err_t reset(esp_err_t (*func)(ct511n_handle_t *),
		ct511n_handle_t *handle,
		const char *success_log,
		int retry_count,
		uint32_t retry_delay_ms);

typedef struct {
	int reg_status;
	char rat[CT511N_RAT_MAX_LEN];
	ct511n_serving_cell_t serving;
	ct511n_neighbor_cell_t neighbors[CT511N_CELLINFO_MAX_NEIGHBORS];
	size_t neighbor_count;
} ct511n_cellinfo_t;

esp_err_t ct511n_uart_init(const ct511n_uart_config_t *config);

esp_err_t ct511n_send_at_command(ct511n_handle_t *handle,
				 const char *command,
				 char *response,
				 size_t response_size,
				 uint32_t timeout_ms,
				 bool *is_ok);

esp_err_t ct511n_create(ct511n_handle_t **out_handle, const ct511n_uart_config_t *config);
void ct511n_destroy(ct511n_handle_t *handle);

esp_err_t ct511n_is_ok(ct511n_handle_t *handle);
esp_err_t ct511n_echo_off(ct511n_handle_t *handle);

esp_err_t ct511n_4g_statue(ct511n_handle_t *handle);
esp_err_t ct511n_4g_rssi(ct511n_handle_t *handle, int *rssi_dbm);
esp_err_t ct511n_4g_open(ct511n_handle_t *handle);
esp_err_t ct511n_4g_clk_set(ct511n_handle_t *handle);
esp_err_t ct511n_4g_clk_get(ct511n_handle_t *handle, char *clk_buf, size_t clk_buf_size);

esp_err_t ct511n_sleep_on(ct511n_handle_t *handle);
esp_err_t ct511n_sleep_off(ct511n_handle_t *handle);
esp_err_t ct511n_sleep_dtr_enable(ct511n_handle_t *handle);
esp_err_t ct511n_sleep_dtr_disable(ct511n_handle_t *handle);


esp_err_t ct511n_4g_dtu_start(ct511n_handle_t *handle);
esp_err_t ct511n_4g_heartpacket_set(ct511n_handle_t *handle, int heartbeat_interval_s);
esp_err_t ct511n_4g_tcp_on(ct511n_handle_t *handle, char *ip_buf, char *port_buf);
esp_err_t ct511n_tcp_single_connect(ct511n_handle_t *handle, const char *ip, const char *port);
esp_err_t ct511n_4g_tcp_send(ct511n_handle_t *handle, const char *data);
esp_err_t ct511n_4g_build_payload(ct511n_handle_t *handle, char *payload, size_t payload_size);

esp_err_t ct511n_4g_dtu_on(ct511n_handle_t *handle);
esp_err_t ct511n_4g_dtu_off(ct511n_handle_t *handle);
esp_err_t ct511n_4g_tcp_stop(ct511n_handle_t *handle);
esp_err_t ct511n_4g_net_close(ct511n_handle_t *handle);

esp_err_t ct511n_reset(ct511n_handle_t *handle);
esp_err_t ct511n_gps_on(ct511n_handle_t *handle);
esp_err_t ct511n_gps_off(ct511n_handle_t *handle);
esp_err_t ct511n_gps_get(ct511n_handle_t *handle, char *gps_buf, size_t gps_buf_size);
esp_err_t ct511n_agnss_get(ct511n_handle_t *handle);
esp_err_t ct511n_agnss_set(ct511n_handle_t *handle);
esp_err_t ct511n_gps_init(ct511n_handle_t *handle);

esp_err_t ct511n_4g_init(ct511n_handle_t *handle);

#ifdef __cplusplus
}
#endif
