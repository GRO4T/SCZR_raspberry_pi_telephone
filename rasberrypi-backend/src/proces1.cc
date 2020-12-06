#include <iostream>
#include "transmission.hpp"
#include "net.hpp"


int main(int argc, char*argv[]) {
  enum class Mode {
      SEND_RECEIVE,
      RECEIVE_ONLY,
      SEND_ONLY
  } mode = Mode::SEND_RECEIVE;

  if (argc < 2) {
    std::cout << "Usage: ./proces1 -host|-client ip port [-s|-r]" << std::endl;
    return 1;
  }

  std::string conn_type = argv[1];

  IPv4 ip("127.0.0.1");
  int port = 8081;
  if (argc >= 4) {
    ip = IPv4(argv[2]);
    port = std::stoi(argv[3]);
  }

  if (argc == 5) {
    std::string arg = argv[4];
    if (arg == "-s") {
      mode = Mode::SEND_ONLY;
    }
    else if (arg == "-r") {
      mode = Mode::RECEIVE_ONLY;
    }
  }

  if (conn_type == "-host") {
    TCPServer srv;
    srv.bind(ip, port);
    srv.reuseaddr();
    srv.listen();
    auto conn = srv.accept();
    transmission::DataTransmitter data_transmitter(SHM_AUDIO_TEST_NAME, std::move(conn));
    if (mode == Mode::SEND_ONLY) {
      data_transmitter.sendOnly();
    }
    else if (mode == Mode::RECEIVE_ONLY) {
      data_transmitter.receiveOnly();
    }
    data_transmitter.transmit();
  }
  else if (conn_type == "-client") {
    auto client = connect(ip, port);
    transmission::DataTransmitter data_transmitter(SHM_AUDIO_TEST_NAME, std::move(client));
    if (mode == Mode::SEND_ONLY) {
      data_transmitter.sendOnly();
    }
    else if (mode == Mode::RECEIVE_ONLY) {
      data_transmitter.receiveOnly();
    }
    data_transmitter.transmit();
  }


}
