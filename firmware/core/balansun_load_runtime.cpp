/* balansun_load_runtime.cpp — GPIO/PWM/HTTP runtime for load channels (Action). */
#include <Arduino.h>
#include <WiFiClient.h>

#include "Actions.h"
#include "actions_logic.h"
#include "balansun_regulation_modes.h"

void Action::InitGpio() {
  int p;
  int q;
  String S;

  if (Host == "localhost" && Idx > 0) {
    p = path_on.indexOf("gpio=");
    if (p >= 0) {
      S = path_on.substring(p + 5);
      q = S.indexOf("&");
      if (q == -1) q = 2;
      GpioOn = S.substring(0, q).toInt();
      pinMode(GpioOn, OUTPUT);
      OutOn = 1 + path_on.indexOf("out=1");
      OutOn = min(OutOn, 1);
      if (path_off.indexOf("init") >= 0) On = true;
      if (path_on.indexOf("init=0") >= 0) digitalWrite(GpioOn, 0);
      if (path_on.indexOf("init=1") >= 0) digitalWrite(GpioOn, 1);
    }
    p = path_off.indexOf("gpio=");
    if (p >= 0) {
      S = path_off.substring(p + 5);
      q = S.indexOf("&");
      if (q == -1) q = 2;
      GpioOff = S.substring(0, q).toInt();
      pinMode(GpioOff, OUTPUT);
      OutOff = 1 + path_off.indexOf("out=1");
      OutOff = min(OutOff, 1);
      if (path_off.indexOf("init") >= 0) On = false;
      if (path_off.indexOf("init=0") >= 0) digitalWrite(GpioOff, 0);
      if (path_off.indexOf("init=1") >= 0) digitalWrite(GpioOff, 1);
    }
    valide = true;
    if (GpioOff < 0 || GpioOff > 33 || GpioOn < 0 || GpioOn > 33) valide = false;
    if (Actif == kModePwm && GpioOn >= 0) {
      ledcSetup(Idx + 2, 4000, 8);
      ledcAttachPin((uint8_t)GpioOn, Idx + 2);
    }
  }
}

void Action::relay_on_from_triac_delay(int triac_delay_percent_percent) {
  if (!valide || Host != "localhost" || GpioOn < 0) return;
  if (triac_delay_percent_percent >= 100) {
    digitalWrite(GpioOff, OutOff);
    On = false;
    return;
  }
  digitalWrite(GpioOn, OutOn);
  On = true;
}

void Action::apply_pwm_from_triac_delay_f(float triac_delay_percent_f) {
  if (!valide || GpioOn < 0) return;
  int vout = int(triac_delay_percent_f * 2.55f);
  if (OutOn == 1) vout = 255 - vout;
  if (vout < 0) vout = 0;
  if (vout > 255) vout = 255;
  ledcWrite(GpioOn, vout);
  On = vout > 0;
}

void Action::tick_half_sine(bool phase_230v, uint8_t pulse_on, uint8_t &pulse_total, int &pulseCounter) {
  if (!valide || GpioOn < 0) return;
  pulseCounter += pulse_on;
  if (((phase_230v && pulse_total == 0) || (!phase_230v && pulse_total == 1)) && pulseCounter >= 100) {
    pulse_total = phase_230v ? 1 : 0;
    digitalWrite(GpioOn, OutOn);
    pulseCounter -= 100;
  } else {
    digitalWrite(GpioOff, OutOff);
  }
}

void Action::tick_pulse_train(uint8_t pulse_on, uint8_t pulse_total, int &pulseCounter) {
  if (!valide || GpioOn < 0) return;
  if (pulseCounter >= pulse_total) pulseCounter = 0;
  if (pulseCounter < static_cast<int>(pulse_on)) {
    digitalWrite(GpioOn, OutOn);
    On = true;
  } else {
    digitalWrite(GpioOff, OutOff);
    On = false;
  }
  pulseCounter++;
  if (pulseCounter >= static_cast<int>(pulse_total)) pulseCounter = 0;
}

void Action::StopOutputs() {
  if (Host != "localhost" && On) {
    CallExterne(Host, path_off, Port);
  }
  On = false;
  if (Host == "localhost") {
    if (GpioOff >= 0) digitalWrite(GpioOff, OutOff);
    if (GpioOn >= 0) digitalWrite(GpioOn, LOW);
  }
}

void Action::CallExterne(String host, String url, int port) {
  WiFiClient clientExt;
  char hostbuf[host.length() + 1];
  host.toCharArray(hostbuf, host.length() + 1);

  if (!clientExt.connect(hostbuf, port)) {
    Serial.println("HTTP relay connect failed: " + host);
    return;
  }
  clientExt.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (clientExt.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println("HTTP relay timeout: " + host);
      clientExt.stop();
      return;
    }
  }

  while (clientExt.available()) {
    (void)clientExt.readStringUntil('\r');
  }
}
