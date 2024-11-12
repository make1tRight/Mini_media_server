#ifndef MEDIA_SOURCE_H
#define MEDIA_SOURCE_H
#include <queue>
#include <mutex>
#include "../rtsp/usage_environment.h"
#include "../threadpool/threadpool.h"

const int FRAME_MAX_SIZE = (1024 * 200);
const int DEFAULT_FRAME_NUM = 4;

class Media_frame {
public:
    Media_frame(): temp(new uint8_t[FRAME_MAX_SIZE]), m_buf(nullptr), m_size(0) {}
    ~Media_frame() { delete [] temp; }

public:
    uint8_t *temp;      //用于临时存储和管理音视频帧数据
    uint8_t *m_buf;
    int m_size;
};


class Media_source {
public:
    explicit Media_source(Usage_environment *env);
    virtual ~Media_source();

    Media_frame *get_frame_from_output_queue(); //从输出队列获取帧
    void put_frame_to_input_queue(Media_frame *frame); //帧送至输入队列
    int get_fps() const { return m_fps; }
    std::string get_source_name() { return m_source_name; }

private:
    static void task_callback(void *arg);
protected:
    virtual void handle_task() = 0;
    void set_fps(int fps) { m_fps = fps; }

protected:
    Usage_environment *m_env;
    Media_frame m_frames[DEFAULT_FRAME_NUM];
    std::queue<Media_frame*> m_frame_input_queue;
    std::queue<Media_frame*> m_frame_output_queue;

    std::mutex m_mutex;
    Threadpool::MyTask m_task;
    int m_fps;
    std::string m_source_name;
};
#endif //MEDIA_SOURCE_H