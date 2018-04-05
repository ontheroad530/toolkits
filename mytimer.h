#ifndef MYTIMER_H
#define MYTIMER_H

#include <thread>
#include <functional>

typedef std::function<void()> Functor;
class MyTimer
{
public:
    MyTimer(int64_t us, double interval, const Functor& functor);
    MyTimer(int64_t us, double interval, Functor&& functor);
    ~MyTimer();

    bool start();
    void stop();

private:
    bool init();
    void thr_run();

private:
    bool        _run;
    int32_t     _epoll_fd;
    int32_t     _timer_id;
    int64_t     _usec;
    double      _interval;
    std::thread _thr;
    Functor     _func;
};

#endif // MYTIMER_H
