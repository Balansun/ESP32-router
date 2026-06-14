#pragma once

#include "balansun_persistence_logic.h"

/*
 * balansun_persistence.h — Facade over config persistence (write coalescing + tiering).
 *
 * Callers no longer call persistConfigToEeprom() directly for high-frequency changes.
 * Instead:
 *   - Critical/user-initiated/rare changes keep calling persistConfigToEeprom() (which
 *     now notifies this facade so the pending deferred batch is cleared too).
 *   - High-frequency changes (MQTT command handlers) call persistence_request_config_deferred().
 *
 * persistence_service() is pumped from balansun_loop(); persistence_flush_all() is invoked
 * before reboot / OTA so no deferred change is lost on a graceful restart.
 */

/** Mark config dirty for a coalesced (deferred) flush. Safe to call very frequently. */
void persistence_request_config_deferred(void);

/** Pump the coalescer from the main loop; flushes when the debounce/max-defer window elapses. */
void persistence_service(void);

/** Force-flush any pending deferred config now (call before reboot / OTA). */
void persistence_flush_all(void);

/** Called by persistConfigToEeprom() after a successful blob write to clear pending state. */
void persistence_notify_flushed(void);

/** True when a deferred config change is staged but not yet written to NVS. */
bool persistence_config_pending(void);

/** Number of coalesced/critical flushes since boot (telemetry / diag). */
uint32_t persistence_flush_count(void);
