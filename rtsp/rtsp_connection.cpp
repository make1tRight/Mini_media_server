#include "rtsp_connection.h"
#include "../rtsp_server.h"
#include "../log/version.h"

static void get_peer_ip(int fd, std::string &ip) {
    struct sockaddr_in addr;
    // memset(&addr, 0, sizeof(addr));

    socklen_t addrlen = sizeof(struct sockaddr_in);
    getpeername(fd, (struct sockaddr*) &addr, &addrlen);
    ip = inet_ntoa(addr.sin_addr);
}

Rtsp_connection *Rtsp_connection::append(Rtsp_server *rtsp_server,
    int client_fd) {
    return new Rtsp_connection(rtsp_server, client_fd);
}

Rtsp_connection::Rtsp_connection(Rtsp_server *rtsp_server, int client_fd):
    Tcp_connection(rtsp_server->env(), client_fd),
    m_rtsp_server(rtsp_server),
    m_method(Rtsp_connection::Method::NONE),
    m_track_id(Media_session::Track_id::Track_id_none),
    m_session_id(rand()),
    m_is_rtp_over_tcp(false),
    m_stream_prefix("track") {
    

    LOGINFO("27::m_is_rtp_over_tcp=%d", m_is_rtp_over_tcp);
    LOGINFO("Rtsp_connection() m_client_fd=%d", m_client_fd);
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        m_rtp_instances[i] = nullptr;
        m_rtcp_instances[i] = nullptr;
    }

    get_peer_ip(client_fd, m_peer_ip);
}


Rtsp_connection::~Rtsp_connection() {
    LOGINFO("~Rtsp_connection() m_client_fd=%d", m_client_fd);

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (m_rtp_instances[i]) {

            Media_session *session = m_rtsp_server->m_sess_mgr->get_session(m_suffix);
            if (!session) {
                session->remove_rtp_instance(m_rtp_instances[i]);
            }
            delete m_rtp_instances[i];
        }

        if (m_rtcp_instances[i]) {
            delete m_rtcp_instances[i];
        }
    }
}

void Rtsp_connection::handle_read_bytes() {
    // LOGINFO("58::m_is_rtp_over_tcp=%d", m_is_rtp_over_tcp);
    if (m_is_rtp_over_tcp) {
        if (m_input_buffer.peek()[0] == '$') {
            handle_rtp_over_tcp();
            return;
        }
    }

    // bool disconnect;
    // if (!parse_request()) {
    //     LOGERR("parse_request error");
    //     disconnect = true;
    // } else {
    //     switch (m_method) {
    //         case OPTIONS: {
    //             if (!handle_cmd_option()) {
    //                 LOGERR("fail to handle OPTIONS command");
    //                 disconnect = true;
    //             }
    //             // disconnect = !handle_cmd_option();
    //             break;
    //         }
    //         case DESCRIBE: {
    //             if (!handle_cmd_describe()) {
    //                 LOGERR("fail to handle DESCRIBE command");
    //                 disconnect = true;
    //             }
    //             // disconnect = !handle_cmd_describe();
    //             break;
    //         }
    //         case SETUP: {
    //             if (!handle_cmd_setup()) {
    //                 LOGERR("fail to handle SETUP command");
    //                 disconnect = true;
    //             }                
    //             // disconnect = !handle_cmd_setup();
    //             break;
    //         }
    //         case PLAY: {
    //             if (!handle_cmd_play()) {
    //                 LOGERR("fail to handle PLAY command");
    //                 disconnect = true;
    //             }
    //             // disconnect = !handle_cmd_play();
    //             break;
    //         }
    //         case TEARDOWN: {
    //             if (!handle_cmd_teardown()) {
    //                 LOGERR("fail to handle TEARDOWN command");
    //                 disconnect = true;
    //             }                
    //             // disconnect = !handle_cmd_teardown();
    //             break;
    //         }
    //         default: {
    //             disconnect = true;
    //             break;
    //         }
    //     }
    // }
    // if (disconnect) {
    //     handle_disconnect();
    // }
    if (!parse_request())
    {   
        LOGERR("parse_request err");
        goto disConnect;
    }   
    switch (m_method)
    {   
        case OPTIONS:
            if (!handle_cmd_option())
                goto disConnect;
            break;
        case DESCRIBE:
            if (!handle_cmd_describe())
                goto disConnect;
            break;
        case SETUP:
            if (!handle_cmd_setup())
                goto disConnect;
            break;
        case PLAY:
            if (!handle_cmd_play())
                goto disConnect;
            break;
        case TEARDOWN:
            if (!handle_cmd_teardown())
                goto disConnect;
            break;

        default:
            goto disConnect;
    }   

    return;
disConnect:
    handle_disconnect();
}

