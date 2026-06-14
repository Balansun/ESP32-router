/*
 * balansun_triac_isr.cpp — Triac ISRs: ZC + 100 µs phase-cut + 10 ms pulse modes.
 */
#include "balansun_triac_isr.h"

#include "balansun_board.h"
#include "balansun_pin_map.h"
#include "balansun_mains_profile.h"
#include "balansun_pulse_modes.h"
#include "balansun_regulation_modes.h"
#include "balansun_triac_logic.h"

#include <esp_arduino_version.h>
#include <esp_timer.h>

#if defined(CONFIG_IDF_TARGET_ESP32) || CONFIG_IDF_TARGET_ESP32S3
#include "soc/gpio_struct.h"
#endif
#if CONFIG_IDF_TARGET_ESP32S3
#include <driver/gpio.h>
#endif

volatile uint32_t last_zc_us = 0;
volatile int phase_delay_ticks = 0;
volatile int triac_delay_percent = 100;
volatile int16_t zc_sync_state = 0;
volatile int IT_half_period = 0;
volatile int IT_half_period_in = 0;
hw_timer_t *timer = nullptr;
hw_timer_t *timer10ms = nullptr;

static volatile uint8_t g_action0_mode = kModeDecoupeOnoff;
static volatile bool g_triac_request_timers = false;
static bool g_triac_phase_timer_running = false;
static bool g_triac_10ms_timer_running = false;
static volatile uint32_t g_triac_dim_mask = 0;

void balansun_triac_refresh_dim_gpio_mask(void) {
  const int gpio = g_pins.triac_dim;
  if (gpio >= 0 && gpio < 32) {
    g_triac_dim_mask = (1UL << static_cast<unsigned>(gpio));
  } else {
    g_triac_dim_mask = 0;
  }
}

void balansun_triac_set_action0_mode(uint8_t mode) { g_action0_mode = mode; }

#if defined(CONFIG_IDF_TARGET_ESP32) || CONFIG_IDF_TARGET_ESP32S3
void IRAM_ATTR balansun_triac_set_dim_level_isr(int level) {
  const uint32_t mask = g_triac_dim_mask;
  if (level) {
    GPIO.out_w1ts = mask;
  } else {
    GPIO.out_w1tc = mask;
  }
}
#else
void IRAM_ATTR balansun_triac_set_dim_level_isr(int level) { digitalWrite(g_pins.triac_dim, level); }
#endif

static inline void IRAM_ATTR triac_gpio_set_level_isr(int level) { balansun_triac_set_dim_level_isr(level); }

static inline int IRAM_ATTR triac_delay_threshold_ticks_isr(void) {
  return triac_delay_threshold_ticks(triac_delay_percent, (int)g_triac_max_delay_ticks_isr);
}

void IRAM_ATTR balansun_zc_isr() {
  IT_half_period += 1;
  const uint32_t now = (uint32_t)esp_timer_get_time();
  if ((uint32_t)(now - last_zc_us) > 2000u) {
    phase_delay_ticks = 0;
    last_zc_us = now;
    triac_gpio_set_level_isr(0);
    IT_half_period_in += 1;
    g_triac_request_timers = true;
    zc_sync_state += 3;
    if (zc_sync_state > 5) zc_sync_state = 5;
  }
}

void IRAM_ATTR balansun_phase_tick_isr() {
  if (g_action0_mode != kModeDecoupeOnoff) {
    triac_gpio_set_level_isr(0);
    return;
  }
  phase_delay_ticks += 1;
  const int threshold = triac_delay_threshold_ticks_isr();
  const int maxTick = (int)g_triac_max_delay_ticks_isr;
  if (phase_delay_ticks > threshold && triac_delay_percent < 98 && zc_sync_state > 0 && threshold <= maxTick) {
    triac_gpio_set_level_isr(1);
  } else {
    triac_gpio_set_level_isr(0);
  }
}

void IRAM_ATTR balansun_half_cycle_tick_isr() {
  zc_sync_state -= 1;
  if (zc_sync_state < -5) zc_sync_state = -5;
  if (zc_sync_state < 0) {
    balansun_pulse_modes_tick_10ms();
  }
}

