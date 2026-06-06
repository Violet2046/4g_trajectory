#include "wifi_cfg.h"
#include "config.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_private/wifi_os_adapter.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_sntp.h"

#include "storage_mgr.h"

#define TAG "wifi_cfg"

/* ---- Single source of truth: SPI flash storage_config ---- */
static bool                  g_configured      = false;
static wifi_cfg_on_configured_t g_on_configured = NULL;

/* Helpers — always read/write the canonical copy in SPI flash */
static void server_save(const char *ip, uint16_t port)
{
	storage_config_t cfg;
	storage_config_read(&cfg);
	strncpy(cfg.server_ip, ip, sizeof(cfg.server_ip)-1);
	snprintf(cfg.server_port, sizeof(cfg.server_port), "%u", port);
	esp_err_t err = storage_config_write(&cfg);

	/* Read-back verify */
	storage_config_t verify;
	if (storage_config_read(&verify) == ESP_OK) {
		ESP_LOGI(TAG, "server_save: %s:%u → flash (write=%d, verify=%s:%s)",
			 ip, port, err, verify.server_ip, verify.server_port);
	} else {
		ESP_LOGI(TAG, "server_save: %s:%u → flash (write=%d, verify=failed)",
			 ip, port, err);
	}
}

static const char *server_get_ip(void)
{
	static storage_config_t cfg;
	if (storage_config_read(&cfg) != ESP_OK)
		storage_config_default(&cfg);
	return cfg.server_ip;
}

static uint16_t server_get_port(void)
{
	static storage_config_t cfg;
	if (storage_config_read(&cfg) != ESP_OK)
		storage_config_default(&cfg);
	return (uint16_t)atoi(cfg.server_port);
}

/* ---- WiFi STA connection state ---- */
static volatile bool g_sta_connected = false;
static volatile bool g_sta_got_ip    = false;
static volatile bool g_ap_sta_connected = false; /* at least one station on AP */
static SemaphoreHandle_t g_sta_sem   = NULL;

/* ---- HTTP handlers ---- */

static const char *HTML_FORM =
"<!DOCTYPE html><html><head><meta charset='utf-8'>"
"<title>4G Tracker 配网</title>"
"<meta name='viewport' content='width=device-width,initial-scale=1'>"
"</head><body style='font-family:sans-serif;padding:20px;max-width:400px;margin:auto'>"
"<h2>4G Tracker 配网</h2>"
"<form method='POST' action='/save'>"
"<label>WiFi SSID:</label><br>"
"<input name='ssid' placeholder='WiFi名称' style='width:100%%;padding:8px;margin:4px 0'><br>"
"<label>WiFi 密码:</label><br>"
"<input name='pass' type='password' placeholder='WiFi密码' style='width:100%%;padding:8px;margin:4px 0'><br>"
"<label>服务器 IP:</label><br>"
"<input name='ip' value='%s' style='width:100%%;padding:8px;margin:4px 0'><br>"
"<label>服务器端口:</label><br>"
"<input name='port' value='%u' type='number' style='width:100%%;padding:8px;margin:4px 0'><br><br>"
"<button type='submit' style='padding:10px 30px;font-size:16px'>保存并测试连接</button>"
"</form></body></html>";

