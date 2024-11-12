#include "media_session.h"


Media_session* Media_session::append(std::string session_name) {
    return new Media_session(session_name);
}

Media_session::Media_session(const std::string &session_name):
    m_session_name(session_name),
    m_is_start_multicast(false) {
    LOGINFO("Media_session() name=%s", session_name.data());
    
    // 初始化两个媒体流(最多2个)
    m_tracks[0].m_track_id = Track_id_0;
    m_tracks[0].m_is_alive = false;

    m_tracks[1].m_track_id = Track_id_1;
    m_tracks[1].m_is_alive = false;

    // 目前没有多播RTP与RTCP实例
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        m_multicast_rtp_instances[i] = nullptr;
        m_multicast_rtcp_instances[i] = nullptr;
    }
}

Media_session::~Media_session() {
    LOGINFO("~Media_session()");

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (m_multicast_rtp_instances[i]) {
            this->remove_rtp_instance(m_multicast_rtp_instances[i]);
            delete m_multicast_rtp_instances[i];
        }

        if (m_multicast_rtcp_instances[i]) {
            // this->remove_rtcp_instance(m_multicast_rtcp_instances[i]);
            delete m_multicast_rtcp_instances[i];
        }
    }

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (m_tracks[i].m_is_alive) {
            Sink *sink = m_tracks[i].m_sink;
            delete sink;
        }
    }
}

std::string Media_session::generate_SDP_description() {
    if (!m_SDP.empty()) {
        return m_SDP;
    }

    std::string ip = "0.0.0.0";
    char buf[2048] = {0};

    snprintf(buf, sizeof(buf),
        "v=0\r\n"                               //版本号
        "o=- 9%ld 1 IN IP4 %s\r\n"              //会话起源字段        // "s=%s\r\n"
        "t=0 0\r\n"                             //0 0表示无限制时间
        "a=control:*\r\n"                       //控制所有轨道
        "a=type:broadcast\r\n",                 //广播会话
        (long)time(nullptr), ip.c_str());//, m_session_name.c_str()
    
    if (is_start_multicast()) {
        /**
         * RTCP反射模式就是接收者之间共享RTCP包
         * 不用反馈给发送端, 可在组播环境中实现流量控制
         */
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
            "a=rtcp-unicast: reflection\r\n");  //使用RTCP反射模式
    }

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        uint16_t port = 0;

        if (m_tracks[i].m_is_alive != true) {
            continue;
        }

        if (is_start_multicast()) { //如果开启了组播, 获取当前轨道的目标端口
            port = get_multicast_dest_rtp_port((Track_id) i);
        }

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),  //加到末尾
            "%s\r\n", m_tracks[i].m_sink->get_media_description(port).c_str());
        
        if (is_start_multicast()) { //开启了组播
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                "c=IN IP4 %s/255\r\n", get_multicast_dest_addr().c_str());
        } else {    //没有开启组播, 不绑定到特定地址
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                "c=IN IP4 0.0.0.0\r\n");
        }

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
            "%s\r\n", m_tracks[i].m_sink->get_attribute().c_str());
        
        // 添加控制字段
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
            "a=control:track%d\r\n", m_tracks[i].m_track_id);
    }

    m_SDP = buf;
    return m_SDP;
}

// 添加数据生产者
bool Media_session::add_sink(Media_session::Track_id track_id, Sink *sink) {
    Track *track = get_track(track_id);

    if (!track) {
        return false;
    }

    track->m_sink = sink;
    track->m_is_alive = true;

    // 设置发送rtp报文回调函数
    sink->set_session_send_packet_callback(Media_session::send_packet_callback, 
        this, track);
    
    return true;
}

// 添加数据消费者
bool Media_session::add_rtp_instance(Media_session::Track_id track_id, 
    Rtp_instance *rtp_instance) {

    Track *track = get_track(track_id);
    if (!track || track->m_is_alive != true) {
        return false;
    }

    track->m_rtp_instances.push_back(rtp_instance);
    return true;
}

// 删除数据消费者
bool Media_session::remove_rtp_instance(Rtp_instance *rtp_instance) {
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (m_tracks[i].m_is_alive == false) {
            continue;
        }

        // 两个轨道，一个音轨一个视频轨，都可拥有多个rtp实例
        std::list<Rtp_instance*>::iterator it = std::find(m_tracks[i].m_rtp_instances.begin(),
            m_tracks[i].m_rtp_instances.end(), 
            rtp_instance);
        
        if (it == m_tracks[i].m_rtp_instances.end()) {
            continue;
        }

        m_tracks[i].m_rtp_instances.erase(it);
        return true;
    }

    return false;
} 

