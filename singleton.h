#ifndef SINGLETON
#define SINGLETON

#include <thread>
#include <boost/noncopyable.hpp>

namespace gyh {

//from muduo

// This doesn't detect inherited member functions!
// http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions
template<typename T>
struct has_no_destroy
{
#ifdef __GXX_EXPERIMENTAL_CXX0X__
  template <typename C> static char test(decltype(&C::no_destroy));
#else
  template <typename C> static char test(typeof(&C::no_destroy));
#endif
  template <typename C> static int32_t test(...);
  const static bool value = sizeof(test<T>(0)) == 1;
};

template<typename T>
class Singleton: boost::noncopyable
{
public:
    static T& instance()
    {
        pthread_once(&m_ponce, &Singleton::init);
        assert(m_value);
        return *m_value;
    }

private:
    Singleton();
    ~Singleton();

    static void init()
    {
        m_value = new T();
        if (!has_no_destroy<T>::value)
        {
          ::atexit(destroy);
        }
    }

    static void destroy()
    {
        typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
        T_must_be_complete_type dummy; (void) dummy;

        delete m_value;
        m_value = nullptr;
    }

private:
    static  pthread_once_t  m_ponce;
    static  T*  m_value;
};

template<typename T>
pthread_once_t Singleton<T>::m_ponce = PTHREAD_ONCE_INIT;

template<typename T>
T*  Singleton<T>::m_value = nullptr;
}

#endif // SINGLETON

