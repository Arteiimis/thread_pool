#include "epoll_server.hpp"
#include <array>
#include <netinet/in.h>
#include <sys/epoll.h>

epollServer::epollServer(int thread_num, int max_thread_num, int queue_max_size)
    : pool(thread_num), is_stop(false),
    sockfd(socket_init()), epoll_fd(epoll_create(1)),
    thread_num(thread_num), max_thread_num(max_thread_num), min_thread_num(thread_num),
    alive_thread_num(0), busy_thread_num(0), main_thread_id(std::this_thread::get_id())
{
    business_queue = std::queue<business_type>();
}

template<class F, class... Args>
auto epollServer::add_business(F&& f, Args&&... args)
-> std::future<typename std::result_of<F(Args...)>::type>
{
    return pool.enqueue(std::forward<F>(f), std::forward<Args>(args)...);
}

int epollServer::epoll_listen()
{
    std::array<struct epoll_event, MAX_EVENTS> events;
    int ready_num;

    while (!is_stop)
    {
        ready_num = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);

        while (ready_num)
        {
            if (events[--ready_num].data.fd == sockfd) {
                add_business([this] { accept(epoll_fd, sockfd); });
            }
            else {
                add_business([this, ready_num, events] { respon(epoll_fd, events[ready_num].data.fd); });
            }
            printf("producer thread TID [0x%x] add a business\n", std::this_thread::get_id());
        }
    }

    return 0;
}

void epollServer::accept(int epoll_fd, int sockfd)
{
    struct sockaddr_in caddr;
    socklen_t caddr_len = sizeof(caddr);
    int cfd = accept(sockfd, (struct sockaddr*) &caddr, &caddr_len);
    if (cfd == -1)
    {
        perror("accept");
        exit(-1);
    }

    struct epoll_event event;
    event.data.fd = cfd;
    event.events = EPOLLIN;
    int res = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &event);
    if (res == -1)
    {
        perror("epoll_ctl");
        exit(-1);
    }
}

epollServer::~epollServer()
{
    stop();
}