#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/uart.h"
#include "driver/gpio.h"

#include "esp_check.h"
#include "esp_log.h"

#include "CT511N.h"

#define CT511N_TAG "CT511N"
#define CT511N_AT_BUF_SIZE 2048

struct ct511n_handle_t {
	ct511n_uart_config_t config;
	SemaphoreHandle_t lock;
	char at_buf[CT511N_AT_BUF_SIZE];
	bool dtr_first_sleep;
};

static bool ct511n_response_has_error(const char *response)
{
	return (strstr(response, "ERROR") != NULL) ||
	       (strstr(response, "FAIL") != NULL);
}

static bool ct511n_response_complete_for_command(const char *command, const char *response)
{
	bool has_ok;
	bool has_success;
	bool has_cclk;

	if (ct511n_response_has_error(response)) {
		return true;
	}

	has_ok = (strstr(response, "OK") != NULL);
	has_success = (strstr(response, "SUCCESS") != NULL);
	has_cclk = (strstr(response, "+CCLK:") != NULL);

	if (strstr(command, CT511N_AT_CMD_CIPOPEN) != NULL) {
		return has_success;
	}

	if (strstr(command, CT511N_AT_CMD_CIPSEND) != NULL) {
		return has_success;
	}

	if (strstr(command, CT511N_AT_CMD_CCLK_GET) != NULL) {
		return has_cclk || has_ok;
	}

	return has_ok;
}


