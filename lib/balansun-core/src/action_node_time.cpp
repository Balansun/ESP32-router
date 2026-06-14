#include <balansun/action_node_time.h>

namespace {

constexpr int kMinValidYear = 2020;

}  // namespace

bool balansun_time_is_valid(time_t now) {
  if (now <= 0) return false;
  struct tm local_tm {};
#if defined(_WIN32)
  localtime_s(&local_tm, &now);
#else
  localtime_r(&now, &local_tm);
#endif
  return local_tm.tm_year + 1900 >= kMinValidYear;
}

int balansun_wall_decihours_from_hm(int hour, int minute) {
  if (hour < 0) hour = 0;
  if (hour > 23) hour = 23;
  if (minute < 0) minute = 0;
  if (minute > 59) minute = 59;
  return hour * 100 + minute;
}

int balansun_wall_decihours_from_time(time_t now, int gmt_offset_sec) {
  if (!balansun_time_is_valid(now)) return 0;
  const time_t local = now + gmt_offset_sec;
  struct tm local_tm {};
#if defined(_WIN32)
  gmtime_s(&local_tm, &local);
#else
  gmtime_r(&local, &local_tm);
#endif
  return balansun_wall_decihours_from_hm(local_tm.tm_hour, local_tm.tm_min);
}
