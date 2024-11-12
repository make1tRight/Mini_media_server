#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H
#include "./live/Inet_address.h"
#include "./media/media_session_manager.h"

class Rtsp_connection;
class Rtsp_server {
public:
    static Rtsp_server *append(Usage_environment *env, 
        Media_session_manager *sess_mgr,
        Inet_address &addr);
    
    Rtsp_server(Usage_environment *env,
        Media_session_manager *sess_mgr,
        Inet_address &addr);

    ~Rtsp_server();

public:
    Media_session_manager *m_sess_mgr;
    void start();
    Usage_environment *env() const {
        return m_env;        
    }

private:
    static void read_callback(void *arg);
    void handle_read();
    static void cb_disconnect(void *arg, int client_fd);
    void handle_disconnect(int client_fd);
    static void cb_close_connect(void *arg);
    void handle_close_connect();
    
private:
    Usage_environment *m_env;
    int m_fd;
    Inet_address m_addr;
    bool m_listen;
    IOEvent *m_accept_IO_event;
    std::mutex m_mutex;

    std::map<int, Rtsp_connection*> m_conn_map; //维护所有连接
    std::vector<int> m_disconn_list;            //所有被取消的连接
    TriggerEvent *m_close_trigger_event;        //关闭连接触发事件
};
#endif // RTSP_SERVER_H