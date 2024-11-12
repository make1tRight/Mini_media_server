#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
#include <map>
#include <sys/timerfd.h>
#include <time.h>
#include <chrono>
#include "../rtsp/event.h"
#include "../rtsp/poller.h"
// #include "../rtsp/event_scheduler.h"

class Event_scheduler;

class Timer {
public:
    typedef int64_t Timestamp;
    typedef uint32_t TimeInterval;
    typedef uint32_t TimerId;

    ~Timer();

    static Timestamp get_cur_time();
    static Timestamp get_cur_timestamp();

private:
    friend class Timer_manager;
    Timer(TimerEvent *event, Timestamp timestamp, 
        TimeInterval timer_interval, TimerId timer_id);


private:
    bool handle_event();
private:
    TimerEvent* m_timer_event;
    Timestamp m_timestamp;
    TimeInterval m_time_interval;
    TimerId m_timer_id;

    bool m_repeat;
};

class Timer_manager {
public:
    Timer_manager(Event_scheduler *scheduler);
    ~Timer_manager();

    static Timer_manager *append(Event_scheduler *scheduler);
    Timer::TimerId add_timer(TimerEvent *event, Timer::Timestamp timestamp,
                            Timer::TimeInterval time_interval);
    bool remove_timer(Timer::TimerId timer_id);

private:
    static void read_callback(void *arg);
    void handle_read();
    void modify_timeout();
private:
    Poller *m_poller;
    std::map<Timer::TimerId, Timer> m_timers;
    std::multimap<Timer::Timestamp, Timer> m_events;

    uint32_t m_last_timer_id;

    int m_timer_fd;
    IOEvent *m_timer_IO_event;
};
#endif //TIMER_H