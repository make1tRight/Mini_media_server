#include "sink.h"


// 初始化RTP头部, 用户环境, 媒体资源, 载荷类型
Sink::Sink(Usage_environment *env, Media_source *media_source, int payload_type):
    m_media_source(media_source),
    m_env(env),
    m_csrc_len(0),
    m_extension(0),
    m_padding(0),
    m_version(RTP_VERSION),
    m_payload_type(payload_type),
    m_marker(0),
    m_seq(0),
    m_ssrc(rand()), //确保每个同步信源具有不同的SSRC
    m_timestamp(0),
    m_timer_id(0),
    m_session_send_packet(nullptr),
    m_arg1(nullptr),
    m_arg2(nullptr) {
    
    LOGINFO("Sink()");
    m_timer_event = TimerEvent::append(this);
    m_timer_event->set_timeout_callback(cb_timeout);
}


Sink::~Sink() {
    LOGINFO("~Sink()");
    delete m_timer_event;
    delete m_media_source;
}

void Sink::stop_timer_event() {
    m_timer_event->stop();
}

void Sink::set_session_send_packet_callback(Session_send_packet_callback cb,
                                        void *arg1, void *arg2) {
    m_session_send_packet = cb;
    m_arg1 = arg1;
    m_arg2 = arg2;
}

void Sink::send_rtp_packet(Rtp_packet *rtp_packet) {
    Rtp_header *rtp_header = rtp_packet->m_rtp_header;
    rtp_header->csrc_len = m_csrc_len;
    rtp_header->extionsion = m_extension;
    rtp_header->padding = m_padding;
    rtp_header->version = m_version;
    rtp_header->payload_type = m_payload_type;
    rtp_header->marker = m_marker;
    rtp_header->seq = htons(m_seq); //16bit
    rtp_header->timestamp = htonl(m_timestamp); //32bit
    rtp_header->ssrc = htonl(m_ssrc); //32bit

    if (m_session_send_packet) {
        m_session_send_packet(m_arg1, m_arg2, 
            rtp_packet, Packet_type::RTPPACKET);
    }
}

void Sink::run_every(int interval) {
    m_timer_id = m_env->scheduler()->add_timer_event_run_every(m_timer_event, interval);
}

void Sink::cb_timeout(void *arg) {
    Sink *sink = (Sink*) arg;
    sink->handle_timeout();
}

void Sink::handle_timeout() {
    Media_frame *frame = m_media_source->get_frame_from_output_queue();
    if (!frame) {
        return;
    }

    this->send_frame(frame); //这里由Sink的子类来发送, 具体是AAC或者H264
    /**
     * 将使用过的frame插入输入队列
     * 插入输入队列后, 调用子线程task, 从文件中读取数据, 再次将输入写入frame
     */ 
    m_media_source->put_frame_to_input_queue(frame);
}