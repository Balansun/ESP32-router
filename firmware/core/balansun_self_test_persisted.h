#pragma once

#include <cstdint>

struct SelfTestPersisted {
  bool pending = false;
  bool skipped = false;
  bool zc_ok = false;
  bool triac_ok = false;
  bool source_ok = false;
  uint32_t run_epoch = 0;
  uint16_t zc_edges_per_sec = 0;
};
