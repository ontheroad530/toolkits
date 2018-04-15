#ifndef THREADLOCAL_H
#define THREADLOCAL_H

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <cassert>

namespace gyh {

//from muduo

#define MCHECK(ret) ({ decltype (ret) errnum = (ret);         \
                       assert(errnum == 0); (void) errnum;})

template<typename T>
class ThreadLocal : boost::noncopyable
{
public:
    ThreadLocal()
    {
        MCHECK( pthread_key_create(&mPKey, &ThreadLocal::desructor) );
    }
    ~ThreadLocal()
    {
        MCHECK( pthread_key_delete(mPKey) );
    }

    T& value()
    {
        T* perThreadValue = static_cast<T*>(pthread_getspecific(mPKey));
        if( !perThreadValue )
        {
            T* obj = new T();
            MCHECK(pthread_setspecific(mPKey, obj));
            perThreadValue = obj;
        }

        return *perThreadValue;
    }
private:
    static void desructor(void* x)
    {
        T* obj = static_cast<T*>(x);
        typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
        (void) sizeof(T_must_be_complete_type);
        delete obj;
    }

private:
    pthread_key_t   mPKey;
};

}


#endif // THREADLOCAL_H
