#include <iostream>
#include <async_simple/coro/Lazy.h>
#include <async_simple/coro/SyncAwait.h>
#include <co_context/generator.hpp>
#include <coroutine>
#include <memory>
#include <thread>
#include <ranges>
#include <ylt/easylog.hpp>
#include "thread_pool.hpp"

using namespace async_simple::coro;
using namespace ylt;
using co_context::generator;

auto test_func(int a, int b) -> int
{
    ELOG_INFO << "thread id: " << std::this_thread::get_id()
        << " a: " << a
        << " b: " << b
        << " a + b: "
        << a + b << "\n";
    return a + b;
}

generator<int> fib(int n)
{
    int a = 1, b = 1;
    co_yield a;
    co_yield b;

    while (true)
    {
        ELOG_INFO << "thread id: " << std::this_thread::get_id();
        int c = a + b;
        co_yield c;
        a = b;
        b = c;
    }
}

// Lazy<int> fib(int n)
// {
//     int a = 1, b = 1;
//     co_yield a;
//     co_yield b;

//     while (true)
//     {
//         int c = a + b;
//         co_yield c;
//         a = b;
//         b = c;
//     }
// }

int main()
{
    using std::views::take;
    threadPool pool(4);

    for (int i = 0; i < 10; ++i)
    {
        auto res = pool.enqueue(test_func, i, i + 1);
        std::cout << res.get() << std::endl;
    }

    for (int i = 0; i < 10; ++i)
    {
        auto res = pool.enqueue(fib, i);
        for (int i : res.get() | take(10))
        {
            std::cout << i << std::endl;
        }
    }
}