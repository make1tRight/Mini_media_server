#ifndef EVENT_SCHEDULER_H
#define EVENT_SCHEDULER_H
#include <vector>
#include <mutex>
#include "poller.h"
#include "select_poller.h"
#include "../timer/timer.h"

class Timer;
class Timer_manager;
class Poller;

// 事件调度管理类
class Event_scheduler {
public:
    enum PollerType {
        POLLER_SELECT,
        POLLER_POLL,
        POLLER_EPOLL
    };

    static Event_scheduler *append(PollerType type);

    explicit Event_scheduler(PollerType type);
    virtual ~Event_scheduler();
public:
    bool add_trigger_event(TriggerEvent* event);
    /**Timer */
    Timer::TimerId add_timer_event_run_after(TimerEvent *event, 
        Timer::TimeInterval interval);
    Timer::TimerId add_timer_event_run_at(TimerEvent *event, 
        Timer::TimeInterval interval);
    Timer::TimerId add_timer_event_run_every(TimerEvent *event, 
        Timer::TimeInterval interval);
    bool add_IO_event(IOEvent *event);
    bool update_IO_event(IOEvent *event);
    bool remove_IO_event(IOEvent *event);

    void loop();
    Poller *poller();
    void set_timer_manager_callback(EventCallback cb, void *arg);


private:
    void handle_trigger_events();
private:
    bool m_quit;
    Poller *m_poller;
    Timer_manager* m_timer_manager;
    std::vector<TriggerEvent*> m_trigger_events;
    std::mutex m_mutex;
};
#endif