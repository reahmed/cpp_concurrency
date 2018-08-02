
/*

Semaphores

A semaphore is a very relaxed type of lockable object. A given semaphore has 
a predefined maximum count, and a current count. You take ownership of a 
semaphore with a wait operation, also referred to as decrementing the 
semaphore, or even just abstractly called P. You release ownership with 
a signal operation, also referred to as incrementing the semaphore, a 
post operation, or abstractly called V. The single-letter operation names 
are from Dijkstra's original paper on semaphores.

Every time you wait on a semaphore, you decrease the current count. If the 
count was greater than zero then the decrement just happens, and the wait 
call returns. If the count was already zero then it cannot be decremented, 
so the wait call will block until another thread increases the count by 
signalling the semaphore.

Every time you signal a semaphore, you increase the current count. If the 
count was zero before you called signal, and there was a thread blocked in 
wait then that thread will be woken. If multiple threads were waiting, only 
one will be woken. If the count was already at its maximum value then the 
signal is typically ignored, although some semaphores may report an error.

Whereas mutex ownership is tied very tightly to a thread, and only the 
thread that acquired the lock on a mutex can release it, semaphore ownership 
is far more relaxed and ephemeral. Any thread can signal a semaphore, at 
any time, whether or not that thread has previously waited for the semaphore.

Semaphores in C++

The C++ standard does not define a semaphore type. You can write your own 
with an atomic counter, a mutex and a condition variable if you need, but 
most uses of semaphores are better replaced with mutexes and/or condition 
variables anyway.

Unfortunately, for those cases where semaphores really are what you want, 
using a mutex and a condition variable adds overhead, and there is nothing 
in the C++ standard to help. Olivier Giroux and Carter Edwards' proposal 
for a std::synchronic class template (N4195) might allow for an efficient 
implementation of a semaphore, but this is still just a proposal.

*/

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <chrono>
#include <queue>

// SEE CURRENT THREAD COUNT WITH
// ps -o nlwp <pid>

// COMPILE with:
// g++ -std=c++11 -pthread <file>.cpp

class SemaphoreReleaseLIFO
{

public:

  void wait()
  {
    // immediately return if count is not 0.
    if (count > 0)
    {
      --count;
      return;
    }

    std::unique_lock<std::mutex> lock(mtx);
    ++num_waiting;

    int my_wait_number = num_waiting;
    
    // cv.wait unlocks mutex and blocks until notified.
    // only unblock the thread whose wait_number equals
    // the number waiting (i.e. the last one to start
    // waiting).
    
    while (count == 0 or (my_wait_number != num_waiting))
      cv.wait(lock);

    std::cout << "releasing waiting thread: " << my_wait_number << std::endl;
    
    // thread is no longer waiting
    --num_waiting;
    --count;
  }

  void signal()
  {
    // ignore signal when at max count
    if (count == N)
    {
      return;
    }

    std::unique_lock<std::mutex> lock(mtx);
    ++count;

    // don't notify if no one is waiting
    
    if (num_waiting == 0)
    {
      return;
    }

    // notify them all now!
    cv.notify_all();
  }

  int count = N;
  int num_waiting = 0;

  
private:

  std::mutex mtx;
  std::condition_variable cv;

  constexpr static int N = 3;
};

int main()
{
  SemaphoreReleaseLIFO sm;

  std::string line;

  std::vector<std::thread*> ths;
  
  while (std::getline(std::cin, line))
  {
    if (line == "q")
    {
      break;
    }
    else if (line == "w")
    {
      // spawn thread that calls wait on semaphore
      ths.push_back(new std::thread(&SemaphoreReleaseLIFO::wait, &sm));
    }
    else if (line == "s")
    {
      // spawn thread that calls signal on semaphore
      ths.push_back(new std::thread(&SemaphoreReleaseLIFO::signal, &sm));
    }
    else
    {
      // ignore line
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));    
    std::cout << "COUNT:   " << sm.count << std::endl;
    std::cout << "WAITING: " << sm.num_waiting << std::endl;  
  }

  return 0;
}