static esp_err_t root_get_handler(httpd_req_t *req)
{
	char buf[2048];
	snprintf(buf, sizeof(buf), HTML_FORM, server_get_ip(), server_get_port());
	httpd_resp_set_type(req, "text/html; charset=utf-8");
	httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

/* ---- Connectivity test (TCP connect) ---- */

static bool test_connectivity(const char *host, uint16_t port, int timeout_ms)
{
	char port_str[8];
	snprintf(port_str, sizeof(port_str), "%u", port);

	struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM };
	struct addrinfo *res = NULL;

	int ret = getaddrinfo(host, port_str, &hints, &res);
	if (ret != 0 || res == NULL) {
		ESP_LOGW(TAG, "DNS resolve failed for %s", host);
		return false;
	}

	int sock = socket(res->ai_family, res->ai_socktype, 0);
	if (sock < 0) { freeaddrinfo(res); return false; }

	struct timeval tv = { .tv_sec = timeout_ms / 1000,
			      .tv_usec = (timeout_ms % 1000) * 1000 };
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	bool ok = (connect(sock, res->ai_addr, res->ai_addrlen) == 0);
	close(sock);
	freeaddrinfo(res);
	ESP_LOGI(TAG, "connectivity to %s:%u → %s", host, port, ok ? "OK" : "FAIL");
	return ok;
}

/* ---- WiFi STA connect ---- */

static bool wifi_sta_connect(const char *ssid, const char *password)
{
	if (ssid == NULL || ssid[0] == '\0') return false;

	g_sta_connected = false;
	g_sta_got_ip = false;

	esp_netif_create_default_wifi_sta();

	wifi_config_t sta_cfg = { .sta = { .threshold.authmode = WIFI_AUTH_WPA2_PSK } };
	strncpy((char *)sta_cfg.sta.ssid, ssid, sizeof(sta_cfg.sta.ssid) - 1);
	if (password && password[0]) {
		strncpy((char *)sta_cfg.sta.password, password,
			sizeof(sta_cfg.sta.password) - 1);
	} else {
		sta_cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
	}

	esp_wifi_set_mode(WIFI_MODE_APSTA);
	esp_wifi_set_config(WIFI_IF_STA, &sta_cfg);
	esp_wifi_connect();

	ESP_LOGI(TAG, "WiFi STA connecting to %s…", ssid);

	if (xSemaphoreTake(g_sta_sem,
			   pdMS_TO_TICKS(CFG_WIFI_STA_CONNECT_TIMEOUT_MS)) != pdTRUE) {
		ESP_LOGW(TAG, "WiFi STA connect timeout");
		esp_wifi_disconnect();
		return false;
	}

	bool result = g_sta_connected && g_sta_got_ip;
	ESP_LOGI(TAG, "WiFi STA connect: %s", result ? "OK" : "FAIL");
	return result;
}

/* ---- Simple URL-decode: + → space, %%XX → char ---- */
static void url_decode(char *dst, const char *src, size_t dst_sz)
{
	char *d = dst;
	const char *s = src;
	while (*s && (size_t)(d - dst) < dst_sz - 1) {
		if (*s == '+') {
			*d++ = ' '; s++;
		} else if (*s == '%' && s[1] && s[2]) {
			unsigned int c;
			if (sscanf(s + 1, "%2x", &c) == 1) {
				*d++ = (char)c; s += 3;
			} else {
				*d++ = *s++;
			}
		} else {
			*d++ = *s++;
		}
	}
	*d = '\0';
}

/* ---- Save POST handler ---- */

