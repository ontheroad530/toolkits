#ifndef LOCK_FREE_QUEUE_H
#define LOCK_FREE_QUEUE_H

#include <stdio.h>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <unistd.h>
#include <sys/eventfd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "lock_free_node.h"

template <class T>
class Lock_Free_Queue
{
public:
    int32_t init(int32_t queue_size, const bool use_efd = true);
    void release();

    int32_t efd() { return _efd; }
    int32_t enqueue(T const& obj);
    int32_t dequeue(T& obj);
    int32_t dequeue(T& obj, int usec);
    std::int32_t queue_counts(void);
    std::int32_t queue_size(void);

private:
    int32_t             _efd;
    std::atomic_int_fast64_t    _head;
    std::atomic_int_fast64_t    _tail;
    Node_Pool<T>                *_node_pool;
    std::atomic_int _queue_counts;
    std::int32_t _queue_size;
};

template <class T>
int32_t Lock_Free_Queue<T>::init(int32_t queue_size, const bool use_efd)
{
    _queue_size = queue_size++;

    void *ptr = std::malloc(sizeof(Node_Pool<T>) + queue_size * sizeof(typename Node_Pool<T>::Node));
    assert(ptr != nullptr);
    memset(ptr, 0, sizeof(Node_Pool<T>) + queue_size * sizeof(typename Node_Pool<T>::Node) );

    _node_pool = new (ptr) Node_Pool<T>;
    int32_t rc = _node_pool->init(queue_size);
    if(rc != 0)
    {
        release();
        return -1;
    }
    typename Node_Pool<T>::Node *node = _node_pool->malloc();
    assert(node != nullptr);

    set_null(node->next());

    _head.store(node->index());
    _tail.store(node->index());

    if( use_efd )
    {
        _efd = eventfd(0, 0);
        if( _efd == -1)
            return -1;
    }
    else
    {
        _efd = -1;
    }
    _queue_counts.store(0);

    return 0;
}

template <class T>
void Lock_Free_Queue<T>::release()
{
    assert(_node_pool != nullptr);

    if(_node_pool != nullptr)
    {
        _node_pool->release();
        ::free(_node_pool);
        _node_pool = nullptr;
    }
}

template <class T>
int32_t Lock_Free_Queue<T>::enqueue(const T& obj)
{
    typedef typename Node_Pool<T>::Node NODE;
    NODE *node =nullptr;
    int32_t pool_size = _node_pool->pool_size();
    std::int64_t nalloc_fail_counts = 0;
    do
    {
        node = _node_pool->malloc();
        if( node == nullptr)
        {
            if( nalloc_fail_counts++ % 10000000 == 0 )
            {
                fprintf(stderr, "WARNING:***LockFreeQueue: enqueue::malloc fail times:%ld, pool-size:%d\n", nalloc_fail_counts, pool_size);
            }
            return -1;
        }
    }while(node==nullptr);

    assert(node != nullptr);
    if(node == nullptr) return -1;

    int32_t node_index = get_ptr(node->index());

    node->object() = obj;
    set_null(node->next());

    while (true)
    {
        int64_t tail = _tail.load();
        NODE *tail_node = _node_pool->pointer(tail);//node_pointer(tail);
        int64_t next = tail_node->next().load();
        NODE* next_ptr = _node_pool->pointer( next );

        int64_t tail2 = _tail.load();

        if( tail == tail2 )
        {
            if( next_ptr == nullptr)
            {
                int64_t new_tail_next = combine_ptr_tag(node_index, get_next_tag(next));
                if( tail_node->next().compare_exchange_weak(next, new_tail_next) )
                {
                    int64_t new_tail = combine_ptr_tag(node_index, get_next_tag(tail));
                    _tail.compare_exchange_strong(tail, new_tail);

                    {
                        if( -1 != _efd)
                        {
                            uint64_t u = 1;
                            assert ( -1 != eventfd_write(_efd, u) );
                        }
                        _queue_counts++;
                    }
                    return 0;
                }
            }
            else
            {
                int64_t new_tail = combine_ptr_tag( get_ptr(tail_node->next()), get_next_tag(tail));
                _tail.compare_exchange_strong(tail, new_tail);
            }
        }
    }
}

template <class T>
int32_t Lock_Free_Queue<T>::dequeue(T &obj)
{
    typedef typename Node_Pool<T>::Node NODE;

    int32_t ret = 0;
    while (true)
    {
        int64_t head = _head.load();
        NODE *head_node = _node_pool->pointer(head);
        int64_t tail = _tail.load();
        int64_t head_next = head_node->next().load();
        NODE* head_next_node = _node_pool->pointer(head_next);

        int64_t head2 = _head.load();

        if( head == head2 )
        {
            if( get_ptr(head) == get_ptr(tail) )
            {
                if(head_next_node == nullptr)
                {
                    ret = -1;
                    break;
                }

                int64_t new_tail = combine_ptr_tag(get_ptr(head_next), get_next_tag(tail) );
                _tail.compare_exchange_strong(tail, new_tail);
            }
            else
            {
                if( head_next_node == nullptr)
                    continue;

                //ret = head_next_node->_value;
                obj = head_next_node->object();

                int64_t new_head = combine_ptr_tag( get_ptr(head_next), get_next_tag(head));
                if(_head.compare_exchange_weak(head, new_head) )
                {
                    _node_pool->free( get_ptr(head) );
                    ret = 0;
                    break;
                }
            }
        }
    }

    if( ( 0 == ret ) && (-1 != _efd) )
    {
        //eventfd--
        uint64_t u = 0;
        assert (-1 != eventfd_read(_efd, &u) );
        u--;
        assert (-1 != eventfd_write(_efd, u) );
    }
    _queue_counts--;

    return ret;
}

template <class T>
int32_t Lock_Free_Queue<T>::dequeue(T& obj, int usec )
{
    if( -1 == _efd)
    {
#if 0
        struct timeval delay;
        delay.tv_sec = usec / 1000000;
        delay.tv_usec = usec % 1000000;
        select(0, NULL,NULL, NULL, &delay);
#endif
        return dequeue(obj);
    }
    fd_set rfds;
    struct timeval tv;
    int retval;

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(_efd, &rfds);

    /* Wait up to sec seconds. */
    tv.tv_sec = usec / 1000000;
    tv.tv_usec = usec % 1000000;

    retval = select(_efd+1, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

    if (retval == -1)
    {
        perror("int32_t Lock_Free_Queue<T>::dequeue(T& obj, int usec ) : select()");
    }
    else if (retval)//YES DATA
    {
        return dequeue(obj);
    }
    else//NO DATA
    {
        //printf("int32_t Lock_Free_Queue<T>::dequeue,TIME-OUT:%d/usec, %d/sec", usec, sec);
    }
    return -1;
}

template <class T>
std::int32_t Lock_Free_Queue<T>::queue_counts(void)//zq+
{
    return _queue_counts.load();
}

template <class T>
std::int32_t Lock_Free_Queue<T>::queue_size(void)//zq+
{
    return _queue_size;
}

#endif // LOCK_FREE_QUEUE_H
