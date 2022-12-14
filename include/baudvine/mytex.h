// Copyright 2022 Dominic van Berkel <dominic@baudvine.net>

// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.

// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
// OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

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
 *
 * Comparison operators compare the referenced value.
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
 *
 * Comparison operators follow std::optional rules - the empty guard (and
 * std::nullopt) compares as less than an engaged lock, and everything else
 * compares as the guarded value.
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

/** @brief A mutex that owns the resource it guards.
 *
 * By default this uses std::shared_mutex, but any class that supports the
 * Lockable requirements should work. If it supports SharedLockable,
 * LockShared() will work as well.
 */
template<typename T, typename Lockable = std::shared_mutex>
class Mytex
{
public:
  using ExclusiveLock = std::unique_lock<Lockable>;
  using SharedLock = std::shared_lock<Lockable>;
  using Guard = MytexGuard<T, ExclusiveLock>;
  using SharedGuard = MytexGuard<const T, SharedLock>;
  using OptionalGuard = OptionalMytexGuard<T, ExclusiveLock>;
  using SharedOptionalGuard = OptionalMytexGuard<const T, SharedLock>;

  /**
   * @brief Construct a new Mytex with an existing mutex and initialize the
   * contained resource.
   *
   * This is useful when you have a specialized mutex that can't be
   * default-constructed, such as:
   * - An inter-process named mutex
   * - A deferred lock that controls multiple mutexes
   * - A std::unique_lock for another mutex
   *
   * @param mutex The mutex to use.
   * @param args  Constructor parameters for the contained object.
   */
  template<typename... Args>
  Mytex(Lockable mutex, Args&&... initialize)
    : mObject(std::forward<Args>(initialize)...)
    , mMutex(std::move(mutex))
  {
  }

  /**
   * @brief Construct a new Mytex and initialize the contained resource.
   *
   * @param args Constructor parameters for the contained object.
   */
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
  Guard Lock() { return { &mObject, ExclusiveLock(mMutex) }; }

  /**
   * @brief Lock the contained resource in shared mode.
   *
   * Only available when Lockable is std::shared_mutex or a compatible type.
   *
   * @returns A MytexGuard with a const reference to the guarded resource. The
   *          lock is released when the guard goes out of scope.
   */
  SharedGuard LockShared() const { return { &mObject, SharedLock(mMutex) }; }

