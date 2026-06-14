/*
 * storage_eeprom_extension.cpp — Variable-length EEPROM tail after the fixed Balansun parameter block.
 * Read/write uses chained uint16 magics; unknown magics are skipped on read (forward-compatible).
 * Called from storage_eeprom.cpp after eeprom_layout_key / WiFi / MQTT / Source strings.
 * See: storage_eeprom_layout.h (ASCII map); /en/project-overview/ § Persistence.
 * User: advanced settings (Ext, Pmqtt, OTA/API, PWM) — /fr/user-guide/#guide-d-interface-web-planning-et-api-de-ce-firmware
 */
#include "storage_eeprom_extension.h"

#include "api_access_token.h"
#include "balansun_board.h"
#include "balansun_temperature_logic.h"
#include "balansun_pmqtt_bindings_nvs.h"
#include "balansun_regulation_persist.h"
#include "balansun_ha_site_persist.h"
#if defined(FLEET_BUNDLE_NATIVE_STUB)
#include "balansun_diag_persist_stub.h"
#else
#include "balansun_diag_persist.h"
#include "balansun_self_test.h"
#include "balansun_diag.h"
#endif

#include <algorithm>

namespace {
constexpr uint8_t kLegacyLoadProfileVersion = 1;

/** Skip legacy 0xE211 tail (load_profile + warm peer fields); read-only migration. */
int eeprom_read_legacy_load_profile_block(int address, IEepromBackend &eeprom, EepromExtensionFields &fields) {
  const int cap = eeprom.capacity();
  if (address < 0 || address + 2 > cap) return address;
  if (eeprom.readUShort(address) != kEepromWarmProfileMagic) {
    fields.legacyLoadProfileBlockPresent = false;
    return address;
  }
  address += static_cast<int>(sizeof(uint16_t));
  fields.legacyLoadProfileBlockPresent = true;
  if (address < cap) {
    const uint8_t version = eeprom.readByte(address++);
    if (version != kLegacyLoadProfileVersion) {
      fields.legacyLoadProfileBlockPresent = false;
      return address;
    }
  }
  if (address < cap) {
    fields.loadProfilePersist = eeprom.readString(address);
    if (fields.loadProfilePersist.length() > kEepromLoadProfileMax) {
      fields.loadProfilePersist.resize(kEepromLoadProfileMax);
    }
    address += static_cast<int>(fields.loadProfilePersist.length() + 1);
  }
  if (address + 4 <= cap) {
    address += static_cast<int>(sizeof(uint32_t));
  }
  if (address + 2 <= cap) {
    address += static_cast<int>(sizeof(uint16_t));
  }
  if (address < cap) {
    const std::string drive = eeprom.readString(address);
    address += static_cast<int>(drive.length() + 1);
  }
  if (address < cap) {
    const std::string order = eeprom.readString(address);
    address += static_cast<int>(order.length() + 1);
  }
  return address;
}
}  // namespace

static void extension_defaults(EepromExtensionFields &fields) {
  fields.pmqttTopic.clear();
  fields.pmqttSchema = "Pw";
  fields.pmqttBindingsJson = "[]";
  fields.jsyMk333SerialBaud = 9600;
  fields.ext_peer_port = 80;
  fields.ext_peer_path = "/api/v1/measurements";
  fields.ext_peer_protocol_mode = 1;
  fields.pwmGpio = -1;
  fields.pwmMode = 0;
  fields.pwmDutyPercent = 0;
  fields.pwmInverted = false;
  fields.fleetTrustKey.clear();
  fields.arduinoOtaPassword.clear();
  fields.httpApiPassword.clear();
  fields.httpCorsEnabled = false;
  fields.tempoRteEnabled = false;
  fields.tempoRteLtarfCache.clear();
  fields.tempoRteStgeCache.clear();
  fields.tempoRteJourCache.clear();
  fields.tempoRteDemainCache.clear();
  fields.tempoRteLastFetchEpoch = 0;
}

