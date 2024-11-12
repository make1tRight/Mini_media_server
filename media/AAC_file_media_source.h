#ifndef AAC_FILE_MEDIA_SOURCE_H
#define AAC_FILE_MEDIA_SOURCE_H
#include <string.h>
#include "media_source.h"

class AAC_file_media_source: public Media_source {
public:
    static AAC_file_media_source *append(Usage_environment *env,
                                        const std::string &file);
    AAC_file_media_source(Usage_environment *env, const std::string &file);
    virtual ~AAC_file_media_source();

protected:
    virtual void handle_task();

private:
    /* Adts首部 */
    struct Adts_header {
        unsigned int syncword;                              //12bit, 说明ADTS帧开始(0xFFF)
        unsigned int id;                                    //1bit, 0->MPEG-4, 1->MEPG-2
        unsigned int layer;                                 //2bit, 00
        unsigned int protection_absent;                     //1bit, 误码校验位, 0有CRC,1无
        unsigned int profile;                               //2bit, 标识9种AAC之一
        unsigned int sampling_frequency_index;              //4bit, 采样率
        unsigned int private_bit;                           //1bit, 编码设置为0解码无用
        unsigned int channel_configuration;                 //3bit, 声道数
        unsigned int original_copy;                         //1bit, 编码设置为0解码无用
        unsigned int home;                                  //1bit, 编码设置为0解码无用

        // 以下参数每一帧都不同
        unsigned int copyright_identification_bit;          //1bit, 编码设置为0解码无用
        unsigned int copyright_identification_stat;         //1bit, 编码设置为0解码无用
        unsigned int aac_frame_length;                      //13bit, ADTS帧长度(包含首部)
        unsigned int adts_buffer_fullness;                  //11bit, 0x7FF->码率可变码流

        // 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧
        // 如果下面这个参数为0说明只有1个AAC原始帧
        unsigned int number_of_raw_data_blocks_in_frame;    //2bit, AAC原始帧数据量

    };
    bool parse_adts_header(uint8_t *in, struct Adts_header *res);
    int get_frame_from_AAC_file(uint8_t *buf, int size);
private:
    FILE *m_file;
    struct Adts_header m_adts_header;
};
#endif //AAC_FILE_MEDIA_SOURCE_H