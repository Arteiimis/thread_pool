#include <iostream>
#include <async_simple/coro/Lazy.h>
#include <async_simple/coro/SyncAwait.h>
#include <coroutine>
#include <memory>
#include <thread>
#include <ranges>
#include <ylt/easylog.hpp>

using namespace async_simple::coro;
using namespace ylt;

auto test_func(int a, int b) -> int
{
    ELOG_INFO << "thread id: " << std::this_thread::get_id()
        << " a: " << a
        << " b: " << b
        << " a + b: "
        << a + b << "\n";
    return a + b;
}

int main()
{
    // // threadPool pool(4);
    // auto f1 = pool.enqueue(test_func, 1, 2);
    // auto f2 = pool.enqueue(test_func, 3, 4);
    // auto f3 = pool.enqueue(test_func, 5, 6);
    // auto f4 = pool.enqueue(test_func, 7, 8);

    // ELOG_INFO << "f1: " << f1.get() << "\n";
    // ELOG_INFO << "f2: " << f2.get() << "\n";
    // ELOG_INFO << "f3: " << f3.get() << "\n";
    // ELOG_INFO << "f4: " << f4.get() << "\n";

    // return 0;
}