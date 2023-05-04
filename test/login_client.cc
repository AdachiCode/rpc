#include "rpc_service.pb.h"
#include "base/thread.h"
#include "base/condition.h"
#include "base/logger.h"
#include "net/event_loop.h"
#include "net/socket.h"
#include "net/tcp_client.h"
#include "net/tcp_connection.h"
#include "rpc_channel.h"

#include <stdio.h>
#include <unistd.h>

class LoginClient : NonCopyable {
 public:
  LoginClient(EventLoop* loop, const InetAddress& serverAddr, bool block)
    : loop_(loop),
      client_(loop, serverAddr),
      channel_(new RpcChannel(block)),
      stub_(channel_.get()),
      connected_(false),
      cond_(mutex_)
  {
    client_.set_connection_call_back(
        std::bind(&LoginClient::onConnection, this, _1));
    client_.set_message_call_back(
        std::bind(&RpcChannel::OnMessage, channel_.get(), _1, _2));
    // client_.enableRetry();
  }

  void connect()
  {
    client_.Connect();
  }

  void ThreadFunc() {
    {
      MutexGuard lock(mutex_);
      while (connected_ != true) {
        cond_.Wait();
      }
      assert(connected_ == true);
    }
    rpc::LoginRequest request;
    request.set_name("yuriyuri");
    request.set_password("lzetta");
    // 如果用shared_ptr管理response，则必须要在NewCallback中保存智能指针，但在阻塞rpc中有时回调函数是不需要的，
    // 就不能用这个方法管理内存，而是用栈变量， Rpc框架(RpcChannel)中不负责管理response内存
    // std::shared_ptr<rpc::LoginResponse> response(new rpc::LoginResponse());
    // stub_.Login(NULL, &request, response.get(), NewCallback(this, &LoginClient::solved, response, conn));
    rpc::LoginResponse response;
    stub_.Login(NULL, &request, &response, NewCallback(this, &LoginClient::solved, &response));
    std::cout << "Login finished\n";
    std::cout << response.DebugString();
    sleep(10);
    // client_.ShutDown();
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    if (conn->connected())
    {
      channel_->set_connection(conn);

      MutexGuard lock(mutex_);
      connected_ = true;
      cond_.Broadcast();
    }
    else
    {
      // loop_->Quit();
    }
  }

  // void solved(std::shared_ptr<rpc::LoginResponse> resp, TcpConnectionPtr conn)
  // {
  //   LOG_INFO << "solved:\n" << resp->DebugString();
  //   // client_.disconnect();
  //   conn->ShutDown();
  // }
  void solved(rpc::LoginResponse *resp)
  {
    std::cout << "solved:\n" << resp->DebugString();
    // client_.disconnect();
    // conn->ShutDown();
  }
  


  EventLoop* loop_;
  TcpClient client_;
  std::unique_ptr<RpcChannel> channel_;
  rpc::LoginService::Stub stub_;

  bool connected_;
  Mutex mutex_;
  Condition cond_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  if (argc > 1)
  {
    EventLoop loop;
    InetAddress serverAddr(argv[1], 9981);

    LoginClient loginClient(&loop, serverAddr, true);
    Thread t1(std::bind(&LoginClient::ThreadFunc, &loginClient), "test");
    t1.Start();
    Thread t2(std::bind(&LoginClient::ThreadFunc, &loginClient), "test");
    t2.Start();
    loginClient.connect();
    loop.Loop();
  }
  else
  {
    printf("Usage: %s host_ip\n", argv[0]);
  }
  google::protobuf::ShutdownProtobufLibrary();
}