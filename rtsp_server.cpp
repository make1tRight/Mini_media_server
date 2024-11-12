#include "rtsp_server.h"
#include "./rtsp/rtsp_connection.h"
#include "./log/log.h"

Rtsp_server *Rtsp_server::append(Usage_environment *env, 
    Media_session_manager *sess_mgr,
    Inet_address &addr) {
    return new Rtsp_server(env, sess_mgr, addr);
}

Rtsp_server::Rtsp_server(Usage_environment *env,
    Media_session_manager *sess_mgr,
    Inet_address &addr):
        m_env(env),
        m_sess_mgr(sess_mgr),
        m_addr(addr),
        m_listen(false) {
    m_fd = mysocket::create_tcp_socket();
    mysocket::set_reuse_addr(m_fd, 1);

    if (!mysocket::bind(m_fd, addr.get_ip(), addr.get_port())) {
        return;
    }

    LOGINFO("rtsp://%s:%d fd=%d", addr.get_ip().data(), addr.get_port(), m_fd);
    

    m_accept_IO_event = IOEvent::append(m_fd, this);
    // 设置socket可读回调函数
    m_accept_IO_event->set_read_callback(read_callback);
    m_accept_IO_event->enable_read_handling();

    m_close_trigger_event = TriggerEvent::append(this);
    // 设置关闭连接回调
    m_close_trigger_event->set_trigger_callback(cb_close_connect);
}

Rtsp_server::~Rtsp_server() {
    if (m_listen) {
        m_env->scheduler()->remove_IO_event(m_accept_IO_event);
    }

    delete m_accept_IO_event;
    delete m_close_trigger_event;

    mysocket::close(m_fd);
}

void Rtsp_server::start() {
    LOGINFO("start()");
    m_listen = true;
    mysocket::listen(m_fd, 60);
    m_env->scheduler()->add_IO_event(m_accept_IO_event);
}

void Rtsp_server::read_callback(void *arg) {
    Rtsp_server *rtsp_server = (Rtsp_server*) arg;
    rtsp_server->handle_read();
}

void Rtsp_server::handle_read() {
    int client_fd = mysocket::accept(m_fd);
    if (client_fd < 0) {
        LOGERR("handle_read error, client_fd=%d", m_fd);
        return;
    }

    Rtsp_connection *conn = Rtsp_connection::append(this, client_fd);
    // 更有效地管理连接的生命周期
    conn->set_disconnect_callback(Rtsp_server::cb_disconnect, this);
    m_conn_map.insert(std::make_pair(client_fd, conn));
}

void Rtsp_server::cb_disconnect(void *arg, int client_fd) {
    Rtsp_server *rtsp_server = (Rtsp_server*) arg;
    rtsp_server->handle_disconnect(client_fd);
}

void Rtsp_server::handle_disconnect(int client_fd) {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_disconn_list.push_back(client_fd);
    m_env->scheduler()->add_trigger_event(m_close_trigger_event);
}

void Rtsp_server::cb_close_connect(void *arg) {
    Rtsp_server *rtsp_server = (Rtsp_server*) arg;
    rtsp_server->handle_close_connect();
}

void Rtsp_server::handle_close_connect() {
    std::lock_guard<std::mutex> lck(m_mutex);
    std::vector<int>::iterator it;

    for (it = m_disconn_list.begin(); it != m_disconn_list.end(); ++it) {
        int cliend_fd = *it;
        std::map<int, Rtsp_connection*>::iterator _it = m_conn_map.find(cliend_fd);
        assert(_it != m_conn_map.end());

        delete _it->second;
        m_conn_map.erase(cliend_fd);
    }

    m_disconn_list.clear();
}