#include <assert.h>
#include <functional>
#include <google/protobuf/stubs/callback.h>
#include "base/logger.h"
#include "rpc_server.h"
using namespace std::placeholders;

RpcServer::RpcServer(EventLoop* loop, const InetAddress& addr) 
    : codec_(std::bind(&RpcServer::OnRpcMessgae, this, _1, _2)),
      server_(loop, addr) {
  server_.set_connection_call_back(std::bind(&RpcServer::OnConnection, this, _1));
  server_.set_message_call_back(std::bind(&RpcServer::OnMessage, this, _1, _2));
}

void RpcServer::RegisterService(google::protobuf::Service *service) {
  services_[service->GetDescriptor()->full_name()] = service; 
}

void RpcServer::OnConnection(TcpConnectionPtr conn) {
  LOG_INFO << "RpcServer::OnConnection - connection is " << (conn->connected() ? "up" : "down"); 
}

void RpcServer::OnMessage(TcpConnectionPtr conn, Buffer *buf) {
  codec_.OnMessage(conn, buf);
}

void RpcServer::OnRpcMessgae(TcpConnectionPtr conn, RpcMessagePtr message) {
  assert(message->type() == rpc::REQUEST);
  assert(message->has_request());
  const auto& it = services_.find(message->service());
  if (it != services_.end()) {
    google::protobuf::Service *service = it->second;
    assert(service);
    const google::protobuf::ServiceDescriptor *desc = service->GetDescriptor();
    assert(desc);
    const google::protobuf::MethodDescriptor * method = desc->FindMethodByName(message->method());
    if (method) {
      std::unique_ptr<google::protobuf::Message> request(service->GetRequestPrototype(method).New());
      if (request->ParseFromString(message->request())) {
        std::unique_ptr<google::protobuf::Message> response(service->GetResponsePrototype(method).New());
        RpcInfo info = {response.get(), static_cast<int64_t>(message->id())};
        service->CallMethod(method, nullptr, request.get(), response.get(), 
                            google::protobuf::NewCallback(this, &RpcServer::OnCallBackDone, conn, &info)); // 只能传两个参数，还不能是const引用
      } else {
        LOG_ERROR << "RpcServer::OnRpcMessgae - invalid request";
      }
    } else {
      LOG_ERROR << "RpcServer::OnRpcMessgae - method not found";
    }
  } else {
    LOG_ERROR << "RpcServer::OnRpcMessgae - service not found";
  }
}

void RpcServer::OnCallBackDone(TcpConnectionPtr conn, RpcInfo *info) {
  rpc::RpcMessage message;
  message.set_type(rpc::RESPONSE);
  message.set_id(info->id);
  message.set_response(info->response->SerializeAsString());
  codec_.Send(conn, message);
}