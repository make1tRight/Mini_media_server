#ifndef MEDIA_SESSION_MANAGER_H
#define MEDIA_SESSION_MANAGER_H
#include "media_session.h"

class Media_session_manager {
public:
    static Media_session_manager *append();
    Media_session_manager();
    ~Media_session_manager();

public:
    bool add_session(Media_session *session);
    bool remove_session(Media_session *session);
    Media_session *get_session(const std::string &name);

private:
    std::map<std::string, Media_session*> m_session_map;
};
#endif // MEDIA_SESSION_MANAGER_H