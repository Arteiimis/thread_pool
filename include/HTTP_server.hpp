#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <pthread.h>
#include <cstdlib>
#include <cassert>
#include <fcntl.h>
#include <stdexcept>
#include <ylt/easylog.hpp>
#include "thread_pool.hpp"

int sock_init()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        throw std::runtime_error("socket");
        return -1;
    }

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(80);
    saddr.sin_addr.s_addr = inet_addr("172.26.149.240");

    int res = bind(sockfd, (struct sockaddr*) &saddr, sizeof(saddr));
    if (res == -1) {
        throw std::runtime_error("bind");
        return -1;
    }

    res = listen(sockfd, 5);
    if (res == -1) {
        throw std::runtime_error("listen");
        return -1;
    }

    return sockfd;
}

class file_info
{
public:
    int fd;
    ssize_t size;
    std::string file_name;

    file_info(const std::string& name)
        : file_name(name)
    {
        int fd = ::open(name.c_str(), O_RDONLY);
        if (fd == -1) {
            throw std::runtime_error("open");
        }
        this->fd = fd;
        this->size = lseek(fd, 0, SEEK_END);
    }

    void open(const std::string& name)
    {
        int fd = ::open(name.c_str(), O_RDONLY);
        if (fd == -1) {
            throw std::runtime_error("open");
        }
        this->fd = fd;
        this->size = lseek(fd, 0, SEEK_END);
    }

    std::string name()
    {
        return file_name;
    }

    ~file_info()
    {
        close(fd);
    }
};

std::string get_filename(char* buff)
{
    char* p = buff;
    char* s = strtok_r(buff, " ", &p);
    assert(s != nullptr);
    ELOG_INFO << "s: " << s;

    s = strtok_r(nullptr, " ", &p);
    return s;
}

void* handle(void* request)
{
    char buff[1024];
    while (true) {
        int res = recv(*(int*) request, buff, 1024, 0);
        if (res <= -1) {
            throw std::runtime_error("recv");
            break;
        }
        ELOG_INFO << "recv: " << buff;

        std::string filename = get_filename(buff);
        std::cout << "filename: " << filename << std::endl;

        std::string path = "/root/thread_pool/src";
        if (filename == "/") {
            filename = "/index.html";
        }
        path += filename;

        try {
            // 打开文件
            file_info file(path);
            std::string header = "HTTP/1.1 200 OK\r\n";
            header += "Content-Type: text/html\r\n";
            header += "Content-Length: " + std::to_string(file.size) + "\r\n";
            header += "\r\n";

            // 发送响应报文, 先发送header, 再发送文件内容
            send(*(int*) request, header.c_str(), header.size(), 0);

            // 使用mmap将文件映射到内存中, 然后发送
            char* p = (char*) mmap(nullptr, file.size, PROT_READ, MAP_PRIVATE, file.fd, 0);
            if (p == MAP_FAILED) {
                throw std::runtime_error("mmap");
            }

            // 发送文件内容, 一次性发送
            send(*(int*) request, p, file.size, 0);
            munmap(p, file.size);
        } catch (const std::exception& e) {
            std::cerr << e.what() << '\n';
        }
    }

    close(*(int*) request);
    delete (int*) request;
    return nullptr;
}

#endif // !HTTP_SERVER_HPP