#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>

// compile with:
// g++ -pthread -std=c++11 cpp_threads.cpp

std::condition_variable cv_pop;
std::condition_variable cv_push;
int pushed_val = 0;

int push_ready = 1;
int pop_ready = 0;

/*

https://en.cppreference.com/w/cpp/thread/condition_variable


The condition_variable class is a synchronization primitive that can be used to 
block a thread, or multiple threads at the same time, until another thread both 
modifies a shared variable (the condition), and notifies the condition_variable.

The thread that intends to modify the variable has to

    acquire a std::mutex (typically via std::lock_guard)
    perform the modification while the lock is held
    execute notify_one or notify_all on the std::condition_variable (the lock 
    does not need to be held for notification) 

Even if the shared variable is atomic, it must be modified under the mutex in 
order to correctly publish the modification to the waiting thread.

Any thread that intends to wait on std::condition_variable has to

    acquire a std::unique_lock<std::mutex>, on the same mutex as used to protect 
    the shared variable execute wait, wait_for, or wait_until. The wait 
    operations atomically release the mutex and suspend the execution of the 
    thread. When the condition variable is notified, a timeout expires, or a 
    spurious wakeup occurs, the thread is awakened, and the mutex is atomically 
    reacquired. The thread should then check the condition and resume waiting 
    if the wake up was spurious. 
*/

int N = 100;

void push(std::mutex& mtx)
{
  for (int i = 1; i < N; ++i)
  {
    std::unique_lock<std::mutex> lock(mtx);

    while (!push_ready)
      cv_push.wait(lock);

    pushed_val = i;
    pop_ready = 1;
    push_ready = 0;      
    
    cv_pop.notify_one();
  }
}

void pop(std::mutex& mtx)
{
  while (1)
  {
    std::unique_lock<std::mutex> lock(mtx);

    while (!pop_ready)
      cv_pop.wait(lock);

    std::cout << pushed_val << " " << std::endl;;
    pop_ready = 0;
    push_ready = 1;

    cv_push.notify_one();

    if (pushed_val == N-1)
      break;
  }
}

void foo(std::mutex& mtx)
{
  std::unique_lock<std::mutex> lock(mtx);
  for (int i = 1; i < 10; ++i)
  {
    std::cout << i << " ";
  }
  std::cout << std::endl;
}

int main() 
{
  std::mutex mtx;
  
  std::thread th1(push, std::ref(mtx));
  std::thread th2(pop,  std::ref(mtx));  

  th1.join();
  th2.join();
  
  std::vector<std::thread> threads;

  for (int i = 0; i < 100; ++i)
    threads.push_back(std::thread(foo, std::ref(mtx)));
  
  for (int i = 0; i < 100; ++i)
    threads[i].join();

  return 0;
}



