#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <type_traits>
#include "task_queue.hpp"

template <typename T>
class threadPool
{
public:
    using task_type = std::function<void()>; // task is a function that returns void and takes no arguments

private:
    taskQueue<task_type> tasks;
    std::vector<std::thread> threads;

    std::condition_variable cv;
    bool stop;
    size_t tasks_num;           // number of tasks that one thread can execute

public:
    threadPool(size_t thread_num, size_t tasks_num)
        : stop(false), tasks_num(tasks_num)
    {
        for (size_t i = 0; i < thread_num; ++i)
        {
            threads.emplace_back([this] {
                while (true)
                {
                    task_type task;
                    {
                        // wait for a task
                        std::unique_lock<std::mutex> lock(tasks.getMutex());
                        cv.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty())
                            return;
                        task = std::move(tasks.deqeue());
                    }
                    task(); // execute task
                }
            });
        }
    }

    template <class F, class... Args>
    auto enqueue(F&& func, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = std::future<typename std::result_of<F(Args...)>::type>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(func), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(tasks.getMutex());
            if (tasks.size() >= tasks_num)
                throw std::runtime_error("too many tasks");
            if (stop)
                throw std::runtime_error("enqueue on stopped thread pool");
            tasks.enqueue([task]() { (*task)(); });
        }
    }
};

#endif