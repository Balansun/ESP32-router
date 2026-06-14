#include <gtest/gtest.h>

#include <balansun/ds18_poll.h>

TEST(Ds18Poll, NotReadyImmediatelyAfterRequest) {
  Ds18PollState state{};
  ds18_poll_begin_request(state, 1000);
  EXPECT_FALSE(ds18_poll_conversion_ready(state, 1000));
  EXPECT_FALSE(ds18_poll_conversion_ready(state, 1799));
}

TEST(Ds18Poll, ReadyAfterConversionWindow) {
  Ds18PollState state{};
  ds18_poll_begin_request(state, 1000);
  EXPECT_TRUE(ds18_poll_conversion_ready(state, 1800));
}

TEST(Ds18Poll, MarkReadClearsPending) {
  Ds18PollState state{};
  ds18_poll_begin_request(state, 1000);
  ds18_poll_mark_read(state);
  EXPECT_FALSE(ds18_poll_conversion_ready(state, 5000));
}

TEST(Ds18Poll, ReRequestResetsWindow) {
  Ds18PollState state{};
  ds18_poll_begin_request(state, 1000);
  ds18_poll_begin_request(state, 2000);
  EXPECT_FALSE(ds18_poll_conversion_ready(state, 2799));
  EXPECT_TRUE(ds18_poll_conversion_ready(state, 2800));
}
