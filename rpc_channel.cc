#include <google/protobuf/stubs/callback.h>
#include "rpc_channel.h"
using namespace std::placeholders;

RpcChannel::RpcChannel(bool block)
    : codec_(std::bind(&RpcChannel::OnRpcMessage, this, _1, _2)),
      id_(1),
      block_(block),
      cond_(block_mutex_),
      wakeup_id_(0) {}

RpcChannel::~RpcChannel() {
  LOG_INFO << "RpcChannel::destructor";
  for (const auto& it: call_backs_) {
    delete it.second.done; // 未调用Run函数，需要手动析构
  }
}

void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
                            ::google::protobuf::RpcController* controller,
                            const ::google::protobuf::Message* request,
                            ::google::protobuf::Message* response,
                            ::google::protobuf::Closure* done) {
  assert(conn_);
  rpc::RpcMessage message;
  message.set_type(rpc::REQUEST);
  message.set_service(method->service()->full_name()); 
  message.set_method(method->name());
  message.set_request(request->SerializeAsString());
  int64_t id = 0;
  {
    MutexGuard lock(mutex_);
    id = id_++;
    message.set_id(id);
    RpcCallBack call_back;
    call_back.done = done;
    call_back.response = response;
    call_backs_[id] = call_back;
  }
  codec_.Send(conn_, message);

  if (block_) {
    MutexGuard lock(block_mutex_);
    while (wakeup_id_ != id) {
      cond_.Wait();
    }
    wakeup_id_ = 0;
  }
}

void RpcChannel::OnMessage(TcpConnectionPtr conn, Buffer *buf) {
  codec_.OnMessage(conn, buf);
}

void RpcChannel::OnRpcMessage(TcpConnectionPtr conn, RpcMessagePtr message) {
  assert(conn_ == conn);  
  assert(message->type() == rpc::RESPONSE);
  assert(message->has_response());
  int64_t id = message->id();
  google::protobuf::Closure *done = nullptr;
  google::protobuf::Message *response = nullptr;
  {
    MutexGuard lock(mutex_);
    const auto& it = call_backs_.find(id);
    if (it != call_backs_.end()) {
      done = it->second.done;
      response = it->second.response;
      call_backs_.erase(it);
    } else {
      LOG_ERROR << "RpcChannel::OnRpcMessgae - id not found";
    }
  }
  if (response) {
    if (!response->ParseFromString(message->response())) {
      LOG_ERROR << "RpcChannel::OnRpcMessgae - invalid response"; 
    }
    if (done) {
      done->Run(); // 执行完回调函数后 会销毁该Closure对象
    }
  } 

  if (block_) {
    MutexGuard lock(block_mutex_);
    wakeup_id_ = id;
    cond_.Broadcast();
  }
}