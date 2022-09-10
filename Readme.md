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
class SafeQueue {
 public:
  void Push(std::string string) {
    auto lines = mLines.Lock();
    lines->push(string);
  }

  std::optional<std::string> Pop() {
    auto lines = mLines.Lock();
    if (lines->empty()) {
      return {};
    } else {
      auto string = lines->front();
      lines->pop();
      return string;
    }
  }

 private:
  baudvine::Mytex<std::queue<std::string>> mLines;
};
```
