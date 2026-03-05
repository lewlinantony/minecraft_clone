#include <threadpool/threadpool.h>

void Threadpool::init(){
    int numThreads = std::thread::hardware_concurrency()-1; // Leave 1 thread free for the main thread
    for(int i=0; i<numThreads; i++){
        workerThreads.emplace_back([this]{
            while(true){
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(workerQueueMutex); // we use unique_lock here because condition_variable requires it 
                    condition.wait(lock, [this]{ return !workerTaskQueue.empty() || stopThreads; }); // wait for a task or stop signal 
                    if(stopThreads) return; // Exit thread if stopping (Hard exit)
                    task = std::move(workerTaskQueue.front());
                    workerTaskQueue.pop_front();
                }
                task(); // Execute the task
            }
        });
    }
}

void Threadpool::cleanup(){
    {
        std::lock_guard<std::mutex> lock(workerQueueMutex);
        stopThreads = true; // Signal threads to stop
    }
    condition.notify_all(); // Wake up all threads to let them exit
    for(std::thread& thread : workerThreads){
        if(thread.joinable()){
            thread.join(); // Wait for all threads to finish
        }
    }
}

void Threadpool::processMainThreadTasks(){

    const double MAX_UPLOAD_TIME_MS = 2.0;
    auto startTime = std::chrono::high_resolution_clock::now();

    while(true){
        std::function<void()> task;
        {
            std::lock_guard<std::mutex> lock(mainThreadQueueMutex);
            if(mainTaskQueue.empty()) break; // No more tasks to process
            task = std::move(mainTaskQueue.front());
            mainTaskQueue.pop();
        }
        task(); // Execute the task

        auto currentTime = std::chrono::high_resolution_clock::now();
        double elapsedTime = std::chrono::duration<double, std::milli>(currentTime - startTime).count();

        // If we ran out of time, stop uploading and save the rest for the next frame
        if (elapsedTime >= MAX_UPLOAD_TIME_MS) {
            break; 
        }        
    }
}

void Threadpool::enqueueBackWorkerTask(std::function<void()> task){
    {
        std::lock_guard<std::mutex> lock(workerQueueMutex);
        workerTaskQueue.push_back(std::move(task));
    }
    condition.notify_one(); // Notify one worker thread that there's a new task    
}

void Threadpool::enqueueFrontWorkerTask(std::function<void()> task){
    {
        std::lock_guard<std::mutex> lock(workerQueueMutex);
        workerTaskQueue.push_front(std::move(task));
    }
    condition.notify_one(); // Notify one worker thread that there's a new task    
}

size_t Threadpool::getWorkerQueueSize() {
    std::lock_guard<std::mutex> lock(workerQueueMutex);
    return workerTaskQueue.size();
}

void Threadpool::enqueueMainTask(std::function<void()> task){
    {
        std::lock_guard<std::mutex> lock(mainThreadQueueMutex);
        mainTaskQueue.push(std::move(task));
    }
}