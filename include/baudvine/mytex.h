#pragma once

#include <mutex>

namespace baudvine {

template <typename T, typename Lock>
class MytexGuard {
 public:
  template <typename L = Lock>
  MytexGuard(T* object, Lock lock) : object_(object), lock_(std::move(lock)) {}

  T& operator*() noexcept { return *object_; }
  T const& operator*() const noexcept { return *object_; }
  T* operator->() noexcept { return &**this; }
  T const* operator->() const noexcept { return &**this; }

 private:
  T* object_;
  Lock lock_;
};

template <typename T, typename Mutex = std::mutex>
class Mytex {
 public:
  template <typename... Args>
  Mytex(Args&&... initialize) : object_(std::forward<Args>(initialize)...) {}

  MytexGuard<T, std::unique_lock<Mutex>> Lock() {
    return {&object_, std::unique_lock<Mutex>(mutex_)};
  }

 private:
  T object_;
  Mutex mutex_;
};
}  // namespace baudvine