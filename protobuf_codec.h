#ifndef RPC_PROTOBUF_CODEC_H
#define RPC_PROTOBUF_CODEC_H

#include "net/buffer.h"
#include "net/tcp_connection.h"

// |--------------|
// |     len      |
// |--------------|
// |   name len   |
// |--------------|
// | messgae name |
// |--------------|
// |   protobuf   |
// |--------------|

class ProtobufCodec : private NonCopyable {
 public:
  typedef std::function<void (TcpConnectionPtr)> ProtobufMessageCallBack;
  explicit ProtobufCodec(const ProtobufMessageCallBack& cb);
  ~ProtobufCodec();
  void OnMessage(TcpConnectionPtr conn, Buffer *buf);
  void Send(TcpConnectionPtr conn, const google::protobuf::Message& message);
 private: 
  ProtobufMessageCallBack message_call_back_;
}

#endif