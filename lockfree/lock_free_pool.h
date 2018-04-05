#ifndef LOCK_FREE_POOL_H
#define LOCK_FREE_POOL_H

#include <set>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <utility>
#include <cstdio>

#include "lock_free_node.h"

#pragma pack (1)

template <class T>
class Lock_Free_Pool
{
public:
    template<class ...Args>
    int32_t init(int32_t pool_size, Args&&... args);
    void release();

    T* malloc();
    int32_t free(T* t);

protected:
    std::atomic_int_least64_t   _pool_size;
    std::atomic_int_fast64_t    _head;
    std::atomic_int_fast64_t    _tail;
    Node_Pool<T*>               *_node_pool;
};

#pragma pack()

template<class T>
template<class ... Args>
int32_t Lock_Free_Pool<T>::init(int32_t pool_size, Args&& ... args)
{
    _pool_size = pool_size;
    pool_size++;

    void *ptr = std::malloc(sizeof(Node_Pool<T*>) + pool_size * sizeof(typename Node_Pool<T*>::Node));
    assert(ptr != nullptr);
    memset(ptr, 0, sizeof(Node_Pool<T*>) + pool_size * sizeof(typename Node_Pool<T*>::Node) );

    _node_pool = new (ptr) Node_Pool<T*>;
    int32_t rc = _node_pool->init(pool_size);
    if(rc != 0)
    {
        release();
        return -1;
    }
    typename Node_Pool<T*>::Node *node = _node_pool->malloc();
    assert(node != nullptr);

    set_null(node->next());

    _head.store(node->index());
    _tail.store(node->index());

    T* t = new T(std::forward<Args>(args)...);
    assert( t != nullptr);
    node->object() = t;

    pool_size--;
    while( pool_size-- )
    {
        T* t = new T(std::forward<Args>(args)...);
        assert( t != nullptr);
        this->free( t);
    }

    return 0;
}

template <class T>
void Lock_Free_Pool<T>::release()
{
    assert(_node_pool != nullptr);

    typedef typename Node_Pool<T*>::Node NODE;
    NODE *tmp = _node_pool->pointer( _head.load() );

    while( tmp )
    {
        delete tmp->object();
        tmp->object() = nullptr;
        tmp = _node_pool->pointer( tmp->next() );
    }

    if(_node_pool != nullptr)
    {
        _node_pool->release();
        ::free(_node_pool);
        _node_pool = nullptr;
    }
}

template <class T>
int32_t Lock_Free_Pool<T>::free(T* obj)
{
    if(obj == nullptr) return 0;

    typedef typename Node_Pool<T*>::Node NODE;
    NODE *node = _node_pool->malloc();
    if(node == nullptr)
    {
        assert(0);
        return -1;
    }

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
T* Lock_Free_Pool<T>::malloc()
{
    T* obj = nullptr;
    typedef typename Node_Pool<T*>::Node NODE;

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
                    obj = nullptr;
                    break;
                }

                int64_t new_tail = combine_ptr_tag(get_ptr(head_next), get_next_tag(tail) );
                _tail.compare_exchange_strong(tail, new_tail);
            }
            else
            {
                if( head_next_node == nullptr)
                    continue;

                obj = head_next_node->object();

                int64_t new_head = combine_ptr_tag( get_ptr(head_next), get_next_tag(head));
                if(_head.compare_exchange_weak(head, new_head) )
                {
                    _node_pool->free( get_ptr(head) );
                    break;
                }
            }
        }
    }

    if(obj != nullptr) obj->reset(); //NOTE: 标准类型无法使用

    return obj;
}

#endif // LOCK_FREE_POOL_H
