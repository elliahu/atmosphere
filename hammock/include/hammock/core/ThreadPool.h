#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>

namespace hammock {

    class ThreadPool {
public:
    ThreadPool()
        : stop(false), activeCount(0) {}

    // Sets (or resets) the number of worker threads.
    // If threads already exist, waits for current work, stops them, and spawns new threads.
    void setThreadCount(uint32_t threadCount) {
        // Stop any existing worker threads.
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : threads) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        threads.clear();

        // Reset stop flag.
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stop = false;
        }

        // Spawn new worker threads.
        for (uint32_t i = 0; i < threadCount; ++i) {
            threads.emplace_back([this] {
                while (true) {
                    std::function<void()> job;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !jobQueue.empty(); });
                        // If stopping and no jobs remain, exit the thread loop.
                        if (stop && jobQueue.empty())
                            return;
                        job = std::move(jobQueue.front());
                        jobQueue.pop();
                        ++activeCount; // mark job as in progress
                    }
                    // Execute job outside the lock.
                    job();
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        --activeCount;
                        // Notify waiters if there are no queued or active jobs.
                        if (jobQueue.empty() && activeCount == 0)
                            finishedCondition.notify_all();
                    }
                }
            });
        }
    }

    // Submit a new job to the pool.
    void submit(std::function<void()> job) {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            jobQueue.push(std::move(job));
        }
        condition.notify_one();
    }

    // Wait until all jobs have been processed.
    void wait() {
        std::unique_lock<std::mutex> lock(queueMutex);
        finishedCondition.wait(lock, [this] {
            return jobQueue.empty() && (activeCount == 0);
        });
    }

    // Destructor: stop all threads and join them.
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : threads) {
            if (worker.joinable())
                worker.join();
        }
    }

private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> jobQueue;
    std::mutex queueMutex;
    std::condition_variable condition;
    std::condition_variable finishedCondition;
    std::atomic<int> activeCount;
    bool stop;
};
}
