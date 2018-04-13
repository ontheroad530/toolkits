TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -pthread -lboost_system

SOURCES += main.cpp \
    mytimer.cpp \
    lockfree/tagged.cpp


HEADERS += \
    myqueue.h \
    mytimer.h \
    lockfree/lock_free_node.h \
    lockfree/lock_free_pool.h \
    lockfree/lock_free_queue.h \
    lockfree/tagged.h \
    threadlocal.h