static esp_err_t save_post_handler(httpd_req_t *req)
{
	char buf[512] = {0};
	int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
	if (len <= 0) { httpd_resp_send_500(req); return ESP_FAIL; }
	buf[len] = '\0';

	char ssid[64] = {0}, pass[64] = {0}, ip[64] = {0}, port_str[16] = {0};
	char *token = strtok(buf, "&");
	while (token) {
		char key[32] = {0}, val[128] = {0};
		if (sscanf(token, "%31[^=]=%127s", key, val) >= 1) {
			char decoded[128];
			url_decode(decoded, val, sizeof(decoded));
			if (strcmp(key, "ssid") == 0) strncpy(ssid, decoded, sizeof(ssid)-1);
			if (strcmp(key, "pass") == 0) strncpy(pass, decoded, sizeof(pass)-1);
			if (strcmp(key, "ip")   == 0) strncpy(ip,   decoded, sizeof(ip)  -1);
			if (strcmp(key, "port") == 0) strncpy(port_str, decoded, sizeof(port_str)-1);
		}
		token = strtok(NULL, "&");
	}

	const char *ip_target = ip[0] ? ip : server_get_ip();
	uint16_t port_target = port_str[0] ? (uint16_t)atoi(port_str) : server_get_port();

	/* Step 1: WiFi STA */
	bool wifi_ok = false, ping_ok = false;
	if (ssid[0]) {
		wifi_ok = wifi_sta_connect(ssid, pass);
		if (wifi_ok) {
			vTaskDelay(pdMS_TO_TICKS(500));
			ping_ok = test_connectivity(CFG_PING_TARGET, CFG_PING_PORT, 5000);
		}
	}

	/* Step 2: Save to SPI Flash (single source of truth) */
	server_save(ip_target, port_target);
	{
	storage_config_t cfg;
	storage_config_read(&cfg);
	strncpy(cfg.wifi_ssid,     ssid, sizeof(cfg.wifi_ssid)-1);
	strncpy(cfg.wifi_password, pass, sizeof(cfg.wifi_password)-1);
	esp_err_t wr = storage_config_write(&cfg);
	if (wr != ESP_OK) {
		ESP_LOGW(TAG, "WiFi config write failed: %s", esp_err_to_name(wr));
	}
	}
	g_configured = true;

	/* Step 3: Notify */
	if (g_on_configured)
		g_on_configured(ssid[0]?ssid:NULL, pass[0]?pass:NULL,
				ip_target, port_target);

	/* Step 4: Result page */
	char resp[1024];
	snprintf(resp, sizeof(resp),
		"<!DOCTYPE html><html><head><meta charset='utf-8'>"
		"<meta name='viewport' content='width=device-width,initial-scale=1'>"
		"</head><body style='font-family:sans-serif;padding:20px;max-width:400px;margin:auto'>"
		"<h2>配网结果</h2>"
		"<p>WiFi: %s → <b>%s</b></p>"
		"<p>连通性 (%s:%u): <b>%s</b></p>"
		"<p>服务器: %s:%u</p>"
		"<p>配置已存入 Flash ✅</p>"
		"<hr><a href='/'>返回</a></body></html>",
		ssid[0]?ssid:"(未设置)",
		wifi_ok?"✅ 连接成功":"❌ 失败",
		CFG_PING_TARGET, (unsigned)CFG_PING_PORT,
		ping_ok?"✅ 可达":"❌ 不可达",
		ip_target, (unsigned)port_target);

	httpd_resp_set_type(req, "text/html; charset=utf-8");
	httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
	return ESP_OK;
}

static httpd_handle_t g_server = NULL;

static void start_http_server(void)
{
	httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
	cfg.max_uri_handlers = 4;
	httpd_start(&g_server, &cfg);

	httpd_uri_t root = { .uri = "/", .method = HTTP_GET, .handler = root_get_handler };
	httpd_register_uri_handler(g_server, &root);

	httpd_uri_t save = { .uri = "/save", .method = HTTP_POST, .handler = save_post_handler };
	httpd_register_uri_handler(g_server, &save);

	/* Redirect GET /save → / */
	httpd_uri_t save_get = { .uri = "/save", .method = HTTP_GET, .handler = root_get_handler };
	httpd_register_uri_handler(g_server, &save_get);
}

/* ---- WiFi event handler ---- */

