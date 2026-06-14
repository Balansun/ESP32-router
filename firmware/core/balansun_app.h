#pragma once

/*
 * balansun_app.h — Application entry: setup/loop, FreeRTOS metering task, overproduction regulation.
 * See: /en/project-overview/ § High-level runtime model.
 */

#include <sys/time.h>

void balansun_setup(void);
void balansun_loop(void);
void balansun_metering_task(void *pvParameters);
void balansun_init_action_gpios(void);
void balansun_apply_surplus_regulation(void);
void balansun_daily_energy_tick(void);
void time_sync_notification(struct timeval *tv);
void balansun_poll_temperature(void);
void balansun_update_status_leds(void);
