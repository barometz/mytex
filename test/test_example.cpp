#include "baudvine/mytex.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <queue>
#include <thread>

using namespace std::chrono_literals;

/** A thread-safe queue that combines Mytex and std::queue. */
class SafeQueue {
 public:
  static void PrintSize(const std::queue<std::string>& queue) {
    std::cout << queue.size() << "\n";
  }

  void Push(std::string string) {
    // To use the guarded queue, you must lock the queue.
    auto lines = mLines.Lock();
    // After that you can use it like all other similar wrappers, such as
    // std::optional and std::unique_ptr.
    // Either use ->
    lines->push(string);
    // Or *obj if you need to pass a bare reference around.
    PrintSize(*lines);
    // The guard returned by Mytex::TryLock additionally supports the
    // optional-like member functions has_value() and value().
  }

  std::optional<std::string> Pop() {
    auto lines = mLines.Lock();
    if (lines->empty()) {
      return {};
      // Much like std::scoped_lock, the guard is released and the mutex
      // unlocked whenever you exit this scope by returning or because of an
      // exception.
    } else {
      auto string = lines->front();
      lines->pop();
      return string;
    }
  }

 private:
  baudvine::Mytex<std::queue<std::string>> mLines;
};

TEST(Example, Queue) {
  SafeQueue queue{};

  const std::array input{
      "On this side, it's a very boring concurrency demo, ",
      "since the behaviour of the Queue class is exactly the same",
      "as it would be with a more traditional std::lock_guard approach.",
      "The difference is all in the implementation: within SafeQueue,",
      "it's impossible to modify the inner Queue without first locking it.",
      "That doesn't mean you can't make mistakes - you can keep an unguarded "
      "pointer to the locked object,",
      "or you can make TOCTOU errors such as",
      "if (!Lock()->empty()) return Lock()->top();",
      "instead of keeping the guard around.",
  };

  std::vector<std::string> output;

  std::atomic_bool stop = false;
  std::thread consumer([&output, &queue, &stop] {
    while (!stop) {
      if (auto line = queue.Pop(); line.has_value()) {
        output.push_back(*line);
      }
    }
  });

  for (auto& line : input) {
    queue.Push(line);
  }

  std::this_thread::sleep_for(50ms);
  stop = true;
  consumer.join();

  EXPECT_THAT(output, testing::ElementsAreArray(input));
}