static void wifi_event_handler(void *arg, esp_event_base_t base,
			       int32_t id, void *data)
{
	if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STACONNECTED) {
		wifi_event_ap_staconnected_t *evt = data;
		g_ap_sta_connected = true;
		ESP_LOGI(TAG, "AP station " MACSTR " connected", MAC2STR(evt->mac));
	} else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STADISCONNECTED) {
		wifi_event_ap_stadisconnected_t *evt = data;
		g_ap_sta_connected = false;
		ESP_LOGI(TAG, "AP station " MACSTR " disconnected", MAC2STR(evt->mac));
	} else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_CONNECTED) {
		g_sta_connected = true;
		ESP_LOGI(TAG, "STA connected");
	} else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
		g_sta_connected = false;
		g_sta_got_ip = false;
		xSemaphoreGive(g_sta_sem);
	} else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t *evt = data;
		g_sta_got_ip = true;
		xSemaphoreGive(g_sta_sem);
		ESP_LOGI(TAG, "STA IP: " IPSTR, IP2STR(&evt->ip_info.ip));
		/* Init SNTP for real-time clock */
		sntp_setoperatingmode(SNTP_OPMODE_POLL);
		sntp_setservername(0, "ntp.aliyun.com");
		sntp_setservername(1, "pool.ntp.org");
		sntp_init();
	}
}

/* ---- Public API ---- */

static bool g_wifi_inited = false;    /* netif + event loop created once */
static bool g_wifi_started = false;   /* esp_wifi_start() called */
static esp_netif_t *g_ap_netif = NULL;
static esp_event_handler_instance_t g_inst_wifi = NULL, g_inst_ip = NULL;

/* ---- WiFi DMA pool — 2 × 1600 B pre-allocated, served via osi_funcs hook ---- */
/* Allocation strategy — 精准分流:
 *   size ∈ [1600, 1700] → from pre-allocated DMA pool (static RX buffers)
 *   other sizes        → pure native pass-through via saved original func ptrs
 * Free strategy:
 *   ptr inside pool range → mark unused
 *   else                  → native pass-through
 */
#define WIFI_DMA_BUF_SIZE  2048
#define WIFI_DMA_BUF_COUNT 2
static uint8_t *s_dma_buf[WIFI_DMA_BUF_COUNT];
static bool     s_dma_used[WIFI_DMA_BUF_COUNT];
static uintptr_t s_pool_start, s_pool_end;  /* address range for free routing */
static wifi_osi_funcs_t s_wifi_osi;
static void  *(*s_orig_malloc)(size_t) = NULL;
static void   (*s_orig_free)(void *)   = NULL;
static void  *(*s_orig_malloc_internal)(size_t) = NULL;
static void  *(*s_orig_wifi_malloc)(size_t) = NULL;

/* Try allocating from the DMA pool (1600-byte blocks only) */
static inline void *pool_try_alloc(size_t size)
{
	/* WiFi 驱动申请 static RX buf 时 size=1600，但我们分配了更大 (2048) 的缓冲防止写越界 */
	if (size >= 1600 && size <= WIFI_DMA_BUF_SIZE) {
		for (int i = 0; i < WIFI_DMA_BUF_COUNT; i++) {
			if (!s_dma_used[i] && s_dma_buf[i]) {
				s_dma_used[i] = true;
				return s_dma_buf[i];
			}
		}
	}
	return NULL;
}

static void *wifi_pool_malloc(size_t size)
{
	void *p = pool_try_alloc(size);
	if (p) return p;
	return s_orig_malloc ? s_orig_malloc(size) : NULL;
}

static void *wifi_pool_malloc_internal(size_t size)
{
	void *p = pool_try_alloc(size);
	if (p) return p;
	return s_orig_malloc_internal ? s_orig_malloc_internal(size) : NULL;
}

static void *wifi_pool_wifi_malloc(size_t size)
{
	void *p = pool_try_alloc(size);
	if (p) return p;
	return s_orig_wifi_malloc ? s_orig_wifi_malloc(size) : NULL;
}

static void wifi_pool_free(void *p)
{
	if (!p) return;
	/* Check address range — belongs to our DMA pool? */
	if ((uintptr_t)p >= s_pool_start && (uintptr_t)p < s_pool_end) {
		for (int i = 0; i < WIFI_DMA_BUF_COUNT; i++) {
			if (p == s_dma_buf[i]) {
				s_dma_used[i] = false;
				return;
			}
		}
		return; /* address in range but not our buf — shouldn't happen */
	}
	/* Not in pool → native pass-through */
	if (s_orig_free) s_orig_free(p);
}

