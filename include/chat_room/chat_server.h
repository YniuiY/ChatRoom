/**
 * 聊天服务器
 * 1. 接收客户端的连接，记录聊天室内的客户端socket
 * 2. 收到客户端消息后，转发给聊天室内其他客户端。
 * 3. 客户端退出后，从聊天室内移除。
 * 4. 支持多客户端同时在线。
 */

#ifndef CHAT_SERVER_H_
#define CHAT_SERVER_H_

#include <common/headers.h>
#include <list>

class ChatServer {
 public:
  static constexpr int MAX_EVENTS{1024};

  ChatServer();
  explicit ChatServer(int port);
  ~ChatServer();

  void Init();
  void Start();
  void Stop();

 private:
  void run();
  void say_hello_to_client(int socket);
  void transpond_to_other_client(int socket, Packet* pack);
  bool recv_pack(int socket, Packet*& pack);

  int socket_;
  int port_;
  int epoll_fd_;
  sockaddr_in server_address_;
  epoll_event events_[MAX_EVENTS];
  std::list<int> clients_socket_list_; // 存储接入聊天室的客户端socket
  bool is_running_;
};

#endif // CHAT_SERVER_H_