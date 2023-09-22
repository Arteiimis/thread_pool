#include "epoll_server.hpp"
#include <array>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

    while (!is_stop) {
        ready_num = epoll_wait(epoll_fd, events.data(), MAX_EVENTS, -1);

        while (ready_num) {
            if (events[--ready_num].data.fd == sockfd) {
                add_business([this] { accept_conn(epoll_fd, sockfd); });
            }
            else {
                add_business([this, ready_num, events] { respon_conn(epoll_fd, events[ready_num].data.fd); });
            }
            printf("producer thread TID [0x%x] add a business\n", std::this_thread::get_id());
        }
    }

    return 0;
}

void epollServer::accept_conn(int epoll_fd, int sockfd)
{
    epoll_event node;           // 用于注册事件
    int serverfd = sockfd;      // 服务器套接字
    int clientfd;               // 客户端套接字
    sockaddr_in client_addr;    // 客户端地址
    socklen_t client_addr_len = sizeof(client_addr);    // 客户端地址长度

    std::string ip; // 客户端ip
    int flags;      // 用于设置套接字为非阻塞

    if ((clientfd = accept(serverfd, (sockaddr*) &client_addr, &client_addr_len)) > 0) {
        flags = fcntl(clientfd, F_GETFL, 0);
        fcntl(clientfd, F_SETFL, flags | O_NONBLOCK);

        printf("accept a new client: %s:%d\n",
                inet_ntoa(client_addr.sin_addr), client_addr.sin_port);

        node.data.fd = clientfd;
        node.events = EPOLLIN | EPOLLET;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientfd, &node) < 0) {
            perror("epoll_ctl error");
            exit(1);
        }
    }
}

void epollServer::respon_conn(int epollf_fd, int sockfd)
{
    int clientfd = sockfd;
    std::string buffer;
    buffer.resize(BUFFER_SIZE);
    int read_size, send_size;
    int flags;

    try {
        while ((read_size = recv(clientfd, &buffer[0], BUFFER_SIZE, 0)) > 0) {
            flags = 0;
            while (read_size > flags) {
                buffer[flags] = toupper(buffer[flags]);
                flags++;
            }
            send_size = send(clientfd, &buffer[0], read_size, 0);
            if (send_size < 0) { throw std::runtime_error("send error"); }

            printf("send %d bytes to client\n", send_size);
        }
        if (read_size == 0) {
            printf("client closed\n");
            if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clientfd, NULL) < 0) {
                perror("epoll_ctl error");
                exit(1);
            }
            close(clientfd);
        }
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

epollServer::~epollServer()
{ stop(); }