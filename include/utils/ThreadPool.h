#pragma once

#include <thread>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <stdexcept>

class ThreadPool {

public:
    ThreadPool(size_t threads);
    ~ThreadPool();

    template<class F, class... Args>
    void enqueue(F&& f, Args&&... args);

private:
    // worker threads
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    // Synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

    void workerLoop();
};


inline ThreadPool::ThreadPool(size_t threads) : stop(false) {
    if (threads == 0) {
        threads = std::thread::hardware_concurrency();
    }
    for (size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            this->workerLoop();
        });
    }
}


inline void ThreadPool::workerLoop(){
    while (true){
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
             this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });

             if (this->stop && this->tasks.empty()) {
                 return; // Exit if stop is true and no tasks are left
             }

             task = std::move(this->tasks.front());
             this->tasks.pop();
        }
        
        task(); // Execute the task
    }
}

template<class F, class... Args>
void ThreadPool::enqueue(F&& f, Args&&... args){
    auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks.emplace(std::move(task));
    }

    condition.notify_one(); // Notify one worker thread that a new task is available
}

inline ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true; // Set the stop flag to true
    }
    condition.notify_all(); // Notify all worker threads to wake up and exit
    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join(); // Wait for all threads to finish
        }
    }
}