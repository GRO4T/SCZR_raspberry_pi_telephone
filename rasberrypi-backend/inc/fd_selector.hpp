#ifndef __FD_SELECTOR_HPP__
#define __FD_SELECTOR_HPP__

#include "net.hpp"
#include "mic.hpp"
#include "mock_mic.hpp"
#include "config.hpp"

#include <chrono>
#include <unordered_set>

#ifdef I_DONT_HAVE_HARDWARE
using MicType = MockMic;
#else
using MicType = Microphone;
#endif

class FDSelector {
    fd_set rd_set;
    fd_set wr_set;
    std::unordered_set<int> rd_fds;
    std::unordered_set<int> wr_fds;
public:
    bool add(const MicType &mic);
    bool add(const TCPServer& server);
    bool addRead(const UDPSocket &socket);
    bool addRead(const TCPConnection& conn);
    bool addWrite(const UDPSocket &socket);
    bool addWrite(const TCPConnection& conn);

    void remove(const MicType &mic);
    void remove(const TCPServer& server);
    void removeRead(const UDPSocket &socket);
    void removeRead(const TCPConnection& conn);
    void removeWrite(const UDPSocket &socket);
    void removeWrite(const TCPConnection& conn);

    bool ready(const MicType &mic);
    bool ready(const TCPServer& server) const;
    bool readyRead(const UDPSocket &socket);
    bool readyRead(const TCPConnection& conn) const;
    bool readyWrite(const UDPSocket &socket);
    bool readyWrite(const TCPConnection& conn) const;

    bool wait(std::chrono::milliseconds ms);
    void clear();
};

#endif
