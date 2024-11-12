#include "event_scheduler.h"


Event_scheduler *Event_scheduler::append(PollerType type) {
    if (type != POLLER_SELECT && type != POLLER_POLL && type != POLLER_EPOLL) {
        return nullptr;
    }

    return new Event_scheduler(type);
}

Event_scheduler::Event_scheduler(PollerType type): m_quit(false) {
    switch (type) {
        case POLLER_SELECT: {
            m_poller = Select_poller::append();
            break;
        }

        // case POLLER_EPOLL: {
        //     m_poller = EPoll_poller::append();
        //     break;
        // }

        default: {
            exit(-1);
            break;
        }
    }

    // select网络模型处理定时器
    m_timer_manager = Timer_manager::append(this);
}

Event_scheduler::~Event_scheduler() {
    delete m_timer_manager;
    delete m_poller;
}

bool Event_scheduler::add_trigger_event(TriggerEvent* event) {
    m_trigger_events.push_back(event);
    return true;
}

bool Event_scheduler::add_IO_event(IOEvent *event) {
    return m_poller->add_IO_event(event);
}

bool Event_scheduler::update_IO_event(IOEvent *event) {
    return m_poller->update_IO_event(event);
}

bool Event_scheduler::remove_IO_event(IOEvent *event) {
    return m_poller->remove_IO_event(event);
}

void Event_scheduler::loop() {
    while (!m_quit) {
        handle_trigger_events();
        m_poller->handle_event();
    }
}

Poller *Event_scheduler::poller() {
    return m_poller;
}

// WIN
// void Event_scheduler::set_timer_manager_callback(EventCallback cb, void *arg) {}

void Event_scheduler::handle_trigger_events() {
    if (!m_trigger_events.empty()) {
        for (std::vector<TriggerEvent*>::iterator it = m_trigger_events.begin();
            it != m_trigger_events.end(); ++it) {
            (*it)->handle_event();
        }

        m_trigger_events.clear();
    }
}

// 循环执行
Timer::TimerId Event_scheduler::add_timer_event_run_every(TimerEvent *event, 
    Timer::TimeInterval interval) {
    Timer::Timestamp timestamp = Timer::get_cur_time();
    timestamp += interval;

    return m_timer_manager->add_timer(event, timestamp, interval);
}

// 延迟执行
Timer::TimerId Event_scheduler::add_timer_event_run_after(TimerEvent *event, 
    Timer::TimeInterval delay) {
    Timer::Timestamp timestamp = Timer::get_cur_time();
    timestamp += delay;

    return m_timer_manager->add_timer(event, timestamp, 0);
}

// 指定时间
Timer::TimerId Event_scheduler::add_timer_event_run_at(TimerEvent *event, 
    Timer::TimeInterval when) {
    return m_timer_manager->add_timer(event, when, 0);
}