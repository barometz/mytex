#pragma once

#include <cassert>
#include <mutex>
#include <optional>

namespace baudvine {
/**
 * @brief A lock_guard-a-like that includes a reference to the guarded
 * resource.
 *
 * \c MytexGuard holds the lock it was created with until it goes out of scope.
 * As long as you hold it, you have exclusive access to the referenced resource.
 */
template <typename T, typename Lock>
class MytexGuard {
 public:
  MytexGuard(T* object, Lock lock) : object_(object), lock_(std::move(lock)) {
    assert(lock_.owns_lock());
  }

  /** @returns A reference to the guarded object. */
  T& operator*() noexcept { return *object_; }
  /** @returns A reference to the guarded object. */
  const T& operator*() const noexcept { return *object_; }
  T* operator->() noexcept { return &**this; }
  const T* operator->() const noexcept { return &**this; }

 private:
  T* object_;
  Lock lock_;
};

/**
 * @brief Same as \c MytexGuard, except it might be empty.
 *
 * This combines MytexGuard and std::optional and squashes the double
 * dereference that would result from Mytex::TryLock() literally returning
 * std::optional<MytexGuard>.
 */
template <typename T, typename Lock>
class OptionalMytexGuard {
 public:
  using value_type = T;

  OptionalMytexGuard() = default;
  OptionalMytexGuard(T* object, Lock lock)
      : data_(std::in_place, object, std::move(lock)) {}

  /** @brief Indicates whether this contains a (locked) value. */
  [[nodiscard]] bool has_value() const noexcept { return data_.has_value(); }
  /** @brief Indicates whether this contains a (locked) value. */
  [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
  /**
   * @returns A reference to the guarded object.
   * @throws std::bad_optional_access
   */
  T& value() { return *data_.value(); }
  /**
   * @returns A reference to the guarded object.
   * @throws std::bad_optional_access
   */
  const T& value() const { return *data_.value(); }  // NOLINT(*-use-nodiscard)

  /** @returns A reference to the guarded object (unchecked). */
  T& operator*() noexcept { return **data_; }
  /** @returns A reference to the guarded object (unchecked). */
  const T& operator*() const noexcept { return **data_; }
  T* operator->() noexcept { return &**this; }
  const T* operator->() const noexcept { return &**this; }

 private:
  std::optional<MytexGuard<T, Lock>> data_{};
};

/** @brief A mutex that owns the resource it guards. */
template <typename T, typename Lockable = std::mutex>
class Mytex {
 public:
  using LockT = std::unique_lock<Lockable>;
  using Guard = MytexGuard<T, LockT>;
  using OptionalGuard = OptionalMytexGuard<T, LockT>;

  /** @brief Construct a new Mytex and initialize the contained resource. */
  template <typename... Args>
  Mytex(Args&&... initialize) : object_(std::forward<Args>(initialize)...) {}

  /**
   * @brief Lock the contained resource.
   *
   * @returns A MytexGuard referencing the guarded resource. The lock is
   *          released when the guard goes out of scope.
   */
  Guard Lock() { return {&object_, LockT(mutex_)}; }

  /**
   * @brief Attempt to lock the contained resource.
   *
   * @returns An OptionalMytexGuard which references the guarded resource if and
   *          only if the lock is held. If held, the lock is released when the
   *          guard goes out of scope.
   */
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
