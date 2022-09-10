#pragma once

#include <cassert>
#include <mutex>
#include <optional>
#include <shared_mutex>

namespace baudvine {
/**
 * @brief A lock_guard-a-like that includes a reference to the guarded
 * resource.
 *
 * \c MytexGuard holds the lock it was created with until it goes out of scope.
 * As long as you hold it, you have exclusive access to the referenced resource.
 */
template<typename T, typename Lock>
class MytexGuard
{
public:
  MytexGuard(T* object, Lock lock)
    : mObject(object)
    , mLock(std::move(lock))
  {
    assert(mLock.owns_lock());
  }

  /** @returns A reference to the guarded object. */
  T& operator*() noexcept { return *mObject; }
  /** @returns A reference to the guarded object. */
  const T& operator*() const noexcept { return *mObject; }
  T* operator->() noexcept { return &**this; }
  const T* operator->() const noexcept { return &**this; }

private:
  T* mObject;
  Lock mLock;
};

/**
 * @brief Same as \c MytexGuard, except it might be empty.
 *
 * This combines MytexGuard and std::optional and squashes the double
 * dereference that would result from Mytex::TryLock() literally returning
 * std::optional<MytexGuard>.
 */
template<typename T, typename Lock>
class OptionalMytexGuard
{
public:
  using value_type = T;

  OptionalMytexGuard() = default;
  OptionalMytexGuard(T* object, Lock lock)
    : mInner(std::in_place, object, std::move(lock))
  {
  }

  /** @brief Indicates whether this contains a (locked) value. */
  [[nodiscard]] bool has_value() const noexcept { return mInner.has_value(); }
  /** @brief Indicates whether this contains a (locked) value. */
  [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
  /**
   * @returns A reference to the guarded object.
   * @throws std::bad_optional_access
   */
  T& value() { return *mInner.value(); }
  /**
   * @returns A reference to the guarded object.
   * @throws std::bad_optional_access
   */
  const T& value() const { return *mInner.value(); } // NOLINT(*-use-nodiscard)

  /** @returns A reference to the guarded object (unchecked). */
  T& operator*() noexcept { return **mInner; }
  /** @returns A reference to the guarded object (unchecked). */
  const T& operator*() const noexcept { return **mInner; }
  T* operator->() noexcept { return &**this; }
  const T* operator->() const noexcept { return &**this; }

private:
  std::optional<MytexGuard<T, Lock>> mInner{};
};

/** @brief A mutex that owns the resource it guards. */
template<typename T, typename Lockable = std::shared_mutex>
class Mytex
{
public:
  using MutLock = std::unique_lock<Lockable>;
  using ConstLock = std::shared_lock<Lockable>;
  using MutGuard = MytexGuard<T, MutLock>;
  using ConstGuard = MytexGuard<const T, ConstLock>;
  using MutOptionalGuard = OptionalMytexGuard<T, MutLock>;
  using ConstOptionalGuard = OptionalMytexGuard<const T, ConstLock>;

  template<typename... Args>
  Mytex(Lockable mutex, Args&&... initialize)
    : mObject(std::forward<Args>(initialize)...)
    , mMutex(std::move(mutex))
  {
  }

  /** @brief Construct a new Mytex and initialize the contained resource. */
  template<typename... Args>
  Mytex(Args&&... initialize)
    : mObject(std::forward<Args>(initialize)...)
  {
  }

  /**
   * @brief Lock the contained resource in exclusive (unique, mutable) mode.
   *
   * @returns A MytexGuard referencing the guarded resource. The lock is
   *          released when the guard goes out of scope.
   */
  MutGuard Lock() { return { &mObject, MutLock(mMutex) }; }

  /**
   * @brief Lock the contained resource in shared mode.
   *
   * Only available when Lockable is std::shared_mutex or a compatible type.
   *
   * @returns A MytexGuard with a const reference to the guarded resource. The
   *          lock is released when the guard goes out of scope.
   */
  ConstGuard LockShared() const { return { &mObject, ConstLock(mMutex) }; }

  /**
   * @brief Attempt to lock the contained resource in exclusive (unique,
   *        mutable) mode.
   *
   * @returns A MutOptionalGuard which references the guarded resource if and
   *          only if the lock is held. If held, the lock is released when the
   *          guard goes out of scope.
   */
  MutOptionalGuard TryLock()
  {
    MutLock lock(mMutex, std::try_to_lock);
    if (lock.owns_lock()) {
      return { &mObject, std::move(lock) };
    }
    return {};
  }

  /**
   * @brief Attempt to lock the contained resource in shared mode.
   * 
   * Only available when Lockable is std::shared_mutex or a compatible type.
   *
   * @returns An ConstOptionalGuard which references the guarded resource if and
   *          only if the lock is held. If held, the lock is released when the
   *          guard goes out of scope.
   */
  ConstOptionalGuard TryLockShared() const
  {
    ConstLock lock(mMutex, std::try_to_lock);
    if (lock.owns_lock()) {
      return { &mObject, std::move(lock) };
    }
    return {};
  }

private:
  T mObject;
  mutable Lockable mMutex;
};
} // namespace baudvine
