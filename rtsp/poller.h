#ifndef POLLER_H
#define POLLER_H
#include <map>
#include "event.h"

class Poller {
public:
    virtual ~Poller() {}

    virtual bool add_IO_event(IOEvent *event) = 0;
    virtual bool update_IO_event(IOEvent *event) = 0;
    virtual bool remove_IO_event(IOEvent *event) = 0;
    virtual void handle_event() = 0;

protected:
    Poller() {}
protected:
    typedef std::map<int, IOEvent*> IOEventMap;
    IOEventMap m_event_map;
};
#endif