#include <iostream>
#include <sys/wait.h>
#include <functional>

#include "transmission.hpp"


using namespace transmission;

struct Client {
  Client(const IPv4 &src, const IPv4 &dest, int src_port, int dest_port)
      : src(src), dest(dest), src_port(src_port), dest_port(dest_port) {}
  IPv4 src;
  IPv4 dest;
  int src_port;
  int dest_port;
};

void runClient(const Client& client, std::string msg) {
  int bufsize = 1024;
  char buf[bufsize];
  UDPSocket socket(client.src, client.src_port);
  socket.setOutgoingAddr(client.dest, client.dest_port);
  FDSelector fd_selector;
  fd_selector.addRead(socket);
  socket.send(msg.c_str(), msg.length());
  while (true) {
    fd_selector.wait(std::chrono::milliseconds(10));
    if (fd_selector.readyRead(socket)) {
      memset(buf, 0, bufsize);
      socket.receive(buf, bufsize);
      std::cout << buf << " received from " << socket.getIncomingAddr() << std::endl;
      socket.send(msg.c_str(), msg.length());
      sleep(1);
    }
  }
}

int forkAndExecute(std::function<void(const Client&, std::string)> func, const Client& client, std::string msg) {
  pid_t child_pid = fork();
  if (child_pid < 0) {
    throw std::runtime_error("Fork failed.");
  }
  else if (child_pid == 0) {
    func(client, msg);
  }
  return child_pid;
}

int main() {
  Client client1(IPv4("127.0.0.1"), IPv4("127.0.0.1"), 8081, 8082);
  Client client2(IPv4("127.0.0.1"), IPv4("127.0.0.1"), 8082, 8081);
  int pid = forkAndExecute(runClient, client1, "PING");
  if (pid > 0)
    pid = forkAndExecute(runClient, client2, "PONG");
  if (pid > 0) {
    wait(NULL);
  }
}