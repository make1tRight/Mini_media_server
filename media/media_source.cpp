#include "media_source.h"


Media_source::Media_source(Usage_environment *env):
    m_env(env), m_fps(0) {
    for (int i = 0; i < DEFAULT_FRAME_NUM; ++i) {
        m_frame_input_queue.push(&m_frames[i]);
    }

    m_task.set_task_callback(task_callback, this);
}

Media_source::~Media_source() {
    LOGINFO("~Media_source()");
}

// 从输出队列取出帧
Media_frame *Media_source::get_frame_from_output_queue() {
    std::lock_guard<std::mutex> lck(m_mutex);

    if (m_frame_output_queue.empty()) {
        return nullptr;
    }

    Media_frame *frame = m_frame_output_queue.front();
    m_frame_output_queue.pop();

    return frame;
}

// 将帧放入队列
void Media_source::put_frame_to_input_queue(Media_frame *frame) {
    // RAII类, 自动加锁与解锁
    std::lock_guard<std::mutex> lck(m_mutex);
    m_frame_input_queue.push(frame);

    m_env->threadpool()->add_task(m_task);
}

// handle_task可由具体的派生类实现，但是只需要调用Media_source
void Media_source::task_callback(void *arg) {
    Media_source *source = (Media_source*) arg;
    source->handle_task();
}