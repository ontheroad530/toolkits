#include "myqueue.h"
#include "mytimer.h"

#include <unistd.h>

#include "threadlocal.h"

void test_lock_queue();
void test_timer();
void test_thread_local();

int main()
{
//    test_lock_queue();
//    test_timer();

    test_thread_local();

    return 0;
}

void test_thread_local()
{
    gyh::ThreadLocal<std::string>   str;

    auto print = [&](){ std::cout << str.value() << std::endl; };
    std::thread thr1([&](){ print(); str.value() = "hello"; print();});
    std::thread thr2([&](){ print(); str.value() = "world"; print();});

    thr1.join();
    thr2.join();
}

void test_timer()
{
    int32_t count = 10;
    MyTimer mytimer(1000, 1000*100, [&](){std::cout << "xxx " << count << std::flush; });
    mytimer.start();

    while(--count > 0)
    {
        usleep(1000*1000);
    }

    mytimer.stop();
}

void test_lock_queue()
{
    MyQueue<int>    myQueue(1024);
    MyQueue<int>    myBak(0);

    auto test_queue = [&](){
        usleep(1000);
        while ( !myQueue.empty() )
        {
            int* a = myQueue.malloc();
            myBak.free(a);
            std::cout << "[S: " << myQueue.size() << "] ";
            usleep(1000);
        };
    };

    auto test_bak = [&](){
        usleep(1000*100);
        while ( !myBak.empty() ) {
            int*a = myBak.malloc();
            myQueue.free(a);
            std::cout << "[B: " << myBak.size() << "] ";
            usleep(1000);
        }
    };

    std::thread t(test_queue);
    std::thread t2(test_bak);

    t.join();
    t2.join();
}
