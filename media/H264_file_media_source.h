#ifndef H264_FILE_MEDIA_SOURCE_H
#define H264_FILE_MEDIA_SOURCE_H
#include "media_source.h"

class H264_file_media_source : public Media_source {
public:
    static H264_file_media_source *append(Usage_environment *env,
                                         const std::string &file);

    H264_file_media_source(Usage_environment *env, const std::string &file);
    virtual ~H264_file_media_source();
protected:
    virtual void handle_task();
private:
    int get_frame_from_H264_file(uint8_t *frame, int size);
private:
    FILE *m_file;
};
#endif //H264_FILE_MEDIA_SOURCE_H