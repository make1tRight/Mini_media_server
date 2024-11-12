#include "Inet_address.h"

Inet_address::Inet_address() {}

Inet_address::Inet_address(std::string ip, uint16_t port):
    m_ip(ip), m_port(port) {
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    m_addr.sin_port = htons(port);
}

void Inet_address::set_addr(std::string ip, uint16_t port) {
    m_ip = ip;
    m_port = port;

    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    m_addr.sin_port = htons(port);
}

std::string Inet_address::get_ip() {
    return m_ip;
}

uint16_t Inet_address::get_port() {
    return m_port;
}

struct sockaddr *Inet_address::get_addr() {
    return (struct sockaddr*) &m_addr; //sockaddr_in -> sockaddr
}