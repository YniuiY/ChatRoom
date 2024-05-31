/**
 * 聊天客户端
 * 1. 连接聊天服务器
 * 2. 另起一个线程接收键盘输入，将键盘输入发送给聊天服务器
 * 3. 另起一个线程接收服务器转发的消息，将消息打印在用户终端。
 */

#include "chat_room/chat_client.h"

ChatClient::ChatClient()
    : server_service_port_{61016},
      server_ip_{"127.0.0.0"},
      is_running_{false},
      epoll_fd_{-1},
      socket_{-1} {}

ChatClient::ChatClient(int port, std::string ip)
    : server_service_port_{port},
      server_ip_{ip},
      is_running_{false},
      epoll_fd_{-1},
      socket_{-1} {}

ChatClient::~ChatClient() {}

void ChatClient::Init() {
  memset(&server_addr_, 0, sizeof(server_addr_));
  server_addr_.sin_family = AF_INET;
  server_addr_.sin_port = server_service_port_;
  inet_pton(AF_INET, server_ip_.c_str(), &server_addr_.sin_addr);

  socket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_ == -1) {
    std::cout << "socket create failed\n";
    exit(1);
  } else {
    std::cout << "socket create success\n";
  }

  epoll_fd_ = epoll_create(6);
  if (epoll_fd_ <= 0) {
    std::cout << "epoll create failed\n";
    exit(1);
  } else {
    std::cout << "epoll create success\n";
  }

  epoll_event event;
  event.data.fd = socket_;
  event.events = EPOLLIN | EPOLLET; // 等待可读
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_, &event) == -1) {
    std::cout << "epoll ctl add socket failed\n";
    exit(1);
  }
}

void ChatClient::Start() {
  is_running_ = true;
  // 连接服务端
  if (connect(socket_, (sockaddr*)&server_addr_, sizeof(server_addr_)) == -1) {
    std::cout << "connect to server failed, errno: " << errno << ", errstr: " << strerror(errno) << std::endl;
    exit(1);
  }
  std::cout << "connect to server success\n";

  // 启动一个线程在epoll 的描述符可读时，接收服务端消息打印在屏幕
  std::thread(&ChatClient::recv_server_transpond, this).detach();

  // 启动一个线程从键盘接收输入，发送给服务端
  std::thread(&ChatClient::send_keyboard_input, this).join();
}

void ChatClient::Stop() {
  is_running_ = false;
  close(socket_);
  close(epoll_fd_);
}

void ChatClient::recv_server_transpond() {
  int triggered_socket_num = 0;
  while (is_running_) {
    triggered_socket_num = epoll_wait(epoll_fd_, events_, MAX_EVENTS, -1);
    if (triggered_socket_num == -1) {
      std::cout << "epoll wait failed\n";
      exit(1);
    }

    int triggered_socket = 0;
    for (int i = 0; i < triggered_socket_num; ++i) {
      triggered_socket = events_[i].data.fd;
      if (triggered_socket == socket_ && events_[i].events & EPOLLIN) {
        // 收到server的消息
        recv_server_msg(triggered_socket);
      } else {
        std::cout << "triggered socket event error\n";
        exit(1);
      }
    }
  }
}

void ChatClient::recv_server_msg(int socket) {
  Packet* pack = new Packet();
  memset(pack, 0, sizeof(pack->header));

  int recv_byte = recvn(socket, &pack->header, sizeof(pack->header), MSG_PEEK);
  if (recv_byte != sizeof(pack->header)) {
    if (recv_byte == 0) {
      std::cout << "Server Closed\n";
      exit(1);
    }
    std::cout << "recv pack header byte error\n";
    exit(1);
  } else {
    std::cout << "recv pack header, data size: " << pack->header.data_size << std::endl;
  }
  int pack_size = pack->header.data_size + sizeof(pack->header);
  pack = (Packet*)realloc(pack, pack_size);

  recv_byte = recvn(socket, pack, pack_size);
  if (recv_byte != pack_size) {
    std::cout << "recv pack byte error\n";
    exit(1);
  }

  std::string recv_msg{pack->data};
  std::cout << "\n\n#########################################\n";
  std::cout << recv_msg << std::endl;
  std::cout << "#########################################\n\n";
}

void ChatClient::send_keyboard_input() {
  std::string input_msg;
  Packet* pack = new Packet();
  memset(pack, 0, sizeof(Packet));

  while (is_running_) {
    std::getline(std::cin, input_msg);
    std::cout << "keyboad input msg: " << input_msg << std::endl;

    pack->header.data_size = input_msg.size();
    int pack_size = sizeof(pack->header) + pack->header.data_size;

    pack = (Packet*)realloc(pack, pack_size);
    if (pack == nullptr) {
      std::cout << "realloc pack failed\n";
      exit(1);
    }
    std::cout << "pack size: " << pack_size << std::endl;
    strcpy(pack->data, input_msg.c_str());

    int send_byte = sendn(socket_, pack, pack_size, 0);
    if (send_byte != pack_size) {
      std::cout << "send byte error\n";
      exit(1);
    } else {
      std::cout << "send byte: " << send_byte << std::endl;
    }
  }

  delete pack;
  pack = nullptr;
}