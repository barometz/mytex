# baudvine::Mytex

`Mytex` is a C++ mutex that owns the resource it's supposed to guard. It is inspired by Rust's [`std::sync::Mutex`](https://doc.rust-lang.org/std/sync/struct.Mutex.html). The basic idea is that generally, a mutex is used to guard access to a specific object or resource anyway, so that might as well be part of its interface.

## Prior art

- The one in Rust's standard library, as mentioned.
- I am not the first to write [one of these for C++](https://github.com/dragazo/rustex) either.
- Read about the "why would you do this" [on Cliffle](https://cliffle.com/blog/rust-mutexes/).

## Example

See [test/test_example.cpp](test/test_example.cpp) for the full annotated demo,
but in summary:

```c++
#include <baudvine/mytex.h>
#include <queue>

class SafeQueue
{
public:
  static void PrintSize(const std::queue<std::string>& queue)
  {
    std::cout << queue.size() << "\n";
  }

  void Push(const std::string& string)
  {
    auto lines = mLines.Lock();
    lines->push(string);
    PrintSize(*lines);
  }

  std::optional<std::string> Pop()
  {
    auto lines = mLines.Lock();
    if (lines->empty()) {
      return {};
    }

    auto string = lines->front();
    lines->pop();
    return string;
  }

  bool Empty() const
  {
    auto lines = mLines.LockShared();
    return lines->empty();
  }

private:
  baudvine::Mytex<std::queue<std::string>> mLines;
};
```

## Shared and exclusive mode

By default, `Mytex` uses `std::shared_mutex` as the inner mutex. In this case,
it supports shared and exclusive locking.

### Exclusive mode
This is the basic mutex behaviour: one lock can be held at any time, and any
call to Lock() will block until the mutex is available again. The guard returned
by `Mytex::Lock()` provides a non-const reference to the guarded object.

### Shared mode
Many shared-mode locks can be held at the same time, but the guard returned by
`Mytex::LockShared()` can only provide a const reference to the guarded object.
While any shared locks are held, `Mytex::Lock()` will block in the same way as
when an exclusive lock is held.
