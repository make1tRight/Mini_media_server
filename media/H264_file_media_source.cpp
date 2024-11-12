#include "H264_file_media_source.h"


static inline int start_code3(uint8_t *buf) {
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1) {
        return 1;
    } else {
        return 0;
    }
}

static inline int start_code4(uint8_t *buf) {
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1) {
        return 1;
    } else {
        return 0;
    }
}

static uint8_t *find_next_start_code(uint8_t *buf, int len) {
    int i;

    if (len < 3) {
        return nullptr;
    }

    for (int i = 0; i < len - 3; ++i) {
        if (start_code3(buf) || start_code4(buf)) {
            return buf;
        }
        ++buf;
    }

    // 这里为什么要分开, 不是很明白
    if (start_code3(buf)) {
        return buf;
    }

    return nullptr;
}

H264_file_media_source *H264_file_media_source::append(Usage_environment *env,
                                        const std::string &file) {
    return new H264_file_media_source(env, file);
}

H264_file_media_source::H264_file_media_source(Usage_environment *env, 
                                             const std::string &file):
 Media_source(env) {
    m_source_name = file;
    // r只读read only, b以binary模式打开文件
    m_file = fopen(file.c_str(), "rb");

    set_fps(25);

    for (int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        m_env->threadpool()->add_task(m_task);
    }
}

H264_file_media_source::~H264_file_media_source() {
    fclose(m_file);
}

// 只处理一帧数据
void H264_file_media_source::handle_task() {
    std::lock_guard<std::mutex> lck(m_mutex);

    if (m_frame_input_queue.empty()) {
        return;
    }

    Media_frame *frame = m_frame_input_queue.front();
    int start_code_num = 0;
    
    while (true) {
        frame->m_size = get_frame_from_H264_file(frame->temp, FRAME_MAX_SIZE);
        if (frame->m_size < 0) {
            return;
        }

        // 起始码有两种0x000001(3bytes)和0x00000001(4bytes)
        if (start_code3(frame->temp)) {
            start_code_num = 3;
        } else {
            start_code_num = 4;
        }

        // 去掉起始码
        frame->m_buf = frame->temp + start_code_num;
        frame->m_size -= start_code_num;

        // 获取NAL Unit的类型
        uint8_t nalu_type = frame->m_buf[0] & 0x1F;
        if (0x09 == nalu_type) {
            // NALU类型9: access unit delimiter, 分隔符
            continue;
        } else if (0x07 == nalu_type || 0x08 == nalu_type) {    //下一帧数据跳出循环
            break;
        } else {
            break;
        }
    }
    m_frame_input_queue.pop();
    m_frame_output_queue.push(frame);
}

// 读取帧数据到frame->temp, 返回帧数据大小
int H264_file_media_source::get_frame_from_H264_file(uint8_t *frame, int size) {
    if (!m_file) {
        return -1;
    }

    int r, frame_size;
    uint8_t *next_start_code;
    // 每次从m_file读取1字节, 要读取size, 到frame
    // 实际读取r字节
    r = fread(frame, 1, size, m_file);  
    if (!start_code3(frame) && !start_code4(frame)) {   //没有起始码
        fseek(m_file, 0, SEEK_SET); //将文件指针重置到文件开头
        LOGERR("Read %s error, no start_code3 or start_code4", m_source_name.c_str());
        return -1;
    }

    // 跳过当前帧的起始码查找下一个起始码
    next_start_code = find_next_start_code(frame + 3, r - 3);
    if (!next_start_code) { //找不到下一个起始码
        fseek(m_file, 0, SEEK_SET);                 //SEEK_SET将文件指针设置为文件开头
        frame_size = r;
        LOGERR("Read %s error, no next_start_code, r=%d", m_source_name.c_str(), r);
    } else {
        // 下一起始符到已读取指针位置是当前帧的大小
        frame_size = (next_start_code - frame);
        fseek(m_file, frame_size - r, SEEK_CUR);    //跳过已读取的帧数据
    }

    return frame_size;
}