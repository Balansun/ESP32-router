#pragma once

#include <cstdint>

constexpr uint32_t kDs18ConversionMs = 800;

struct Ds18PollState {
  uint32_t last_request_ms = 0;
  bool pending = false;
};

void ds18_poll_begin_request(Ds18PollState &state, uint32_t now_ms);

bool ds18_poll_conversion_ready(const Ds18PollState &state, uint32_t now_ms,
                                  uint32_t conversion_ms = kDs18ConversionMs);

void ds18_poll_mark_read(Ds18PollState &state);
