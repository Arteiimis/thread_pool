#ifndef TASK_QUEUE_HPP
#define TASK_QUEUE_HPP

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <utility>

template <typename T>
class taskQueue
{
private:
    std::queue<int> tasks;
    std::mutex m;

public:
    taskQueue() = default;
    taskQueue(taskQueue&& other) = delete;
    ~taskQueue() = default;

    bool empty()
    {
        std::unique_lock<std::mutex> lock(m);
        return tasks.empty();
    }

    int size()
    {
        std::unique_lock<std::mutex> lock(m);
        return tasks.size();
    }

    void enqueue(T& task)
    {
        std::unique_lock<std::mutex> lock(m);
        tasks.emplace(task);
    }

    T deqeue()
    {
        std::unique_lock<std::mutex> lock(m);
        if(!tasks.empty())
        {
            T task = std::move(tasks.front());
            tasks.pop();
            return task;
        }
        else
            return nullptr;
    }

    std::mutex& getMutex()
    {
        return m;
    }
};

#endif