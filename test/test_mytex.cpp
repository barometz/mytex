#include "baudvine/mytex.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <thread>

TEST(Mytex, DefaultCtor) {
  baudvine::Mytex<int> underTest;
  EXPECT_EQ(*underTest.Lock(), int{});
}

TEST(Mytex, BasicCreateLockDestroy) {
  baudvine::Mytex<int> underTest(5);
  EXPECT_EQ(*underTest.Lock(), 5);
  *underTest.Lock() = 6;
  EXPECT_EQ(*underTest.Lock(), 6);
}

TEST(Mytex, GuardAccessors) {
  baudvine::Mytex<int> underTest(6);
  auto number = underTest.Lock();
  EXPECT_EQ(*number, 6);

  baudvine::Mytex<std::vector<int>> guardedVector;
  auto guard = guardedVector.Lock();
  (*guard).push_back(55);
  EXPECT_EQ(guard->size(), 1);
}

TEST(Mytex, TryLock) {
  baudvine::Mytex<int> underTest(6);

  {
    auto number = underTest.TryLock();
    EXPECT_THAT(number, testing::Optional(6));
  }

  EXPECT_THAT(underTest.TryLock(), testing::Optional(6));
}

TEST(Mytex, TryLockFailure) {
  baudvine::Mytex<int> underTest(6);
  auto number = underTest.TryLock();

  EXPECT_FALSE(underTest.TryLock().has_value());
  EXPECT_FALSE(static_cast<bool>(underTest.TryLock()));
  EXPECT_THROW(underTest.TryLock().value(), std::bad_optional_access);
  std::thread([&underTest] {
    EXPECT_FALSE(underTest.TryLock().has_value());
  }).join();
}

TEST(Mytex, OptionalGuardAccessors) {
  baudvine::Mytex<int> underTest(6);
  auto number = underTest.TryLock();

  EXPECT_THAT(number, testing::Optional(6));
  EXPECT_TRUE(number.has_value());
  EXPECT_EQ(number.value(), 6);
  EXPECT_EQ(*number, 6);

  baudvine::Mytex<std::vector<int>> guardedVector;
  auto guard = guardedVector.TryLock();
  (*guard).push_back(55);
  EXPECT_EQ(guard->size(), 1);
}

TEST(Mytex, MoveGuard) {
  baudvine::Mytex<int> underTest;
  auto guard = underTest.Lock();
  auto newGuard = std::move(guard);
  EXPECT_FALSE(underTest.TryLock().has_value());
}

TEST(Mytex, MoveMytex) {
  // Only works if Lockable is moveable.
  std::mutex mutex;
  baudvine::Mytex<int, std::unique_lock<std::mutex>> underTest(
      std::unique_lock(mutex, std::defer_lock), 2022);

  baudvine::Mytex<int, std::unique_lock<std::mutex>> moved(
      std::move(underTest));
  EXPECT_EQ(*moved.Lock(), 2022);
}
