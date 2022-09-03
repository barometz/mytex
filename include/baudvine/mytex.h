#pragma once

#include <mutex>
#include <optional>

namespace baudvine {
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

/// @brief Same as \c MytexGuard, except it might be empty.
template <typename T, typename Lock>
class OptionalMytexGuard {
public:
  OptionalMytexGuard() = default;
  OptionalMytexGuard(T* object, Lock lock)
      : data_(Data{object, std::move(lock)}) {}

  [[nodiscard]] bool has_value() const noexcept { return data_.has_value(); }
  T& value() { return *data_.value().object; }
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

template <typename T, typename Lockable = std::mutex>
class Mytex {
 public:
  using LockT = std::unique_lock<Lockable>;

  template <typename... Args>
  Mytex(Args&&... initialize) : object_(std::forward<Args>(initialize)...) {}

  MytexGuard<T, LockT> Lock() { return {&object_, LockT(mutex_)}; }

  OptionalMytexGuard<T, LockT> TryLock() {
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
