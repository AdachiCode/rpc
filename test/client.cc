#include "rpc.pb.h"
#include "protobuf_codec.h"
#include "base/logger.h"
#include "base/mutex.h"
#include "net/event_loop.h"
#include "net/tcp_client.h"

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <functional>

using namespace std::placeholders;

google::protobuf::Message* messageToSend;

class RpcClient
{
 public:
  RpcClient(EventLoop* loop,
              const InetAddress& serverAddr)
  : loop_(loop),
    client_(loop, serverAddr),
    codec_(std::bind(&RpcClient::OnProtobufMessage, this, _1, _2))
  {
    client_.set_connection_call_back(
        std::bind(&RpcClient::onConnection, this, _1));
    client_.set_message_call_back(
        std::bind(&ProtobufCodec::OnMessage, &codec_, _1, _2));
  }

  void connect()
  {
    client_.Connect();
  }

  void OnProtobufMessage(TcpConnectionPtr conn, MessagePtr message) {
    rpc::RpcMessage *rpcm = dynamic_cast<rpc::RpcMessage *>(message.get());
    std::cout << "id = " <<rpcm->id() << std::endl;
    std::cout << "method = " << rpcm->method() << std::endl;
    for (int i = 0; i < rpcm->args_size(); ++i) {
      std::cout << "args " << i << " = " << rpcm->args(i) << std::endl; 
    }
    conn->ShutDown();
  }

 private:

  void onConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << "Connection is up";

    if (conn->connected())
    {
      codec_.Send(conn, *messageToSend);
    }
    else
    {
      loop_->Quit();
    }
  }

  EventLoop* loop_;
  TcpClient client_;
  ProtobufCodec codec_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 2)
  {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress serverAddr(argv[1], port);

    rpc::RpcMessage rpc;
    rpc.set_id(1);
    rpc.set_method("for test");
    rpc.add_args("first args");
    rpc.add_args("second args");
  
    messageToSend = &rpc;

    RpcClient client(&loop, serverAddr);
    client.connect();
    loop.Loop();
  }
  else
  {
    printf("Usage: %s host_ip port [q|e]\n", argv[0]);
  }
}
