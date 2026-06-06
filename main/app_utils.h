#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gptimer.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "CT511N.h"
#include "W25Q64.h"
#include "sensor_hub.h"

/* 内存诊断宏：在模块 deinit 后调用，显示释放后的空闲内存 */
#define LOG_MEM_AFTER(tag, module)  do {                                     \
	ESP_LOGW(tag, "MEM after %s: Free=%lu  MaxBlock=%lu  DMA_MaxBlock=%lu", \
		 module,                                                         \
		 (unsigned long)esp_get_free_heap_size(),                          \
		 (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT), \
		 (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_DMA)); \
} while (0)



#ifdef __cplusplus

extern "C" {

#endif



/* ========================================================================= */

/*  Constants                                                                */

/* ========================================================================= */

#include "config.h"



/* ========================================================================= */

/*  Timer helpers                                                            */

/* ========================================================================= */

esp_err_t timer_start(gptimer_handle_t timer, uint32_t interval_s);

esp_err_t timer_stop(gptimer_handle_t timer);



/* ========================================================================= */

/*  Low-power helpers                                                        */

/* ========================================================================= */

void low_power_enter(gptimer_handle_t timer,

		     ct511n_handle_t *ct511n,

		     w25q64_handle_t *w25q64,

		     sensor_hub_t *hub);

void low_power_exit(gptimer_handle_t timer,

		    ct511n_handle_t *ct511n,

		    w25q64_handle_t *w25q64,

		    sensor_hub_t *hub);



/**

 * @brief  Configure GPIO wakeup for Light-sleep.

 *

 * BMI160 INT1 (CFG_BMI160_INT1_PIN) is set as a wakeup source so that any-motion

 * can wake the CPU from Light-sleep.  Call once on entry to LOW_POWER.

 */

void low_power_sleep_configure(void);



/**

 * @brief  Tear down GPIO wakeup config after leaving LOW_POWER.

 */

void low_power_sleep_unconfigure(void);



/* ========================================================================= */

/*  Upload helpers                                                           */

/* ========================================================================= */

/** Upload all pending records from flash (or RAM fallback). */

void upload_send_all(ct511n_handle_t *ct511n);



/* ========================================================================= */

/*  EPD helpers                                                              */

/* ========================================================================= */



/** Initialise EPD display on shared SPI bus. */

esp_err_t epd_app_init(spi_host_device_t host, void *flash_handle);



/** BLE-image-received callback for EPD display. */

int epd_ble_img_ready(uint32_t total_bytes);

/** Set by epd_ble_img_ready() in BLE task, consumed by main loop. */
extern volatile bool g_need_epd_update;



/* ========================================================================= */

/*  Sample helpers                                                           */

/* ========================================================================= */

/**

 * @brief  Sample GPS + BMI160 + AK09911C, log summary every 2 s.

 * @param  hub     sensor hub handle

 * @param  ct511n  CT511N handle for GPS

 * @param  count   current sample count (used for log throttling)

 * @return true if GPS has fix

 */

bool sample_sensors(sensor_hub_t *hub, ct511n_handle_t *ct511n,

    uint32_t count);



#ifdef __cplusplus

}

#endif