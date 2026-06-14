#include "balansun_pin_logic.h"

namespace {

bool is_reserved(int gpio, bool s3) {
  if (gpio < 0) return true;
  if (gpio > 48) return true;
  if (!s3) return false;
  if (gpio >= 26 && gpio <= 37) return true;
  if (gpio >= 6 && gpio <= 11) return true;
  return false;
}

bool is_strapping_forbidden_output(int gpio) {
  return gpio == 0 || gpio == 2 || gpio == 12 || gpio == 15;
}

bool is_input_only_classic(int gpio) { return gpio >= 34 && gpio <= 39; }

bool is_adc_capable(int gpio, bool s3) {
  if (s3) {
    if (gpio < 1) return false;
    return gpio <= 10;
  }
  if (gpio < 32) return false;
  return gpio <= 39;
}

bool map_has_duplicate_gpio(const int8_t *vals, int n) {
  for (int a = 0; a < n; a++) {
    if (vals[a] < 0) continue;
    for (int b = a + 1; b < n; b++) {
      if (vals[b] < 0) continue;
      if (vals[b] == vals[a]) return true;
    }
  }
  return false;
}

}  // namespace

bool balansun_pin_logic_is_allowed(BalansunPinFunction fn, int gpio, bool target_s3) {
  if (fn == BalansunPinFunction::Temp && gpio == kBalansunPinTempDisabled) return true;
  if (gpio == kBalansunPinTempDisabled) return false;
  if (is_reserved(gpio, target_s3)) return false;

  switch (fn) {
    case BalansunPinFunction::TriacDim:
      if (is_input_only_classic(gpio)) return false;
      if (is_strapping_forbidden_output(gpio)) return false;
      return true;
    case BalansunPinFunction::ZeroCross:
      return true;
    case BalansunPinFunction::UartRx:
    case BalansunPinFunction::UartTx:
      return !is_strapping_forbidden_output(gpio);
    case BalansunPinFunction::Temp:
      return true;
    case BalansunPinFunction::Analog0:
    case BalansunPinFunction::Analog1:
    case BalansunPinFunction::Analog2:
      return is_adc_capable(gpio, target_s3);
    default:
      return false;
  }
}

bool balansun_pin_map_validate(const BalansunPinMap &map, bool target_s3, std::string &err) {
  const int8_t vals[] = {map.triac_dim, map.zero_cross, map.uart_rx, map.uart_tx,
                         map.temp,      map.analog0,    map.analog1, map.analog2};
  const BalansunPinFunction fns[] = {BalansunPinFunction::TriacDim,  BalansunPinFunction::ZeroCross,
                                  BalansunPinFunction::UartRx,  BalansunPinFunction::UartTx,
                                  BalansunPinFunction::Temp,    BalansunPinFunction::Analog0,
                                  BalansunPinFunction::Analog1, BalansunPinFunction::Analog2};

  for (int i = 0; i < 8; i++) {
    if (!balansun_pin_logic_is_allowed(fns[i], vals[i], target_s3)) {
      err = "pin assignment not allowed for this GPIO or target";
      return false;
    }
  }
  if (map.uart_rx == map.uart_tx) {
    err = "uart_rx and uart_tx must differ";
    return false;
  }
  if (map_has_duplicate_gpio(vals, 8)) {
    err = "duplicate GPIO in pin map";
    return false;
  }
  return true;
}
