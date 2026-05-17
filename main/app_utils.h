#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gptimer.h"
#include "esp_err.h"

#include "CT511N.h"
#include "W25Q64.h"
#include "sensor_hub.h"

#ifdef __cplusplus
extern "C" {
#endif

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

/* ========================================================================= */
/*  Upload helpers                                                           */
/* ========================================================================= */
/** Upload all pending records from flash (or RAM fallback). */
void upload_send_all(ct511n_handle_t *ct511n);

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
