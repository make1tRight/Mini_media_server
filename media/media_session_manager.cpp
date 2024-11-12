#include "media_session_manager.h"


Media_session_manager *Media_session_manager::append() {
    return new Media_session_manager();
}

Media_session_manager::Media_session_manager() {
    LOGINFO("Media_session_manager()");
}

Media_session_manager::~Media_session_manager() {
    LOGINFO("~Media_session_manager()");
}

bool Media_session_manager::add_session(Media_session *session) {
    if (m_session_map.find(session->name()) != m_session_map.end()) {
        return false;
    } else {
        m_session_map.insert(std::make_pair(session->name(), session));
        return true;
    }
}

bool Media_session_manager::remove_session(Media_session *session) {
    std::map<std::string, Media_session*>::iterator it = 
        m_session_map.find(session->name());
    if (it == m_session_map.end()) {
        return false;
    } else {
        m_session_map.erase(it);
        return true;
    }
}

Media_session *Media_session_manager::get_session(const std::string &name) {
    std::map<std::string, Media_session*>::iterator it;
    // for (it = m_session_map.begin(); it != m_session_map.end(); ++it) {
    //     if (it->first == name) {
    //         return it->second;
    //     }
    // }
    it = m_session_map.find(name);
    if (it == m_session_map.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}