int storage_eeprom_extension_read(int address, IEepromBackend &eeprom, EepromExtensionFields &fields,
                                  const EepromMainsHook &read_mains) {
  extension_defaults(fields);
  const int cap = eeprom.capacity();
  if (address < 0 || address + 2 > cap) return address;
  const uint16_t mag = eeprom.readUShort(address);
  if (mag != kEepromExtMagic) return address;
  address += static_cast<int>(sizeof(uint16_t));
  // Block: kEepromExtMagic — Pmqtt topic, schema, JsyMk333 serial baud
  fields.pmqttTopic = eeprom.readString(address);
  address += static_cast<int>(fields.pmqttTopic.length() + 1);
  if (address >= cap) return address;
  fields.pmqttSchema = eeprom.readString(address);
  address += static_cast<int>(fields.pmqttSchema.length() + 1);
  if (address + 4 > cap) return address;
  fields.jsyMk333SerialBaud = eeprom.readULong(address);
  address += static_cast<int>(sizeof(uint32_t));
  if (address + 2 <= cap) {
    const uint16_t bindsMag = eeprom.readUShort(address);
    if (bindsMag == kEepromPmqttBindingsMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      if (address < cap) {
        fields.pmqttBindingsJson = eeprom.readString(address);
        if (fields.pmqttBindingsJson.empty()) fields.pmqttBindingsJson = "[]";
        address += static_cast<int>(fields.pmqttBindingsJson.length() + 1);
        {
          char nvsBuf[4096];
          size_t nvsLen = 0;
          if (balansun_pmqtt_bindings_nvs_load(nvsBuf, sizeof(nvsBuf), &nvsLen) && nvsLen > 0) {
            fields.pmqttBindingsJson = nvsBuf;
          }
        }
      }
    }
  }
  fields.ext_peer_port = 80;
  fields.ext_peer_path = "/api/v1/measurements";
  // Block: kEepromExtPortPathMagic — Source Ext HTTP port + path
  if (address + 2 <= cap) {
    const uint16_t tailMag = eeprom.readUShort(address);
    if (tailMag == kEepromExtPortPathMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      if (address + 2 <= cap) {
        unsigned int rp = eeprom.readUShort(address);
        address += static_cast<int>(sizeof(uint16_t));
        fields.ext_peer_port = static_cast<uint16_t>((rp == 0) ? 80 : rp);
        if (address < cap) {
          fields.ext_peer_path = eeprom.readString(address);
          address += static_cast<int>(fields.ext_peer_path.length() + 1);
        }
        if (fields.ext_peer_path.empty() || fields.ext_peer_path[0] != '/') {
          fields.ext_peer_path = "/api/v1/measurements";
        }
      }
    }
  }
  // Block: kEepromArduinoOtaPassMagic
  if (address + 2 <= cap) {
    const uint16_t otaMag = eeprom.readUShort(address);
    if (otaMag == kEepromArduinoOtaPassMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      if (address < cap) {
        fields.arduinoOtaPassword = eeprom.readString(address);
        address += static_cast<int>(fields.arduinoOtaPassword.length() + 1);
      }
    }
  }
  // Block: mains profile (optional hook — balansun_mains_profile_read_from_eeprom)
  if (read_mains) address = read_mains(address);
  // Block: kEepromHttpApiPassMagic
  if (address + 2 <= cap) {
    const uint16_t httpMag = eeprom.readUShort(address);
    if (httpMag == kEepromHttpApiPassMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      if (address < cap) {
        fields.httpApiPassword = eeprom.readString(address);
        address += static_cast<int>(fields.httpApiPassword.length() + 1);
      }
    }
  }
  // Block: kEepromHttpCorsMagic
  if (address + 2 <= cap) {
    const uint16_t corsMag = eeprom.readUShort(address);
    if (corsMag == kEepromHttpCorsMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      if (address < cap) {
        fields.httpCorsEnabled = eeprom.readByte(address) != 0;
        address += 1;
      }
    }
  }
  // Block: kEepromPwmMagic
  if (address + 2 <= cap) {
    const uint16_t pwmMag = eeprom.readUShort(address);
    if (pwmMag == kEepromPwmMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      if (address < cap) {
        const int8_t g = static_cast<int8_t>(eeprom.readByte(address));
        fields.pwmGpio = g;
        address += 1;
      }
      if (address < cap) {
        fields.pwmMode = eeprom.readByte(address);
        if (fields.pwmMode > 2) fields.pwmMode = 0;
        address += 1;
      }
      if (address < cap) {
        fields.pwmDutyPercent = eeprom.readByte(address);
        address += 1;
      }
      if (address < cap) {
        fields.pwmInverted = eeprom.readByte(address) != 0;
        address += 1;
      }
    }
  }
  // Block: kEepromFleetTrustMagic
  if (address + 2 <= cap) {
    const uint16_t fleetMag = eeprom.readUShort(address);
    if (fleetMag == kEepromFleetTrustMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      if (address < cap) {
        fields.fleetTrustKey = eeprom.readString(address);
        address += static_cast<int>(fields.fleetTrustKey.length() + 1);
      }
    }
  }
  // Block: kEepromTempoRteMagic
  if (address + 2 <= cap) {
    const uint16_t tempoMag = eeprom.readUShort(address);
    if (tempoMag == kEepromTempoRteMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      if (address < cap) {
        fields.tempoRteEnabled = eeprom.readByte(address) != 0;
        address += 1;
      }
      if (address < cap) {
        fields.tempoRteLtarfCache = eeprom.readString(address);
        if (fields.tempoRteLtarfCache.length() > static_cast<size_t>(kEepromTempoRteLabelMax)) {
          fields.tempoRteLtarfCache.resize(kEepromTempoRteLabelMax);
        }
        address += static_cast<int>(fields.tempoRteLtarfCache.length() + 1);
      }
      if (address < cap) {
        fields.tempoRteStgeCache = eeprom.readString(address);
        address += static_cast<int>(fields.tempoRteStgeCache.length() + 1);
      }
      if (address < cap) {
        fields.tempoRteJourCache = eeprom.readString(address);
        if (fields.tempoRteJourCache.length() > static_cast<size_t>(kEepromTempoRteLabelMax)) {
          fields.tempoRteJourCache.resize(kEepromTempoRteLabelMax);
        }
        address += static_cast<int>(fields.tempoRteJourCache.length() + 1);
      }
      if (address < cap) {
        fields.tempoRteDemainCache = eeprom.readString(address);
        if (fields.tempoRteDemainCache.length() > static_cast<size_t>(kEepromTempoRteLabelMax)) {
          fields.tempoRteDemainCache.resize(kEepromTempoRteLabelMax);
        }
        address += static_cast<int>(fields.tempoRteDemainCache.length() + 1);
      }
      if (address + 4 <= cap) {
        fields.tempoRteLastFetchEpoch = eeprom.readULong(address);
        address += static_cast<int>(sizeof(uint32_t));
      }
    }
  }
  // Block: kEepromActionsJsonMagic — load_channels JSON (gen-2 EEPROM)
  if (address + 2 <= cap) {
    const uint16_t actionsMag = eeprom.readUShort(address);
    if (actionsMag == kEepromActionsJsonMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      if (address < cap) {
        fields.actionsJson = eeprom.readString(address);
        if (fields.actionsJson.length() > static_cast<size_t>(kEepromActionsJsonMax)) {
          fields.actionsJson.resize(static_cast<size_t>(kEepromActionsJsonMax));
        }
        address += static_cast<int>(fields.actionsJson.length() + 1);
        fields.actionsJsonPresent = !fields.actionsJson.empty();
      }
    }
  }
  address = balansun_regulation_persist_read(address, eeprom, fields);
  address = balansun_ha_site_persist_read(address, eeprom, fields);
#if !defined(FLEET_BUNDLE_NATIVE_STUB)
  address = balansun_diag_persist_read(address, eeprom, g_self_test, g_triac_cal);
#endif
  // Block: kEepromStatusLedMagic
  if (address + 2 <= cap) {
    const uint16_t ledMag = eeprom.readUShort(address);
    if (ledMag == kEepromStatusLedMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      fields.statusLedPersistPresent = true;
      if (address < cap) {
        fields.statusLedMode = eeprom.readByte(address);
        if (fields.statusLedMode > 2) fields.statusLedMode = 1;
        address += 1;
      }
      if (address < cap) {
        fields.statusLedGpioActivity = static_cast<int8_t>(eeprom.readByte(address));
        address += 1;
      }
      if (address < cap) {
        fields.statusLedGpioRegulation = static_cast<int8_t>(eeprom.readByte(address));
        address += 1;
      }
      if (address < cap) {
        fields.statusLedRgbGpio = static_cast<int8_t>(eeprom.readByte(address));
        address += 1;
      }
      if (address < cap) {
        fields.statusLedActiveLow = eeprom.readByte(address) != 0;
        address += 1;
      }
      if (address < cap) {
        fields.statusLedColorActivityR = eeprom.readByte(address);
        address += 1;
      }
      if (address < cap) {
        fields.statusLedColorActivityG = eeprom.readByte(address);
        address += 1;
      }
      if (address < cap) {
        fields.statusLedColorActivityB = eeprom.readByte(address);
        address += 1;
      }
      if (address < cap) {
        fields.statusLedColorRegulationR = eeprom.readByte(address);
        address += 1;
      }
      if (address < cap) {
        fields.statusLedColorRegulationG = eeprom.readByte(address);
        address += 1;
      }
      if (address < cap) {
        fields.statusLedColorRegulationB = eeprom.readByte(address);
        address += 1;
      }
    }
  }
  // Block: kEepromStatusLedAdvMagic
  if (address + 2 <= cap) {
    const uint16_t advMag = eeprom.readUShort(address);
    if (advMag == kEepromStatusLedAdvMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      fields.statusLedAdvPersistPresent = true;
      if (address < cap) {
        fields.statusLedColorRebootR = eeprom.readByte(address);
        address += 1;
      }
      if (address < cap) {
        fields.statusLedColorRebootG = eeprom.readByte(address);
        address += 1;
      }
      if (address < cap) {
        fields.statusLedColorRebootB = eeprom.readByte(address);
        address += 1;
      }
      if (address < cap) {
        fields.statusLedColorApR = eeprom.readByte(address);
        address += 1;
      }
      if (address < cap) {
        fields.statusLedColorApG = eeprom.readByte(address);
        address += 1;
      }
      if (address < cap) {
        fields.statusLedColorApB = eeprom.readByte(address);
        address += 1;
      }
    }
  }
  // Block: kEepromTemperatureMagic
  if (address + 2 <= cap) {
    const uint16_t tempMag = eeprom.readUShort(address);
    if (tempMag == kEepromTemperatureMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      fields.temperaturePersistPresent = true;
      if (address < cap) {
        const int8_t g = static_cast<int8_t>(eeprom.readByte(address));
        fields.tempGpio = g;
        address += 1;
      }
      for (int i = 0; i < EepromExtensionFields::kTemperatureSlotStoredMax; i++) {
        if (address >= cap) break;
        fields.temperatureSlots[i].enabled = eeprom.readByte(address) != 0;
        address += 1;
        if (address >= cap) break;
        fields.temperatureSlots[i].label = eeprom.readString(address);
        if (fields.temperatureSlots[i].label.length() > kEepromTemperatureLabelMax) {
          fields.temperatureSlots[i].label =
              fields.temperatureSlots[i].label.substr(0, kEepromTemperatureLabelMax);
        }
        address += static_cast<int>(fields.temperatureSlots[i].label.length() + 1);
        if (address + 8 > cap) break;
        const uint32_t lo = eeprom.readULong(address);
        address += static_cast<int>(sizeof(uint32_t));
        const uint32_t hi = eeprom.readULong(address);
        address += static_cast<int>(sizeof(uint32_t));
        fields.temperatureSlots[i].address =
            (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
      }
    }
  }
  address = eeprom_read_legacy_load_profile_block(address, eeprom, fields);
  // Block: kEepromHttpApiTokensMagic (tail append — do not insert earlier in chain)
  if (address + 2 <= cap) {
    const uint16_t tokMag = eeprom.readUShort(address);
    if (tokMag == kEepromHttpApiTokensMagic) {
      address += static_cast<int>(sizeof(uint16_t));
      if (address < cap) {
        uint8_t n = eeprom.readByte(address);
        address += 1;
        if (n > static_cast<uint8_t>(EepromExtensionFields::kApiAccessTokenStoredMax)) {
          n = static_cast<uint8_t>(EepromExtensionFields::kApiAccessTokenStoredMax);
        }
        fields.apiAccessTokenCount = n;
        for (uint8_t i = 0; i < n && address < cap; i++) {
          if (address >= cap) break;
          fields.apiAccessTokens[i].id = eeprom.readByte(address);
          address += 1;
          if (address < cap) {
            fields.apiAccessTokens[i].label = eeprom.readString(address);
            if (fields.apiAccessTokens[i].label.length() >
                static_cast<size_t>(kEepromApiAccessTokenLabelMax)) {
              fields.apiAccessTokens[i].label.resize(static_cast<size_t>(kEepromApiAccessTokenLabelMax));
            }
            address += static_cast<int>(fields.apiAccessTokens[i].label.length() + 1);
          }
          if (address < cap) {
            fields.apiAccessTokens[i].token_hex = eeprom.readString(address);
            if (fields.apiAccessTokens[i].token_hex.length() !=
                static_cast<size_t>(kEepromApiAccessTokenSecretHexLen)) {
              fields.apiAccessTokens[i].token_hex.clear();
            }
            address += static_cast<int>(fields.apiAccessTokens[i].token_hex.length() + 1);
          }
        }
      }
    }
  }
  return address;
}

int storage_eeprom_extension_write(int address, IEepromBackend &eeprom, const EepromExtensionFields &fields,
                                   const EepromMainsHook &write_mains, bool *out_truncated) {
  // out_truncated is set when the parameter stream cannot fit in capacity and trailing
  // blocks (e.g. API tokens) would be dropped — callers surface this instead of silently
  // losing config. Every capacity-forced exit jumps to `done` with trunc=true.
  if (out_truncated) *out_truncated = false;
  bool trunc = false;
  const int cap = eeprom.capacity();
  if (address < 0 || address + 2 > cap) { trunc = true; goto done; }
  // Block: kEepromExtMagic (write order is the extension reference)
  eeprom.writeUShort(address, kEepromExtMagic);
  address += static_cast<int>(sizeof(uint16_t));
  eeprom.writeString(address, fields.pmqttTopic);
  address += static_cast<int>(fields.pmqttTopic.length() + 1);
  if (address >= cap) { trunc = true; goto done; }
  {
    std::string sch = fields.pmqttSchema.empty() ? std::string("Pw") : fields.pmqttSchema;
    eeprom.writeString(address, sch);
    address += static_cast<int>(sch.length() + 1);
  }
  if (address + 4 > cap) { trunc = true; goto done; }
  eeprom.writeULong(address, fields.jsyMk333SerialBaud);
  address += static_cast<int>(sizeof(uint32_t));
  if (address + 2 > cap) { trunc = true; goto done; }
  eeprom.writeUShort(address, kEepromPmqttBindingsMagic);
  address += static_cast<int>(sizeof(uint16_t));
  {
    std::string binds = fields.pmqttBindingsJson.empty() ? std::string("[]") : fields.pmqttBindingsJson;
    if (binds.length() > 4096) binds = binds.substr(0, 4096);
    if (!balansun_pmqtt_bindings_nvs_save(binds.c_str(), binds.length())) {
      trunc = true;
    }
    eeprom.writeString(address, "[]");
    address += 3;
  }
  // Block: kEepromExtPortPathMagic
  if (address + 2 > cap) { trunc = true; goto done; }
  eeprom.writeUShort(address, kEepromExtPortPathMagic);
  address += static_cast<int>(sizeof(uint16_t));
  if (address + 2 > cap) { trunc = true; goto done; }
  {
    unsigned int wp = fields.ext_peer_port;
    if (wp == 0 || wp > 65535u) wp = 80;
    eeprom.writeUShort(address, static_cast<uint16_t>(wp));
    address += static_cast<int>(sizeof(uint16_t));
  }
  {
    std::string pathW =
        fields.ext_peer_path.empty() ? std::string("/api/v1/measurements") : fields.ext_peer_path;
    if (pathW.length() > 48) pathW = pathW.substr(0, 48);
    eeprom.writeString(address, pathW);
    address += static_cast<int>(pathW.length() + 1);
  }
  // Block: kEepromArduinoOtaPassMagic
  if (address + 2 > cap) { trunc = true; goto done; }
  eeprom.writeUShort(address, kEepromArduinoOtaPassMagic);
  address += static_cast<int>(sizeof(uint16_t));
  {
    std::string pw = fields.arduinoOtaPassword;
    if (pw.length() > 64) pw = pw.substr(0, 64);
    eeprom.writeString(address, pw);
    address += static_cast<int>(pw.length() + 1);
  }
  // Block: mains profile hook
  if (write_mains) address = write_mains(address);
  // Block: kEepromHttpApiPassMagic
  if (address + 2 > cap) { trunc = true; goto done; }
  eeprom.writeUShort(address, kEepromHttpApiPassMagic);
  address += static_cast<int>(sizeof(uint16_t));
  {
    std::string pw = fields.httpApiPassword;
    if (pw.length() > 64) pw = pw.substr(0, 64);
    eeprom.writeString(address, pw);
    address += static_cast<int>(pw.length() + 1);
  }
  // Block: kEepromHttpCorsMagic
  if (address + 2 > cap) { trunc = true; goto done; }
  eeprom.writeUShort(address, kEepromHttpCorsMagic);
  address += static_cast<int>(sizeof(uint16_t));
  if (address < cap) {
    eeprom.writeByte(address, fields.httpCorsEnabled ? 1 : 0);
    address += 1;
  }
  // Block: kEepromPwmMagic
  if (address + 2 > cap) { trunc = true; goto done; }
  eeprom.writeUShort(address, kEepromPwmMagic);
  address += static_cast<int>(sizeof(uint16_t));
  if (address < cap) {
    int8_t g = fields.pwmGpio;
    if (g < -1) g = -1;
    eeprom.writeByte(address, static_cast<uint8_t>(g));
    address += 1;
  }
  if (address < cap) {
    uint8_t m = fields.pwmMode > 2 ? 0 : fields.pwmMode;
    eeprom.writeByte(address, m);
    address += 1;
  }
  if (address < cap) {
    uint8_t d = fields.pwmDutyPercent > 100 ? 100 : fields.pwmDutyPercent;
    eeprom.writeByte(address, d);
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.pwmInverted ? 1 : 0);
    address += 1;
  }
  // Block: kEepromFleetTrustMagic
  if (address + 2 > cap) { trunc = true; goto done; }
  eeprom.writeUShort(address, kEepromFleetTrustMagic);
  address += static_cast<int>(sizeof(uint16_t));
  if (address < cap) {
    std::string key = fields.fleetTrustKey;
    if (key.length() > 64) key = key.substr(0, 64);
    eeprom.writeString(address, key);
    address += static_cast<int>(key.length() + 1);
  }
  // Block: kEepromTempoRteMagic
  if (address + 2 > cap) { trunc = true; goto done; }
  eeprom.writeUShort(address, kEepromTempoRteMagic);
  address += static_cast<int>(sizeof(uint16_t));
  if (address < cap) {
    eeprom.writeByte(address, fields.tempoRteEnabled ? 1 : 0);
    address += 1;
  }
  if (address < cap) {
    std::string lt = fields.tempoRteLtarfCache;
    if (lt.length() > static_cast<size_t>(kEepromTempoRteLabelMax)) lt = lt.substr(0, kEepromTempoRteLabelMax);
    eeprom.writeString(address, lt);
    address += static_cast<int>(lt.length() + 1);
  }
  if (address < cap) {
    std::string st = fields.tempoRteStgeCache;
    if (st.length() > 4) st = st.substr(0, 4);
    eeprom.writeString(address, st);
    address += static_cast<int>(st.length() + 1);
  }
  if (address < cap) {
    std::string j = fields.tempoRteJourCache;
    if (j.length() > static_cast<size_t>(kEepromTempoRteLabelMax)) j = j.substr(0, kEepromTempoRteLabelMax);
    eeprom.writeString(address, j);
    address += static_cast<int>(j.length() + 1);
  }
  if (address < cap) {
    std::string d = fields.tempoRteDemainCache;
    if (d.length() > static_cast<size_t>(kEepromTempoRteLabelMax)) d = d.substr(0, kEepromTempoRteLabelMax);
    eeprom.writeString(address, d);
    address += static_cast<int>(d.length() + 1);
  }
  if (address + 4 <= cap) {
    eeprom.writeULong(address, fields.tempoRteLastFetchEpoch);
    address += static_cast<int>(sizeof(uint32_t));
  }
  // Block: kEepromActionsJsonMagic
  if (address + 2 <= cap && fields.actionsJsonPresent && !fields.actionsJson.empty()) {
    std::string json = fields.actionsJson;
    if (json.length() > static_cast<size_t>(kEepromActionsJsonMax)) {
      json = json.substr(0, static_cast<size_t>(kEepromActionsJsonMax));
    }
    eeprom.writeUShort(address, kEepromActionsJsonMagic);
    address += static_cast<int>(sizeof(uint16_t));
    if (address < cap) {
      // Clamp to remaining capacity (reserve 1 byte for the trailing NUL) so the address
      // never runs past the backing store; report truncation if the JSON had to be cut.
      const int remaining = cap - address - 1;
      if (remaining < static_cast<int>(json.length())) {
        json = json.substr(0, remaining > 0 ? static_cast<size_t>(remaining) : 0);
        trunc = true;
      }
      eeprom.writeString(address, json);
      address += static_cast<int>(json.length() + 1);
    }
  }
  address = balansun_regulation_persist_write(address, eeprom, fields);
  address = balansun_ha_site_persist_write(address, eeprom, fields);
#if !defined(FLEET_BUNDLE_NATIVE_STUB)
  address = balansun_diag_persist_write(address, eeprom, g_self_test, g_triac_cal);
#endif
  // Block: kEepromStatusLedMagic
  if (address + 2 > cap) {
    trunc = true;
    goto done;
  }
  eeprom.writeUShort(address, kEepromStatusLedMagic);
  address += static_cast<int>(sizeof(uint16_t));
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedMode > 2 ? 1 : fields.statusLedMode);
    address += 1;
  }
  if (address < cap) {
    int8_t g = fields.statusLedGpioActivity;
    eeprom.writeByte(address, static_cast<uint8_t>(g));
    address += 1;
  }
  if (address < cap) {
    int8_t g = fields.statusLedGpioRegulation;
    eeprom.writeByte(address, static_cast<uint8_t>(g));
    address += 1;
  }
  if (address < cap) {
    int8_t g = fields.statusLedRgbGpio;
    eeprom.writeByte(address, static_cast<uint8_t>(g));
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedActiveLow ? 1 : 0);
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedColorActivityR);
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedColorActivityG);
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedColorActivityB);
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedColorRegulationR);
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedColorRegulationG);
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedColorRegulationB);
    address += 1;
  }
  // Block: kEepromStatusLedAdvMagic
  if (address + 2 > cap) {
    trunc = true;
    goto done;
  }
  eeprom.writeUShort(address, kEepromStatusLedAdvMagic);
  address += static_cast<int>(sizeof(uint16_t));
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedColorRebootR);
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedColorRebootG);
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedColorRebootB);
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedColorApR);
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedColorApG);
    address += 1;
  }
  if (address < cap) {
    eeprom.writeByte(address, fields.statusLedColorApB);
    address += 1;
  }
  // Block: kEepromTemperatureMagic
  if (address + 2 > cap) {
    trunc = true;
    goto done;
  }
  eeprom.writeUShort(address, kEepromTemperatureMagic);
  address += static_cast<int>(sizeof(uint16_t));
  if (address < cap) {
    int8_t g = fields.tempGpio;
    if (!balansun_temp_logic_is_allowed_gpio(g)) g = static_cast<int8_t>(pinTemp);
    eeprom.writeByte(address, static_cast<uint8_t>(g));
    address += 1;
  }
  for (int i = 0; i < EepromExtensionFields::kTemperatureSlotStoredMax; i++) {
    if (address >= cap) {
      trunc = true;
      goto done;
    }
    eeprom.writeByte(address, fields.temperatureSlots[i].enabled ? 1 : 0);
    address += 1;
    if (address >= cap) {
      trunc = true;
      goto done;
    }
    std::string lbl = fields.temperatureSlots[i].label;
    if (lbl.length() > kEepromTemperatureLabelMax) lbl = lbl.substr(0, kEepromTemperatureLabelMax);
    eeprom.writeString(address, lbl);
    address += static_cast<int>(lbl.length() + 1);
    if (address + 8 > cap) {
      trunc = true;
      goto done;
    }
    const uint64_t addr = fields.temperatureSlots[i].address;
    eeprom.writeULong(address, static_cast<uint32_t>(addr & 0xFFFFFFFFu));
    address += static_cast<int>(sizeof(uint32_t));
    eeprom.writeULong(address, static_cast<uint32_t>((addr >> 32) & 0xFFFFFFFFu));
    address += static_cast<int>(sizeof(uint32_t));
  }
  // Block: kEepromHttpApiTokensMagic (tail append). Wrapped in a scope so its locals do
  // not stay live at `done:` (goto must not cross an in-scope initialization).
  {
    if (address + 2 > cap) { trunc = true; goto done; }
    eeprom.writeUShort(address, kEepromHttpApiTokensMagic);
    address += static_cast<int>(sizeof(uint16_t));
    if (address >= cap) { trunc = true; goto done; }
    uint8_t n = fields.apiAccessTokenCount;
    if (n > static_cast<uint8_t>(EepromExtensionFields::kApiAccessTokenStoredMax)) {
      n = static_cast<uint8_t>(EepromExtensionFields::kApiAccessTokenStoredMax);
    }
    eeprom.writeByte(address, n);
    address += 1;
    for (uint8_t i = 0; i < n; i++) {
      if (address >= cap) { trunc = true; goto done; }
      eeprom.writeByte(address, fields.apiAccessTokens[i].id);
      address += 1;
      if (address >= cap) { trunc = true; goto done; }
      std::string lbl = fields.apiAccessTokens[i].label;
      if (lbl.length() > static_cast<size_t>(kEepromApiAccessTokenLabelMax)) {
        lbl = lbl.substr(0, static_cast<size_t>(kEepromApiAccessTokenLabelMax));
      }
      eeprom.writeString(address, lbl);
      address += static_cast<int>(lbl.length() + 1);
      if (address >= cap) { trunc = true; goto done; }
      std::string hx = fields.apiAccessTokens[i].token_hex;
      if (hx.length() != static_cast<size_t>(kEepromApiAccessTokenSecretHexLen)) continue;
      eeprom.writeString(address, hx);
      address += static_cast<int>(hx.length() + 1);
    }
  }
done:
  if (out_truncated) *out_truncated = trunc;
  return address;
}