bool Rtsp_connection::parse_request() {
    /** rtsp报文示例
     * DESCRIBE rtsp://example.com/media.mp4 RTSP/1.0\r\n
     * CSeq: 2\r\n
     * User-Agent: LibVLC/3.0.11.1 (LIVE555 Streaming Media v2016.11.28)\r\n
     * Accept: application/sdp\r\n
     * \r\n
     */

    // 解析第一行
    const char *crlf = m_input_buffer.find_CRLF();
    if (crlf == nullptr) {
        m_input_buffer.retrieve_all();
        return false;
    }

    bool ret = parse_request1(m_input_buffer.peek(), crlf);
    if (ret == false) {
        m_input_buffer.retrieve_all();
        return false;
    } else {
        m_input_buffer.retrieve_until(crlf + 2);
    }

    // 解析后面所有行
    crlf = m_input_buffer.find_last_CRLF();
    if (crlf == nullptr) {
        m_input_buffer.retrieve_all();
        return false;
    }

    ret = parse_request2(m_input_buffer.peek(), crlf);
    if (ret == false) {
        m_input_buffer.retrieve_all();
        return false;
    } else {
        m_input_buffer.retrieve_until(crlf + 2);
        return true;
    }
}

bool Rtsp_connection::parse_request1(const char *begin, const char *end) {
    std::string msg(begin, end);
    char method[64] = {0};
    char url[512] = {0};
    char version[64] = {0};

    if (sscanf(msg.c_str(), "%s %s %s", method, url, version) != 3) {
        return false;
    }

    if (!strcmp(method, "OPTIONS")) {
        m_method = OPTIONS;
    } else if (!strcmp(method, "DESCRIBE")) {
        m_method = DESCRIBE;
    } else if (!strcmp(method, "SETUP")) {
        m_method = SETUP;
    } else if (!strcmp(method, "PLAY")) {
        m_method = PLAY;
    } else if (!strcmp(method, "TEARDOWN")) {
        m_method = TEARDOWN;
    } else {
        m_method = NONE;
        return false;
    }

    if (strncmp(url, "rtsp://", 7) != 0) {
        return false;
    }

    uint16_t port = 0;
    char ip[64] = {0};
    char suffix[64] = {0};

    // 匹配直到冒号的所有字符, 存储到ip中
    // hu匹配无符号短整数
    if (sscanf(url + 7, "%[^:]:%hu/%s", ip, &port, suffix) == 3) {

    } else if (sscanf(url + 7, "%[^/]/%s", ip, suffix) == 2) {
        port = 554; //rtsp视频流协议的默认端口是554
    } else {
        return false;
    }

    m_url = url;
    m_suffix = suffix;

    return true;
}

bool Rtsp_connection::parse_request2(const char *begin, const char *end) {
    std::string msg(begin, end);

    if (!parse_CSeq(msg)) { //parse_CSeq提取并记录序列号以便生成正确的响应头部
        return false;
    }

    // 状态机模式
    if (m_method == OPTIONS) {
        return true;
    } else if (m_method == DESCRIBE) {
        return parse_describe(msg);
    } else if (m_method == SETUP) {
        return parse_setup(msg);
    } else if (m_method == PLAY) {
        return parse_play(msg);
    } else if (m_method == TEARDOWN) {
        return true;
    } else {
        return false;
    }
}

// 解析序号行
// CSeq: 2\r\n
bool Rtsp_connection::parse_CSeq(std::string &msg) {
    std::size_t pos = msg.find("CSeq");

    // npos表示没有匹配到, 取反就是有
    // static const size_t npos = -1
    if (pos != std::string::npos) {
        uint32_t cseq = 0;

        /**
         * '*' -> 跳过当前位置到:之间的所有字符
         * CSeq: 2\r\n
         */
        sscanf(msg.c_str() + pos, "%*[^:]: %u", &cseq);
        m_CSeq = cseq;
        return true;
    }

    return false;
}

// 解析描述行
// Accept: application/sdp\r\n
bool Rtsp_connection::parse_describe(std::string &msg) {
    if ((msg.rfind("Accept") == std::string::npos)
        || (msg.rfind("sdp") == std::string::npos)) {   //两个都要找到
        return false;
    }

    return true;
}

