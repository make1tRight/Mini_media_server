#include "config.h"

int main() {
    srand(time(nullptr));

    Event_scheduler *scheduler = Event_scheduler::append(Event_scheduler::POLLER_SELECT);

    Threadpool *threadpool = Threadpool::append(1);

    Media_session_manager *sess_mgr = Media_session_manager::append();

    Usage_environment *env = Usage_environment::append(scheduler, threadpool);

    Inet_address rtspAddr("127.0.0.1", 8888);

    Rtsp_server *rtsp_server = Rtsp_server::append(env, sess_mgr, rtspAddr);

    LOGINFO("------------session init start------------"); {
        Media_session *session = Media_session::append("test");
        Media_source *source = H264_file_media_source::append(env, "./root/demo.h264");
        Sink *sink = H264_file_sink::append(env, source);
        session->add_sink(Media_session::Track_id_0, sink);

        source = AAC_file_media_source::append(env, "./root/demo.aac");
        sink = AAC_file_sink::append(env, source);
        session->add_sink(Media_session::Track_id_1, sink);

        sess_mgr->add_session(session);
    }
    LOGINFO("------------session init end------------");

    rtsp_server->start();
    env->scheduler()->loop();

    return 0;
}