#if CONFIG_IDF_TARGET_ESP32S3
/** Arduino attachInterrupt() calls gpio_hal_input_enable() which can fault on S3 DevKit. */
static void IRAM_ATTR balansun_zc_isr_gpio_thunk(void *arg) {
  (void)arg;
  balansun_zc_isr();
}

static void balansun_triac_attach_zc_interrupt_s3(void) {
  static bool isr_service_ready = false;
  if (!isr_service_ready) {
    const esp_err_t err = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    isr_service_ready = (err == ESP_OK || err == ESP_ERR_INVALID_STATE);
    if (!isr_service_ready) {
      return;
    }
  }
  const gpio_num_t zc = static_cast<gpio_num_t>(g_pins.zero_cross);
  if (!GPIO_IS_VALID_GPIO(zc)) {
    return;
  }
  gpio_reset_pin(zc);
  gpio_config_t cfg = {};
  cfg.pin_bit_mask = 1ULL << static_cast<unsigned>(zc);
  cfg.mode = GPIO_MODE_INPUT;
  cfg.pull_up_en = GPIO_PULLUP_DISABLE;
  cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
  cfg.intr_type = GPIO_INTR_POSEDGE;
  gpio_config(&cfg);
  gpio_isr_handler_remove(zc);
  gpio_isr_handler_add(zc, balansun_zc_isr_gpio_thunk, nullptr);
}
#endif

static void balansun_triac_start_phase_timer(void) {
  if (g_triac_phase_timer_running) {
    return;
  }
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  timer = timerBegin(1000000u);
  if (timer != nullptr) {
    timerAttachInterrupt(timer, balansun_phase_tick_isr);
    timerAlarm(timer, 100, true, 0);
    timerStart(timer);
    g_triac_phase_timer_running = true;
  }
#else
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &balansun_phase_tick_isr, true);
  timerAlarmWrite(timer, 100, true);
  timerAlarmEnable(timer);
  g_triac_phase_timer_running = true;
#endif
}

static void balansun_triac_start_10ms_timer(void) {
  if (g_triac_10ms_timer_running) {
    return;
  }
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  timer10ms = timerBegin(1000000u);
  if (timer10ms != nullptr) {
    timerAttachInterrupt(timer10ms, balansun_half_cycle_tick_isr);
    timerAlarm(timer10ms, 10000, true, 0);
    timerStart(timer10ms);
    g_triac_10ms_timer_running = true;
  }
#else
  timer10ms = timerBegin(1, 80, true);
  timerAttachInterrupt(timer10ms, &balansun_half_cycle_tick_isr, true);
  timerAlarmWrite(timer10ms, 10000, true);
  timerAlarmEnable(timer10ms);
  g_triac_10ms_timer_running = true;
#endif
}

void balansun_triac_poll_hw(void) {
#ifndef METER_ONLY_BUILD
  if (!g_triac_request_timers) {
    return;
  }
  if (!g_triac_10ms_timer_running) {
    balansun_triac_start_10ms_timer();
  }
  if (!g_triac_phase_timer_running) {
    balansun_triac_start_phase_timer();
  }
#endif
}

void balansun_triac_enable_zc_interrupt(void) {
#ifndef METER_ONLY_BUILD
  static bool attached = false;
  if (attached) {
    return;
  }
#if CONFIG_IDF_TARGET_ESP32S3
  balansun_triac_attach_zc_interrupt_s3();
#else
  attachInterrupt(g_pins.zero_cross, balansun_zc_isr, RISING);
#endif
  attached = true;
#endif
}

void balansun_triac_hw_init(void) {
#ifndef METER_ONLY_BUILD
  balansun_triac_refresh_dim_gpio_mask();
  auto end_timer = [](hw_timer_t *&t, void (*isr)()) {
    if (t == nullptr) return;
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    timerDetachInterrupt(t);
    timerStop(t);
    timerEnd(t);
#else
    timerAlarmDisable(t);
    timerDetachInterrupt(t);
    timerEnd(t);
#endif
    t = nullptr;
    (void)isr;
  };

  end_timer(timer, balansun_phase_tick_isr);
  end_timer(timer10ms, balansun_half_cycle_tick_isr);
  g_triac_phase_timer_running = false;
  g_triac_10ms_timer_running = false;
  g_triac_request_timers = false;
#else
  (void)timer;
  (void)timer10ms;
#endif
}
