#pragma once

#include <cstdint>

/*
 * balansun_config_store.h — Per-domain NVS config store (Phase 2).
 *
 * Backs the in-RAM EEPROM image with independent NVS keys per byte-range domain so a commit
 * only rewrites the domains that changed. Falls back transparently: on first boot after upgrade
 * it migrates from the legacy monolithic "eeprom" NVS blob. Pure split/CRC logic lives in
 * balansun_config_store_logic.h (host-tested).
 */

/**
 * Populate @p image (size @p size, must equal kEepromSize) from the per-domain NVS keys.
 * If the domain keys are absent, treats @p image (already loaded from the legacy blob by
 * EEPROM.begin) as the source and migrates it into per-domain keys.
 */
void balansun_config_store_load(uint8_t *image, int size);

/** Bit mask for balansun_config_store_commit() — limits which NVS keys are written. */
constexpr uint8_t kBalansunConfigDomainHist = 1u << 0;
constexpr uint8_t kBalansunConfigDomainAnchor = 1u << 1;
constexpr uint8_t kBalansunConfigDomainParams = 1u << 2;
constexpr uint8_t kBalansunConfigDomainAll =
    kBalansunConfigDomainHist | kBalansunConfigDomainAnchor | kBalansunConfigDomainParams;

/** Write only the selected domains whose bytes changed since the last commit. Returns true on success. */
bool balansun_config_store_commit(const uint8_t *image, int size,
                               uint8_t domain_mask = kBalansunConfigDomainAll);

/** Force the params domain to be rewritten on the next commit (config persist). */
void balansun_config_store_mark_params_dirty(void);

/** Remove all per-domain NVS keys (config/history); legacy flash image unchanged. */
void balansun_config_store_wipe(void);

/** Drop the frozen Arduino "eeprom" NVS blob after migration to reclaim partition space. */
void balansun_config_store_reclaim_legacy_nvs(void);

/** True when per-domain balansun_cfg keys are present (legacy Arduino EEPROM NVS blob not required). */
bool balansun_config_store_domains_ready(void);

/** True when balansun_cfg has at least one domain (safe to drop legacy "eeprom" NVS before EEPROM.begin). */
bool balansun_config_store_any_domain_present(void);

/** True if the last load migrated from the legacy monolithic blob. */
bool balansun_config_store_migrated();

/** Count of domains written by the most recent commit (telemetry; 0 = nothing changed). */
int balansun_config_store_last_written();
