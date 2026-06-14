#include "balansun_meter_logic.h"

static void merge_channel(MeterChannelState &dst, const MeterChannelState &src) {
  dst = src;
}

bool balansun_meter_logic_apply_fields(RmsRuntime &rt, const MeterSnapshotFields &fields, std::string *errOut) {
  bool any = false;
  if (fields.has_house) {
    merge_channel(rt.house, fields.house);
    any = true;
  }
  if (fields.has_second) {
    merge_channel(rt.second, fields.second);
    any = true;
  }
  if (fields.has_raw) {
    rt.raw = fields.raw;
    any = true;
  }
  if (!any) {
    if (errOut) *errOut = "no_meter_fields";
    return false;
  }
  rt.energie_active_valide = true;
  return true;
}
