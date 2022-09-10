#pragma once

#include <mutex>
#include <optional>

namespace baudvine {
/** @brief A lock_guard-a-like that includes a reference to the guarded
 * resource.
 *
 * \c MytexGuard holds the lock it was created with until it goes out of scope.
 * As long as you hold it, you have exclusive access to the referenced resource.
 */
template <typename T, typename Lock>
class MytexGuard {
 public:
  MytexGuard(T* object, Lock lock) : object_(object), lock_(std::move(lock)) {}

  T& operator*() noexcept { return *object_; }
  T const& operator*() const noexcept { return *object_; }
  T* operator->() noexcept { return &**this; }
  T const* operator->() const noexcept { return &**this; }

 private:
  T* object_;
  Lock lock_;
};

/** @brief Same as \c MytexGuard, except it might be empty.
 *
 * Without this, \c Mytex::TryLock() would return \c std::optional<MytexGuard>
 * which introduces a second layer of indirection. \c OptionalMytexGuard
 * integrates \c std::optional, exposes some of its functionality, and lets you
 * get at the optionally-contained value with one dereference.
 */
template <typename T, typename Lock>
class OptionalMytexGuard {
 public:
  OptionalMytexGuard() = default;
  OptionalMytexGuard(T* object, Lock lock)
      : data_(Data{object, std::move(lock)}) {}

  [[nodiscard]] bool has_value() const noexcept { return data_.has_value(); }
  [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
  T& value() { return *data_.value().object; }
  // The user may call these functions just for the exception, so:
  // NOLINTNEXTLINE(modernize-use-nodiscard)
  const T& value() const { return *data_.value().object; }

  T& operator*() noexcept { return *data_->object; }
  T const& operator*() const noexcept { return *data_->object; }
  T* operator->() noexcept { return &**this; }
  T const* operator->() const noexcept { return &**this; }

 private:
  struct Data {
    T* object;
    Lock lock;
  };
  std::optional<Data> data_{};
};

/** @brief A mutex that owns the resource it guards.
 */
template <typename T, typename Lockable = std::mutex>
class Mytex {
 public:
  using LockT = std::unique_lock<Lockable>;
  using Guard = MytexGuard<T, LockT>;
  using OptionalGuard = OptionalMytexGuard<T, LockT>;

  template <typename... Args>
  Mytex(Args&&... initialize) : object_(std::forward<Args>(initialize)...) {}

  Guard Lock() { return {&object_, LockT(mutex_)}; }

  OptionalGuard TryLock() {
    LockT lock(mutex_, std::try_to_lock);
    if (lock.owns_lock()) {
      return {&object_, std::move(lock)};
    }
    return {};
  }

 private:
  T object_;
  Lockable mutex_;
};
}  // namespace baudvine
