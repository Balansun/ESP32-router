/*
 * linky_meter.cpp — Source Linky: TIC UART on Serial2, Enedis frame decode.
 * See: /en/hardware-pinout/ — source_linky; GUIDE A.4.
 */
#include "balansun_meter_sources_enable.h"

#if BALANSUN_ENABLE_SOURCE_LINKY
#include "balansun_globals.h"
#include "balansun_source.h"
// ****************************
// * LINKY metering source (TIC serial) *
// ****************************

float deltaWS=0;
float deltaWI=0;
void linky_meter_setup(){
  Serial2.begin(9600, SERIAL_7E1, g_pins.uart_rx, g_pins.uart_tx);  // Linky TIC: 7E1
}

void linky_meter_poll() {  // Read Linky TIC serial port
  int V = 0;
  long OldWh = 0;
  float deltaWh = 0;
  float Pmax = 0;
  float Pmin = 0;
  unsigned long Tm = 0;
  float deltaT = 0;
  while (Serial2.available() > 0) {
    V = Serial2.read();
    DataRawLinky[IdxDataRawLinky] = char(V);
    IdxDataRawLinky = (IdxDataRawLinky + 1) % 10000;
    switch (V) {
      case 2:  //STX (Start Text)
        break;
      case 3:  //ETX (End Text)
        previousETX = millis();
        cptLEDyellow = 4;
        LFon = false;
        break;
      case 10:  // Line Feed. Debut Groupe
        LFon = true;
        IdxBufDecodLinky = IdxDataRawLinky;
        break;
      case 13:       // Line Feed. Debut Groupe
        if (LFon) {  //Debut groupe OK
          LFon = false;
          int nb_tab = 0;
          String code = "";
          String val = "";
          int checksum = 0;
          int checkLinky = -1;
          
          while (IdxBufDecodLinky != IdxDataRawLinky) {
            if (DataRawLinky[IdxBufDecodLinky] == char(9)) {  //Tabulation
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
            if (checkLinky == -1 && nb_tab == 2) {
              checkLinky = (int)DataRawLinky[IdxBufDecodLinky];
              checksum += 18;            //2 tabulations
              checksum = checksum & 63;  //0x3F
              checksum = checksum + 32;  //0x20
            }
          }
          if (code.indexOf("EAST") == 0 || code.indexOf("EAIT") == 0 || code=="SINSTS" || code.indexOf("SINSTI") == 0) {           
            if (checksum != checkLinky) {
              Debug.println("Erreur checksum code : " + code + " " + String(checksum) + "," + String(checkLinky));
              Serial.println("Erreur checksum code : " + code + " " + String(checksum) + "," + String(checkLinky));
            }
          }
          if (code.indexOf("EAST") == 0) {
           
            OldWh = house_energy_import_wh;
            if (OldWh == 0) { OldWh = val.toInt(); }
            house_energy_import_wh = val.toInt();
            Tm = millis();
            deltaT = float(Tm - TlastEASTvalide);
            deltaT = deltaT / float(3600000);
            if (house_energy_import_wh == OldWh) {  // no Wh delta
              Pmax = 1.3 / deltaT;
              moyPWS = min(moyPWS, Pmax);
            } else {
              TlastEASTvalide = Tm;
              deltaWh = float(house_energy_import_wh - OldWh);
              deltaWS=deltaWh / deltaT;
              Pmin=(deltaWh-1) /deltaT;
              moyPWS = max(moyPWS, Pmin);  // power ramp-up step
            }
            moyPWS = 0.05 * deltaWS + 0.95 * moyPWS;
            EASTvalid = true;
            if (!EAITvalid && Tm > 8000) {  // CACSI: EAIT may never be set
              EAITvalid=true;
            }
          }
          if (code.indexOf("EAIT") == 0) {
            LinkyEaitFromTic = true;
            OldWh = house_energy_export_wh;
            if (OldWh == 0) { OldWh = val.toInt(); }
            house_energy_export_wh = val.toInt();
            Tm = millis();
            deltaT = float(Tm - TlastEAITvalide);
            deltaT = deltaT / float(3600000);
            if (house_energy_export_wh == OldWh) {  //Pas de resultat en Wh
              Pmax = 1.3 / deltaT;
              moyPWI = min(moyPWI, Pmax);
            } else {
              TlastEAITvalide = Tm;
              deltaWh = float(house_energy_export_wh - OldWh);
              deltaWI=deltaWh / deltaT;
              Pmin=(deltaWh-1) /deltaT;
              moyPWI = max(moyPWI, Pmin);  // power ramp-up step
            }
            moyPWI = 0.05 * deltaWI + 0.95 * moyPWI;
            EAITvalid = true;
          }
          if ( EASTvalid && EAITvalid) {
            meter_reading_valid=true;
          }
          if (code == "SINSTS") {  // apparent import; exact match avoids SINSTS1 (three-phase)
            house_apparent_import_va = val.toInt();
            moyPVAS = 0.05 * float(house_apparent_import_va) + 0.95 * moyPVAS;
            moyPWS=min(moyPWS,moyPVAS);
            if (moyPVAS > 0) {
              COSphiS = moyPWS / moyPVAS;
              COSphiS = min(float(1.0), COSphiS);
              house_power_factor=COSphiS;
            }
            house_active_import_w = int(COSphiS * float(house_apparent_import_va));
          }
          if (code.indexOf("SINSTI") == 0) {  // apparent export
            LinkySinstiSeen = true;
            house_apparent_export_va = val.toInt();
            moyPVAI = 0.05 * float(house_apparent_export_va) + 0.95 * moyPVAI;
            moyPWI=min(moyPWI,moyPVAI);
            if (moyPVAI > 0) {
              COSphiI = moyPWI / moyPVAI;
              COSphiI = min(float(1.0), COSphiI);
              house_power_factor=COSphiI;
            }
            house_active_export_w = int(COSphiI * float(house_apparent_export_va));
          }
          if (code.indexOf("DATE") == 0) {
            esp_task_wdt_reset();  // WDT reset on each Linky frame
          }
          if (code.indexOf("URMS1") == 0) {
            house_voltage_v=val.toFloat();  //phase 1 uniquement
          }
          if (code.indexOf("IRMS1") == 0) {
            house_current_a=val.toFloat();  //Phase 1 uniquement
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
}
#endif /* BALANSUN_ENABLE_SOURCE_LINKY */
