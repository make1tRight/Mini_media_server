#include "tcp_connection.h"
#include "../rtsp/sockets_options.h"

Tcp_connection::Tcp_connection(Usage_environment *env, int client_fd):
    m_env(env), m_client_fd(client_fd) {
    m_client_IO_event = IOEvent::append(client_fd, this);
    m_client_IO_event->set_read_callback(read_callback);
    m_client_IO_event->set_write_callback(write_callback);
    m_client_IO_event->set_error_callback(error_callback);
    m_client_IO_event->enable_read_handling();

    m_env->scheduler()->add_IO_event(m_client_IO_event);
}

void Tcp_connection::set_disconnect_callback(DisconnectCallback cb, void *arg) {
    m_disconnect_callabck = cb;
    m_arg = arg;
}

void Tcp_connection::enable_read_handling() {
    if (m_client_IO_event->is_read_handling()) {
        return;
    }

    m_client_IO_event->enable_read_handling();
    m_env->scheduler()->update_IO_event(m_client_IO_event);
}

void Tcp_connection::enable_write_handling() {
    if (m_client_IO_event->is_write_handling()) {
        return;
    }

    m_client_IO_event->enable_write_handling();
    m_env->scheduler()->update_IO_event(m_client_IO_event);
}

void Tcp_connection::enable_error_handling() {
    if (m_client_IO_event->is_error_handling()) {
        return;
    }

    m_client_IO_event->enable_error_handling();
    m_env->scheduler()->update_IO_event(m_client_IO_event);
}

void Tcp_connection::disable_read_handling() {
    if (!m_client_IO_event->is_read_handling()) {
        return;
    }

    m_client_IO_event->disable_read_handling();
    m_env->scheduler()->update_IO_event(m_client_IO_event);
}

void Tcp_connection::disable_write_handling() {
    if (!m_client_IO_event->is_write_handling()) {
        return;
    }

    m_client_IO_event->disable_write_handling();
    m_env->scheduler()->update_IO_event(m_client_IO_event);
}
void Tcp_connection::disable_error_handling() {
    if (!m_client_IO_event->is_error_handling()) {
        return;
    }

    m_client_IO_event->disable_error_handling();
    m_env->scheduler()->update_IO_event(m_client_IO_event);
}

void Tcp_connection::handle_read() { //读取数据
    // LOGINFO("");
    int ret = m_input_buffer.read(m_client_fd);

    if (ret <= 0) {
        LOGERR("read error,fd=%d,ret=%d", m_client_fd, ret);
        handle_disconnect();
        return;
    }

    handle_read_bytes();
}

void Tcp_connection::handle_read_bytes() {
    LOGINFO("handle_read_bytes");
    m_input_buffer.retrieve_all();
}

void Tcp_connection::handle_write() {
    LOGINFO("handle_write");
    m_output_buffer.retrieve_all();
}

void Tcp_connection::handle_error() {
    // LOGINFO("Errors occured during TCP connection handling.");
    LOGINFO("handle_error");
}

void Tcp_connection::handle_disconnect() {
    if (m_disconnect_callabck) {
        m_disconnect_callabck(m_arg, m_client_fd);
    }
}

void Tcp_connection::read_callback(void *arg) {
    Tcp_connection *tcp_conn = (Tcp_connection*) arg;
    tcp_conn->handle_read();
}

void Tcp_connection::write_callback(void *arg) {
    Tcp_connection *tcp_conn = (Tcp_connection*) arg;
    tcp_conn->handle_write();
}

void Tcp_connection::error_callback(void *arg) {
    Tcp_connection *tcp_conn = (Tcp_connection*) arg;
    tcp_conn->handle_error();
}

Tcp_connection::~Tcp_connection() {
    m_env->scheduler()->remove_IO_event(m_client_IO_event);
    delete m_client_IO_event;
    mysocket::close(m_client_fd);
}