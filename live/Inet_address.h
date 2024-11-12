#ifndef INET_ADDRESS_H
#define INET_ADDRESS_H
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Inet_address {
public:
    Inet_address();
    Inet_address(std::string ip, uint16_t port);
    void set_addr(std::string ip, uint16_t port);

    std::string get_ip();
    uint16_t get_port();
    struct sockaddr *get_addr();

private:
    std::string m_ip;
    uint16_t m_port;
    struct sockaddr_in m_addr; //in专用于IPv4, sockaddr更通用
};
#endif