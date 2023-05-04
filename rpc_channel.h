#ifndef RPC_RPC_CHANNEL_H
#define RPC_RPC_CHANNEL_H

#include <google/protobuf/service.h>
#include "base/mutex.h"
#include "rpc_codec.h"

// rpc服务的调用方
class RpcChannel : public google::protobuf::RpcChannel {
 public:
  RpcChannel();
  ~RpcChannel();
  void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                  ::google::protobuf::RpcController* controller,
                  const ::google::protobuf::Message* request,
                  ::google::protobuf::Message* response,
                  ::google::protobuf::Closure* done) override;
  void OnMessage(TcpConnectionPtr conn, Buffer *buf);
  void OnRpcMessage(TcpConnectionPtr conn, RpcMessagePtr message);
  void set_connection(TcpConnectionPtr conn) { conn_ = conn; }
 private:
  RpcCodec codec_;
  TcpConnectionPtr conn_; // 负责发送
  Mutex mutex_;           // 如果有多个线程使用rpc服务，要上锁
  int64_t id_; // GUARDED_BY(mutex_)  在收到服务器的回应信息时根据id找到每次rpc调用的回调函数和response指针(保存返回值)
  struct RpcCallBack {
    google::protobuf::Closure *done;
    google::protobuf::Message *response;
  };
  std::unordered_map<int64_t, RpcCallBack> call_backs_; // GUARDED_BY(mutex_) 
};

#endif