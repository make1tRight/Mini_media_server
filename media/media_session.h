#ifndef MEDIA_SESSION_H
#define MEDIA_SESSION_H
#include <string>
#include <list>
#include <algorithm>
#include <assert.h>
#include "sink.h"
#include "../rtp/rtp_instance.h"

const int MEDIA_MAX_TRACK_NUM = 2;
class Media_session {
public:
    enum Track_id {
        Track_id_none = -1,
        Track_id_0 = 0,
        Track_id_1 = 1
    };

    static Media_session* append(std::string session_name);
    explicit Media_session(const std::string &session_name);
    ~Media_session();

public:
    std::string name() const { return m_session_name; }
    std::string generate_SDP_description();
    // 添加数据生产者
    bool add_sink(Media_session::Track_id track_id, Sink *sink);    
    // 添加数据消费者
    bool add_rtp_instance(Media_session::Track_id track_id, Rtp_instance *rtp_instance);
    // 删除数据消费者
    bool remove_rtp_instance(Rtp_instance *rtp_instance); 

    bool start_multicast();
    bool is_start_multicast();
    std::string get_multicast_dest_addr() const { return m_multicast_addr; }
    uint16_t get_multicast_dest_rtp_port(Track_id track_id);

private:
    class Track {
    public:
        Sink *m_sink;
        int m_track_id;
        bool m_is_alive;
        std::list<Rtp_instance*> m_rtp_instances; 
    };

    Track *get_track(Media_session::Track_id track_id);
    static void send_packet_callback(void *arg1, void *arg2, 
        void *packet, Sink::Packet_type packet_type);
    void handle_send_rtp_packet(Media_session::Track *track, Rtp_packet *rtp_packet);

private:
    std::string m_session_name;
    std::string m_SDP;
    Track m_tracks[MEDIA_MAX_TRACK_NUM];
    bool m_is_start_multicast;  //是否启用多播
    std::string m_multicast_addr;
    Rtp_instance *m_multicast_rtp_instances[MEDIA_MAX_TRACK_NUM];
    Rtcp_instance *m_multicast_rtcp_instances[MEDIA_MAX_TRACK_NUM];
};
#endif // MEDIA_SESSION_H