  /**
   * @brief Attempt to lock the contained resource in exclusive (unique,
   *        mutable) mode.
   *
   * @returns An OptionalMytexGuard which references the guarded resource if and
   *          only if the lock is held. If held, the lock is released when the
   *          guard goes out of scope.
   */
  OptionalGuard TryLock()
  {
    ExclusiveLock lock(mMutex, std::try_to_lock);
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
   * @returns A OptionalMytexGuard which references the guarded resource if and
   *          only if the lock is held. If held, the lock is released when the
   *          guard goes out of scope.
   */
  SharedOptionalGuard TryLockShared() const
  {
    SharedLock lock(mMutex, std::try_to_lock);
    if (lock.owns_lock()) {
      return { &mObject, std::move(lock) };
    }
    return {};
  }

private:
  T mObject;
  mutable Lockable mMutex;
};

template<typename T, typename Lock, typename U, typename L>
inline bool
operator==(const MytexGuard<T, Lock>& lhs, const MytexGuard<U, L>& rhs)
{
  return *lhs == *rhs;
}

template<typename T, typename Lock, typename U, typename L>
inline bool
operator!=(const MytexGuard<T, Lock>& lhs, const MytexGuard<U, L>& rhs)
{
  return !(lhs == rhs);
}

template<typename T, typename Lock, typename U, typename L>
inline bool
operator<(const MytexGuard<T, Lock>& lhs, const MytexGuard<U, L>& rhs)
{
  return *lhs < *rhs;
}

template<typename T, typename Lock, typename U, typename L>
inline bool
operator>(const MytexGuard<T, Lock>& lhs, const MytexGuard<U, L>& rhs)
{
  return rhs < lhs;
}

template<typename T, typename Lock, typename U, typename L>
inline bool
operator<=(const MytexGuard<T, Lock>& lhs, const MytexGuard<U, L>& rhs)
{
  return !(lhs > rhs);
}

template<typename T, typename Lock, typename U, typename L>
inline bool
operator>=(const MytexGuard<T, Lock>& lhs, const MytexGuard<U, L>& rhs)
{
  return !(lhs < rhs);
}

template<typename T, typename Lock, typename U>
inline bool
operator==(const MytexGuard<T, Lock>& lhs, const U& rhs)
{
  return *lhs == rhs;
}

template<typename T, typename Lock, typename U>
inline bool
operator==(const U& lhs, const MytexGuard<T, Lock>& rhs)
{
  return lhs == *rhs;
}

template<typename T, typename Lock, typename U>
inline bool
operator!=(const MytexGuard<T, Lock>& lhs, const U& rhs)
{
  return !(lhs == rhs);
}

template<typename T, typename Lock, typename U>
inline bool
operator!=(const U& lhs, const MytexGuard<T, Lock>& rhs)
{
  return !(lhs == rhs);
}

template<typename T, typename Lock, typename U>
inline bool
operator<(const MytexGuard<T, Lock>& lhs, const U& rhs)
{
  return *lhs < rhs;
}

template<typename T, typename Lock, typename U>
inline bool
operator<(const U& lhs, const MytexGuard<T, Lock>& rhs)
{
  return lhs < *rhs;
}

template<typename T, typename Lock, typename U>
inline bool
operator>(const MytexGuard<T, Lock>& lhs, const U& rhs)
{
  return rhs < lhs;
}

template<typename T, typename Lock, typename U>
inline bool
operator>(const U& lhs, const MytexGuard<T, Lock>& rhs)
{
  return rhs < lhs;
}

template<typename T, typename Lock, typename U>
inline bool
operator>=(const MytexGuard<T, Lock>& lhs, const U& rhs)
{
  return !(lhs < rhs);
}

template<typename T, typename Lock, typename U>
inline bool
operator>=(const U& lhs, const MytexGuard<T, Lock>& rhs)
{
  return !(lhs < rhs);
}

template<typename T, typename Lock, typename U>
inline bool
operator<=(const MytexGuard<T, Lock>& lhs, const U& rhs)
{
  return !(lhs > rhs);
}

template<typename T, typename Lock, typename U>
inline bool
operator<=(const U& lhs, const MytexGuard<T, Lock>& rhs)
{
  return !(lhs > rhs);
}

template<typename T, typename L1, typename U, typename L2>
inline bool
operator==(const OptionalMytexGuard<T, L1>& lhs,
           const OptionalMytexGuard<U, L2>& rhs)
{
  if (lhs.has_value()) {
    return rhs.has_value() && *lhs == *rhs;
  }
  return !rhs.has_value();
}

template<typename T, typename L1, typename U, typename L2>
inline bool
operator!=(const OptionalMytexGuard<T, L1>& lhs,
           const OptionalMytexGuard<U, L2>& rhs)
{
  return !(lhs == rhs);
}

template<typename T, typename L1, typename U, typename L2>
inline bool
operator<(const OptionalMytexGuard<T, L1>& lhs,
          const OptionalMytexGuard<U, L2>& rhs)
{
  if (lhs.has_value()) {
    return rhs.has_value() && *lhs < *rhs;
  }
  return rhs.has_value();
}

template<typename T, typename L1, typename U, typename L2>
inline bool
operator>(const OptionalMytexGuard<T, L1>& lhs,
          const OptionalMytexGuard<U, L2>& rhs)
{
  return rhs < lhs;
}

template<typename T, typename L1, typename U, typename L2>
inline bool
operator<=(const OptionalMytexGuard<T, L1>& lhs,
           const OptionalMytexGuard<U, L2>& rhs)
{
  return !(lhs > rhs);
}

template<typename T, typename L1, typename U, typename L2>
inline bool
operator>=(const OptionalMytexGuard<T, L1>& lhs,
           const OptionalMytexGuard<U, L2>& rhs)
{
  return !(lhs < rhs);
}

template<typename T, typename Lock, typename U>
inline bool
operator==(const OptionalMytexGuard<T, Lock>& lhs, const U& rhs)
{
  return lhs.has_value() && *lhs == rhs;
}

template<typename T, typename Lock, typename U>
inline bool
operator==(const U& lhs, const OptionalMytexGuard<T, Lock>& rhs)
{
  return rhs.has_value() && lhs == *rhs;
}

template<typename T, typename Lock, typename U>
inline bool
operator!=(const OptionalMytexGuard<T, Lock>& lhs, const U& rhs)
{
  return !(lhs == rhs);
}

template<typename T, typename Lock, typename U>
inline bool
operator!=(const U& lhs, const OptionalMytexGuard<T, Lock>& rhs)
{
  return !(lhs == rhs);
}

template<typename T, typename Lock, typename U>
inline bool
operator<(const OptionalMytexGuard<T, Lock>& lhs, const U& rhs)
{
  return !lhs.has_value() || *lhs < rhs;
}

template<typename T, typename Lock, typename U>
inline bool
operator<(const U& lhs, const OptionalMytexGuard<T, Lock>& rhs)
{
  return rhs.has_value() && lhs < *rhs;
}

template<typename T, typename Lock, typename U>
inline bool
operator>(const OptionalMytexGuard<T, Lock>& lhs, const U& rhs)
{
  return rhs < lhs;
}

template<typename T, typename Lock, typename U>
inline bool
operator>(const U& lhs, const OptionalMytexGuard<T, Lock>& rhs)
{
  return rhs < lhs;
}

template<typename T, typename Lock, typename U>
inline bool
operator<=(const OptionalMytexGuard<T, Lock>& lhs, const U& rhs)
{
  return !(lhs > rhs);
}

template<typename T, typename Lock, typename U>
inline bool
operator<=(const U& lhs, const OptionalMytexGuard<T, Lock>& rhs)
{
  return !(lhs > rhs);
}

template<typename T, typename Lock, typename U>
inline bool
operator>=(const OptionalMytexGuard<T, Lock>& lhs, const U& rhs)
{
  return !(lhs < rhs);
}

template<typename T, typename Lock, typename U>
inline bool
operator>=(const U& lhs, const OptionalMytexGuard<T, Lock>& rhs)
{
  return !(lhs < rhs);
}

template<typename T, typename Lock>
inline bool
operator==(const OptionalMytexGuard<T, Lock>& lhs, std::nullopt_t /*rhs*/)
{
  return !lhs.has_value();
}

template<typename T, typename Lock>
inline bool
operator==(std::nullopt_t /*lhs*/, const OptionalMytexGuard<T, Lock>& rhs)
{
  return !rhs.has_value();
}

template<typename T, typename Lock>
inline bool
operator!=(const OptionalMytexGuard<T, Lock>& lhs, std::nullopt_t rhs)
{
  return !(lhs == rhs);
}

template<typename T, typename Lock>
inline bool
operator!=(std::nullopt_t lhs, const OptionalMytexGuard<T, Lock>& rhs)
{
  return !(lhs == rhs);
}

template<typename T, typename Lock>
inline bool
operator<(const OptionalMytexGuard<T, Lock>& /*lhs*/, std::nullopt_t /*rhs*/)
{
  return false;
}

template<typename T, typename Lock>
inline bool
operator<(std::nullopt_t /*lhs*/, const OptionalMytexGuard<T, Lock>& rhs)
{
  return rhs.has_value();
}

template<typename T, typename Lock>
inline bool
operator>(const OptionalMytexGuard<T, Lock>& lhs, std::nullopt_t rhs)
{
  return rhs < lhs;
}

template<typename T, typename Lock>
inline bool
operator>(std::nullopt_t lhs, const OptionalMytexGuard<T, Lock>& rhs)
{
  return rhs < lhs;
}

template<typename T, typename Lock>
inline bool
operator<=(const OptionalMytexGuard<T, Lock>& lhs, std::nullopt_t rhs)
{
  return !(lhs > rhs);
}

template<typename T, typename Lock>
inline bool
operator<=(std::nullopt_t lhs, const OptionalMytexGuard<T, Lock>& rhs)
{
  return !(lhs > rhs);
}

template<typename T, typename Lock>
inline bool
operator>=(const OptionalMytexGuard<T, Lock>& lhs, std::nullopt_t rhs)
{
  return !(lhs < rhs);
}

template<typename T, typename Lock>
inline bool
operator>=(std::nullopt_t lhs, const OptionalMytexGuard<T, Lock>& rhs)
{
  return !(lhs < rhs);
}
} // namespace baudvine
