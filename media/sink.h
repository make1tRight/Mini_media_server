#ifndef SINK_H
#define SINK_H
#include <netinet/in.h>
#include "../rtsp/event.h"
#include "../timer/timer.h"
#include "../rtsp/usage_environment.h"
#include "../media/media_source.h"
#include "../rtp/rtp.h"


// 媒体关键类(H264|AAC)
class Sink {
public:
    enum Packet_type {
        UNKNOW = -1,
        RTPPACKET = 0
    };
    typedef void (*Session_send_packet_callback)(void *arg1, void *arg2,
                                                 void *packet, Packet_type packet_type);
    
    Sink(Usage_environment *env, Media_source *media_source, int payload_type);
    virtual ~Sink();
    void stop_timer_event();
    virtual std::string get_media_description(uint16_t port) = 0;
    virtual std::string get_attribute() = 0;

    void set_session_send_packet_callback(Session_send_packet_callback cb,
                                         void *arg1, void *arg2);

protected:
    virtual void send_frame(Media_frame *frame) = 0;
    void send_rtp_packet(Rtp_packet *rtp_packet);
    void run_every(int interval);
protected:
    Usage_environment *m_env;
    Media_source *m_media_source;
    Session_send_packet_callback m_session_send_packet;
    void *m_arg1;
    void *m_arg2;

    uint8_t m_csrc_len;
    uint8_t m_extension;
    uint8_t m_padding;
    uint8_t m_version;
    uint8_t m_payload_type;
    uint8_t m_marker;
    uint16_t m_seq;
    uint32_t m_timestamp;
    uint32_t m_ssrc;
private:
    static void cb_timeout(void *arg);
    void handle_timeout();
private:
    TimerEvent *m_timer_event;
    Timer::TimerId m_timer_id;
};
#endif //SINK_H