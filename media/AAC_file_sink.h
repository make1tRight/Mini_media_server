#ifndef AAC_FILE_SINK_H
#define AAC_FILE_SINK_H

#include "sink.h"
#include "media_source.h"
#include "../rtsp/usage_environment.h"
class AAC_file_sink: public Sink {
public:
    static AAC_file_sink *append(Usage_environment *env,
                                 Media_source *media_source);
    AAC_file_sink(Usage_environment *env, Media_source *media_source, int payload_type);
    virtual ~AAC_file_sink();
    virtual std::string get_media_description(uint16_t port);
    virtual std::string get_attribute();

protected:
    virtual void send_frame(Media_frame *frame);

private:
    Rtp_packet m_rtp_packet;
    uint32_t m_sample_rate; //采样频率
    uint32_t m_channels;    //声道数
    int m_fps;
}; 
#endif //AAC_FILE_SINK_H