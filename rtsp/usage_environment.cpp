#include "usage_environment.h"


Usage_environment *Usage_environment::append(Event_scheduler *scheduler, Threadpool *threadpool) {
    return new Usage_environment(scheduler, threadpool);
}

Usage_environment::Usage_environment(Event_scheduler *scheduler, Threadpool *threadpool):
    m_scheduler(scheduler), m_threadpool(threadpool) {}

Usage_environment::~Usage_environment() {

}

Event_scheduler *Usage_environment::scheduler() {
    return m_scheduler;
}

Threadpool *Usage_environment::threadpool() {
    return m_threadpool;
}