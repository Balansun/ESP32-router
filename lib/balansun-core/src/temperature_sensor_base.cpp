#include <balansun/temperature_sensor_iface.h>

bool TemperatureSensorBase::isValidC(float c) {
  return c > -100.0f && c < 125.0f;
}
