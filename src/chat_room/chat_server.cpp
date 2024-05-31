/**
 * 聊天服务器
 * 1. 接收客户端的连接，记录聊天室内的客户端socket
 * 2. 收到客户端消息后，转发给聊天室内其他客户端。
 * 3. 客户端退出后，从聊天室内移除。
 * 4. 支持多客户端同时在线。
 */

#include "chat_room/chat_server.h"

#include <thread>

ChatServer::ChatServer() :port_{61016}, is_running_{false} {}

ChatServer::ChatServer(int port) :port_{port}, is_running_{false} {}

ChatServer::~ChatServer() {
  close(socket_);
  close(epoll_fd_);
  for (auto client_socket : clients_socket_list_) {
    close(client_socket);
  }
}

void ChatServer::Init() {
  memset(&server_address_, 0, sizeof(server_address_));
  server_address_.sin_family = AF_INET;
  server_address_.sin_port = port_;
  server_address_.sin_addr.s_addr = INADDR_ANY;

  socket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_ == -1) {
    std::cout << "soekct create failed\n";
  } else {
    std::cout << "socket create success, socket: " << socket_ << "\n";
  }
  bind(socket_, (sockaddr*)&(server_address_), sizeof(server_address_));
  if (listen(socket_, SOMAXCONN) == -1) {
    std::cout << "socket listen failed\n";
  } else {
    std::cout << "socket listen success\n";
  }

  epoll_fd_ = epoll_create(6);
  if (epoll_fd_ < 0) {
    std::cout << "epoll create failed\n";
  } else {
    std::cout << "epoll create success\n";
  }

  epoll_event event;
  event.data.fd = socket_;
  event.events = EPOLLIN | EPOLLET;

  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket_, &event) != 0) {
    std::cout << "Add socket to epoll event failed\n";
  }
}

void ChatServer::Start() {
  is_running_ = true;
  std::thread(&ChatServer::run, this).join();
}

void ChatServer::Stop() {
  is_running_ = false;
  close(socket_);
  close(epoll_fd_);
  for (auto client_socket : clients_socket_list_) {
    close(client_socket);
  }
}

void ChatServer::run() {
  while (is_running_) {
    int triggered_socket_num = epoll_wait(epoll_fd_, events_, MAX_EVENTS, -1);
    if (triggered_socket_num == -1) {
      std::cout << "epoll wait failed\n";
      continue;
    }
    std::cout << "epoll wait success, triggered socket num: " << triggered_socket_num << "\n";

    for (int i = 0; i < triggered_socket_num; ++i) {
      int triggered_socket = events_[i].data.fd;
      
      if (triggered_socket == socket_) {
        // 新客户端接入请求
        sockaddr_in client_sockaddr;
        bzero(&client_sockaddr, sizeof(client_sockaddr));
        uint32_t addr_len{sizeof(client_sockaddr)};

        int client_socket = accept(triggered_socket, (sockaddr*)&(client_sockaddr), &addr_len);
        if (client_socket == -1) {
          std::cerr << "accept client connect failed errno: " << errno << " errstr: " << strerror(errno) << "\n";
          exit(1);
        }

        // 存储接入的客户端
        clients_socket_list_.emplace_back(client_socket);
        std::cout << "Accept client in chat room\n";

        // epoll加入对新客户端socket的监听
        epoll_event event;
        event.data.fd = client_socket;
        event.events = EPOLLIN | EPOLLET;

        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_socket, &event) != 0) {
          std::cout << "epoll add client socket failed\n";
          exit(0);
        }

        // 向新客户端发送欢迎消息
        say_hello_to_client(client_socket);
      } else {
        // 接收客户端发送的消息
        Packet* pack = new Packet();
        recv_pack(triggered_socket, pack);

        // 转发给聊天室内其他客户端
        for (auto client_socket : clients_socket_list_) {
          if (client_socket != triggered_socket) {
            transpond_to_other_client(client_socket, pack);
          } else {
            std::cout << "ignore self message\n";
          }
        }

        // 释放pack内存
        delete pack;
        pack = nullptr;
      }
    }
  }
}

void ChatServer::say_hello_to_client(int socket) {
  Packet* pack = new Packet();
  std::string hello_client{"Hello Client Welcome Join"};
  int pack_size = sizeof(pack->header) + hello_client.size();

  pack->header.data_size = hello_client.size();
  pack = (Packet*)realloc(pack, pack_size);
  strcpy(pack->data, hello_client.c_str());

  int send_byte = sendn(socket, pack, pack_size, 0);
  if (send_byte != pack_size) {
    std::cout << "send hello to client failed\n";
  } else {
    std::cout << "Server say hello to client\n";
  }

  delete pack;
  pack = nullptr;
}

void ChatServer::recv_pack(int socket, Packet*& pack) {
  memset(pack, 0, sizeof(pack));

  int recv_byte = recvn(socket, &pack->header, sizeof(pack->header), MSG_PEEK);
  if (recv_byte != sizeof(pack->header)) {
    std::cout << "Peek pack header failed\n";
  } else {
    std::cout << "server peek pack header, data size: " << pack->header.data_size << std::endl;
  }
  int pack_size = pack->header.data_size + sizeof(pack->header);

  pack = (Packet*)realloc(pack, pack_size);
  recv_byte = recvn(socket, pack, pack_size, 0);
  if (recv_byte != pack_size) {
    std::cout << "Rcev pack failed\n";
  } else {
    std::string msg{pack->data};
    std::cout << "server recv client msg: " << msg << std::endl;
  }
}

void ChatServer::transpond_to_other_client(int socket, Packet* pack) {
  int pack_size = sizeof(pack->header) + pack->header.data_size;
  int send_byte = sendn(socket, (void*)pack, pack_size, 0);
  if (send_byte != pack_size) {
    std::cout << "transpond to other client send failed\n";
    exit(1);
  } else {
    std::string msg{pack->data};
    std::cout << "transpond to other client send success, msg: " << msg << std::endl;
  }
}