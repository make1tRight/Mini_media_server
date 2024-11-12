#include "timer.h"
#include "../rtsp/event_scheduler.h"

// 设置定时器的触发时间(可重复触发)
static bool timerfd_set_time(int fd, Timer::Timestamp when, Timer::TimeInterval period) {
    struct itimerspec new_val;
    // it_value -> 第一次到期时间值
    new_val.it_value.tv_sec = when / 1000; //获取秒
    new_val.it_value.tv_nsec = when % 1000 * 1000 * 1000; //余数转换为纳秒
    // it_interval -> 定时器的周期性时间间隔
    new_val.it_interval.tv_sec = period / 1000;
    new_val.it_interval.tv_nsec = period % 1000 * 1000 * 1000;

    // TFD_TIMER_ABSTIME -> 按绝对时间点触发
    int old_val = timerfd_settime(fd, TFD_TIMER_ABSTIME, &new_val, nullptr);

    if (old_val < 0) {
        return false;
    }

    return true;
}

Timer::Timer(TimerEvent *event, Timestamp timestamp, 
    TimeInterval timer_interval, TimerId timer_id):
    m_timer_event(event),
    m_timestamp(timestamp),
    m_time_interval(timer_interval),
    m_timer_id(timer_id) {
    if (timer_interval > 0) {
        m_repeat = true;
    } else {
        m_repeat = false; //timer_interval=0就是一次性的
    }
}

Timer::~Timer() {}

Timer::Timestamp Timer::get_cur_time() {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now); //返回自系统启动以来的时间
    return (now.tv_sec * 1000 + now.tv_nsec / 1000000);
}

Timer::Timestamp Timer::get_cur_timestamp() {
    // 时间间隔转换为毫秒, 返回转换后时间间隔的数值
    // 1970-01-01 00:00:00
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

bool Timer::handle_event() {
    if (!m_timer_event) {
        return false;
    }
    return m_timer_event->handle_event();
}

Timer_manager::Timer_manager(Event_scheduler *scheduler):
    m_poller(scheduler->poller()),
    m_last_timer_id(0) {
    
    // 创造定时器文件描述符
    // TFD_CLOEXEC保证文件描述符被用于执行新程序的时候自动关闭
    m_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    if (m_timer_fd < 0) {
        LOGERR("create Timer_fd error");
        return;
    } else {
        LOGINFO("fd=%d", m_timer_fd);
    }

    // 初始化IO_event对象
    m_timer_IO_event = IOEvent::append(m_timer_fd, this);
    // 设置回调函数
    m_timer_IO_event->set_read_callback(read_callback);
    // 启用读事件
    m_timer_IO_event->enable_read_handling();
    // 设置超时时间
    modify_timeout();
    // 将事件添加到poller中
    m_poller->add_IO_event(m_timer_IO_event);
}

Timer_manager::~Timer_manager() {
    m_poller->remove_IO_event(m_timer_IO_event);
    delete m_timer_IO_event;
}

Timer_manager *Timer_manager::append(Event_scheduler *scheduler) {
    if (!scheduler) {
        return nullptr;
    }
    return new Timer_manager(scheduler);
}

Timer::TimerId Timer_manager::add_timer(TimerEvent *event, Timer::Timestamp timestamp,
                        Timer::TimeInterval time_interval) {
    ++m_last_timer_id;
    Timer timer(event, timestamp, time_interval, m_last_timer_id);

    m_timers.insert(std::make_pair(m_last_timer_id, timer));
    m_events.insert(std::make_pair(timestamp, timer));

    modify_timeout();
    return m_last_timer_id;
}

bool Timer_manager::remove_timer(Timer::TimerId timer_id) {
    std::map<Timer::TimerId, Timer>::iterator it = m_timers.find(timer_id);
    if (it != m_timers.end()) {
        m_timers.erase(timer_id);
        // 还需要删除m_events的事件, 这里后面再解开注释优化
        // auto range = m_events.equal_range(it->second.m_timestamp);
        // for (auto event_it = range.first; event_it != range.second; ++event_it) {
        //     if (event_it->second.m_timer_id == timer_id) {
        //         m_events.erase(event_it);
        //         // break;
        //     }
        // }
    }

    modify_timeout();
    return true;
}

void Timer_manager::read_callback(void *arg) {
    Timer_manager *timer_manager = (Timer_manager*) arg;
    timer_manager->handle_read();
}

void Timer_manager::handle_read() {
    Timer::Timestamp timestamp = Timer::get_cur_time();
    // 定时器列表为空代表没有要处理的事件
    // 事件列表为空代表没有定时器事件需要处理
    if (!m_timers.empty() && !m_events.empty()) {
        std::multimap<Timer::Timestamp, Timer>::iterator it = m_events.begin();
        Timer timer = it->second;

        // int expire = timer.m_timestamp - timestamp;
        // if (timestamp > timer.m_timestamp || expire == 0) {
        if (timestamp >= timer.m_timestamp) {
            // 第一次处理
            bool timer_event_is_stop = timer.handle_event();
            m_events.erase(it);

            // 如果是重复的, 再次触发
            if (timer.m_repeat) {
                if (timer_event_is_stop) { //如果timerevent停止, 从定时器列表删除timer
                    m_timers.erase(timer.m_timer_id);
                } else { //如果event没有停止, 
                    // 更新时间戳(当前时间+定时器时间间隔)
                    // 更新后的定时器事件重新插入事件列表, 以便下次处理
                    timer.m_timestamp = timestamp + timer.m_time_interval;
                    m_events.insert(std::make_pair(timer.m_timestamp, timer));
                }
            } else {
                m_timers.erase(timer.m_timer_id);
            }
        }
    }
    modify_timeout();
}

void Timer_manager::modify_timeout() {
    std::multimap<Timer::Timestamp, Timer>::iterator it = m_events.begin();
    if (it != m_events.end()) {
        Timer timer = it->second;
        timerfd_set_time(m_timer_fd, timer.m_timestamp, timer.m_time_interval);
    } else { //m_events.begin() == m_events.end()
        timerfd_set_time(m_timer_fd, 0, 0);
    }
}