#ifndef SOCKETS_OPTIONS_H
#define SOCKETS_OPTIONS_H
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>



namespace mysocket {
    int create_tcp_socket();
    int create_udp_socket();

    bool bind(int sockfd, std::string ip, uint16_t port);
    bool listen(int sockfd, int backlog);
    int accept(int sockfd);
    int write(int sockfd, const void *buf, int size);
    int sendto(int sockfd, const void *buf, int len, const struct sockaddr *addr);
    
    int set_nonblocking(int sockfd);
    int set_block(int sockfd, int write_timeout);

    void set_reuse_addr(int sockfd, int on);
    void set_reuse_port(int sockfd);

    void set_nonblocking_and_close_on_exec(int sockfd);
    void ignore_sigpipe_on_socket(int sockfd);

    void set_no_delay(int sockfd);
    void set_keep_alive(int sockfd);
    void set_no_sigpipe(int sockfd);
    void set_sendbuf_size(int sockfd, int size);
    void set_recvbuf_size(int sockfd, int size);
    std::string get_peer_ip(int sockfd);
    int16_t get_peer_port(int sockfd);
    int get_peer_addr(int sockfd, struct sockaddr_in *addr);
    void close(int sockfd);
    bool connect(int sockfd, std::string ip, uint16_t port, int timeout);
    std::string get_local_ip();
}
#endif //SOCKETS_OPTIONS_H