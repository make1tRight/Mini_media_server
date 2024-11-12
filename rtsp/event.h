#ifndef EVENT_H
#define EVENT_H
#include "../log/log.h"
typedef void (*EventCallback)(void*);

class TriggerEvent {
public:
    static TriggerEvent* append(void *arg);
    static TriggerEvent* append();

    TriggerEvent(void *arg);
    ~TriggerEvent();

    void set_arg(void *arg) { m_arg = arg; }
    void set_trigger_callback(EventCallback cb) { m_trigger_callback = cb; }
    void handle_event();
private:
    void *m_arg;
    EventCallback m_trigger_callback;
};

class TimerEvent {
public:
    static TimerEvent *append(void *arg);
    static TimerEvent *append();

    TimerEvent(void *arg);
    ~TimerEvent();

    void set_arg(void *arg) { m_arg = arg; }
    void set_timeout_callback(EventCallback cb) { m_timeout_callback = cb; }
    bool handle_event();

    void stop();
private:
    void *m_arg;
    EventCallback m_timeout_callback;
    bool m_isStop;
};

class IOEvent {
public:
    enum IOEventType {
        EVENT_NONE = 0,
        EVENT_READ = 1,
        EVENT_WRITE = 2,
        EVENT_ERROR = 4
    };

    static IOEvent *append(int fd, void *arg);
    static IOEvent *append(int fd);

    IOEvent(int fd, void *arg);
    ~IOEvent();

    int get_fd() const { return m_fd; }
    int get_event() const { return m_event; }
    void set_Revent(int event) { m_Revent = event; }
    void set_arg(void *arg) { m_arg = arg; }

    void set_read_callback(EventCallback cb) { m_read_callback = cb; }
    void set_write_callback(EventCallback cb) { m_write_callback = cb; }
    void set_error_callback(EventCallback cb) { m_error_callback = cb; }

    void enable_read_handling() { m_event |= EVENT_READ; }
    void enable_write_handling() { m_event |= EVENT_WRITE; }
    void enable_error_handling() { m_event |= EVENT_ERROR; }
    
    void disable_read_handling() { m_event &= ~EVENT_READ; }
    void disable_write_handling() { m_event &= ~EVENT_WRITE; }
    void disable_error_handling() { m_event &= ~EVENT_ERROR; }

    bool is_none_handling() const { return m_event == EVENT_NONE; }
    bool is_read_handling() const { return (m_event & EVENT_READ) != 0; }
    bool is_write_handling() const { return (m_event & EVENT_WRITE) != 0; }
    bool is_error_handling() const { return (m_event & EVENT_ERROR) != 0; }

    void handle_event();

private:
    int m_fd;
    void *m_arg;
    int m_event;
    int m_Revent;

    EventCallback m_read_callback;
    EventCallback m_write_callback;
    EventCallback m_error_callback;
};
#endif