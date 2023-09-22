#ifndef EPOLL_SERVER_HPP
#define EPOLL_SERVER_HPP

#include "thread_pool.hpp"
#include <cstddef>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <type_traits>
#include <sys/epoll.h>
#include <vector>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 80
#define BUFFER_SIZE 1024
#define MAX_EVENTS 1024

class epollServer
{
public:
    using business_type = std::function<void()>;            // 任务类型
    using business_socket_type = std::function<void(int)>;  // 任务类型

public:
    epollServer(int thread_num, int max_thread_num, int queue_max_size);
    ~epollServer();

    void start();
    void stop();

    template<class F, class... Args>
    auto add_business(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    void manager_job();

    void set_thread_num(int thread_num);
    void set_max_thread_num(int max_thread_num);
    void set_min_thread_num(int min_thread_num);

    void accept(int epoll_fd, int sockfd);
    void respon(int epoll_fd, int sockfd);

    int epoll_listen();
    int socket_init();
    int get_thread_num();
    int get_max_thread_num();
    int get_min_thread_num();

    int get_working_thread_num();
    int get_busy_thread_num();

    std::thread::id get_main_thread_id();
    std::mutex& get_thread_lock();
    std::condition_variable& get_not_full();
    std::condition_variable& get_not_empty();

    std::queue<business_type>& get_business_queue();

private:
    threadPool pool;

    bool is_stop;                   // 是否停止
    int sockfd;                     // 监听的套接字
    int epoll_fd;                   // 监听的文件描述符
    int thread_num;                 // 线程池中的线程数
    int max_thread_num;             // 线程池中的最大线程数
    int min_thread_num;             // 线程池中的最小线程数
    int alive_thread_num;           // 线程池中的存活线程数
    int busy_thread_num;            // 正在忙碌的线程数
    std::thread::id main_thread_id; // 主线程的id

    std::mutex thread_lock;             // 线程锁
    std::condition_variable not_full;   // 生产者条件变量
    std::condition_variable not_empty;  // 消费者条件变量

    std::queue<business_type> business_queue;   // 任务队列
};

#endif // !EPOLL_SERVER_HPP