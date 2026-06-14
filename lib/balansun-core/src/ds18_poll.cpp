#include <balansun/ds18_poll.h>

void ds18_poll_begin_request(Ds18PollState &state, uint32_t now_ms) {
  state.last_request_ms = now_ms;
  state.pending = true;
}

bool ds18_poll_conversion_ready(const Ds18PollState &state, uint32_t now_ms, uint32_t conversion_ms) {
  if (!state.pending) return false;
  return static_cast<int32_t>(now_ms - state.last_request_ms) >= static_cast<int32_t>(conversion_ms);
}

void ds18_poll_mark_read(Ds18PollState &state) { state.pending = false; }
