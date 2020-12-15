#ifndef __NET_HPP__
#define __NET_HPP__

#include "error.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <memory>

class IPv4 {
    in_addr addr;
public:
    IPv4(const IPv4 &) = default;
    explicit IPv4(const std::string &);
    explicit IPv4(const char *);
    explicit IPv4(unsigned long int);

    const in_addr &address() const noexcept;
    operator std::string() const;
};

class IPv6 {
    in6_addr addr;
public:
    IPv6(const IPv6 &) = default;
    explicit IPv6(const char *);
    explicit IPv6(const std::string &);

    const in6_addr &address() const noexcept;
};

std::ostream &operator<<(std::ostream &os, const IPv4 &ip4);

class UDPSocket {
    int sockfd;
    sockaddr_in addr;
    sockaddr_in incoming_addr;
    sockaddr_in outgoing_addr;
public:
    UDPSocket(const IPv4 &addr, int port = 8080);
    ~UDPSocket();
    void setOutgoingAddr(const IPv4 &ip4, int port = 8080);
    std::string getOutgoingAddr() const;
    std::string getIncomingAddr() const;
    int write(const char *buf, std::size_t bufsize);
    int read(char *buf, std::size_t bufsize);
    int fd() const;
};

class TCPConnection {
private:
    int sockfd;
    struct sockaddr_in6 addr;
public:
    TCPConnection(int sockfd, const struct sockaddr_in6 &addr);
    TCPConnection(const TCPConnection &) = delete;
    ~TCPConnection();

    std::size_t read(char *buffer, std::size_t buffer_size) const;
    std::size_t write(const char *buffer, std::size_t buffer_size) const;
    void shutdown(int how = SHUT_RDWR);
    void setsockopt(int level, int optname, const void *optval, socklen_t optlen);

    std::string address() const;
    short unsigned int port() const;

    int fd() const noexcept;
};

class Connection : public TCPConnection {
private:
    enum PacketState {
      Start,
      RecvOrSendingPayload,
    } state;
    std::size_t already_sent;
    uint16_t payload_size;
    std::array<char, 0x4000> buffer;

    bool recv(std::string &, std::size_t);
    bool send(const std::string &, std::size_t &);
public:
    Connection(int sockfd, const struct sockaddr_in6 &addr);
    Connection(const Connection &) = delete;
    virtual ~Connection() {}

    bool recvPayload(std::string &);
    bool sendPayload(std::string &);

    void sendFile(std::fstream &file, std::size_t file_size);
    void recvFile(std::fstream &file, std::size_t file_size);

    bool sendfileChunk(std::fstream &file, std::size_t file_size);
    bool recvfileChunk(std::fstream &file, std::size_t file_size);
};

std::unique_ptr<Connection> connect(const IPv4 &ip, unsigned short port);
std::unique_ptr<Connection> connect(const IPv6 &ip, unsigned short port);
std::unique_ptr<Connection> connect(const std::string &host, const std::string &port);
std::unique_ptr<Connection> connect(const std::string &url);

class TCPServer {
    int sockfd;
public:
    TCPServer();
    ~TCPServer();
    void bind(const IPv4 &ip, unsigned short port);
    void bind(const IPv6 &ip, unsigned short port);
    void listen(std::size_t n = SOMAXCONN);
    void reuseaddr(bool reuse = true);

    std::unique_ptr<Connection> accept();

    int fd() const noexcept;
};

#endif