// 解析创建会话请求
// SETUP rtsp://192.168.1.1/track1 RTSP/1.0
bool Rtsp_connection::parse_setup(std::string &msg) {
    m_track_id = Media_session::Track_id_none;
    std::size_t pos;

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        pos = m_url.find(m_stream_prefix + std::to_string(i));
        if (pos != std::string::npos) {
            if (i == 0) {
                m_track_id = Media_session::Track_id_0;
            } else if (i == 1) {
                m_track_id = Media_session::Track_id_1;
            }
        }
    }

    if (m_track_id == Media_session::Track_id_none) {
        return false;
    }

    pos = msg.find("Transport");
    // for (const auto& m : msg) {
    //     printf("%c", m);
    // }
    if (pos != std::string::npos) {
        if ((pos = msg.find("RTP/AVP/TCP")) != std::string::npos) {
            uint8_t rtp_channel, rtcp_channel;
            m_is_rtp_over_tcp = true;
            LOGINFO("330::m_is_rtp_over_tcp=%d", m_is_rtp_over_tcp);

            /**
             * %hhu -> 将char类型转换为unsigned int, 占用1个字节
             * %hu -> 占用2个字节
             * %hhd -> 将char类型转换为signed int, 溢出会被截断成-1
             * RTP, RTCP信道号
             * Transport: RTP/AVP/TCP;unicast;interleaved=0-1
             */
            if (sscanf(msg.c_str() + pos, "%*[^;];%*[^;];%*[^=]=%hhu-%hhu",
                &rtp_channel, &rtcp_channel) != 2) {
                return false;
            }

            // c和s可以通过一个TCP连接分离/重组音频音视频流和控制流
            // 所以不需要2个端口
            m_rtp_channel = rtp_channel;
            
            return true;

        } else if ((pos = msg.find("RTP/AVP")) != std::string::npos) {
            uint16_t rtp_port = 0, rtcp_port = 0;

            if (((msg.find("unicast", pos)) != std::string::npos)) {
                if (sscanf(msg.c_str() + pos, "%*[^;];%*[^;];%*[^=]=%hu-%hu",
                    &rtp_port, &rtcp_port) != 2) {
                    return false;
                }
            } else if ((msg.find("multicast", pos)) != std::string::npos) {
                // 多播不需要具体端口
                return true;
            } else {    //未知传输模式(UDP/TCP), 返回false
                return false;
            }

            // UDP传输需要2个端口, 一个用于传输RTP数据, 一个用于传输UDP数据
            m_peer_rtp_port = rtp_port;
            m_peer_rtcp_port = rtcp_port;
        } else {

            return false;
        }

        return true;
    }

    return false;
}


// PLAY rtsp://example.com/media.mp4 RTSP/1.0
// CSeq: 5
// Session: 12345678
bool Rtsp_connection::parse_play(std::string &msg) {
    std::size_t pos = msg.find("Session");

    if (pos != std::string::npos) {
        uint32_t session_id = 0;
        // sscanf返回成功赋值的字段个数
        if (sscanf(msg.c_str() + pos, "%*[^:]: %u", &session_id) != 1) {
            return false;
        }
        return true;
    }
    return false;
}

