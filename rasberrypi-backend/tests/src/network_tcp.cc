#include <iostream>
#include <sys/wait.h>
#include <functional>
#include <unistd.h>

#include "net.hpp"
#include "fd_selector.hpp"

void runClient(const IPv4& ip, int port, std::string msg) {
  auto client = connect(ip, port);
  FDSelector fd_selector;
  fd_selector.addRead(*client);
  client->sendPayload(msg);
  while (true) {
    fd_selector.wait(std::chrono::milliseconds(10));
    if (fd_selector.readyRead(*client)) {
      std::string buffer;
      client->recvPayload(buffer);
      std::cout << "Client received: " << buffer << std::endl;
      client->sendPayload(msg);
      sleep(1);
    }
  }
}

void runServer(const IPv4& ip, int port, std::string msg) {
  TCPServer srv;
  srv.bind(ip, port);
  srv.listen();
  auto conn = srv.accept();
  FDSelector fd_selector;
  fd_selector.addRead(*conn);
  conn->sendPayload(msg);
  while (true) {
    fd_selector.wait(std::chrono::milliseconds(10));
    if (fd_selector.readyRead(*conn)) {
      std::string buffer;
      conn->recvPayload(buffer);
      std::cout << "Server received: " << buffer << std::endl;
      conn->sendPayload(msg);
    }
  }
}

int forkAndExecute(std::function<void(const IPv4&, int, std::string)> func, const IPv4& ip, int port, std::string msg) {
  pid_t child_pid = fork();
  if (child_pid < 0) {
    throw std::runtime_error("Fork failed.");
  } else if (child_pid == 0) {
    func(ip, port, msg);
  }
  return child_pid;
}

int main(int argc, char *argv[]) {
  const IPv4& ip = IPv4("127.0.0.1");
  int port = 8081;
  int pid = forkAndExecute(runServer, ip, port, "Hello it's a server (PING)");
  if (pid > 0)
    pid = forkAndExecute(runClient, ip, port, "Hello it's a client (PONG)");
  if (pid > 0) {
    wait(NULL);
  }
}