#include "baudvine/mytex.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <thread>

TEST(Mytex, DefaultCtor)
{
  // A mytex can be initialized without explicitly initializing the contents (on
  // this side) - the field will still be value-initialized.
  baudvine::Mytex<int> underTest;
  EXPECT_EQ(*underTest.Lock(), int{});
}

TEST(Mytex, BasicCreateLockDestroy)
{
  baudvine::Mytex<int> underTest(5);

  {
    // Acquire a lock
    auto guard = underTest.Lock();
    // Check and modify the value
    EXPECT_EQ(*guard, 5);
    *guard = 6;
    // And implicitly release the lock.
  }

  EXPECT_EQ(*underTest.Lock(), 6);
}

TEST(Mytex, WithStdMutex)
{
  // Mytex uses std::shared_mutex by default, but any Lockable or SharedLockable
  // type works.
  baudvine::Mytex<int, std::mutex> underTest(1996);
  *underTest.Lock() += 4;
  EXPECT_EQ(*underTest.Lock(), 2000);
}

TEST(Mytex, GuardAccessors)
{
  // The guard types have multiple ways to get at the guarded values, but
  // nothing too surprising if you've ever used std::unique_ptr or
  // std::optional.
  baudvine::Mytex<int> underTest(6);
  auto number = underTest.Lock();
  EXPECT_EQ(*number, 6);

  // The guarded value can be any type, of course, so you'll want to get at the
  // members with ->.
  baudvine::Mytex<std::vector<int>> guardedVector;
  auto guard = guardedVector.Lock();
  (*guard).push_back(55);
  EXPECT_EQ(guard->size(), 1);
}

TEST(Mytex, TryLock)
{
  baudvine::Mytex<int> underTest(6);

  {
    // Mytex::TryLock() returns a different type of guard that combines the
    // regular MytexGuard and an std::optional. TryLock() could just return
    // std::optional<MytexGuard>, but without Rust's dereference cleverness
    // that's pretty annoying to work with.
    auto number = underTest.TryLock();
    EXPECT_THAT(number, testing::Optional(6));
  }

  EXPECT_THAT(underTest.TryLock(), testing::Optional(6));
}

TEST(Mytex, TryLockFailure)
{
  baudvine::Mytex<int> underTest(6);
  auto number = underTest.TryLock();

  // It's already locked, so a second TryLock() returns the equivalent of
  // std::nullopt:
  EXPECT_FALSE(underTest.TryLock().has_value());
  EXPECT_FALSE(static_cast<bool>(underTest.TryLock()));
  EXPECT_THROW(underTest.TryLock().value(), std::bad_optional_access);
  std::thread([&underTest] {
    EXPECT_FALSE(underTest.TryLock().has_value());
  }).join();
}

TEST(Mytex, OptionalGuardAccessors)
{
  baudvine::Mytex<int> underTest(6);
  auto number = underTest.TryLock();

  // The optional-ish guard type includes some of optional's member functions,
  // so it's easy to use with - for example - gMock's optional matcher.
  EXPECT_THAT(number, testing::Optional(6));
  EXPECT_TRUE(number.has_value());
  EXPECT_EQ(number.value(), 6);
  EXPECT_EQ(*number, 6);

  // And of course it can do everyting the regular MytexGuard can do.
  baudvine::Mytex<std::vector<int>> guardedVector;
  auto guard = guardedVector.TryLock();
  (*guard).push_back(55);
  EXPECT_EQ(guard->size(), 1);
}

TEST(Mytex, MoveGuard)
{
  baudvine::Mytex<int> underTest;
  auto guard = underTest.Lock();
  auto newGuard = std::move(guard);
  EXPECT_FALSE(underTest.TryLock().has_value());
}

TEST(Mytex, MoveMytex)
{
  // Only works if both T and Lockable are movable.
  std::mutex mutex;
  baudvine::Mytex<int, std::unique_lock<std::mutex>> underTest(
    std::unique_lock(mutex, std::defer_lock), 2022);

  baudvine::Mytex<int, std::unique_lock<std::mutex>> moved(
    std::move(underTest));
  EXPECT_EQ(*moved.Lock(), 2022);
}

TEST(Mytex, SharedLock)
{
  baudvine::Mytex<int> underTest(500);

  {
    // Many shared locks can be acquired at the same time
    auto guard1 = underTest.LockShared();
    auto guard2 = underTest.LockShared();
    std::thread([&] { auto guard3 = underTest.LockShared(); }).join();
    std::thread([&] {
      EXPECT_THAT(underTest.TryLockShared(), testing::Optional(500));
    }).join();

    // But while any shared-mode locks are held, no exclusive lock can be
    // acquired
    std::thread([&] { EXPECT_FALSE(underTest.TryLock().has_value()); }).join();

    // And this not compile, because *guard3 is const int&.
    // *guard1 = 4;
  }

  // Once all shared locks are released, an exclusive lock can be acquired
  // again.
  EXPECT_THAT(underTest.TryLock(), testing::Optional(500));
}

