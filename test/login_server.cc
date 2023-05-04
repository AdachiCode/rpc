#include "rpc_service.pb.h"
#include "base/logger.h"
#include "net/event_loop.h"
#include "net/socket.h"
#include "rpc_server.h"
#include "net/tcp_connection.h"
#include "rpc_channel.h"

#include <unistd.h>



class LoginServiceImpl : public rpc::LoginService
{
 public:
  virtual void Login(::google::protobuf::RpcController* controller,
                       const ::rpc::LoginRequest* request,
                       ::rpc::LoginResponse* response,
                       ::google::protobuf::Closure* done)
  {
    LOG_INFO << "LoginServiceImpl::Login";
    printf("name : %s\npasswd : %s\n", request->name().c_str(), request->password().c_str());
    response->set_success(true);
    done->Run();
  }
};


int main()
{
  LOG_INFO << "pid = " << getpid();
  EventLoop loop;
  InetAddress listenAddr(9981);
  LoginServiceImpl impl;
  RpcServer server(&loop, listenAddr);
  server.RegisterService(&impl);
  server.Start();
  loop.Loop();
  google::protobuf::ShutdownProtobufLibrary();
}
