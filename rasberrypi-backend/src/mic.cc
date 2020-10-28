#include "mic.hpp"
#include "error.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

Microphone::Microphone(const char* path) {
    USB = ::open(path, O_RDWR | O_NOCTTY);
    if (USB < 0)
        throw BackendException();

    struct termios tty;
    struct termios tty_old;
    memset (&tty, 0, sizeof tty);

    if ( tcgetattr ( USB, &tty ) != 0 )
        throw BackendException();

    tty_old = tty;

    cfsetospeed (&tty, (speed_t)B2000000);
    cfsetispeed (&tty, (speed_t)B2000000);

    tty.c_cflag     &=  ~PARENB;            // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;

    tty.c_cflag     &=  ~CRTSCTS;           // no flow control
    tty.c_cc[VMIN]   =  1;                  // read doesn't block
    tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
    tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

    cfmakeraw(&tty);
    
    tcflush( USB, TCIFLUSH );
    if ( tcsetattr ( USB, TCSANOW, &tty ) != 0)
        throw BackendException();
}

Microphone::~Microphone() {
    ::close(USB);
}

std::size_t Microphone::read(char* buffer, std::size_t buffer_size) const {
    std::size_t already_read = 0;
    while (already_read < buffer_size) {
        ssize_t currently_read = ::read(USB, &buffer[already_read], buffer_size - already_read);
        if (currently_read < 0) 
            throw BackendException();
        else if (currently_read == 0)
            return already_read;
        already_read += currently_read;
    }
    return already_read;
}