esp_err_t ct511n_create(ct511n_handle_t **out_handle, const ct511n_uart_config_t *config)
{
	ct511n_handle_t *handle;
	esp_err_t err;

	if ((out_handle == NULL) || (config == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	handle = (ct511n_handle_t *)calloc(1, sizeof(ct511n_handle_t));
	if (handle == NULL) {
		return ESP_ERR_NO_MEM;
	}

	handle->config = *config;
	handle->lock = xSemaphoreCreateMutex();
	if (handle->lock == NULL) {
		free(handle);
		return ESP_ERR_NO_MEM;
	}

	err = uart_driver_install(handle->config.uart_port, CT511N_AT_BUF_SIZE, 0, 0, NULL, 0);
	if (err != ESP_OK) {
		vSemaphoreDelete(handle->lock);
		free(handle);
		return err;
	}

	err = ct511n_uart_init(&handle->config);
	if (err != ESP_OK) {
		uart_driver_delete(handle->config.uart_port);
		vSemaphoreDelete(handle->lock);
		free(handle);
		return err;
	}

	/* DTR pin init */
	handle->dtr_first_sleep = true;
	if (handle->config.dtr_pin != GPIO_NUM_NC) {
		gpio_set_direction(handle->config.dtr_pin, GPIO_MODE_OUTPUT);
		gpio_set_level(handle->config.dtr_pin, 0);
	}

	*out_handle = handle;
	return ESP_OK;
}

void ct511n_destroy(ct511n_handle_t *handle)
{
	if (handle == NULL) {
		return;
	}

	uart_driver_delete(handle->config.uart_port);
	if (handle->lock != NULL) {
		vSemaphoreDelete(handle->lock);
	}
	free(handle);
}

esp_err_t ct511n_uart_init(const ct511n_uart_config_t *cfg)
{
	if (cfg == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	if (cfg->uart_config.baud_rate <= 0) {
		return ESP_ERR_INVALID_ARG;
	}
	uart_config_t uart_cfg = cfg->uart_config;
	ESP_ERROR_CHECK(uart_set_pin(cfg->uart_num, cfg->tx_pin, cfg->rx_pin, cfg->rts_pin, cfg->cts_pin));
	ESP_ERROR_CHECK(uart_param_config(cfg->uart_num, &uart_cfg));

	esp_err_t err = uart_enable_rx_intr(cfg->uart_num);
	if (err != ESP_OK) {
		return err;
	}

	return ESP_OK;
}


esp_err_t ct511n_send_at_command(ct511n_handle_t *handle,
				 const char *command,
				 char *response,
				 size_t response_size,
				 uint32_t timeout_ms,
				 bool *is_ok)
{
	int written;
	int rd;
	int total = 0;
	bool response_complete = false;
	TickType_t start_tick;
	TickType_t timeout_tick;
	TickType_t last_rx_tick;
	TickType_t tail_quiet_tick;

	if ((handle == NULL) || (command == NULL) || (response == NULL) || (response_size == 0)) {
		return ESP_ERR_INVALID_ARG;
	}

	if (xSemaphoreTake(handle->lock, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
		return ESP_ERR_TIMEOUT;
	}

	/* Flush stale URC / unsolicited data before sending a new command.
	 * Without this, left-over messages (RDY, +CEREG, +CPIN etc.) from
	 * module startup or previous operations contaminate the response. */
	uart_flush_input(handle->config.uart_port);

	written = uart_write_bytes(handle->config.uart_port, command, strlen(command));
	if (written > 0) {
		uart_write_bytes(handle->config.uart_port, "\r\n", 2);
	}

	start_tick = xTaskGetTickCount();
	timeout_tick = pdMS_TO_TICKS(timeout_ms);
	tail_quiet_tick = pdMS_TO_TICKS(50);
	if (timeout_tick == 0) {
		timeout_tick = 1;
	}
	if (tail_quiet_tick == 0) {
		tail_quiet_tick = 1;
	}
	last_rx_tick = start_tick;

	while (((xTaskGetTickCount() - start_tick) < timeout_tick) && (total < (int)(response_size - 1))) {
		size_t buffered_len = 0;
		TickType_t now_tick = xTaskGetTickCount();
		uart_get_buffered_data_len(handle->config.uart_port, &buffered_len);
		if (buffered_len == 0) {
			if (response_complete && ((now_tick - last_rx_tick) >= tail_quiet_tick)) {
				break;
			}
			vTaskDelay(pdMS_TO_TICKS(10));
			continue;
		}

		size_t remain = (response_size - 1) - (size_t)total;
		size_t to_read = (buffered_len < remain) ? buffered_len : remain;
		rd = uart_read_bytes(
			handle->config.uart_port,
			(uint8_t *)&response[total],
			to_read,
			pdMS_TO_TICKS(20));
		if (rd > 0) {
			total += rd;
			response[total] = '\0';
			last_rx_tick = xTaskGetTickCount();
			if (ct511n_response_complete_for_command(command, response)) {
				response_complete = true;
			}
		}
	}

	if (total == 0) {
		xSemaphoreGive(handle->lock);
		return ESP_ERR_TIMEOUT;
	}

	response[total] = '\0';
	if (is_ok != NULL) {
		*is_ok = ct511n_response_complete_for_command(command, response) &&
			!ct511n_response_has_error(response);
	}

	xSemaphoreGive(handle->lock);
	return ESP_OK;
}

esp_err_t ct511n_is_ok (ct511n_handle_t *handle){
	bool is_ok;
	esp_err_t err = ct511n_send_at_command(handle,
					CT511N_AT_CMD_AT,
					handle->at_buf,
					sizeof(handle->at_buf),
					CT511N_RETRY_DELAY_MID_MS,
					&is_ok);
	if (err != ESP_OK) {
		return err;
	}
	if (is_ok) {
		return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t ct511n_echo_off(ct511n_handle_t *handle){
	char response[CT511N_AT_BUF_SIZE];
	bool is_ok;
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_ATE_OFF, response, sizeof(response), CT511N_RETRY_DELAY_MID_MS, &is_ok);
	if (err != ESP_OK) {
		return err;
	}
	if(strcmp(response, "OK") == 0)
		return ESP_OK;
	return ESP_FAIL;

}

esp_err_t ct511n_4g_statue(ct511n_handle_t *handle){
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_CEREG, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	int n = 0;
	int matched = 0;
	int stat = -1;
	char *cereg;

	if (err != ESP_OK) {
		return err;
	}

	cereg = strstr(handle->at_buf, "+CEREG:");
	if (cereg == NULL) {
		return ESP_FAIL;
	}

	/* Compatible with forms like: +CEREG: <stat>  or  +CEREG: <n>,<stat> */
	matched = sscanf(cereg, "+CEREG: %d,%d", &n, &stat);
	ESP_LOGI(CT511N_TAG, "CEREG response: n=%d stat=%d matched=%d", n, stat, matched);
	if (matched != 0 && n != 0 && (stat == 1 || stat == 5)) {
		return ESP_OK;
	}

	return ESP_FAIL;
}

esp_err_t ct511n_4g_rssi(ct511n_handle_t *handle, int *rssi_dbm){
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_CSQ, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	char *csq_str = strstr(handle->at_buf, "+CSQ: ");
	if (csq_str == NULL) {
		return ESP_FAIL;
	}
	int rssi;
	if (sscanf(csq_str, "+CSQ: %d,", &rssi) != 1) {
		return ESP_FAIL;
	}
	if (rssi == 99) {
		return ESP_FAIL; // Unknown or undetectable signal
	}
	*rssi_dbm = -113 + 2 * rssi; // Convert to dBm
	return ESP_OK;
}

esp_err_t ct511n_4g_open(ct511n_handle_t *handle){

	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_NETOPEN, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if(err != ESP_OK) {
		return err;
	}
	if ((strstr(handle->at_buf, "SUCCESS")!=NULL) || 
		(strstr(handle->at_buf, "ERROR: 902"))!=NULL) {
		return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t ct511n_4g_clk_set(ct511n_handle_t *handle){
	esp_err_t err = ct511n_send_at_command(handle,
					CT511N_AT_CMD_QNTP,
					handle->at_buf,
					sizeof(handle->at_buf),
					8000,
					NULL);
	if (err != ESP_OK) {
		return err;
	}
	if ((strstr(handle->at_buf, "OK") != NULL) || (strstr(handle->at_buf, "+QNTP:") != NULL)) {
		err = ct511n_send_at_command(handle,
					CT511N_AT_CMD_CCLK_GET,
					handle->at_buf,
					sizeof(handle->at_buf),
					2000,
					NULL);
		if (err != ESP_OK) {
			return err;
		}
		if ((strstr(handle->at_buf, "OK") != NULL) && (strstr(handle->at_buf, "+CCLK:") != NULL)) {
			return ESP_OK;
		}
		return ESP_FAIL;
	}
	return ESP_FAIL;
}
	
esp_err_t ct511n_4g_clk_get(ct511n_handle_t *handle, char *clk_buf, size_t clk_buf_size){
	esp_err_t err;
	char *cclk_str;
	char *quote_begin;
	char *quote_end;
	size_t time_len;

	if ((handle == NULL) || (clk_buf == NULL) || (clk_buf_size == 0)) {
		return ESP_ERR_INVALID_ARG;
	}

	err = ct511n_send_at_command(handle, CT511N_AT_CMD_CCLK_GET, handle->at_buf, sizeof(handle->at_buf), 1000, NULL);
	ESP_LOGI(CT511N_TAG, "CCLK response: %s", handle->at_buf);
	if (err != ESP_OK) {
		return err;
	}

	cclk_str = strstr(handle->at_buf, "+CCLK: ");
	ESP_LOGI(CT511N_TAG, "str: %s", cclk_str);
	if (cclk_str == NULL) {
		return ESP_FAIL;
	}

	quote_begin = strchr(cclk_str, '"');
	if (quote_begin == NULL) {
		return ESP_FAIL;
	}
	quote_begin++;
	quote_end = strchr(quote_begin, '"');
	if (quote_end == NULL) {
		return ESP_FAIL;
	}

	time_len = (size_t)(quote_end - quote_begin);
	if ((time_len + sizeof("cclk=") + 1) > clk_buf_size) {
		return ESP_ERR_INVALID_SIZE;
	}

	/* copy time string replacing commas to avoid AT parser field-split */
	{
		size_t i;
		int w = snprintf(clk_buf, clk_buf_size, "cclk=");
		for (i = 0; i < time_len && (size_t)(w + 1) < clk_buf_size; i++) {
			char ch = quote_begin[i];
			clk_buf[w++] = (ch == ',') ? ' ' : ch;
		}
		clk_buf[w] = '\0';
	}
	return ESP_OK;
}

esp_err_t ct511n_sleep_on(ct511n_handle_t *handle){
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_SYSSLEEP_ON, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if (strstr(handle->at_buf, "OK") != NULL) {
		return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t ct511n_sleep_off(ct511n_handle_t *handle){
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_SYSSLEEP_OFF, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if (strstr(handle->at_buf, "OK") != NULL) {
		return ESP_OK;
	}
	return ESP_FAIL;
}
esp_err_t ct511n_sleep_dtr_enable(ct511n_handle_t *handle)
{
	esp_err_t err;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	if (handle->config.dtr_pin == GPIO_NUM_NC) {
		return ESP_ERR_NOT_SUPPORTED;
	}

	/* Step 1: wake from any existing sleep (AT+SYSSLEEP=0) */
	err = ct511n_sleep_on(handle);
	if (err != ESP_OK) {
		ESP_LOGE(CT511N_TAG, "sleep_dtr_enable: SYSSLEEP=0 failed");
		return err;
	}

	/* Step 2: enable DTR hardware sleep control */
	err = ct511n_send_at_command(handle, "AT+CSCLK=1", handle->at_buf,
				     sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if (strstr(handle->at_buf, "OK") == NULL) {
		ESP_LOGE(CT511N_TAG, "sleep_dtr_enable: CSCLK=1 rejected");
		return ESP_FAIL;
	}

	/* Step 3: first time needs DTR low→high toggle; afterwards just set high */
	if (handle->dtr_first_sleep) {
		gpio_set_level(handle->config.dtr_pin, 0);
		vTaskDelay(pdMS_TO_TICKS(50));
		handle->dtr_first_sleep = false;
	}
	gpio_set_level(handle->config.dtr_pin, 1);

	ESP_LOGI(CT511N_TAG, "DTR sleep enabled");
	return ESP_OK;
}

esp_err_t ct511n_sleep_dtr_disable(ct511n_handle_t *handle)
{
	esp_err_t err;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	if (handle->config.dtr_pin == GPIO_NUM_NC) {
		return ESP_ERR_NOT_SUPPORTED;
	}

	/* Step 1: pull DTR low to wake the module */
	gpio_set_level(handle->config.dtr_pin, 0);
	vTaskDelay(pdMS_TO_TICKS(50));

	/* Step 2: wake from sleep (AT+SYSSLEEP=0) */
	err = ct511n_sleep_on(handle);
	if (err != ESP_OK) {
		ESP_LOGW(CT511N_TAG, "sleep_dtr_disable: SYSSLEEP=0 failed (ignored)");
	}

	/* Step 3: disable DTR hardware sleep control */
	err = ct511n_send_at_command(handle, "AT+CSCLK=0", handle->at_buf,
				     sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if (strstr(handle->at_buf, "OK") == NULL) {
		ESP_LOGE(CT511N_TAG, "sleep_dtr_disable: CSCLK=0 rejected");
		return ESP_FAIL;
	}

	ESP_LOGI(CT511N_TAG, "DTR sleep disabled");
	return ESP_OK;
}

esp_err_t ct511n_4g_dtu_start(ct511n_handle_t *handle){
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_CIPMODE_ON, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if(strstr(handle->at_buf,"OK") != NULL){
		return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t ct511n_4g_heartpacket_set(ct511n_handle_t *handle, int heartbeat_interval_s){
	char cmd_buf[64];
	snprintf(cmd_buf, sizeof(cmd_buf), "%s=%d", CT511N_AT_CMD_MCIPCFG, heartbeat_interval_s);
	esp_err_t err = ct511n_send_at_command(handle, cmd_buf, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if(strstr(handle->at_buf,"OK") != NULL){
		return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t ct511n_4g_tcp_on(ct511n_handle_t *handle, char* ip_buf, char* port_buf){
	char cmd_buf[128];
	snprintf(cmd_buf, sizeof(cmd_buf), "%s=1,\"TCP\",\"%s\",%s", CT511N_AT_CMD_CIPOPEN, ip_buf, port_buf);
	esp_err_t err = ct511n_send_at_command(handle, cmd_buf, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_LONG_MS, NULL);
	if(err != ESP_OK)
		return ESP_FAIL;
	if (strstr(handle->at_buf, "+CIPOPEN: SUCCESS") != NULL ||
	    strstr(handle->at_buf, "+CIPOPEN:SUCCESS") != NULL) {
		return ESP_OK;
	}
	ESP_LOGI(CT511N_TAG, "tcpOn response: %s", handle->at_buf);

	return ESP_FAIL;
}

esp_err_t ct511n_4g_tcp_send(ct511n_handle_t *handle, const char *data){
	char cmd_buf[640];
	size_t data_len;
	esp_err_t err;

	if ((handle == NULL) || (data == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	data_len = strlen(data);
	if (data_len > 512) {
		return ESP_ERR_INVALID_SIZE;
	}

	if (strchr(data, '"') != NULL || strchr(data, ',') != NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	snprintf(cmd_buf, sizeof(cmd_buf), "%s=1,,,,%s", CT511N_AT_CMD_CIPSEND, data);
	err = ct511n_send_at_command(handle, cmd_buf, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_LONG_MS, NULL);
	if (err != ESP_OK) {
		ESP_LOGI(CT511N_TAG, "cipsend failed: %s", handle->at_buf);
		return err;
	}

	if ((strstr(handle->at_buf, "+CIPSEND: SUCCESS") != NULL) ||
	    (strstr(handle->at_buf, "+CIPSEND:SUCCESS") != NULL) ||
	    (strstr(handle->at_buf, "SEND OK") != NULL)) {
		return ESP_OK;
	}

	ESP_LOGE(CT511N_TAG, "cipsend rsp not success: %s", handle->at_buf);

	return ESP_FAIL;
}

static esp_err_t ct511n_qicsgp_set(ct511n_handle_t *handle)
{
	esp_err_t err;

	err = ct511n_send_at_command(handle,
				    "AT+QICSGP=1,1,\"\",\"\",\"\"",
				    handle->at_buf,
				    sizeof(handle->at_buf),
				    CT511N_RETRY_DELAY_MID_MS,
				    NULL);
	if (err != ESP_OK) {
		return err;
	}
	if (strstr(handle->at_buf, "OK") == NULL) {
		return ESP_FAIL;
	}

	return ESP_OK;
}

static const char *s_tcp_target_ip;
static const char *s_tcp_target_port;

static esp_err_t ct511n_4g_tcp_on_retry(ct511n_handle_t *handle)
{
	esp_err_t err;

	if ((s_tcp_target_ip == NULL) || (s_tcp_target_port == NULL)) {
		return ESP_ERR_INVALID_STATE;
	}

	/* close any lingering link before trying to connect */
	(void)ct511n_4g_tcp_stop(handle);
	vTaskDelay(pdMS_TO_TICKS(CT511N_RETRY_DELAY_SHORT_MS));

	err = ct511n_4g_tcp_on(handle, (char *)s_tcp_target_ip, (char *)s_tcp_target_port);
	if (err != ESP_OK) {
		ESP_LOGI(CT511N_TAG, "tcpOn response: %s", handle->at_buf);
	}
	return err;
}

esp_err_t ct511n_tcp_single_connect(ct511n_handle_t *handle, const char *ip, const char *port)
{
	esp_err_t err;

	if ((handle == NULL) || (ip == NULL) || (port == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	ESP_LOGI(CT511N_TAG, "tcp_single_connect start ip=%s port=%s", ip, port);

	err = reset(ct511n_is_ok, handle, "AT handshake ok", 30, CT511N_RETRY_DELAY_SHORT_MS);
	if (err != ESP_OK) {
		ESP_LOGW(CT511N_TAG, "AT handshake failed, try modem reset and retry handshake");
		(void)ct511n_reset(handle);
		vTaskDelay(pdMS_TO_TICKS(CT511N_RETRY_DELAY_LONG_MS));
		err = reset(ct511n_is_ok, handle, "AT handshake ok(after reset)", 30, CT511N_RETRY_DELAY_SHORT_MS);
	}
	if (err != ESP_OK) {
		return err;
	}

	// err = reset(ct511n_echo_off, handle, "echo off ok", 30, CT511N_RETRY_DELAY_SHORT_MS);
	// if (err != ESP_OK) {
	// 	return err;
	// }

	err = reset(ct511n_4g_statue, handle, "4G register ok", 30, CT511N_RETRY_DELAY_SHORT_MS);
	if (err != ESP_OK) {
		return err;
	}

	err = reset(ct511n_qicsgp_set, handle, "QICSGP ok", 30, CT511N_RETRY_DELAY_SHORT_MS);
	if (err != ESP_OK) {
		return ESP_FAIL;
	}

	err = reset(ct511n_4g_open, handle, "netOpen ok", 30, CT511N_RETRY_DELAY_SHORT_MS);
	if (err != ESP_OK) {
		return err;
	}

	err = reset(ct511n_4g_clk_set, handle, "CCLK set ok", 30, CT511N_RETRY_DELAY_SHORT_MS);
	if(err != ESP_OK) {
		ESP_LOGE(CT511N_TAG, "Failed to set clock before TCP connect");
		return err;
	}
	
	s_tcp_target_ip = ip;
	s_tcp_target_port = port;
	err = reset(ct511n_4g_tcp_on_retry, handle, "CIPOPEN ok", 10, CT511N_RETRY_DELAY_MID_MS);
	if (err != ESP_OK) {
		s_tcp_target_ip = NULL;
		s_tcp_target_port = NULL;
		return err;
	}
	s_tcp_target_ip = NULL;
	s_tcp_target_port = NULL;

	err = reset(ct511n_4g_clk_set, handle, "CCLK set ok", 30, CT511N_RETRY_DELAY_SHORT_MS);
	if(err != ESP_OK) {
		ESP_LOGE(CT511N_TAG, "Failed to set clock after TCP connect");
		return err;
	}

	return ESP_OK;
}

esp_err_t ct511n_4g_build_payload(ct511n_handle_t *handle, char *payload, size_t payload_size)
{
	esp_err_t err;
	int rssi_dbm = 0;
	char clk_str[64];
	char gps_str[256];

	if ((handle == NULL) || (payload == NULL) || (payload_size == 0)) {
		return ESP_ERR_INVALID_ARG;
	}

	err = ct511n_4g_rssi(handle, &rssi_dbm);
	if (err != ESP_OK) {
		rssi_dbm = -1;
	}

	err = ct511n_4g_clk_get(handle, clk_str, sizeof(clk_str));
	if(err != ESP_OK) {
		ESP_LOGE(CT511N_TAG, "Failed to get clock for payload: %s", handle->at_buf);
		return err;
	}

	err = ct511n_gps_get(handle, gps_str, sizeof(gps_str));
	if(err != ESP_OK) {
		ESP_LOGE(CT511N_TAG, "Failed to get gps for payload: %s", handle->at_buf);
		return err;
	}

	payload[0] = '\0';
	
	snprintf(payload,
		 payload_size,
		 "rssi_dbm=%d cereg=%.220s gps=%.220s",
		 rssi_dbm,
		 clk_str,
		 gps_str);
	
	return ESP_OK;
}

esp_err_t ct511n_4g_dtu_on(ct511n_handle_t *handle){
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_ATO, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if(strstr(handle->at_buf,"OK") != NULL){
		return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t ct511n_4g_dtu_off(ct511n_handle_t *handle){
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_TRANSPARENT_EXIT, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	return ESP_OK;
}

esp_err_t ct511n_4g_tcp_stop(ct511n_handle_t *handle){
	char cmd_buf[64];
	snprintf(cmd_buf, sizeof(cmd_buf), "%s=1", CT511N_AT_CMD_CIPCLOSE);
	esp_err_t err = ct511n_send_at_command(handle, cmd_buf, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if (strstr(handle->at_buf, "OK") != NULL && strstr(handle->at_buf, "CLOSED") != NULL) {
		return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t ct511n_reset(ct511n_handle_t *handle){
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_RESET, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if (strstr(handle->at_buf, "OK") != NULL) {
		return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t ct511n_gps_on(ct511n_handle_t *handle){
	esp_err_t err;
	TickType_t start;
	TickType_t deadline;

	err = ct511n_send_at_command(handle, CT511N_AT_CMD_MGPSC, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}

	/* Check if $HOSTSLEEP URC came together with OK */
	if (strstr(handle->at_buf, "$HOSTSLEEP") != NULL) {
		return ESP_OK;
	}

	if (strstr(handle->at_buf, "OK") == NULL) {
		ESP_LOGI(CT511N_TAG, "GPS on response: %s", handle->at_buf);
		return ESP_FAIL;
	}

	/* OK seen but $HOSTSLEEP not yet - poll UART with bounded timeout */
	start = xTaskGetTickCount();
	deadline = pdMS_TO_TICKS(3000);

	do {
		size_t buffered_len;
		TickType_t now = xTaskGetTickCount();

		uart_get_buffered_data_len(handle->config.uart_port, &buffered_len);
		if (buffered_len > 0) {
			size_t remain = sizeof(handle->at_buf) - 1;
			size_t to_read = (buffered_len < remain) ? buffered_len : remain;
			int rd = uart_read_bytes(
				handle->config.uart_port,
				(uint8_t *)handle->at_buf,
				to_read,
				pdMS_TO_TICKS(20));
			if (rd > 0) {
				handle->at_buf[rd] = '\0';
				if (strstr(handle->at_buf, "$HOSTSLEEP") != NULL) {
					ESP_LOGI(CT511N_TAG, "GPS on: $HOSTSLEEP received after %d ms",
						(int)((now - start) * portTICK_PERIOD_MS));
					return ESP_OK;
				}
			}
		}
		vTaskDelay(pdMS_TO_TICKS(50));
	} while ((xTaskGetTickCount() - start) < deadline);

	ESP_LOGE(CT511N_TAG, "GPS on: $HOSTSLEEP timeout");
	return ESP_FAIL;
}

esp_err_t ct511n_gps_warm(ct511n_handle_t *handle){
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_GPSMODE_WARM, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if (strstr(handle->at_buf, "OK") != NULL) {
		return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t ct511n_gps_echo_off(ct511n_handle_t *handle){
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_MGPSGET, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if (strstr(handle->at_buf, "OK") != NULL) {
		return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t ct511n_gps_get(ct511n_handle_t *handle, char *gps_buf, size_t gps_buf_size){
	esp_err_t err;
	char *gps_start;
	char *gps_end;
	size_t len;

	if ((handle == NULL) || (gps_buf == NULL) || (gps_buf_size == 0)) {
		return ESP_ERR_INVALID_ARG;
	}

	err = ct511n_send_at_command(handle, CT511N_AT_CMD_GPSST, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}

	gps_start = strstr(handle->at_buf, "+GPSST: ");
	if (gps_start == NULL) {
		ESP_LOGE(CT511N_TAG, "Failed to parse GPSST response: %s", handle->at_buf);
		return ESP_FAIL;
	}
	gps_start += strlen("+GPSST: ");

	gps_end = strchr(gps_start, ';');
	if (gps_end == NULL) {
		ESP_LOGE(CT511N_TAG, "Failed to parse GPSST response: %s", handle->at_buf);
		return ESP_FAIL;
	}

	len = (size_t)(gps_end - gps_start);
	if (len >= gps_buf_size) {
		len = gps_buf_size - 1;
	}
	/* copy replacing commas to avoid AT parser field-split */
	{
		size_t i;
		for (i = 0; i < len; i++) {
			char ch = gps_start[i];
			gps_buf[i] = (ch == ',') ? ' ' : ch;
		}
		gps_buf[len] = '\0';
	}

	return ESP_OK;
}

esp_err_t ct511n_agnss_get(ct511n_handle_t *handle){
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_AGNSSSET, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if (strstr(handle->at_buf, "OK") != NULL) {
		return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t ct511n_agnss_set(ct511n_handle_t *handle){
	esp_err_t err = ct511n_send_at_command(handle, CT511N_AT_CMD_AGNSSGET_SET, handle->at_buf, sizeof(handle->at_buf), CT511N_RETRY_DELAY_MID_MS, NULL);
	if (err != ESP_OK) {
		return err;
	}
	if (strstr(handle->at_buf, "OK") != NULL) {
		return ESP_OK;
	}
	return ESP_FAIL;
}

esp_err_t ct511n_gps_init(ct511n_handle_t *handle){
	esp_err_t err;

	err = reset(ct511n_gps_on, handle, "GPS on ok", 30, CT511N_RETRY_DELAY_MID_MS);
	if (err != ESP_OK) {
		return err;
	}

	err = reset(ct511n_gps_echo_off, handle, "GPS echo off ok", 30, CT511N_RETRY_DELAY_MID_MS);
	if (err != ESP_OK) {
		return err;
	}

	err = reset(ct511n_gps_warm, handle, "GPS warm ok", 30, CT511N_RETRY_DELAY_MID_MS);
	if (err != ESP_OK) {
		ESP_LOGE(CT511N_TAG, "Failed to warm GPS");
	}

	err = reset(ct511n_agnss_set, handle, "AGNSS set ok", 30, CT511N_RETRY_DELAY_MID_MS);
	if (err != ESP_OK) {
		ESP_LOGE(CT511N_TAG, "Failed to set AGNSS");
	}

	err = reset(ct511n_agnss_get, handle, "AGNSS get ok", 30, CT511N_RETRY_DELAY_MID_MS);
	if (err != ESP_OK) {
		ESP_LOGE(CT511N_TAG, "Failed to get AGNSS");
	}



	return ESP_OK;
}

esp_err_t reset(esp_err_t (*func)(ct511n_handle_t *),
		ct511n_handle_t *handle,
		const char *success_log,
		int retry_count,
		uint32_t retry_delay_ms){
	int reg_retry;
	esp_err_t err;
	const char *fail_msg;

	if ((func == NULL) || (handle == NULL) || (success_log == NULL) || (retry_count <= 0)) {
		return ESP_ERR_INVALID_ARG;
	}

	err = ESP_FAIL;
	for (reg_retry = 0; reg_retry < retry_count; reg_retry++) {
		err = func(handle);
		if (err == ESP_OK) {
			ESP_LOGI(CT511N_TAG, "%s after %d retry", success_log, reg_retry + 1);
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(retry_delay_ms));
	}

	if (err != ESP_OK) {
		fail_msg = handle->at_buf;
		ESP_LOGE(CT511N_TAG, "%s", fail_msg);
		return err;
	}
	return ESP_OK;
}
