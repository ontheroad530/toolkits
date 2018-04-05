#ifndef LOCK_FREE_NODE_H
#define LOCK_FREE_NODE_H

#include <cinttypes>
#include <cassert>
#include <atomic>

#include "tagged.h"

/**
 * TODO: modify node pool's algorithm, such as exchanger backoff ...
*/

//use array(node pool) to support aba solution
#pragma pack (1)
template <class T>
class Node_Pool
{
public:
    class Node
    {
    public:
        ~Node(){}
        std::atomic_int_fast64_t& index() {return _index;}
        std::atomic_int_fast64_t& next() {return _next;}
        std::atomic_int_fast64_t& prev() {return _prev;}
        T& object(){return _obj;}
    private:

        std::atomic_int_fast64_t  _index;
        std::atomic_int_fast64_t  _next;
        std::atomic_int_fast64_t  _prev;
        char    _temp0[8];
        T       _obj;
        char    _temp1[16 - (sizeof(T) % 16 )];
    };

    int32_t init(int32_t pool_size);
    int32_t pool_size(void){ return _size; }//zq+
    void release();

    Node* pointer(int32_t i);
    Node* pointer(int64_t i);

    void free(int64_t index);
    void free(int32_t index);

    Node* malloc();
    void free(Node* node);

    //NOTE: public for test
public:
//private:

    int32_t                                 _size;
    int32_t                                 _temp[1];
    std::atomic_int_fast64_t                _head_index;
    Node                                    _nodes[0];
};

#pragma pack()


template <class T>
int32_t Node_Pool<T>::init(int32_t pool_size)
{
    _size = pool_size;
    _head_index.store(0);
    //    _nodes = new Node [_size];
    //    assert(nodes != nullptr);

    Node *n = &_nodes[0];
    new (n) Node;

    n->index().store(0);
    n->next().store(1);
    set_null(n->prev());

    int64_t index = 1;
    int64_t next = 2;
    int64_t prev = 0;

    for(int32_t i = 1; i < _size; ++i)
    {
        n = &_nodes[i];
        new (n) Node;
        n->index() = index++; n->next() = next++; n->prev() = prev++;
    }

    n = &_nodes[_size - 1];
    new (n) Node;
    set_null(n->next());

    return 0;
}

template <class T>
void Node_Pool<T>::release()
{
    Node *n = nullptr;
    for (int i = 0; i < _size; ++i)
    {
        n = &_nodes[i];
        n->~Node();
    }
}

template <class T>
typename Node_Pool<T>::Node*Node_Pool<T>::pointer(int32_t i)
{
    if(i < 0 || i  >= _size)
    {
        assert(0);
        return nullptr;
    }
    return &_nodes[i];
}

template <class T>
typename Node_Pool<T>::Node* Node_Pool<T>::pointer(int64_t ii)
{
    int32_t i = get_ptr(ii);
    if(i < 0 || i  >= _size)
    {
        //assert(0);
        return nullptr;
    }
    return &_nodes[i];
}

template<class T>
typename Node_Pool<T>::Node* Node_Pool<T>::malloc()
{
    Node *mb = nullptr;
    int64_t old_head = _head_index.load();

    while (true)
    {
        if( is_null(old_head) )
            return nullptr;

        mb = pointer( get_ptr(old_head) );
        int32_t new_head_tag = get_next_tag(old_head);
        int64_t new_head = combine_ptr_tag(mb->next(), new_head_tag);

        if( _head_index.compare_exchange_weak(old_head, new_head))
        {
            set_null( mb->next() );
            return mb;
        }
    }
}

template<class T>
void Node_Pool<T>::free(int64_t index)
{
    Node *mb = pointer(index);
    if (mb == nullptr) return;
    this->free(mb);
}

template<class T>
void Node_Pool<T>::free(int32_t index)
{
    Node *mb = pointer(index);
    if (mb == nullptr) return;
    this->free(mb);
}

template<class T>
void Node_Pool<T>::free(Node* mb)
{
    int64_t old_head = _head_index.load();
    int32_t new_head_ptr = get_ptr(mb->index());

    while (true)
    {
        int32_t old_head_tag = get_tag(old_head);
        int64_t new_head = combine_ptr_tag(new_head_ptr, old_head_tag);
        mb->next().store(get_ptr(old_head));

        if( _head_index.compare_exchange_weak(old_head, new_head) )
            return;
    }
}

#endif // LOCK_FREE_NODE_H
