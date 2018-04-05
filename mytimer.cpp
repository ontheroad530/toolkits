#include "mytimer.h"

#include <sys/timerfd.h>
#include <sys/epoll.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <unistd.h>

MyTimer::MyTimer(int64_t us, double interval, const Functor& functor)
    :_run(false), _epoll_fd(-1), _timer_id(-1), _usec(us), _interval(interval), _func(functor)
{
    init();
}

MyTimer::MyTimer(int64_t us, double interval, Functor &&functor)
    :_run(false), _epoll_fd(-1), _timer_id(-1), _usec(us), _interval(interval), _func(std::move(functor) )
{
    init();
}

MyTimer::~MyTimer()
{
    if(_epoll_fd != -1)
        close(_epoll_fd);
    if(_timer_id != -1)
        close(_timer_id);
}

bool MyTimer::init()
{
    _timer_id = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_NONBLOCK);
    _epoll_fd = epoll_create(1);
    assert(_timer_id != -1 && _epoll_fd != -1);

    return _timer_id != -1 && _epoll_fd != -1;
}

bool MyTimer::start()
{
    itimerspec new_value;
    bzero(&new_value, sizeof new_value);
    new_value.it_value.tv_nsec = _usec * 1000;
    new_value.it_interval.tv_nsec = _interval * 1000;

    int32_t ret = timerfd_settime(_timer_id, 0, &new_value, NULL);
    if(ret != 0) return false;

    epoll_event eev;
    eev.data.fd = _timer_id;
    eev.events = EPOLLIN | EPOLLPRI;
    ret = epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _timer_id, &eev);
    if( ret < 0) return false;

    _run = true;
    _thr = std::thread(&MyTimer::thr_run, this);


    return ret == 0;
}

void MyTimer::stop()
{
    itimerspec new_value;
    bzero(&new_value, sizeof new_value);

    timerfd_settime(_timer_id, 0, &new_value, NULL);

    _run = false;
    _thr.join();
}

void MyTimer::thr_run()
{
    while(_run){
        epoll_event eev;
        int32_t num = epoll_wait(_epoll_fd, &eev, 1, 100);
        if( num > 0 )
        {
            if( eev.data.fd == _timer_id)
            {
                if( _func )
                {
                    _func();

                    uint64_t exp = 0;
                    read(_timer_id, &exp, sizeof(uint64_t));
                }
            }
        }
        else
        {
            std::cout << "timeout" << std::endl;
        }
    }
}


