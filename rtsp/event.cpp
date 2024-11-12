#include "event.h"


/*************TriggerEvent *******************/
TriggerEvent* TriggerEvent::append(void *arg) {
    return new TriggerEvent(arg);
}

TriggerEvent* TriggerEvent::append() {
    return new TriggerEvent(nullptr);
}

// 还没实现log
TriggerEvent::TriggerEvent(void *arg): 
    m_arg(arg), m_trigger_callback(nullptr) {
    LOGINFO("TriggerEvent()");
}

TriggerEvent::~TriggerEvent() {
    LOGINFO("~TriggerEvent()");
}

void TriggerEvent::handle_event() {
    if (m_trigger_callback) {
        m_trigger_callback(m_arg);
    }
}

TimerEvent *TimerEvent::append(void *arg) {
    return new TimerEvent(arg);
}

TimerEvent *TimerEvent::append() {
    return new TimerEvent(nullptr);
}

/*************TimerEvent *******************/
// 还没实现log
TimerEvent::TimerEvent(void *arg): 
        m_arg (arg), m_timeout_callback(nullptr) {
    LOGINFO("TimerEvent()");
}

TimerEvent::~TimerEvent() {
    LOGINFO("~TimerEvent()");
}

bool TimerEvent::handle_event() {
    if (m_isStop) {
        return m_isStop;
    }

    if (m_timeout_callback) {
        m_timeout_callback(m_arg);
    }

    return m_isStop;
}

void TimerEvent::stop() {
    m_isStop = true;
}

/*************IOEvent *******************/
IOEvent *IOEvent::append(int fd, void *arg) {
    if (fd < 0) {
        return nullptr;
    }

    return new IOEvent(fd, arg);
}

IOEvent *IOEvent::append(int fd) {
    if (fd < 0) {
        return nullptr;
    }

    return new IOEvent(fd, nullptr);
}

IOEvent::IOEvent(int fd, void *arg):
        m_fd(fd), 
        m_arg(arg), 
        m_event(EVENT_NONE), 
        m_Revent(EVENT_NONE),
        m_read_callback(nullptr), 
        m_write_callback(nullptr), 
        m_error_callback(nullptr) {
    LOGINFO("IOEvent() fd=%d", m_fd);
}

IOEvent::~IOEvent() {
    LOGINFO("~IOEvent() fd=%d", m_fd);
}


void IOEvent::handle_event() {
    if (m_read_callback && (m_Revent & EVENT_READ)) {
        m_read_callback(m_arg);
    }
    if (m_write_callback && (m_Revent & EVENT_WRITE)) {
        m_write_callback(m_arg);
    }
    if (m_error_callback && (m_Revent & EVENT_ERROR)) {
        m_error_callback(m_arg);
    }
}