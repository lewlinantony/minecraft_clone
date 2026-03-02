#pragma once

#include <thread>
#include <vector>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <stdexcept>
#include <glm/glm.hpp>
#include <deque>


class ThreadPool {

public:
    ThreadPool(size_t threads);
    ~ThreadPool();

    void enqueue(std::function<void()> task);
    void priority_enqueue(std::function<void()> task);


private:
    // worker threads
    std::vector<std::thread> workers;
    std::deque<std::function<void()>> tasks;

    // Synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

    void workerLoop();
};

