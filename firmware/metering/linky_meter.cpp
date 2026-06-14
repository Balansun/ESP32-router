/*
 * linky_meter.cpp — Source Linky: TIC UART on Serial2, Enedis frame decode.
 * See: /en/hardware-pinout/ — source_linky; GUIDE A.4.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_LINKY
#include "linky_meter.h"

#include "balansun_globals.h"
#include "balansun_source.h"

float deltaWS = 0;
float deltaWI = 0;

void LinkyMeter::setup() {
  Serial2.begin(9600, SERIAL_7E1, g_pins.uart_rx, g_pins.uart_tx);
}

void LinkyMeter::poll() {
  pollTransport();
}

bool LinkyMeter::pollTransport() {
  int v = 0;
  long old_wh = 0;
  float delta_wh = 0;
  float pmax = 0;
  float pmin = 0;
  unsigned long tm = 0;
  float delta_t = 0;
  while (Serial2.available() > 0) {
    v = Serial2.read();
    DataRawLinky[IdxDataRawLinky] = char(v);
    IdxDataRawLinky = (IdxDataRawLinky + 1) % 10000;
    switch (v) {
      case 2:
        break;
      case 3:
        previousETX = millis();
        cptLEDyellow = 4;
        LFon = false;
        break;
      case 10:
        LFon = true;
        IdxBufDecodLinky = IdxDataRawLinky;
        break;
      case 13:
        if (LFon) {
          LFon = false;
          int nb_tab = 0;
          String code = "";
          String val = "";
          int checksum = 0;
          int check_linky = -1;

          while (IdxBufDecodLinky != IdxDataRawLinky) {
            if (DataRawLinky[IdxBufDecodLinky] == char(9)) {
              nb_tab++;
            } else {
              if (nb_tab == 0) {
                code += DataRawLinky[IdxBufDecodLinky];
              }
              if (nb_tab == 1) {
                val += DataRawLinky[IdxBufDecodLinky];
              }
              if (nb_tab <= 1) {
                checksum += (int)DataRawLinky[IdxBufDecodLinky];
              }
            }
            IdxBufDecodLinky = (IdxBufDecodLinky + 1) % 10000;
            if (check_linky == -1 && nb_tab == 2) {
              check_linky = (int)DataRawLinky[IdxBufDecodLinky];
              checksum += 18;
              checksum = checksum & 63;
              checksum = checksum + 32;
            }
          }
          if (code.indexOf("EAST") == 0 || code.indexOf("EAIT") == 0 || code == "SINSTS" ||
              code.indexOf("SINSTI") == 0) {
            if (checksum != check_linky) {
              Debug.println("Erreur checksum code : " + code + " " + String(checksum) + "," + String(check_linky));
              Serial.println("Erreur checksum code : " + code + " " + String(checksum) + "," + String(check_linky));
            }
          }
          if (code.indexOf("EAST") == 0) {
            old_wh = house_energy_import_wh;
            if (old_wh == 0) {
              old_wh = val.toInt();
            }
            house_energy_import_wh = val.toInt();
            tm = millis();
            delta_t = float(tm - TlastEASTvalide);
            delta_t = delta_t / float(3600000);
            if (house_energy_import_wh == old_wh) {
              pmax = 1.3 / delta_t;
              moyPWS = min(moyPWS, pmax);
            } else {
              TlastEASTvalide = tm;
              delta_wh = float(house_energy_import_wh - old_wh);
              deltaWS = delta_wh / delta_t;
              pmin = (delta_wh - 1) / delta_t;
              moyPWS = max(moyPWS, pmin);
            }
            moyPWS = 0.05 * deltaWS + 0.95 * moyPWS;
            EASTvalid = true;
            if (!EAITvalid && tm > 8000) {
              EAITvalid = true;
            }
          }
          if (code.indexOf("EAIT") == 0) {
            LinkyEaitFromTic = true;
            old_wh = house_energy_export_wh;
            if (old_wh == 0) {
              old_wh = val.toInt();
            }
            house_energy_export_wh = val.toInt();
            tm = millis();
            delta_t = float(tm - TlastEAITvalide);
            delta_t = delta_t / float(3600000);
            if (house_energy_export_wh == old_wh) {
              pmax = 1.3 / delta_t;
              moyPWI = min(moyPWI, pmax);
            } else {
              TlastEAITvalide = tm;
              delta_wh = float(house_energy_export_wh - old_wh);
              deltaWI = delta_wh / delta_t;
              pmin = (delta_wh - 1) / delta_t;
              moyPWI = max(moyPWI, pmin);
            }
            moyPWI = 0.05 * deltaWI + 0.95 * moyPWI;
            EAITvalid = true;
          }
          if (EASTvalid && EAITvalid) {
            meter_reading_valid = true;
          }
          if (code == "SINSTS") {
            house_apparent_import_va = val.toInt();
            moyPVAS = 0.05 * float(house_apparent_import_va) + 0.95 * moyPVAS;
            moyPWS = min(moyPWS, moyPVAS);
            if (moyPVAS > 0) {
              COSphiS = moyPWS / moyPVAS;
              COSphiS = min(float(1.0), COSphiS);
              house_power_factor = COSphiS;
            }
            house_active_import_w = int(COSphiS * float(house_apparent_import_va));
          }
          if (code.indexOf("SINSTI") == 0) {
            LinkySinstiSeen = true;
            house_apparent_export_va = val.toInt();
            moyPVAI = 0.05 * float(house_apparent_export_va) + 0.95 * moyPVAI;
            moyPWI = min(moyPWI, moyPVAI);
            if (moyPVAI > 0) {
              COSphiI = moyPWI / moyPVAI;
              COSphiI = min(float(1.0), COSphiI);
              house_power_factor = COSphiI;
            }
            house_active_export_w = int(COSphiI * float(house_apparent_export_va));
          }
          if (code.indexOf("DATE") == 0) {
            esp_task_wdt_reset();
          }
          if (code.indexOf("URMS1") == 0) {
            house_voltage_v = val.toFloat();
          }
          if (code.indexOf("IRMS1") == 0) {
            house_current_a = val.toFloat();
          }
          if (code.indexOf("STGE") == 0) {
            String stge = val;
            stge.trim();
            if (!tempoRteEnabled || balansun_active_source_get() == SourceId::Linky) {
              if (stge.length() >= 2) {
                STGEt = stge.substring(1, 2);
              } else {
                STGEt = stge;
              }
            }
          }
          if (!tempoRteEnabled || balansun_active_source_get() == SourceId::Linky) {
            if (code.indexOf("linky_ltarf") == 0) {
              LTARF = val;
              LTARF.trim();
            }
          }
        }
        break;
      default:
        break;
    }
  }
  return true;
}

void LinkyMeter::appendDiagnostics(JsonObject doc, int linky_tail_max) {
  JsonObject ly = doc["linky"].to<JsonObject>();
  ly["ltarf"] = LTARF;
  ly["idx_raw"] = IdxDataRawLinky;
  int tailMax = linky_tail_max;
  if (tailMax <= 0 || tailMax > 2048) {
    tailMax = 768;
  }
  const int idx = IdxDataRawLinky;
  const int start = (idx - tailMax + 10000) % 10000;
  String tail;
  tail.reserve((unsigned)tailMax);
  int k = start;
  for (int i = 0; i < tailMax; i++) {
    char c = DataRawLinky[k];
    if (c == 0) {
      c = ' ';
    }
    if ((unsigned char)c < 32) {
      c = '.';
    }
    tail += c;
    k = (k + 1) % 10000;
  }
  ly["tail"] = tail;
  ly["tail_len"] = tailMax;
  ly["linky_eait_from_tic"] = LinkyEaitFromTic;
  ly["linky_sinsti_seen"] = LinkySinstiSeen;
  ly["cacsi_no_export"] = (!LinkySinstiSeen && !LinkyEaitFromTic && EASTvalid);
}

IMeterDriver *balansun_meter_instance_linky() {
  static LinkyMeter instance;
  return &instance;
}

#endif /* BALANSUN_ENABLE_SOURCE_LINKY */
