#include <chrono>
#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

// SEE CURRENT THREAD COUNT WITH
// ps -o nlwp <pid>

// COMPILE with:
// g++ -std=c++17 -pthread <file>.cpp

using namespace std::chrono_literals;

class ThreadPool {

    // TODOs:
    // 1. Support jobs with input / output.
    // 2. Support timing out on jobs.
    // 3. Implement hard-stop that doesn't wait for job queue to clear out.
    
public:

    enum class Status { WAITING, RUNNING, DONE, ERROR };

    using JobType = std::function<void()>;

    struct JobInfo {
        JobInfo(JobType job, int priority, Status status)
                : job(job), priority(priority), status(status) {}
        JobType job;
        int priority;
        Status status;
    };

    ThreadPool(std::size_t numThreads) {
        for (std::size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back(std::thread(&ThreadPool::runThread, this));
        }
    }

    ~ThreadPool() {
        // unblock any threads that are waiting for a job.
        {
            std::unique_lock<std::mutex> lock(mtx);
            stop = true;
            cv.notify_all();
        }

        // Wait for the task queue to clear out.
        for (auto& thread : threads) {
            thread.join();
        }
    }

    // Lower priority number = higher priority.
    size_t submit(JobType job, int priority) {
        
        std::unique_lock<std::mutex> lock(mtx);

        jobs.emplace_back(job, priority, Status::WAITING);
        size_t jobIdx = jobs.size() - 1;
        std::cout << "Queuing JobIdx: " << jobIdx << ", priority: " << priority << std::endl;
        queue.emplace(priority, jobIdx);

        // notify threads waiting for a job.
        cv.notify_all();

        // return id that clients can use to check status.
        return jobs.size() - 1;
    }

    Status getjobstatus(size_t jobId) {
        return jobs[jobId].status;
    }

private:
    void runThread() {

        while (true) {

            size_t jobIdx;
            {
                std::unique_lock<std::mutex> lock(mtx);

                // if job queue is empty, wait for a job to be submitted.
                while (!stop && queue.size() == 0) {
                    cv.wait(lock);
                }

                // if stop is called, keep processing tasks until queue is cleared out.
                if (stop && queue.size() == 0) {
                    break;
                }

                jobIdx = queue.begin()->second;

                // remove job from the queue.
                queue.erase(queue.begin());
                std::cout << "Running JobIdx: " << jobIdx << std::endl;
            }

            jobs[jobIdx].status = Status::RUNNING;
            jobs[jobIdx].job();
            jobs[jobIdx].status = Status::DONE;
        }
    }

private:
    bool stop = false;
    std::mutex mtx;
    std::condition_variable cv;

    std::multimap<int, size_t> queue;
    std::vector<JobInfo> jobs;
    std::vector<std::thread> threads;
};


void sleep() {
    std::this_thread::sleep_for(1s);
}

int main() {

    auto start = std::chrono::system_clock::now();
    {
        ThreadPool tp(4);
        for (int i = 0; i < 11; ++i) {
            tp.submit(&sleep, 11 - i);
        }

        std::this_thread::sleep_for(5s);

        for (int i = 0; i < 11; ++i) {
            tp.submit(&sleep, 11 - i);
        }
    }
    auto end = std::chrono::system_clock::now();

    // Expect total runtime to be around 8 seconds.  
    // 11 tasks launch at t = 0 and complete around t = 3s with 4 workers.
    // 11 tasks launch at t = 5s, complete 3 seconds later.

    std::cout << "Total runtime: "
              << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
              << std::endl;

    return 0;
}
