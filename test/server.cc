#include "rpc.pb.h"
#include "protobuf_codec.h"
#include "base/logger.h"
#include "base/mutex.h"
#include "net/event_loop.h"
#include "net/tcp_server.h"

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <functional>

using namespace std::placeholders;

class RpcServer 
{
 public:
  RpcServer(EventLoop* loop,
              const InetAddress& listenAddr)
  : server_(loop, listenAddr),
    codec_(std::bind(&RpcServer::OnProtobufMessage, this, _1, _2))
  {
    server_.set_connection_call_back(
        std::bind(&RpcServer::onConnection, this, _1));
    server_.set_message_call_back(
        std::bind(&ProtobufCodec::OnMessage, &codec_, _1, _2));
  }

  void start()
  {
    server_.Start();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << "Connection is up";
  }

  void OnProtobufMessage(TcpConnectionPtr conn, MessagePtr message) {
    rpc::RpcMessage *rpcm = dynamic_cast<rpc::RpcMessage *>(message.get());
    std::cout << "id = " <<rpcm->id() << std::endl;
    std::cout << "method = " << rpcm->method() << std::endl;
    for (int i = 0; i < rpcm->args_size(); ++i) {
      std::cout << "args " << i << " = " << rpcm->args(i) << std::endl; 
    }

    rpcm->set_id(2);
    rpcm->set_method("for test2");
    rpcm->add_args("first args2");
    rpcm->add_args("second args2");
    codec_.Send(conn, *rpcm);
  }

  TcpServer server_;
  ProtobufCodec codec_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress serverAddr(port);
    RpcServer server(&loop, serverAddr);
    server.start();
    loop.Loop();
  }
  else
  {
    printf("Usage: %s port\n", argv[0]);
  }
}
