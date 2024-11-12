#include "select_poller.h"
#include "../log/log.h"
// fd_set m_read_set;
// fd_set m_write_set;
// fd_set m_exception_set;
// int m_max_sockets_num;
// std::vector<IOEvent*> m_IO_events; //存储临时活跃的IO事件对象

Select_poller::Select_poller() { //初始化fd_set, 确保在select系统调用之前没有先前的fd
    FD_ZERO(&m_read_set);
    FD_ZERO(&m_write_set);
    FD_ZERO(&m_exception_set);
}

Select_poller::~Select_poller() {

}

Select_poller* Select_poller::append() {
    return new Select_poller();
}

bool Select_poller::add_IO_event(IOEvent *event) {
    return update_IO_event(event);
}

bool Select_poller::update_IO_event(IOEvent *event) {
    int fd = event->get_fd();

    if (fd < 0) {
        LOGERR("fd=%d", fd);
        return false;
    }

    FD_CLR(fd, &m_read_set);
    FD_CLR(fd, &m_write_set);
    FD_CLR(fd, &m_exception_set);

    IOEventMap::iterator it = m_event_map.find(fd);
    if (it != m_event_map.end()) { //select表里面有则进行对应的任务
        if (event->is_read_handling()) {
            FD_SET(fd, &m_read_set);
        }
        if (event->is_write_handling()) {
            FD_SET(fd, &m_write_set);
        }
        if (event->is_error_handling()) {
            FD_SET(fd, &m_exception_set);
        }

        m_event_map.insert(std::make_pair(fd, event)); 
    } else {                        //select表为空, 添加IO事件
        if (event->is_read_handling()) {
            FD_SET(fd, &m_read_set);
        }
        if (event->is_write_handling()) {
            FD_SET(fd, &m_write_set);
        }
        if (event->is_error_handling()) {
            FD_SET(fd, &m_exception_set);
        }

        m_event_map.insert(std::make_pair(fd, event));
    }

    if (m_event_map.empty()) { //更新最大socket数
        m_max_sockets_num = 0;
    } else {
        m_max_sockets_num = m_event_map.rbegin()->first + 1;//反向迭代器, 最后一个fd+1
    }

    return true;
}

bool Select_poller::remove_IO_event(IOEvent *event) {
    int fd = event->get_fd();

    if (fd < 0) {
        return false;
    }

    FD_CLR(fd, &m_read_set);
    FD_CLR(fd, &m_write_set);
    FD_CLR(fd, &m_exception_set);

    IOEventMap::iterator it = m_event_map.find(fd);
    if (it != m_event_map.end()) {
        m_event_map.erase(it);
    }

    if (m_event_map.empty()) {  //更新最大socket数
        m_max_sockets_num = 0;
    } else {
        m_max_sockets_num = m_event_map.rbegin()->first + 1;
    }

    return true;
}

void Select_poller::handle_event() {
    fd_set read_set = m_read_set;
    fd_set write_set = m_write_set;
    fd_set exception_set = m_exception_set;

    // 指定select最大阻塞时间, 
    struct timeval timeout;
    int ret, r_event;

    timeout.tv_sec = 1000;//秒
    timeout.tv_usec = 0;
    // LOGINFO("m_event_map.size()=%d, select io start", m_event_map.size());

    ret = select(m_max_sockets_num, &read_set, &write_set, &exception_set, &timeout);

    if (ret < 0) {
        return;
    } else {
        // LOGINFO("check active fd ret=%d", ret);
    }

    // LOGINFO("select io end");

    for (IOEventMap::iterator it = m_event_map.begin(); it != m_event_map.end(); ++it) {
        r_event = 0;
        if (FD_ISSET(it->first, &read_set)) {
            r_event |= IOEvent::EVENT_READ;
        }
        if (FD_ISSET(it->first, &write_set)) {
            r_event |= IOEvent::EVENT_WRITE;
        }
        if (FD_ISSET(it->first, &exception_set)) {
            r_event |= IOEvent::EVENT_ERROR;
        }

        if (r_event != 0) {
            it->second->set_Revent(r_event);
            m_IO_events.push_back(it->second);
        }
    }

    for (auto &io_event : m_IO_events) {
        io_event->handle_event();
    }

    m_IO_events.clear();
}