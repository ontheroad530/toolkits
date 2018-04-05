#ifndef MYQUEUE_H
#define MYQUEUE_H

#include <list>
#include <mutex>
#include <thread>
#include <iostream>
#include <condition_variable>


template<typename T>
class MyQueue
{
public:
    MyQueue(size_t queue_size) :m_queueSize(queue_size)
    {
        for(size_t i = 0; i < queue_size; ++i)
        {
            m_queue.push_back( new T() );
        }
    }

    T*  malloc()
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        m_notEmpty.wait( locker, [this]{ return !m_queue.empty(); } );
        T* x = m_queue.front();
        m_queue.pop_front();

        return x;
    }

    void free(T* t)
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        m_queue.push_back(t);
        m_notEmpty.notify_one();
    }

    bool empty()
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_queue.empty();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_queue.size();
    }

private:
    size_t     m_queueSize;
    std::mutex  m_mutex;
    std::list<T*>    m_queue;
    std::condition_variable m_notEmpty;
};

#endif // MYQUEUE_H
