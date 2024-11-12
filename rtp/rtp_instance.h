#ifndef RTP_INSTANCE_H
#define RTP_INSTANCE_H
#include <stdint.h>
#include "rtp.h"
#include "../live/Inet_address.h"
#include "../rtsp/sockets_options.h"


class Rtp_instance {
public:
    enum Rtp_type {
        RTP_OVER_UDP,
        RTP_OVER_TCP
    };

    static Rtp_instance *append_over_udp(int local_sockfd, uint16_t local_port,
                                        std::string dest_ip, uint16_t dest_port) {
        return new Rtp_instance(local_sockfd, local_port, dest_ip, dest_port);
    }

    static Rtp_instance *append_over_tcp(int sockfd, uint8_t rtp_channel) {
        return new Rtp_instance(sockfd, rtp_channel);
    }

    ~Rtp_instance() {
        mysocket::close(m_sockfd);
    }

    uint16_t get_local_port() const { return m_local_port; }
    uint16_t get_peer_port() { return m_dest_addr.get_port(); }
    
    // RTP通常是单向传输协议
    int send(Rtp_packet *rtp_packet) {
        switch (m_rtp_type) {
            case Rtp_instance::RTP_OVER_UDP: {
                return send_over_udp(rtp_packet->m_buf4, rtp_packet->m_size);
                break;
            }

            case Rtp_instance::RTP_OVER_TCP: {
                /* 以下内容是发送到TCP报文的数据部分, 而不是TCP首部 */
                // $指示这是一个RTP数据包
                rtp_packet->m_buf[0] = '$';
                // 储存RTP通道编号
                rtp_packet->m_buf[1] = (uint8_t)m_rtp_channel;
                // 储存RTP数据包（23这2个字节）
                rtp_packet->m_buf[2] = (uint8_t)(((rtp_packet->m_size) & 0xFF00) >> 8);
                rtp_packet->m_buf[3] = (uint8_t)((rtp_packet->m_size) & 0xFF);

                return send_over_tcp(rtp_packet->m_buf, 4 + rtp_packet->m_size);
                break;
            }

            default: {
                return -1;
                break;
            }
        }
    }

    bool alive() const { return m_is_alive; }
    int set_alive(bool alive) { m_is_alive = alive; return 0; }
    void set_session_id(uint16_t session_id) { m_session_id = session_id; }
    uint16_t get_session_id() const { return m_session_id; }

public:
    Rtp_instance(int local_sock_fd, uint16_t local_port, 
                const std::string &dest_ip, uint16_t dest_port):
        m_rtp_type(RTP_OVER_UDP),
        m_sockfd(local_sock_fd), m_local_port(local_port),
        m_dest_addr(dest_ip, dest_port),
        m_is_alive(false),
        m_session_id(0),
        m_rtp_channel(0) {
    }

    Rtp_instance(int sockfd, uint8_t rtp_channel):
        m_rtp_type(RTP_OVER_TCP),
        m_sockfd(sockfd), m_local_port(0),
        m_is_alive(false),
        m_session_id(0),
        m_rtp_channel(rtp_channel) {
    }

private:
    int send_over_udp(void *buf, int size) {
        // sendto适用于udp, 可以为每条消息指定不同目标地址
        return mysocket::sendto(m_sockfd, buf, size, m_dest_addr.get_addr());
    }

    int send_over_tcp(void *buf, int size) {
        // write适用于tcp, 因为已经建立了连接, 所以不需要指定地址
        return mysocket::write(m_sockfd, buf, size);
    }

private:
    Rtp_type m_rtp_type;
    int m_sockfd;
    uint16_t m_local_port;  //udp
    Inet_address m_dest_addr;   //udp
    bool m_is_alive;
    uint16_t m_session_id;
    uint8_t m_rtp_channel;
};

class Rtcp_instance {
public:
    static Rtcp_instance *append(int local_sockfd, uint16_t local_port,
                                std::string dest_ip, uint16_t dest_port) {
        return new Rtcp_instance(local_sockfd, local_port, dest_ip, dest_port);
    }

    int send(void *buf, int size) {
        return mysocket::sendto(m_local_sockfd, buf, size, m_dest_addr.get_addr());
    }

    int recv(void *buf, int size, Inet_address *addr) {
        // socklen_t addr_len = sizeof(struct sockaddr);
        // int recv_bytes = recvfrom(m_local_sockfd, buf, size, 0, addr->get_addr(), &addr_len);

        // if (recv_bytes < 0) {
        //     return -1;
        // }
        // return recv_bytes;
        return 0;
    }

    uint16_t get_local_port() const { return m_local_port; }
    int alive() const { return m_is_alive; }
    int set_alive(bool alive) { m_is_alive = alive; return 0; }
    void set_session_id(uint16_t session_id) { m_session_id = session_id; }
    uint16_t get_session_id() const { return m_session_id; }

public:
    Rtcp_instance(int local_sockfd, uint16_t local_port, 
                std::string dest_ip, uint16_t dest_port):
     m_local_sockfd(local_sockfd), 
     m_local_port(local_port), 
     m_dest_addr(dest_ip, dest_port),
     m_is_alive(false),
     m_session_id(0) {
    }

    ~Rtcp_instance() {
        mysocket::close(m_local_sockfd);
    }

private:
    int m_local_sockfd;
    uint16_t m_local_port;
    Inet_address m_dest_addr;
    bool m_is_alive;
    uint16_t m_session_id;
};
#endif // RTP_INSTANCE_H