TEST(Mytex, GuardEquality)
{
  baudvine::Mytex<int16_t> one(6);
  baudvine::Mytex<int32_t> two(6);

  EXPECT_EQ(one.LockShared(), one.LockShared());
  EXPECT_EQ(one.Lock(), two.LockShared());
  *one.Lock() = 5;
  EXPECT_NE(one.LockShared(), two.Lock());
  EXPECT_EQ(one.Lock(), 5);
  EXPECT_NE(two.Lock(), 5);
}

TEST(Mytex, GuardComparison)
{
  baudvine::Mytex<uint16_t> one(1);
  baudvine::Mytex<uint32_t> two(2);

  EXPECT_GE(two.Lock(), one.LockShared());
  EXPECT_GE(two.LockShared(), two.LockShared());
  EXPECT_LE(one.Lock(), two.LockShared());
  EXPECT_LE(one.LockShared(), one.LockShared());
  EXPECT_LT(one.Lock(), two.LockShared());
  EXPECT_GT(two.Lock(), one.LockShared());

  EXPECT_GT(5U, one.Lock());
  EXPECT_GE(5U, one.Lock());
  EXPECT_LE(1U, two.Lock());
  EXPECT_LT(1U, two.Lock());

  *one.Lock() = 2;
  EXPECT_GE(two.LockShared(), one.Lock());
  EXPECT_LE(one.LockShared(), two.Lock());

  *two.Lock() = 1;
  EXPECT_LT(two.LockShared(), one.Lock());
  EXPECT_GT(one.LockShared(), two.Lock());
}

TEST(Mytex, OptionalGuardComparison)
{
  baudvine::Mytex<uint16_t> one(1);
  baudvine::Mytex<uint32_t> two(2);

  // The basics are passed on to MytexGuard
  EXPECT_EQ(one.TryLockShared(), one.TryLockShared());
  EXPECT_NE(one.TryLock(), two.TryLockShared());
  EXPECT_LT(one.TryLockShared(), two.TryLockShared());
  EXPECT_LE(one.TryLockShared(), two.TryLockShared());
  EXPECT_GT(two.TryLock(), one.TryLock());
  EXPECT_GE(two.TryLock(), one.TryLock());
  EXPECT_GE(one.TryLockShared(), one.TryLockShared());
  EXPECT_LE(one.TryLockShared(), one.TryLockShared());

  // Comparison with empty guards.
  baudvine::Mytex<uint64_t> three(3);
  auto threeGuard = three.Lock();
  EXPECT_EQ(three.TryLock(), three.TryLock());
  EXPECT_LT(three.TryLock(), one.TryLock());
  EXPECT_GT(one.TryLock(), three.TryLock());
  EXPECT_NE(three.TryLock(), one.TryLock());
  EXPECT_LE(three.TryLock(), one.TryLock());
  EXPECT_GE(one.TryLock(), three.TryLock());

  // Comparison with value type, just like MytexGuard
  EXPECT_EQ(one.TryLock(), 1U);
  EXPECT_NE(two.TryLock(), 5U);
  EXPECT_GT(two.TryLock(), 1U);
  EXPECT_GE(two.TryLock(), 1U);
  EXPECT_LT(0U, one.TryLock());
  EXPECT_LE(0U, one.TryLock());

  // Comparison between MytexGuard and OptionalMytexGuard unfortunately doesn't
  // quite work, so you need to explicitly deref in that case.
  EXPECT_LT(one.TryLock(), *two.Lock());

  {
    // Additionally, you can compare with nullopt for emptiness
    // EXPECT_NE(one.TryLock(), std::nullopt);
    // EXPECT_GT(one.TryLock(), std::nullopt);
    // EXPECT_GE(one.TryLock(), std::nullopt);
    // EXPECT_LT(std::nullopt, one.TryLock());
    // EXPECT_LE(std::nullopt, one.TryLock());
    // auto guard = one.Lock();
    // EXPECT_EQ(one.TryLock(), std::nullopt);
    // As well as compare to non-optional guards
    // EXPECT_NE(two.TryLock(), guard);
    // EXPECT_NE(guard, two.TryLock());
    // *guard = 2;
    // EXPECT_EQ(two.TryLock(), guard);
  }
}
