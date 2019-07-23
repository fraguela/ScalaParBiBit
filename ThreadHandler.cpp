#include "ThreadHandler.h"

ThreadHandler::ThreadHandler(int n) :
nthreads_(n), func_([]{}), count_(0), ready_(false), finish_(false)
{
  for (int i = 0; i < nthreads_; i++) {
    threads_.emplace_back(&ThreadHandler::main, this);
  }
}

void ThreadHandler::main()
{
  while (!finish_) {
    std::unique_lock<std::mutex> my_lock(mutex_);
    while (!ready_) {
      cond_var_.wait(my_lock);
    }
    if(++count_ == nthreads_) {
      ready_ = false;
    }
    my_lock.unlock();
    
    if (!finish_) {
      func_();
    }

    while(ready_) {} // ensure all threads restarted
    my_lock.lock();
    count_--;
    my_lock.unlock();
    
    //wait();
  }
}

void ThreadHandler::wait() noexcept
{
   while(ready_ || count_) {};
}

void ThreadHandler::launchTheads()
{
  if (nthreads_) {
    wait();
  
    {
      std::lock_guard<std::mutex> my_guard_lock(mutex_);
      ready_ = true;
    }
    cond_var_.notify_all();
  }

}

ThreadHandler::~ThreadHandler()
{
  wait();
  
  finish_ = true;
  launchTheads();
  for (auto& thread : threads_) {
    thread.join();
  }
}
