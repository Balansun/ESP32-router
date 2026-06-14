#include "balansun_persistence.h"

#include <Arduino.h>

#include "storage_eeprom.h"

/*
 * balansun_persistence.cpp — Arduino glue for the write-coalescing facade.
 * Pure decision logic lives in balansun_persistence_logic.h (unit-tested on host).
 */

namespace {
PersistCoalescerState g_state;
PersistCoalescerConfig g_cfg;

/** Reentrancy guard: persistConfigToEeprom() calls persistence_notify_flushed(). */
bool g_flushing = false;

void do_flush() {
  g_flushing = true;
  persistConfigToEeprom();  // serializes ALL globals -> includes any pending deferred change
  g_flushing = false;
  // persistence_notify_flushed() already cleared g_state via the call above; belt-and-braces:
  persist_after_flush(g_state, millis());
}
}  // namespace

void persistence_request_config_deferred(void) { persist_mark_dirty(g_state, millis()); }

void persistence_service(void) {
  if (persist_should_flush(g_state, g_cfg, millis())) {
    do_flush();
  }
}

void persistence_flush_all(void) {
  if (g_state.dirty) {
    do_flush();
  }
}

void persistence_notify_flushed(void) {
  // Any path that actually wrote the blob (critical save, deferred flush, history import)
  // clears the pending batch — the blob now contains the latest of every global.
  if (g_flushing) return;  // do_flush() handles its own clear to keep flush_count consistent
  persist_after_flush(g_state, millis());
}

bool persistence_config_pending(void) { return g_state.dirty; }

uint32_t persistence_flush_count(void) { return g_state.flush_count; }