bool Rtsp_connection::handle_cmd_option() {
    snprintf(m_buffer, sizeof(m_buffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %u\r\n"
        "Public: DESCRIBE, ANNOUNCE, SETUP, PLAY, RECORD, PAUSE, GET_PARAMETER, TEARDOWN\r\n"
        "Server: %s\r\n"
        "\r\n",
        m_CSeq, PROJECT_VERSION);

    if (send_msg(m_buffer, strlen(m_buffer)) < 0) {
        LOGERR("failed to send OPTIONS response");
        return false;
    }
    return true;
}

// 生成并发送包含SDP会话描述信息的RTSP响应报文
bool Rtsp_connection::handle_cmd_describe() {
    // 获取会话信息
    Media_session *session = m_rtsp_server->m_sess_mgr->get_session(m_suffix);

    if (!session) {
        LOGERR("can not find session:%s", m_suffix.c_str());
        return false;
    }

    // 发送SDP会话描述信息
    std::string sdp = session->generate_SDP_description();

    // 构造RTSP响应头
    memset((void*)m_buffer, 0, sizeof(m_buffer));
    snprintf((char*)m_buffer, sizeof(m_buffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %u\r\n"
        "Content-Length: %u\r\n"
        "Content-Type: application/sdp\r\n"
        "\r\n"
        "%s",
        m_CSeq, (unsigned int)sdp.size(), sdp.c_str());

    if (send_msg(m_buffer, strlen(m_buffer)) < 0) {
        return false;
    }
    return true;
}

bool Rtsp_connection::handle_cmd_setup() {
    char session_name[100];
    if (sscanf(m_suffix.c_str(), "%[^/]/", session_name) != 1) {
        return false;
    }

    Media_session *session = m_rtsp_server->m_sess_mgr->get_session(session_name);
    // LOGINFO("451::m_is_rtp_over_tcp=%d", m_is_rtp_over_tcp);
    if (!session) {
        LOGERR("can not find session:%s", session_name);
        return false; 
    }

    if (m_track_id >= MEDIA_MAX_TRACK_NUM || m_rtp_instances[m_track_id] || m_rtcp_instances[m_track_id]) {
        return false;
    }

    if (session->is_start_multicast()) {
        snprintf((char*)m_buffer, sizeof(m_buffer),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Transport: RTP/AVP;multicast;"
            "destination=%s;source=%s;port=%d-%d;ttl=255\r\n"
            "Session: %08x\r\n"
            "\r\n",
            m_CSeq,
            session->get_multicast_dest_addr().c_str(),
            mysocket::get_local_ip().c_str(),
            session->get_multicast_dest_rtp_port(m_track_id),
            session->get_multicast_dest_rtp_port(m_track_id) + 1,
            m_session_id);
    } 
    else {
        // LOGINFO("479::m_is_rtp_over_tcp=%d", m_is_rtp_over_tcp);
        if (m_is_rtp_over_tcp) {
            append_rtp_over_tcp(m_track_id, m_client_fd, m_rtp_channel);
            m_rtp_instances[m_track_id]->set_session_id(m_session_id);

            session->add_rtp_instance(m_track_id, m_rtp_instances[m_track_id]);

            snprintf((char*)m_buffer, sizeof(m_buffer),
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %d\r\n"
                "Server: %s\r\n"
                "Transport: RTP/AVP/TCP;unicast;interleaved=%hhu-%hhu\r\n"
                "Session: %08x\r\n"
                "\r\n",
                m_CSeq, PROJECT_VERSION,
                m_rtp_channel,
                m_rtp_channel + 1,
                m_session_id);
        } else {
            // 创建rtp over udp
            if (append_rtp_rtcp_over_udp(m_track_id, m_peer_ip,
                m_peer_rtp_port, m_peer_rtcp_port) != true) {
                LOGERR("failed to append_rtp_rtcp_over_udp");
                return false;
            }

            m_rtp_instances[m_track_id]->set_session_id(m_session_id);
            m_rtcp_instances[m_track_id]->set_session_id(m_session_id);

            //会话管理只需要集中在RTP实例, RTCP不参与媒体数据传输只负责控制
            session->add_rtp_instance(m_track_id, m_rtp_instances[m_track_id]);

            snprintf((char*)m_buffer, sizeof(m_buffer),
                 "RTSP/1.0 200 OK\r\n"
                 "CSeq: %u\r\n"
                 "Server: %s\r\n"
                 "Transport: RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu\r\n"
                 "Session: %08x\r\n"
                 "\r\n",
                 m_CSeq, PROJECT_VERSION,
                 m_peer_rtp_port, 
                 m_peer_rtcp_port,
                 m_rtp_instances[m_track_id]->get_local_port(),
                 m_rtcp_instances[m_track_id]->get_local_port(),
                 m_session_id);
        }
    }

    if (send_msg(m_buffer, strlen(m_buffer)) < 0) {
        return false;
    }
    return true;
}

// 输出RTSP规范内容
bool Rtsp_connection::handle_cmd_play() {
    snprintf((char*)m_buffer, sizeof(m_buffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Server: %s\r\n"
        "Range: npt=0.000-\r\n"
        "Session: %08x; timeout=60\r\n"
        "\r\n",
        m_CSeq, PROJECT_VERSION,
        m_session_id);

    if (send_msg(m_buffer, strlen(m_buffer)) < 0) {
        return false;
    }

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (m_rtp_instances[i]) {
            m_rtp_instances[i]->set_alive(true);
        }

        if (m_rtcp_instances[i]) {
            m_rtcp_instances[i]->set_alive(true);
        }
    }

    return true;
}

bool Rtsp_connection::handle_cmd_teardown() {
    snprintf((char*)m_buffer, sizeof(m_buffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Server: %s\r\n"
        "\r\n",
        m_CSeq, PROJECT_VERSION);
    
    // strlen获取字符串长度, 不包括\0
    // sizeof获取字节数
    if (send_msg(m_buffer, strlen(m_buffer)) < 0) {
        return false;
    }
    return true;
}

// 使用系统调用write
int Rtsp_connection::send_msg(void *buf, int size) {
    LOGINFO("%s", buf);
    int ret;

    m_output_buffer.append(buf, size);
    ret = m_output_buffer.write(m_client_fd);
    // 信息发完重置指针
    m_output_buffer.retrieve_all();

    return ret;
}

int Rtsp_connection::send_msg() {
    int ret = m_output_buffer.write(m_client_fd);
    m_output_buffer.retrieve_all();
    return ret;
}

bool Rtsp_connection::append_rtp_rtcp_over_udp(Media_session::Track_id track_id,
    std::string peer_ip, uint16_t peer_rtp_port, uint16_t peer_rtcp_port) {
    int rtp_sockfd, rtcp_sockfd;
    int16_t rtp_port, rtcp_port;
    bool ret;

    if (m_rtp_instances[track_id] || m_rtcp_instances[track_id]) { /************************ */
        return false;
    }

    int i;
    for (i = 0; i < 10; ++i) {
        rtp_sockfd = mysocket::create_udp_socket();
        if (rtp_sockfd < 0) {
            return false;
        }

        rtcp_sockfd = mysocket::create_udp_socket();
        if (rtcp_sockfd < 0) {
            mysocket::close(rtp_sockfd);
            return false;
        }

        uint16_t port = rand() & 0xfffe;
        if (port < 10000) {
            port += 10000;
        }

        rtp_port = port;
        rtcp_port = port + 1;

        ret = mysocket::bind(rtp_sockfd, "0.0.0.0", rtp_port);
        if (ret != true) {
            mysocket::close(rtp_sockfd);
            mysocket::close(rtcp_sockfd);
            continue;
        }

        ret = mysocket::bind(rtcp_sockfd, "0.0.0.0", rtcp_port); //*
        if (ret != true) {
            mysocket::close(rtp_sockfd);
            mysocket::close(rtcp_sockfd);
            continue;
        }

        break;
    }

    if (i == 10) {
        return false;
    }

    m_rtp_instances[track_id] = Rtp_instance::append_over_udp(rtp_sockfd, rtp_port,
                                                            peer_ip, peer_rtp_port);
    m_rtcp_instances[track_id] = Rtcp_instance::append(rtcp_sockfd, rtcp_port,
                                                            peer_ip, peer_rtcp_port);
    return true;
}

bool Rtsp_connection::append_rtp_over_tcp(Media_session::Track_id track_id,
    int sockfd, uint8_t rtp_channel) {
    m_rtp_instances[track_id] = Rtp_instance::append_over_tcp(sockfd, rtp_channel);
    return true;
}

void Rtsp_connection::handle_rtp_over_tcp() {
    int num = 0;
    while (true) {
        num += 1;
        uint8_t *buf = (uint8_t*) m_input_buffer.peek();
        uint8_t rtp_channel = buf[1];
        uint16_t rtp_size = (buf[2] << 8) | buf[3];
        uint16_t buf_size = 4 + rtp_size;

        if (m_input_buffer.readable_bytes() < buf_size) {
            return;
        } else {    //每个客户端都有4个channel; 13->rtp, 24->rtcp
            if (0x00 == rtp_channel) {
                Rtp_header rtp_header;
                parse_rtp_header(buf + 4, &rtp_header);

                LOGINFO("num=%d,rtp_size=%d", num, rtp_size);
            } else if (0x01 == rtp_channel) {
                Rtcp_header rtcp_header;
                parse_rtcp_header(buf + 4, &rtcp_header);

                LOGINFO("num=%d,rtcp_header.packet_type=%d,rtp_size=%d",
                    num, rtcp_header.packet_type, rtp_size);
            } else if (0x02 == rtp_channel) {
                Rtp_header rtp_header;
                parse_rtp_header(buf + 4, &rtp_header);

                LOGINFO("num=%d,rtp_size=%d", num, rtp_size);
            } else if (0x03 == rtp_channel) {
                Rtcp_header rtcp_header;
                parse_rtcp_header(buf + 4, &rtcp_header);

                LOGINFO("num=%d,rtcp_header.packet_type=%d,rtp_size=%d",
                    num, rtcp_header.packet_type, rtp_size);
            }

            m_input_buffer.retrieve(buf_size);
        }
    }
}