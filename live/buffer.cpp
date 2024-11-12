#include "buffer.h"
#include "../rtsp/sockets_options.h"
const int Buffer::initial_size = 1024;
const char *Buffer::k_CRLF = "\r\n";
// const int MAX_EXTRA_BUF = 65536;

Buffer::Buffer():
    m_buffer_size(initial_size),
    m_read_idx(0),
    m_write_idx(0)  {
    m_buffer = (char*)malloc(m_buffer_size);
}

Buffer::~Buffer() {
    free(m_buffer);
}

int Buffer::read(int fd) {
    char extra_buf[65536];//到extra_buf里面解析
    const int writable = writable_bytes();
    const int n = ::recv(fd, extra_buf, sizeof(extra_buf), 0);
    // assert(n > 0);
    if (n <= 0) {
        printf("%s", errno);
        return -1;
    } else if (n <= writable) {
        std::copy(extra_buf, extra_buf + n, begin_write());
        m_write_idx += n;
    } else { //最多拷贝writable
        std::copy(extra_buf, extra_buf + writable, begin_write());
        m_write_idx += writable;
        append(extra_buf + writable, n - writable);
    }

    return n;
}

int Buffer::write(int fd) {
    return mysocket::write(fd, peek(), readable_bytes());
}