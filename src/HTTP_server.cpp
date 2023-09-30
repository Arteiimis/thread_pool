#include <cstdio>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include <pthread.h>
#include <cstdlib>
#include <cassert>
#include <fcntl.h>
#include <stdexcept>

using namespace std;

int socketInit()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket");
        return -1;
    }

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(80);
    saddr.sin_addr.s_addr = inet_addr("172.23.73.141");

    int res = bind(sockfd, (struct sockaddr*) &saddr, sizeof(saddr));
    if (res == -1)
    {
        perror("bind");
        return -1;
    }

    res = listen(sockfd, 5);
    if (res == -1)
    {
        perror("listen");
        return -1;
    }

    return sockfd;
}

struct file_info
{
    int fd;
    int size;

    file_info(int fd, int size)
        : fd(fd), size(size) { }
};


file_info open_file(string name)
{
    int fd = open(name.c_str(), O_RDONLY);
    if (fd == -1)
    {
        throw std::runtime_error("open");
        exit(-1);
    }

    int size = lseek(fd, 0, SEEK_END);
    if (size == -1)
    {
        throw std::runtime_error("lseek");
        exit(-1);
    }

    return file_info{ fd, size };
}

char* get_filename(char* buff)
{
    char* p = nullptr;
    char* s = strtok_r(buff, " ", &p);
    assert(s != nullptr);
    printf("s: %s\n", s);

    s = strtok_r(nullptr, " ", &p);

    return s;
}

void* thread_func(void* arg)
{
    int cfd = *(int*) arg;
    while (true)
    {
        char buff[1024] = { 0 };
        int res = recv(cfd, buff, sizeof(buff), 0);
        if (res <= 0)
        {
            break;
        }

        printf("recv: \n%s\n", buff);

        std::string filename = get_filename(buff);
        printf("filename: %s\n", filename.c_str());

        string path = "/home/artemiss/cpp_test/9_19/";
        if (filename == "/")
        {
            filename = "/index.html";
        }
        else
        {
            path += filename;
        }

        int file_size = 0;
        try
        {
            file_info fi = open_file(path);
            file_size = fi.size;
            printf("file_size: %d\n", file_size);
        }
        catch (const std::runtime_error& e)
        {
            printf("error: %s\n", e.what());
            file_size = 0;
        }

        // 组装响应报文
        string header = "HTTP/1.1 200 OK\r\n";
        header += "Content-Type: text/html\r\n";
        header += "Content-Length: " + to_string(file_size) + "\r\n";
        header += "\r\n";

        send(cfd, header.c_str(), header.size(), 0);

        string body;
        if (file_size > 0)
        {
            body.resize(file_size);
            int fd = open(path.c_str(), O_RDONLY);
            if (fd == -1)
            {
                throw std::runtime_error("open");
                exit(-1);
            }
            else
            {
                int res = read(fd, &body[0], file_size);
                if (res == -1)
                {
                    throw std::runtime_error("read");
                    exit(-1);
                }
            }

            send(cfd, body.c_str(), body.size(), 0);

            // send(cfd, buff, strlen(buff), 0);
        }

        close(cfd);
        return nullptr;
    }

    close(cfd);
    return nullptr;
}

int main()
{
    int socketfd = socketInit();
    if (socketfd == -1)
    {
        exit(-1);
    }

    while (true)
    {
        int c = accept(socketfd, nullptr, nullptr);
        if (c == -1)
        {
            perror("accept");
            continue;
        }

        std::shared_ptr<int> cfd(new int(c));
        pthread_t tid;
        pthread_create(&tid, nullptr, thread_func, cfd.get());
    }
}