/* Forward declaration — release_dma defined after reserve_dma */
static void wifi_cfg_release_dma(void);

void wifi_cfg_reserve_dma(void)
{
	/* 如果之前分配过，先释放再重新分配（确保每次 WiFi 启动都有干净池） */
	if (s_dma_buf[0] != NULL) {
		wifi_cfg_release_dma();
	}

	for (int i = 0; i < WIFI_DMA_BUF_COUNT; i++) {
		s_dma_buf[i] = heap_caps_malloc(WIFI_DMA_BUF_SIZE, MALLOC_CAP_DMA);
		s_dma_used[i] = false;
	}
	s_pool_start = (uintptr_t)s_dma_buf[0];
	s_pool_end   = (uintptr_t)s_dma_buf[WIFI_DMA_BUF_COUNT - 1] + WIFI_DMA_BUF_SIZE;
	if (s_pool_start == 0) s_pool_end = 0; /* disable range check if alloc failed */

	/* Clone default osi_funcs, then override allocators */
	memcpy(&s_wifi_osi, &g_wifi_osi_funcs, sizeof(wifi_osi_funcs_t));
	/* ESP-IDF v6.0 要求版本号必须正确，某些场景下 g_wifi_osi_funcs 可能未充分初始化 */
	s_wifi_osi._version = ESP_WIFI_OS_ADAPTER_VERSION;
	s_orig_malloc         = s_wifi_osi._malloc;
	s_orig_free           = s_wifi_osi._free;
	s_orig_malloc_internal = s_wifi_osi._malloc_internal;
	s_orig_wifi_malloc    = s_wifi_osi._wifi_malloc;
	s_wifi_osi._malloc             = wifi_pool_malloc;
	s_wifi_osi._free               = wifi_pool_free;
	s_wifi_osi._malloc_internal    = wifi_pool_malloc_internal;
	s_wifi_osi._wifi_malloc        = wifi_pool_wifi_malloc;
	/* All other func pointers stay as default — NEVER set any to NULL */

	ESP_LOGI(TAG, "WiFi DMA pool: %d x %d B  range=[%08x..%08x]",
		 WIFI_DMA_BUF_COUNT, WIFI_DMA_BUF_SIZE,
		 (unsigned)s_pool_start, (unsigned)s_pool_end);
}

/** 释放 DMA 池并还原 osi_funcs（解除劫持），由 wifi_cfg_deinit() 自动调用 */
static void wifi_cfg_release_dma(void)
{
	/* 先用原始 free 释放池中的 DMA 块 */
	for (int i = 0; i < WIFI_DMA_BUF_COUNT; i++) {
		if (s_dma_buf[i]) {
			if (s_orig_free) {
				s_orig_free(s_dma_buf[i]);
			} else {
				free(s_dma_buf[i]);
			}
			s_dma_buf[i] = NULL;
		}
		s_dma_used[i] = false;
	}
	s_pool_start = s_pool_end = 0;

	/* 恢复默认 osi_funcs（解除劫持），保留修正后的 version */
	if (g_wifi_osi_funcs._malloc != NULL) {
		memcpy(&s_wifi_osi, &g_wifi_osi_funcs, sizeof(wifi_osi_funcs_t));
		s_wifi_osi._version = ESP_WIFI_OS_ADAPTER_VERSION;
	}

	/* 清除原始函数指针缓存 */
	s_orig_malloc = NULL;
	s_orig_free   = NULL;
	s_orig_malloc_internal = NULL;
	s_orig_wifi_malloc     = NULL;

	ESP_LOGI(TAG, "WiFi DMA pool released");
}

