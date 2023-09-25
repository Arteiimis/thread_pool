#include <iostream>
#include <memory>
#include <thread>
#include <ranges>
#include "epoll_server.hpp"

void test_func(int a, int b)
{
    std::cout << a << " " << b << std::endl;
}

int main()
{
    epollServer server(4, 10, 100);
    server.start();
    server.stop();
    return 0;
}


