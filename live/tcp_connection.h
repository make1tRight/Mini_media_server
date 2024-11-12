#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H
#include "../rtsp/usage_environment.h"
#include "../rtsp/event.h"
#include "buffer.h"
#include "../log/log.h"
const int BUFFER_SIZE = 2048;

class Tcp_connection {
public:
    typedef void (*DisconnectCallback)(void *, int);
    Tcp_connection(Usage_environment *env, int client_fd);
    virtual ~Tcp_connection();

    void set_disconnect_callback(DisconnectCallback cb, void *arg);

protected:
    void enable_read_handling();
    void enable_write_handling();
    void enable_error_handling();

    void disable_read_handling();
    void disable_write_handling();
    void disable_error_handling();

    void handle_read();
    virtual void handle_read_bytes();
    virtual void handle_write();
    virtual void handle_error();

    void handle_disconnect();
private:
    static void read_callback(void *arg);
    static void write_callback(void *arg);
    static void error_callback(void *arg);

protected:
    Usage_environment *m_env;
    int m_client_fd;
    IOEvent *m_client_IO_event;
    DisconnectCallback m_disconnect_callabck;
    void *m_arg;
    Buffer m_input_buffer;
    Buffer m_output_buffer;
    char m_buffer[BUFFER_SIZE];
};
#endif //TCP_CONNECTION_H