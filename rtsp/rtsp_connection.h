#ifndef RTSP_CONNECTION_H
#define RTSP_CONNECTION_H
#include "../live/tcp_connection.h"
#include "../media/media_session.h"
#include "./sockets_options.h"

class Rtsp_server;
class Rtsp_connection : public Tcp_connection {
public:
    enum Method {
        OPTIONS, DESCRIBE, SETUP, 
        PLAY, TEARDOWN, NONE
    };

    static Rtsp_connection *append(Rtsp_server *arg, int client_fd);
    Rtsp_connection(Rtsp_server *rtsp_server, int client_fd);
    virtual ~Rtsp_connection();

protected:
    virtual void handle_read_bytes();

private:
    bool parse_request();
    bool parse_request1(const char *begin, const char *end);
    bool parse_request2(const char *begin, const char *end);

    bool parse_CSeq(std::string &msg);
    bool parse_describe(std::string &msg);
    bool parse_setup(std::string &msg);
    bool parse_play(std::string &msg);

    bool handle_cmd_option();
    bool handle_cmd_describe();
    bool handle_cmd_setup();
    bool handle_cmd_play();
    bool handle_cmd_teardown();

    int send_msg(void *buf, int size);
    int send_msg();

    bool append_rtp_rtcp_over_udp(Media_session::Track_id track_id,
        std::string peer_ip, uint16_t peer_rtp_port, uint16_t peer_rtcp_port);
    bool append_rtp_over_tcp(Media_session::Track_id track_id,
        int sockfd, uint8_t rtp_channel);

    void handle_rtp_over_tcp();
    
private:
    Rtsp_server *m_rtsp_server;
    std::string m_peer_ip;
    Method m_method;
    std::string m_url;
    std::string m_suffix;                   //域名后缀, 这里是请求资源
    uint32_t m_CSeq;
    std::string m_stream_prefix;            //数据流名称
    uint16_t m_peer_rtp_port;
    uint16_t m_peer_rtcp_port;

    Media_session::Track_id m_track_id;     //拉流setup请求时的track_id
    Rtp_instance *m_rtp_instances[MEDIA_MAX_TRACK_NUM];
    Rtcp_instance *m_rtcp_instances[MEDIA_MAX_TRACK_NUM];

    int m_session_id;
    bool m_is_rtp_over_tcp;
    uint8_t m_rtp_channel;
};
#endif // RTSP_CONNECTION_H