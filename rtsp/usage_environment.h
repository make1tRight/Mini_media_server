#ifndef USAGE_ENVIRONMENT_H
#define USAGE_ENVIRONMENT_H
#include "../threadpool/threadpool.h"
#include "event_scheduler.h"

class Usage_environment { //提供统一接口访问事件调度和线程池
public:
    static Usage_environment *append(Event_scheduler *scheduler, Threadpool *threadpool);

    Usage_environment(Event_scheduler *scheduler, Threadpool *threadpool);
    ~Usage_environment();

    Event_scheduler *scheduler();
    Threadpool *threadpool();


private:
    Event_scheduler *m_scheduler;
    Threadpool *m_threadpool;
};
#endif //USAGE_ENVIRONMENT_H