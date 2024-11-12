#include "sockets_options.h"
#include "../log/log.h"

int mysocket::create_tcp_socket() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    return sockfd;
}

int mysocket::create_udp_socket() {
    int sockfd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    return sockfd;
}

bool mysocket::bind(int sockfd, std::string ip, uint16_t port) {
    struct sockaddr_in addr = {0};
    // bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);

    if (::bind(sockfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        LOGERR("::bind error,fd=%d,ip=%s,port=%d", sockfd, ip.c_str(), port);
        return false;
    }
    return true;
}

bool mysocket::listen(int sockfd, int backlog) {
    if (::listen(sockfd, backlog) < 0) {
        LOGERR("::listen error,fd=%d,backlog=%d", sockfd, backlog);
        return false;
    }
    return true;
}

int mysocket::accept(int sockfd) {
    struct sockaddr_in addr = {0};
    // bzero(&addr, sizeof(addr));
    socklen_t addr_len = sizeof(struct sockaddr_in);

    int connfd = ::accept(sockfd, (struct sockaddr*) &addr, &addr_len);
    set_nonblocking_and_close_on_exec(connfd);
    ignore_sigpipe_on_socket(connfd);

    return connfd;
}

int mysocket::write(int sockfd, const void *buf, int size) {
    return ::write(sockfd, buf, size);
}

int mysocket::sendto(int sockfd, const void *buf, int len, const struct sockaddr *addr) {
    socklen_t addr_len = sizeof(struct sockaddr);
    return ::sendto(sockfd, (char*)buf, len, 0, addr, addr_len);
}

int mysocket::set_nonblocking(int sockfd) {
    // int flags = ::fcntl(sockfd, F_GETFL, 0);
    // flags |= O_NONBLOCK;
    // int ret = ::fcntl(sockfd, F_SETFL, flags);

    // // 当进程调用exec系列函数时, 文件描述符会自动关闭防止资源泄漏
    // // exec系列函数的作用可以暂时理解成新进程替换旧进程
    // // 如果不设置, 新进程可能会继承上一次的文件描述符
    // flags = ::fcntl(sockfd, F_GETFD, 0);
    // flags |= FD_CLOEXEC;
    // ret = ::fcntl(sockfd, F_SETFD, flags);

    int flags = fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(sockfd, F_SETFL, flags);

    return 0;
}

int mysocket::set_block(int sockfd, int write_timeout) {
    // int flags = ::fcntl(sockfd, F_GETFL, 0);
    // ::fcntl(sockfd, F_SETFL, flags & (~O_NONBLOCK));
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags & (~O_NONBLOCK));

    if (write_timeout > 0) {
        struct timeval tv = {write_timeout / 1000, (write_timeout % 1000) * 1000};
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*) &tv, sizeof(tv));
    }

    return 0;
}

void mysocket::set_reuse_addr(int sockfd, int on) {
    int optval = on ? 1 : 0;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*) &optval, sizeof(optval));
}

void mysocket::set_reuse_port(int sockfd) {
// #ifdef SO_REUSEPORT
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*) &on, sizeof(on));
// #endif
}

void mysocket::set_nonblocking_and_close_on_exec(int sockfd) {
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);

    // close-on-exec
    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
}

void mysocket::ignore_sigpipe_on_socket(int sockfd) {
    int option = 1;
    // 尝试写入一个已经被另一端关闭的socket, 操作系统发送SIGPIPE信号, 默认使系统异常退出
    // 设置MSG_NOSIGNAL, 不会触发SIGPIPE, 返回一个错误, 避免对端关闭连接导致程序崩溃
    setsockopt(sockfd, SOL_SOCKET, MSG_NOSIGNAL, &option, sizeof(option));
}

void mysocket::set_no_delay(int sockfd) {
#ifdef TCP_NODELAY
    int on = 1;
    int ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &on, sizeof(on));
#endif
}

void mysocket::set_keep_alive(int sockfd) {
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char*) &on, sizeof(on));
}

void mysocket::set_no_sigpipe(int sockfd) {
#ifdef SO_NOSIGPIPE
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (char*) &on, sizeof(on));
    // setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, (char*) &on, sizeof(on));
#endif
}

void mysocket::set_sendbuf_size(int sockfd, int size) {
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &size, sizeof(size));
}

void mysocket::set_recvbuf_size(int sockfd, int size) {
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &size, sizeof(size));
}

// 获取对端ip
std::string mysocket::get_peer_ip(int sockfd) {
    struct sockaddr_in addr = {0};
    // bzero(&addr, sizeof(addr));

    socklen_t addr_len = sizeof(struct sockaddr_in);

    // 如果成功获取到对端地址信息
    if (getpeername(sockfd, (struct sockaddr*) &addr, &addr_len) == 0) {
        return inet_ntoa(addr.sin_addr); //network to address
    }

    return "0.0.0.0";
}

int16_t mysocket::get_peer_port(int sockfd) {
    struct sockaddr_in addr = {0};
    // bzero(&addr, sizeof(addr));

    socklen_t addr_len = sizeof(struct sockaddr_in);
    if (getpeername(sockfd, (struct sockaddr*) &addr, &addr_len) == 0) {
        return ntohs(addr.sin_port);    //network to host short
    }

    return 0;
}

int mysocket::get_peer_addr(int sockfd, struct sockaddr_in *addr) {
    socklen_t addr_len = sizeof(struct sockaddr_in);
    return getpeername(sockfd, (struct sockaddr*) &addr, &addr_len);
    // return getpeername(sockfd, (struct sockaddr*) addr, &addr_len);
}

void mysocket::close(int sockfd) {
    int ret = ::close(sockfd);
}

bool mysocket::connect(int sockfd, std::string ip, uint16_t port, int timeout) {
    bool is_connected = true;

    if (timeout > 0) { //这里不明白为什么阻塞模式的设置和timeout有关
        mysocket::set_nonblocking(sockfd);
    }

    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);
    // bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (::connect(sockfd, (struct sockaddr*) &addr, addr_len) < 0) {
        if (timeout > 0) {
            is_connected = false;
            fd_set fd_write;
            FD_ZERO(&fd_write);
            FD_SET(sockfd, &fd_write);

            struct timeval tv = {timeout / 1000, timeout % 1000 * 1000};
            select(sockfd + 1, nullptr, &fd_write, nullptr, &tv);
            if (FD_ISSET(sockfd, &fd_write)) {
                is_connected = true;
            }
            mysocket::set_block(sockfd, 0);
        } else {
            is_connected = false;
        }
    }

    return is_connected;
}

std::string mysocket::get_local_ip() {
    return "0.0.0.0";
}