bool Media_session::start_multicast() {
    struct sockaddr_in addr = {0};
    // memset(&addr, 0, sizeof(addr));

    // RFC 2365定义的局部管理组播地址range
    uint32_t range = 0xE8FFFFFF - 0xE8000100;
    addr.sin_addr.s_addr = htonl(0xE8000100 + (rand()) % range);
    m_multicast_addr = inet_ntoa(addr.sin_addr);

    int rtp_sockfd_1, rtcp_sockfd_1;
    int rtp_sockfd_2, rtcp_sockfd_2;

    uint16_t rtp_port_1, rtcp_port_1;
    uint16_t rtp_port_2, rtcp_port_2;
    bool ret;

    rtp_sockfd_1 = mysocket::create_udp_socket();
    assert(rtp_sockfd_1 > 0);
    rtp_sockfd_2 = mysocket::create_udp_socket();
    assert(rtp_sockfd_2 > 0);

    rtcp_sockfd_1 = mysocket::create_udp_socket();
    assert(rtcp_sockfd_1 > 0);
    rtcp_sockfd_2 = mysocket::create_udp_socket();
    assert(rtcp_sockfd_2 > 0);

    uint16_t port = rand() % 0xfffe;
    if (port < 10000) {
        port += 10000;
    }

    rtp_port_1 = port;
    rtcp_port_1 = port + 1;
    rtp_port_2 = rtcp_port_1 + 1;
    rtcp_port_2 = rtp_port_2 + 1;

    // 1个track分别对应1个rtp和1个rtcp实例
    m_multicast_rtp_instances[Track_id_0] = Rtp_instance::append_over_udp(
        rtp_sockfd_1, 0, m_multicast_addr, rtp_port_1
    );
    m_multicast_rtp_instances[Track_id_1] = Rtp_instance::append_over_udp(
        rtp_sockfd_2, 0, m_multicast_addr, rtp_port_2
    );
    m_multicast_rtcp_instances[Track_id_0] = Rtcp_instance::append(
        rtcp_sockfd_1, 0, m_multicast_addr, rtcp_port_1
    );
    m_multicast_rtcp_instances[Track_id_1] = Rtcp_instance::append(
        rtcp_sockfd_2, 0, m_multicast_addr, rtcp_port_2
    );

    this->add_rtp_instance(Track_id_0, m_multicast_rtp_instances[Track_id_0]);
    this->add_rtp_instance(Track_id_1, m_multicast_rtp_instances[Track_id_1]);
    m_is_start_multicast = true;
    return true;
}

bool Media_session::is_start_multicast() {
    return m_is_start_multicast;
}

uint16_t Media_session::get_multicast_dest_rtp_port(Track_id track_id) {
    if (track_id > Track_id_1 || !m_multicast_rtp_instances[track_id]) {
        return -1;
    }

    return m_multicast_rtp_instances[track_id]->get_peer_port();
}

// 根据track_id搜索对应的track
Media_session::Track *Media_session::get_track(Media_session::Track_id track_id) {
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i) {
        if (m_tracks[i].m_track_id == track_id) {
            return &m_tracks[i];
        }
    }
    return nullptr;
}

void Media_session::send_packet_callback(void *arg1, void *arg2, 
    void *packet, Sink::Packet_type packet_type) {
    Rtp_packet *rtp_packet = (Rtp_packet*) packet;

    Media_session *session = (Media_session*) arg1;
    Media_session::Track *track = (Media_session::Track*) arg2;

    // 实际发送rtp报文的函数
    session->handle_send_rtp_packet(track, rtp_packet);
}

void Media_session::handle_send_rtp_packet(Media_session::Track *track, 
    Rtp_packet *rtp_packet) {

    std::list<Rtp_instance*>::iterator it;
    for (it = track->m_rtp_instances.begin(); it != track->m_rtp_instances.end(); ++it) {
        Rtp_instance *rtp_instance = *it;
        if (rtp_instance->alive()) {    //向所有活跃的接收者发送rtp包
            rtp_instance->send(rtp_packet);
        }
    }
}