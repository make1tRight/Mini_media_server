#ifndef H264_FILE_SINK_H
#define H264_FILE_SINK_H
#include "sink.h"

class H264_file_sink: public Sink {
public:
    static H264_file_sink *append(Usage_environment *env, Media_source *media_source);

    H264_file_sink(Usage_environment *env, Media_source *media_source);
    virtual ~H264_file_sink();
    virtual std::string get_media_description(uint16_t port);
    virtual std::string get_attribute();
    virtual void send_frame(Media_frame *frame);

private:
    Rtp_packet m_rtp_packet;
    int m_clock_rate;
    int m_fps;
};
#endif //H264_FILE_SINK_H