/* Cache the saved config so we don't re-read from SPI Flash every time */
static storage_config_t g_saved_cfg;
static bool g_saved_cfg_valid = false;

static void wifi_auto_connect(void)
{
	if (!g_saved_cfg_valid) {
		storage_config_default(&g_saved_cfg);
		if (storage_config_read(&g_saved_cfg) != ESP_OK) {
			g_saved_cfg.wifi_ssid[0] = '\0';
		}
		g_saved_cfg_valid = true;
	}

	if (g_saved_cfg.wifi_ssid[0] != '\0') {
		ESP_LOGI(TAG, "found saved WiFi: %s - auto-connecting...",
			 g_saved_cfg.wifi_ssid);
		bool ok = wifi_sta_connect(g_saved_cfg.wifi_ssid,
					   g_saved_cfg.wifi_password);
		ESP_LOGI(TAG, "auto-connect %s", ok ? "OK" : "FAIL");
	}
}

esp_err_t wifi_cfg_start(wifi_cfg_on_configured_t on_configured)
{
	g_on_configured = on_configured;
	g_configured = false;

	if (!g_sta_sem) {
		g_sta_sem = xSemaphoreCreateBinary();
	}

	wifi_cfg_reserve_dma();   /* 分配 DMA 池 + 强制 version=8 */

	if (!g_wifi_inited) {
		/* One-time init */
		esp_netif_init();
		esp_event_loop_create_default();

		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		/* --- 极限压缩 WiFi 内存池 --- */
		cfg.static_rx_buf_num  = 2;
		cfg.dynamic_rx_buf_num = 2;
		cfg.dynamic_tx_buf_num = 2;   /* 默认 16 → 2 (绝对下限) */
		cfg.rx_mgmt_buf_num    = 2;   /* 默认 5 → 2 */
		cfg.ampdu_rx_enable    = 0;
		cfg.ampdu_tx_enable    = 0;
		/* --------------------------------------- */
		cfg.osi_funcs = &s_wifi_osi;
		ESP_LOGW(TAG, "--- Before WiFi Init ---");
		ESP_LOGW(TAG, "Free 8BIT Heap: %d", heap_caps_get_free_size(MALLOC_CAP_8BIT));
		ESP_LOGW(TAG, "Free DMA Heap : %d", heap_caps_get_free_size(MALLOC_CAP_DMA));
		ESP_LOGW(TAG, "Largest Free DMA Block: %d",
			 heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
		heap_caps_print_heap_info(MALLOC_CAP_8BIT);
		if (esp_wifi_init(&cfg) != ESP_OK) {
			ESP_LOGE(TAG, "esp_wifi_init failed (heap low?)");
			return ESP_ERR_NO_MEM;
		}

		esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
						    wifi_event_handler, NULL, &g_inst_wifi);
		esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
						    wifi_event_handler, NULL, &g_inst_ip);
		g_wifi_inited = true;
	}

	if (g_wifi_started) {
		/* Already running — just reconfigure / restart STA */
		wifi_auto_connect();
		return ESP_OK;
	}

	/* Create AP netif once */
	if (!g_ap_netif) {
		g_ap_netif = esp_netif_create_default_wifi_ap();
	}

	wifi_config_t ap_cfg = {
		.ap = {
			.ssid = CFG_WIFI_AP_SSID,
			.ssid_len = strlen(CFG_WIFI_AP_SSID),
			.password = CFG_WIFI_AP_PASSWORD,
			.max_connection = CFG_WIFI_AP_MAX_CONN,
			.authmode = WIFI_AUTH_WPA_WPA2_PSK,
		},
	};
	if (strlen(CFG_WIFI_AP_PASSWORD) == 0) ap_cfg.ap.authmode = WIFI_AUTH_OPEN;

	esp_wifi_set_mode(WIFI_MODE_AP);
	esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
	esp_wifi_start();
	g_wifi_started = true;

	ESP_LOGI(TAG, "AP started: SSID=%s", CFG_WIFI_AP_SSID);
	start_http_server();
	ESP_LOGI(TAG, "HTTP server on :80");

	wifi_auto_connect();

	return ESP_OK;
}

