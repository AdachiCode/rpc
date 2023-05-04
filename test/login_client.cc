#include "rpc_service.pb.h"

#include "base/logger.h"
#include "net/event_loop.h"
#include "net/socket.h"
#include "net/tcp_client.h"
#include "net/tcp_connection.h"
#include "rpc_channel.h"

#include <stdio.h>
#include <unistd.h>

class RpcClient : NonCopyable {
 public:
  RpcClient(EventLoop* loop, const InetAddress& serverAddr)
    : loop_(loop),
      client_(loop, serverAddr),
      channel_(new RpcChannel),
      stub_(channel_.get())
  {
    client_.set_connection_call_back(
        std::bind(&RpcClient::onConnection, this, _1));
    client_.set_message_call_back(
        std::bind(&RpcChannel::OnMessage, channel_.get(), _1, _2));
    // client_.enableRetry();
  }

  void connect()
  {
    client_.Connect();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    if (conn->connected())
    {
      //channel_.reset(new RpcChannel(conn));
      channel_->set_connection(conn);
      rpc::LoginRequest request;
      request.set_name("yuriyuri");
      request.set_password("lzetta");
      // sudoku::SudokuResponse* response = new sudoku::SudokuResponse;
      std::shared_ptr<rpc::LoginResponse> response(new rpc::LoginResponse());
      stub_.Login(NULL, &request, response.get(), NewCallback(this, &RpcClient::solved, response, conn));
    }
    else
    {
      loop_->Quit();
    }
  }

  void solved(std::shared_ptr<rpc::LoginResponse> resp, TcpConnectionPtr conn)
  {
    LOG_INFO << "solved:\n" << resp->DebugString();
    // client_.disconnect();
    conn->ShutDown();
  }

  EventLoop* loop_;
  TcpClient client_;
  std::unique_ptr<RpcChannel> channel_;
  rpc::LoginService::Stub stub_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    EventLoop loop;
    InetAddress serverAddr(argv[1], 9981);

    RpcClient rpcClient(&loop, serverAddr);
    rpcClient.connect();
    loop.Loop();
  }
  else
  {
    printf("Usage: %s host_ip\n", argv[0]);
  }
  google::protobuf::ShutdownProtobufLibrary();
}