#ifndef RPC_RPC_CODEC_H
#define RPC_RPC_CODEC_H

#include <functional>
#include "base/logger.h"
#include "protobuf_codec.h"
#include "rpc.pb.h"
using namespace std::placeholders;

typedef std::shared_ptr<rpc::RpcMessage> RpcMessagePtr;

// 在 message 和 rpc_message之间转换
class RpcCodec : private NonCopyable {
 public:
  typedef std::function<void (TcpConnectionPtr, RpcMessagePtr)> RpcMessageCallBack;
  RpcCodec(const RpcMessageCallBack& cb)
      : codec_(std::bind(&RpcCodec::OnRpcMessage, this, _1, _2)),
        message_call_back_(cb) {}
  // 解码前
  void OnMessage(TcpConnectionPtr conn, Buffer *buf) {
    codec_.OnMessage(conn, buf);
  }
  // 解码后
  void OnRpcMessage(TcpConnectionPtr conn, MessagePtr message) {
    RpcMessagePtr rpc_message = std::dynamic_pointer_cast<rpc::RpcMessage>(message);
    if (rpc_message) {
      message_call_back_(conn, rpc_message);
    } else {
      LOG_ERROR << "RpcCodec::OnMessage() - dynamic_pointer_cast error";
    }
  }
  void Send(TcpConnectionPtr conn, const rpc::RpcMessage& message) {
    codec_.Send(conn, message);
  }
 private:
  ProtobufCodec codec_;
  RpcMessageCallBack message_call_back_;
};

#endif