/* ---- STA-only: boot-time connect without AP ---- */
esp_err_t wifi_cfg_start_sta_only(void)
{
	g_configured = false;

	if (!g_sta_sem) g_sta_sem = xSemaphoreCreateBinary();

	wifi_cfg_reserve_dma();   /* 分配 DMA 池 + 强制 version=8 */

	/* 1. 完全清理之前的 WiFi 状态（包括事件回调、netif），确保干净的重新初始化 */
	wifi_cfg_deinit();

	/* 2. 重新初始化基础网络栈 */
	esp_netif_init();
	/* 默认事件循环是单例，重复创建返回 ESP_ERR_INVALID_STATE，忽略即可 */
	esp_err_t loop_err = esp_event_loop_create_default();
	if (loop_err != ESP_OK && loop_err != ESP_ERR_INVALID_STATE) {
		ESP_LOGW(TAG, "event_loop: 0x%x (ignored)", loop_err);
	}

	/* 3. 配置最小内存池并传入修正后的 osi_funcs */
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.static_rx_buf_num  = 2;
	cfg.dynamic_rx_buf_num = 2;
	cfg.dynamic_tx_buf_num = 2;
	cfg.rx_mgmt_buf_num    = 2;
	cfg.ampdu_rx_enable    = 0;
	cfg.ampdu_tx_enable    = 0;
	cfg.osi_funcs = &s_wifi_osi;
	if (esp_wifi_init(&cfg) != ESP_OK) {
		ESP_LOGE(TAG, "esp_wifi_init failed");
		return ESP_ERR_NO_MEM;
	}

	/* 4. 注册事件 */
	esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
					    wifi_event_handler, NULL, &g_inst_wifi);
	esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
					    wifi_event_handler, NULL, &g_inst_ip);
	g_wifi_inited = true;

	/* 5. 纯 STA 模式启动 */
	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_start();
	g_wifi_started = true;

	wifi_auto_connect();

	return (g_sta_connected && g_sta_got_ip) ? ESP_OK : ESP_FAIL;
}

/* ---- AP-only: config mode without STA ---- */
esp_err_t wifi_cfg_start_ap_only(wifi_cfg_on_configured_t on_configured)
{
	g_on_configured = on_configured;
	g_configured = false;

	if (!g_sta_sem) g_sta_sem = xSemaphoreCreateBinary();

	wifi_cfg_reserve_dma();   /* 分配 DMA 池 + 强制 version=8 */

	if (!g_wifi_inited) {
		esp_netif_init();
		esp_event_loop_create_default();

		wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
		cfg.static_rx_buf_num  = 2;
		cfg.dynamic_rx_buf_num = 2;
		cfg.dynamic_tx_buf_num = 2;
		cfg.rx_mgmt_buf_num    = 2;
		cfg.ampdu_rx_enable    = 0;
		cfg.ampdu_tx_enable    = 0;
		cfg.osi_funcs = &s_wifi_osi;
		if (esp_wifi_init(&cfg) != ESP_OK) {
			ESP_LOGE(TAG, "esp_wifi_init failed");
			return ESP_ERR_NO_MEM;
		}
		esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
						    wifi_event_handler, NULL, &g_inst_wifi);
		esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
						    wifi_event_handler, NULL, &g_inst_ip);
		g_wifi_inited = true;
	}

	if (g_wifi_started) {
		ESP_LOGW(TAG, "WiFi already running — stop first");
		return ESP_ERR_INVALID_STATE;
	}

	/* AP netif once */
	if (!g_ap_netif) g_ap_netif = esp_netif_create_default_wifi_ap();

	wifi_config_t ap_cfg = {
		.ap = {
			.ssid = CFG_WIFI_AP_SSID,
			.ssid_len = strlen(CFG_WIFI_AP_SSID),
			.password = CFG_WIFI_AP_PASSWORD,
			.max_connection = CFG_WIFI_AP_MAX_CONN,
			.authmode = WIFI_AUTH_WPA_WPA2_PSK,
		},
	};
	if (strlen(CFG_WIFI_AP_PASSWORD) == 0) ap_cfg.ap.authmode = WIFI_AUTH_OPEN;

	esp_wifi_set_mode(WIFI_MODE_AP);
	esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
	esp_wifi_start();
	g_wifi_started = true;

	ESP_LOGI(TAG, "AP started: SSID=%s", CFG_WIFI_AP_SSID);
	start_http_server();
	ESP_LOGI(TAG, "HTTP server on :80");

	return ESP_OK;
}

