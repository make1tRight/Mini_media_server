#include "H264_file_sink.h"

H264_file_sink *H264_file_sink::append(Usage_environment *env,
                                        Media_source *media_source) {
    if (!media_source) {
        return nullptr;
    }

    return new H264_file_sink(env, media_source);
}

H264_file_sink::H264_file_sink(Usage_environment *env, Media_source *media_source):
     Sink(env, media_source, RTP_PAYLOAD_TYPE_H264),
     m_clock_rate(90000), //H264编码常用时钟频率90000, 能够轻松与多种帧率换算
     m_fps(media_source->get_fps()) {
    LOGINFO("H264_file_sink()");
    run_every(1000 / m_fps);
}

H264_file_sink::~H264_file_sink() {
    LOGINFO("~H264_file_sink()");
}

std::string H264_file_sink::get_media_description(uint16_t port) {
    char buf[100] = {0};
    // memset(&buf, 0, sizeof(buf));
    // AVP表示会话要用RTP协议来传输音视频数据
    sprintf(buf, "m=video %hu RTP/AVP %d", port, m_payload_type);

    return std::string(buf);
}

std::string H264_file_sink::get_attribute() {
    char buf[100];
    sprintf(buf, "a=rtpmap:%d H264/%d\r\n", m_payload_type, m_clock_rate);
    sprintf(buf + strlen(buf), "a=framerate:%d", m_fps);

    return std::string(buf);
}

void H264_file_sink::send_frame(Media_frame *frame) {
    // 发送RTP数据包
    Rtp_header *rtp_header = m_rtp_packet.m_rtp_header;
    uint8_t nalu_type = frame->m_buf[0];

    if (frame->m_size <= RTP_MAX_PKT_SIZE) { //媒体帧大小不超过RTP包最大大小
        memcpy(rtp_header->payload, frame->m_buf, frame->m_size);
        m_rtp_packet.m_size = RTP_HEADER_SIZE + frame->m_size;
        send_rtp_packet(&m_rtp_packet);
        ++m_seq;

        if ((nalu_type & 0x1F) == 7 || (nalu_type & 0x1F) == 8) {
            // SPS, PPS不需要加时间戳
            // SPS对标识符, 帧数及参考帧数目, 解码图像尺寸和帧场模式等解码参数进行标识记录
            // PPS对熵编码类型, 有效参考图像的数目和初始化等解码参数进行标识记录
            return;
        }
    } else {    //超过了RTP包最大大小, 拆分后通过RTP发送
        int pkt_num = frame->m_size / RTP_MAX_PKT_SIZE;         //有几个完整的包
        int remain_pkt_size = frame->m_size % RTP_MAX_PKT_SIZE; //剩余不完整包的大小
        int i, pos = 1;

        // 发送完整的包
        for (i = 0; i < pkt_num; ++i) {
            // NALU类型设置为28, 标识这个NALU是分片的
            /**
             * FU Indicator
             *  0 1 2 3 4 5 6 7
             * +-+-+-+-+-+-+-+-+
             * |F|NRI|  Type   |
             * +-+-+-+-+-+-+-+-+
             * 值是28表示该RTP包是个分片, H.264规范
             */
            rtp_header->payload[0] = (nalu_type & 0x60) | 28;
            rtp_header->payload[1] = (nalu_type & 0x1F);

            if (i == 0) {   //第一包数据
                /* FU Header */
                rtp_header->payload[1] |= 0x80; //start
            } else if (remain_pkt_size == 0 && i == pkt_num - 1) {  //最后一包数据
                /**
                 *  FU Header
                 *  0 1 2 3 4 5 6 7
                 * +-+-+-+-+-+-+-+-+
                 * |S|E|R|  Type   |
                 * +-+-+-+-+-+-+-+-+
                 * E表示是当前分片最后一个RTP包
                 * S表示是当前分片的第一包
                 */
                rtp_header->payload[1] |= 0x40; //end
            }

            memcpy(rtp_header->payload + 2, frame->m_buf + pos, RTP_MAX_PKT_SIZE);
            m_rtp_packet.m_size = RTP_HEADER_SIZE + 2 + RTP_MAX_PKT_SIZE;
            send_rtp_packet(&m_rtp_packet);

            ++m_seq;
            pos += RTP_MAX_PKT_SIZE;
        }

        if (remain_pkt_size > 0) {
            rtp_header->payload[0] = (nalu_type & 0x60) | 28;
            rtp_header->payload[1] = (nalu_type & 0x1F);
            rtp_header->payload[1] |= 0x40; //end

            memcpy(rtp_header->payload + 2, frame->m_buf + pos, remain_pkt_size);
            m_rtp_packet.m_size = RTP_HEADER_SIZE + 2 + remain_pkt_size;
            send_rtp_packet(&m_rtp_packet);

            ++m_seq;
        }
    }

    m_timestamp += m_clock_rate / m_fps;
}