#include "AAC_file_media_source.h"
AAC_file_media_source *AAC_file_media_source::append(Usage_environment *env,
                                    const std::string &file) {
    return new AAC_file_media_source(env, file);
}

AAC_file_media_source::AAC_file_media_source(Usage_environment *env, 
                                            const std::string &file):
    Media_source(env) {
    m_source_name = file;
    m_file = fopen(file.c_str(), "rb");

    set_fps(43);

    for (int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        m_env->threadpool()->add_task(m_task);
    }
}

AAC_file_media_source::~AAC_file_media_source() {
    fclose(m_file);
}

void AAC_file_media_source::handle_task() {
    std::lock_guard<std::mutex> lck(m_mutex);

    if (m_frame_input_queue.empty()) {
        return;
    }

    Media_frame *frame = m_frame_input_queue.front();
    frame->m_size = get_frame_from_AAC_file(frame->temp, FRAME_MAX_SIZE);
    if (frame->m_size < 0) {
        return;
    }

    frame->m_buf = frame->temp;

    m_frame_input_queue.pop();
    m_frame_output_queue.push(frame);
}

// 解析adts的header
bool AAC_file_media_source::parse_adts_header(uint8_t *in, struct Adts_header *res) {
    memset(res, 0, sizeof(*res));
    if ((in[0] == 0xFF) && ((in[1] & 0xF0) == 0xF0)) {  //首先检查最前面0xFFF没问题
        res->id = ((unsigned int) in[1] & 0x08) >> 3;
        res->layer = ((unsigned int) in[1] & 0x06) >> 1;
        res->protection_absent = ((unsigned int) in[1] & 0x01);
        res->profile = ((unsigned int) in[2] & 0xC0) >> 6;
        res->sampling_frequency_index = ((unsigned int) in[2] & 0x3C) >> 2;
        res->private_bit = ((unsigned int) in[2] & 0x02) >> 1;
        res->channel_configuration = ((((unsigned int) in[2] & 0x01) << 2) |
                                    (((unsigned int) in[3] & 0xC0) >> 6));
        res->original_copy = ((unsigned int) in[3] & 0x20) >> 5;
        res->home = ((unsigned int) in[3] & 0x10) >> 4;
        res->copyright_identification_bit = ((unsigned int) in[3] & 0x08) >> 3;
        res->copyright_identification_stat = ((unsigned int) in[3] & 0x04) >> 2;
        res->aac_frame_length = ((((unsigned int) in[3] & 0x03) << 11) |
                                (((unsigned int) in[4] & 0xFF) << 3) |
                                (((unsigned int) in[5] & 0xE0) >> 5));
        res->adts_buffer_fullness = ((((unsigned int) in[5] & 0x1F) << 6) |
                                    (((unsigned int) in[6] & 0xFC) >> 2));
        res->number_of_raw_data_blocks_in_frame = ((unsigned int) in[6] & 0x03);

        return true;
    } else {
        LOGERR("fail to parse adts header");
        return false;
    }
}

// 从.aac文件中获取帧数据
int AAC_file_media_source::get_frame_from_AAC_file(uint8_t *buf, int size) {
    if (!m_file) {
        return -1;
    }
    
    // 存储adts首部, 占7个字节
    uint8_t tmp_buf[7];
    int ret;

    ret = fread(tmp_buf, 1, 7, m_file);

    if (ret <= 0) {
        fseek(m_file, 0, SEEK_SET);
        ret = fread(tmp_buf, 1, 7, m_file);
        if (ret <= 0) {
            return -1;
        }
    }

    if (!parse_adts_header(tmp_buf, &m_adts_header)) {
        return -1;
    }

    if (m_adts_header.aac_frame_length > size) {
        return -1;
    }

    memcpy(buf, tmp_buf, 7);    //将tmp_buf中存储的adts首部7字节拷贝到buf中
    ret = fread(buf + 7, 1, m_adts_header.aac_frame_length - 7, m_file);    //读取音频数据
    if (ret < 0) {
        LOGERR("read error");
        return -1;
    }

    return m_adts_header.aac_frame_length;
}