void wifi_cfg_stop(void)
{
	if (g_server) { httpd_stop(g_server); g_server = NULL; }
	esp_wifi_stop();
	g_wifi_started = false;
}

/** Full deinit — releases ALL WiFi driver memory (~30KB+). Use before BLE init. */
void wifi_cfg_deinit(void)
{
	if (!g_wifi_inited) {
		ESP_LOGI(TAG, "WiFi already deinit'd — skipping");
		return;
	}

	/* 1. 停止 WiFi 并释放驱动内部资源 */
	wifi_cfg_stop();
	esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, g_inst_wifi);
	esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, g_inst_ip);
	g_inst_wifi = g_inst_ip = NULL;

	/* 2. deinit 驱动（此时 _free 仍是我们的池分配器，驱动释放的内存会标记池块为未使用） */
	esp_wifi_deinit();
	g_wifi_inited  = false;
	g_wifi_started = false;
	g_ap_netif     = NULL;

	/* 3. 释放底层 netif */
	esp_netif_deinit();

	/* 4. 释放 DMA 池内存并还原 osi_funcs */
	wifi_cfg_release_dma();

	ESP_LOGI(TAG, "WiFi fully deinitialized — memory released");
	ESP_LOGW(TAG, "MEM after deinit: Free=%lu  MaxBlock=%lu  DMA_MaxBlock=%lu",
		 (unsigned long)esp_get_free_heap_size(),
		 (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
		 (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
}

bool wifi_cfg_is_done(void)          { return g_configured; }
bool wifi_cfg_is_sta_connected(void)  { return g_sta_connected && g_sta_got_ip; }
bool wifi_cfg_is_ap_sta_connected(void) { return g_ap_sta_connected; }
const char *wifi_cfg_get_server_ip(void) { return server_get_ip(); }
uint16_t wifi_cfg_get_server_port(void) { return server_get_port(); }

int wifi_cfg_tcp_send(const char *data)
{
	if (!g_sta_connected || !g_sta_got_ip || data == NULL) return -1;

	char port_str[8];
	snprintf(port_str, sizeof(port_str), "%u", server_get_port());

	struct addrinfo hints = { .ai_family = AF_INET, .ai_socktype = SOCK_STREAM };
	struct addrinfo *res = NULL;
	if (getaddrinfo(server_get_ip(), port_str, &hints, &res) != 0 || res == NULL) {
		ESP_LOGW(TAG, "TCP send: DNS failed for %s", server_get_ip());
		return -1;
	}

	int sock = socket(res->ai_family, res->ai_socktype, 0);
	if (sock < 0) { freeaddrinfo(res); return -1; }

	struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) {
		ESP_LOGW(TAG, "TCP send: connect failed");
		close(sock); freeaddrinfo(res); return -1;
	}
	freeaddrinfo(res);

	int len = strlen(data);
	int sent = send(sock, data, len, 0);
	close(sock);

	ESP_LOGI(TAG, "WiFi TCP sent %d/%d bytes", sent > 0 ? sent : 0, len);
	return sent;
}
