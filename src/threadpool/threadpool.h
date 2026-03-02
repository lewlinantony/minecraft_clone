#pragma once

#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <shared_mutex>


class Threadpool{
    public:
        void init();
        void cleanup();
        void processMainThreadTasks();
        
        void enqueueWorkerTask(std::function<void()> task);
        size_t getWorkerQueueSize();

        void enqueueMainTask(std::function<void()> task);        
        
    private:
        
        std::deque<std::function<void()>> workerTaskQueue;
        std::mutex workerQueueMutex;
        
        std::queue<std::function<void()>> mainTaskQueue;
        std::mutex mainThreadQueueMutex;        

        std::vector<std::thread> workerThreads;       
        std::condition_variable condition;
        bool stopThreads = false; // Flag to signal threads to stop
    };