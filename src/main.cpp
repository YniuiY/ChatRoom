#include <cstdint>
#include <csignal>

#include "chat_room/chat_client.h"
#include "chat_room/chat_server.h"

enum class RunningRole : std::uint8_t {
  SERVER,
  CLIENT
};

static std::string h0{"server"};
static std::string h1{"client"};
static RunningRole role;
static int port{31013};
static ChatServer server(port);
static std::string ip{"127.0.0.1"};
static ChatClient client(port, ip);

void SignalProcess(int sig) {
  switch (role) {
    case RunningRole::SERVER:
      std::cout << "\nStop Server\n";
      server.Stop();
      exit(1);
      break;
    case RunningRole::CLIENT:
      std::cout << "\nStop Client\n";
      client.Stop();
      exit(1);
      break;
    default:
      break;
  }
}

int main (int argc, char* argv[]) {
  bool is_unix_possible{false};
  switch (argc) {
    case 2:
      break;
    default:
      std::cout << "Usage: ./run server/client\n";
      exit(1);
  }

  signal(SIGINT, SignalProcess);

  if (h0.compare(argv[1]) == 0) {
    // chat room server
    role = RunningRole::SERVER;
    server.Init();
    server.Start();
    server.Stop();
  } else if (h1.compare(argv[1]) == 0) {
    // chat room client
    role = RunningRole::CLIENT;
    client.Init();
    client.Start();
    client.Stop();
  } else {
    std::cout << "Usage: ./run server/client\n";
  }
  return 0;
}