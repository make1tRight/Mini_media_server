#ifndef SELECT_POLLER_H
#define SELECT_POLLER_H
#include <vector>
#include "poller.h"

class Select_poller : public Poller {
public:
    Select_poller();
    virtual ~Select_poller();

    static Select_poller* append();
    
    // 派生类必须实现基类定义好的纯虚函数
    virtual bool add_IO_event(IOEvent *event);
    virtual bool update_IO_event(IOEvent *event);
    virtual bool remove_IO_event(IOEvent *event);

    virtual void handle_event();

private:
    fd_set m_read_set;
    fd_set m_write_set;
    fd_set m_exception_set;
    int m_max_sockets_num;
    std::vector<IOEvent*> m_IO_events; //存储临时活跃的IO事件对象
};
#endif //SELECT_POLLER_Hexception