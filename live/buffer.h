#ifndef BUFFER_H
#define BUFFER_H
#include <algorithm>
#include <assert.h>

class Buffer {
public:
    static const int initial_size;

    explicit Buffer();
    ~Buffer();

    int readable_bytes() const { return m_write_idx - m_read_idx; }
    int writable_bytes() const { return m_buffer_size - m_write_idx; }
    int prependable_bytes() const { return m_read_idx; }
    char *peek() { return begin() + m_read_idx; }
    const char *peek() const { return begin() + m_read_idx; }


    const char *find_CRLF() const { //返回指向CRLF指针
        const char *crlf = std::search(peek(), begin_write(), k_CRLF, k_CRLF + 2);
        return crlf == begin_write() ? nullptr : crlf;
    }
    const char *find_CRLF(const char *start) const {
        // 确保指定的start有效
        assert(peek() <= start);//未读
        assert(start <= begin_write());//已写

        const char *crlf = std::search(start, begin_write(), k_CRLF, k_CRLF + 2);//指定位置开始找
        return crlf == begin_write() ? nullptr : crlf;
    }
    const char *find_last_CRLF() const { //返回指向最后一个CRLF指针
        const char *crlf = std::find_end(peek(), begin_write(), k_CRLF, k_CRLF + 2);
        return crlf == begin_write() ? nullptr : crlf;
    }


    void retrieve_read_zero() { m_read_idx = 0; }
    void retrieve(int len) {
        // 防止超过可读字节
        assert(len <= readable_bytes());
        if (len < readable_bytes()) {
            m_read_idx += len;
        } else {
            retrieve_all();
        }
    }    
    void retrieve_until(const char *end) {
        assert(peek() <= end);
        assert(end <= begin_write());
        retrieve(end - peek());
    }
    void retrieve_all() { m_read_idx = 0; m_write_idx = 0; }


    char *begin_write() { return begin() + m_write_idx; }
    const char *begin_write() const { return begin() + m_write_idx; }
    void unwrite(int len) {
        assert(len <= readable_bytes());
        m_write_idx -= len;
    }


    void ensure_writable_bytes(int len) {
        if (writable_bytes() < len) {
            make_space(len);
        }
        assert(writable_bytes() >= len);
    }

    void make_space(int len) {
        if (writable_bytes() + prependable_bytes() < len) {
            // 扩容
            m_buffer_size = m_write_idx + len;
            m_buffer = (char*) realloc(m_buffer, m_buffer_size);
        } else { //空间足够则移动未读数据到缓冲区起始位置, 相当于更新了缓冲区
            int readable = readable_bytes();
            std::copy(begin() + m_read_idx, begin() + m_write_idx, begin());
            m_read_idx = 0;
            m_write_idx = m_read_idx + readable;
            assert(readable == readable_bytes());
        }
    }

    void append(const char *data, int len) {
        ensure_writable_bytes(len);
        std::copy(data, data + len, begin_write());

        assert(len <= writable_bytes());
        m_write_idx += len;//将data加入到写入部分
    }

    void append(const void *data, int len) { append((const char *)(data), len); }
    
    int read(int fd);
    int write(int fd);

private:
    char *begin() { return m_buffer; }
    const char *begin() const { return m_buffer; }

private:
    char *m_buffer;
    int m_buffer_size;
    int m_read_idx;
    int m_write_idx; //指向写入内容末尾

    static const char *k_CRLF; //回车换行
};
#endif //BUFFER_H