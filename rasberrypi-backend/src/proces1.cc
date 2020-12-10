#include <iostream>
#include "transmission.hpp"
#include "net.hpp"

void runHost(IPv4& ip, int port, transmission::DataTransmitter::Mode transmissionMode) {
  TCPServer srv;
  srv.bind(ip, port);
  srv.reuseaddr();
  srv.listen();
  auto conn = srv.accept();
  transmission::DataTransmitter data_transmitter(SHM_AUDIO_TEST_NAME, std::move(conn));
  data_transmitter.setMode(transmissionMode);
  data_transmitter.transmit();
}

void runClient(IPv4& ip, int port, transmission::DataTransmitter::Mode transmissionMode) {
  auto client = connect(ip, port);
  transmission::DataTransmitter data_transmitter(SHM_AUDIO_TEST_NAME, std::move(client));
  data_transmitter.setMode(transmissionMode);
  data_transmitter.transmit();
}

int main(int argc, char*argv[]) {
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
  transmission::DataTransmitter::Mode transmissionMode;
  if (argc == 5) {
    std::string arg = argv[4];
    if (arg == "-s") {
      transmissionMode = transmission::DataTransmitter::Mode::SEND_ONLY;
    }
    else if (arg == "-r") {
      transmissionMode = transmission::DataTransmitter::Mode::RECEIVE_ONLY;
    }
  }

  if (conn_type == "-host") {
    runHost(ip, port, transmissionMode);
  }
  else if (conn_type == "-client") {
    runClient(ip, port, transmissionMode);
  }
}


