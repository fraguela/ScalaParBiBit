#ifndef THREADHANDLER_H_
#define THREADHANDLER_H_

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

class ThreadHandler {

  const int nthreads_;
  std::vector<std::thread> threads_;
  std::function<void()> func_;
  std::mutex mutex_;
  std::condition_variable cond_var_;
  volatile int count_;
  volatile bool ready_, finish_;

public:

  ThreadHandler(int n);
  
  void main();

  void launchTheads();
  
  template<class F, class... Args>
  void setFunction(F&& f, Args&&... args) {
    func_ = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
  }
  
  /// ensure all threads finished. Spin wait
  void wait() noexcept;

  ~ThreadHandler();
};

#endif
