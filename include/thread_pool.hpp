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
#include <future>
#include <queue>
#include <vector>

class threadPool
{
public:
    using task_type = std::function<void()>;
    threadPool(size_t);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~threadPool();

private:
    std::vector<std::thread> workers;
    std::queue<task_type> tasks;
    std::condition_variable cv;
    std::mutex mutex;
    bool stop{ false };
};

inline threadPool::threadPool(size_t threads)
{
    for (size_t i = 0; i < threads; i++)
    {
        workers.emplace_back([this] {
            for (;;)
            {
                task_type task;
                {
                    std::unique_lock<std::mutex> lock(this->mutex);
                    this->cv.wait(lock,
                        [this] { return this->stop || !this->tasks.empty(); });
                    if (this->stop && this->tasks.empty())
                        return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
        });
    }
}

template<class F, class... Args>
auto threadPool::enqueue(F&& func, Args&&... args)
->std::future<typename std::result_of<F(Args...)> ::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    // 将函数和参数绑定，构造一个packaged_task, 并将其future返回
    auto task = std::make_shared<std::packaged_task<return_type()> >(
        std::bind(std::forward<F>(func), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(mutex);

        // 禁止在线程池停止后添加新的任务
        if (stop)
            throw std::runtime_error("enqueue on stopped thread pool");

        // 此时task是一个shared_ptr，所以需要用*task()来调用
        tasks.emplace([task]() { (*task)(); });
    }
    cv.notify_one();
    return res;
};

inline threadPool::~threadPool()
{
    {
        std::unique_lock<std::mutex> lock(mutex);
        stop = true;
    }
    cv.notify_all();
    for (std::thread& worker : workers)
        worker.join();
}

#endif