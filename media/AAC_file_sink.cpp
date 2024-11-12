#include "AAC_file_sink.h"

AAC_file_sink *AAC_file_sink::append(Usage_environment *env,
                                Media_source *media_source) {
    return new AAC_file_sink(env, media_source, RTP_PAYLOAD_TYPE_AAC);
}

AAC_file_sink::AAC_file_sink(Usage_environment *env, Media_source *media_source, int payload_type):
     Sink(env, media_source, payload_type),
     m_sample_rate(44100),
     m_channels(2),
     m_fps(media_source->get_fps()) {
    LOGINFO("AAC_file_sink()");
    m_marker = 1;
    run_every(1000 / m_fps);
}

AAC_file_sink::~AAC_file_sink() {
    LOGINFO("~AAC_file_sink()");
}


std::string AAC_file_sink::get_media_description(uint16_t port) {
    char buf[100] = {0};
    sprintf(buf, "m=audio %hu RTP/AVP %d", port, m_payload_type);

    return std::string(buf);
}

/**
 * AAC编码标准支持的16个采样率
 * 44100Hz(CD), 48000Hz(DVD)最常用 
*/
static uint32_t AAC_sample_rate[16] = {
    97000, 88200, 64000, 48000,
    44100, 33200, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0
};

std::string AAC_file_sink::get_attribute() {
    char buf[500] = {0};
    // RTP分配给AAC音频的载荷类型通常是97
    sprintf(buf, "a=rtpmap:97 mpeg4-generic/%u/%u\r\n", m_sample_rate, m_channels);

    uint8_t index = 0;
    for (index = 0; index < 16; ++index) {
        if (AAC_sample_rate[index] == m_sample_rate) {
            break;
        }
    }
    // 没有找到规定的采样率返回空字符串
    if (index == 16) {
        return "";
    }

    // 生成配置信息
    uint8_t profile = 1;    //用于表示AAC的9种配置类型之一 AAC-LC
    char config_str[10] = {0};
    sprintf(config_str, "%02x%02x",
        (uint8_t)((profile + 1) << 3) | (index >> 1),
        (uint8_t)((index << 7) | (m_channels << 3)));

    sprintf(buf + strlen(buf),
        "a=fmtp:%d profile-level-id=1;"
        "mode=AAC-hbr;"
        "sizelength=13;indexlength=3;indexdeltalength=3;"
        "config=%04u",
        m_payload_type,
        atoi(config_str));
    
    return std::string(buf);
}

// 将AAC数据封装成RTP包, 发送给接收端
void AAC_file_sink::send_frame(Media_frame *frame) {
    // 这里不是真的往rtp协议首部加东西, 是他通过RTP协议传输RTP载荷的最前面数据
    Rtp_header *rtp_header = m_rtp_packet.m_rtp_header;
    int frame_size = frame->m_size - 7; //去掉AAC头部

    /**
     * AAC作为RTP载荷要遵循的格式
     * 第一个字节 - 0x00
     * 第二个字节 - 0x10
     * 第三、四个字节保存AAC data的大小
     * 第三字节保存数据大小高8位、第四字节的高5位保存AAC数据大小的低5位
     */
    rtp_header->payload[0] = 0x00;
    rtp_header->payload[1] = 0x10;
    // RTP载荷最多使用13位来表示AAC数据大小, 所以最高3位被截断了
    rtp_header->payload[2] = (frame_size & 0x1FE0) >> 5;    //高8位
    rtp_header->payload[3] = (frame_size & 0x1F) << 3;      //低5位

    /* 去掉AAC头部 */
    memcpy(rtp_header->payload + 4, frame->m_buf + 7, frame_size);
    // 跳过RTP首部12字节+传输AAC载荷首部4字节+去掉AAC头部后数据帧大小
    m_rtp_packet.m_size = RTP_HEADER_SIZE + 4 + frame_size;

    send_rtp_packet(&m_rtp_packet);

    ++m_seq;

    /* （1000/m_fps) 表示1帧多少毫秒 */
    m_timestamp += m_sample_rate * (1000 / m_fps) / 1000;   //计算每一帧增长多少秒
}