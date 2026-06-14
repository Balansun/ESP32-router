#include <balansun/triac_calibration.h>

#include <algorithm>

namespace {
int lerp(int x0, int y0, int x1, int y1, int x) {
  if (x1 == x0) return y0;
  return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}
}  // namespace

int balansun_triac_calibration_apply_open_percent(const TriacCalibrationTable &cal, int open_percent) {
  if (!cal.enabled) return open_percent;
  const int op = std::max(0, std::min(100, open_percent));
  const TriacCalPoint &p0 = cal.points[0];
  const TriacCalPoint &p1 = cal.points[1];
  const TriacCalPoint &p2 = cal.points[2];
  if (p0.duty_pct == 0 && p1.duty_pct == 0 && p2.duty_pct == 0) {
    return op;
  }
  int target_w = 0;
  if (op <= (int)p1.duty_pct) {
    target_w = lerp(p0.duty_pct, p0.measured_w, p1.duty_pct, p1.measured_w, op);
  } else {
    target_w = lerp(p1.duty_pct, p1.measured_w, p2.duty_pct, p2.measured_w, op);
  }
  if (target_w <= 0) return op;
  const int nominal_w = std::max(1, (int)p1.measured_w);
  int corrected = (op * nominal_w) / target_w;
  return std::max(0, std::min(100, corrected));
}
