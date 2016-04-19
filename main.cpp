#include "myqueue.h"

#include <unistd.h>

MyQueue<int>    myQueue(1024);
MyQueue<int>    myBak(0);
void test_bak();
void test_queue();

int main()
{
    std::thread t(test_queue);
    std::thread t2(test_bak);

    t.join();
    t2.join();

    return 0;
}

void test_queue()
{
    usleep(1000);
    while ( !myQueue.empty() )
    {
        int* a = myQueue.malloc();
        myBak.free(a);
        std::cout << "[S: " << myQueue.size() << "] ";
        usleep(1000);
    }
}

void test_bak()
{
    usleep(1000*100);
    while ( !myBak.empty() ) {
        int*a = myBak.malloc();
        myQueue.free(a);
        std::cout << "[B: " << myBak.size() << "] ";
        usleep(1000);
    }
}
