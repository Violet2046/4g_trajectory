#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gptimer.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_sleep.h"

#include "CT511N.h"
#include "W25Q64.h"
#include "sensor_hub.h"
#include "ST7789.h"
#include "ble_img_rx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*  Constants                                                                */
/* ========================================================================= */
#include "pin_config.h"

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
 * BMI160 INT1 (LP_WAKEUP_GPIO) is set as a wakeup source so that any-motion
 * can wake the CPU from Light-sleep.  Call once on entry to LOW_POWER.
 */
void low_power_sleep_configure(void);

/**
 * @brief  Enter Light-sleep and wait for wakeup.
 *
 * CPU is halted; any-motion on GPIO LP_WAKEUP_GPIO (or any other enabled
 * wakeup source) will resume execution.  The BMI160 ISR fires before this
 * function returns.
 */
void low_power_sleep_enter(void);

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
/*  IMG_RECEIVE helpers — display + BLE image receive                       */
/* ========================================================================= */

/**
 * @brief  Initialise the display (ST7789) and start BLE advertising.
 *
 * Called once when entering STATE_IMG_RECEIVE.  Registers an internal
 * callback that displays the received image automatically.
 *
 * @param  host          SPI host used (shared with W25Q64)
 * @param  flash_handle  W25Q64 handle (for image cache writes)
 * @return ESP_OK on success.
 */
esp_err_t img_recv_enter(spi_host_device_t host,
			 w25q64_handle_t *flash_handle);

/**
 * @brief  Exit IMG_RECEIVE: stop BLE, flush RAM cache, turn off display.
 */
void img_recv_exit(void);

/**
 * @brief  Perform one iteration of the IMG_RECEIVE polling loop.
 *
 * Must be called periodically from the main loop while in IMG_RECEIVE state.
 * Keeps the watchdog fed and checks BLE state.  Returns false when the
 * caller should transition back to STATE_ACTIVE.
 *
 * @param  hub     sensor hub (for sampling during image reception)
 * @param  ct511n  CT511N handle (for GPS during image reception)
 * @param  count   current sample count
 * @return true if still receiving; false if image transfer is complete
 */
bool img_recv_poll(sensor_hub_t *hub, ct511n_handle_t *ct511n,
		   uint32_t count);

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
