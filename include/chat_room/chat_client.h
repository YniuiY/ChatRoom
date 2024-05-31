/**
 * 聊天客户端
 * 1. 连接聊天服务器
 * 2. 另起一个线程接收键盘输入，将键盘输入发送给聊天服务器
 * 3. 另起一个线程接收服务器转发的消息，将消息打印在用户终端。
 */

#ifndef CHAT_CLIENT_H_
#define CHAT_CLIENT_H_

#include "common/headers.h"

class ChatClient {
 public:
  static constexpr int MAX_EVENTS{1024};

  ChatClient();
  explicit ChatClient(int port);
  ChatClient(int port, std::string ip);
  ~ChatClient();

  void Init();
  void Start();
  void Stop();

 private:
  void send_keyboard_input();
  void recv_server_transpond();
  void recv_server_msg(int socket);

  int socket_;
  int server_service_port_; // 服务端提供服务的端口
  std::string server_ip_;
  sockaddr_in server_addr_;
  int epoll_fd_;
  epoll_event events_[MAX_EVENTS];
  std::string msg_;
  bool is_running_;
};


#endif // CHAT_CLIENT_H_