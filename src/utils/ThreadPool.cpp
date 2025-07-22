#include "utils/ThreadPool.h"

// making the worker threads
ThreadPool::ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; i++) {
        //emplace_back convert the function ref into a thread and its apprently more efficient than push_back
        workers.emplace_back([this] {
            this->workerLoop();
        });
    }
}


void ThreadPool::workerLoop(){
    while (true){
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            // this reference is just used to safety
             this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });

             if (this->stop && this->tasks.empty()) {
                 return; // Exit if stop is true and no tasks are left
             }

             task = std::move(this->tasks.front());
             this->tasks.pop_front();
        }
        
        task(); // Execute the task
    }
}


void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }
        tasks.emplace_back(std::move(task));
    }

    condition.notify_one(); // Notify one worker thread that a new task is available
}

void ThreadPool::priority_enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop) {
            throw std::runtime_error("priority enqueue on stopped ThreadPool");
        }
        tasks.emplace_front(std::move(task));
    }

    condition.notify_one(); // Notify one worker thread that a new task is available
}

ThreadPool::~ThreadPool() {
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