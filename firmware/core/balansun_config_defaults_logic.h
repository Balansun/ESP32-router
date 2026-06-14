#pragma once

#include <ArduinoJson.h>

#include "balansun_config_defaults_json.h"
#include "balansun_hw_profile_logic.h"

bool balansun_config_global_key_is_factory_default(const char *key);

void balansun_config_append_export(JsonObject out, bool sparse);
void balansun_config_append_export_metadata(JsonObject cfg, const BalansunHwCaps &caps);
bool config_merge_from_backup_json(JsonObject root, String &err);

void balansun_backup_append_device_block(JsonObject device, const BalansunHwCaps &caps, bool sparse_export);
bool balansun_backup_time_differs_from_factory();
void balansun_backup_append_time(JsonObject timeObj, bool sparse);

bool balansun_backup_is_sparse_import(JsonObject root);
