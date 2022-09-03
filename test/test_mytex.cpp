#include "baudvine/mytex.h"

#include <gtest/gtest.h>

#include <thread>

TEST(Mytex, BasicCreateLockDestroy) {
  baudvine::Mytex<int> underTest(5);
  EXPECT_EQ(*underTest.Lock(), 5);
  *underTest.Lock() = 6;
  EXPECT_EQ(*underTest.Lock(), 6);
}

TEST(Mytex, TryLock) {
  baudvine::Mytex<int> underTest(6);
  {
    auto number = underTest.TryLock();
    EXPECT_TRUE(number.has_value());
    EXPECT_EQ(number.value(), 6);
    EXPECT_EQ(*number, 6);

    EXPECT_FALSE(underTest.TryLock().has_value());
    std::thread([&underTest] {
      EXPECT_FALSE(underTest.TryLock().has_value());
    }).join();
  }
  EXPECT_TRUE(underTest.TryLock().has_value());
}