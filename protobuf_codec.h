#ifndef RPC_PROTOBUF_CODEC_H
#define RPC_PROTOBUF_CODEC_H

#include <memory>
#include <google/protobuf/message.h>
#include "net/buffer.h"
#include "net/tcp_connection.h"

// |--------------|
// |     len      | ---------------|   Header
// |--------------|                |
// |   name len   | ------|   }    |
// |--------------|       |   }<---|    Body 
// | messgae name |   <---|   }   
// |--------------|           }
// |   protobuf   |           }
// |--------------|

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

class ProtobufCodec : private NonCopyable {
 public:
  typedef std::function<void (TcpConnectionPtr, MessagePtr)> ProtobufMessageCallBack; // 传出参数用指针
  enum ErrorType {
    kNoError = 0,
    kInvalidLength,
    kInvalidNameLen,
    kUnKnownMessageType,
    kParseProtobufError
  };
  explicit ProtobufCodec(const ProtobufMessageCallBack& cb);
  void OnMessage(TcpConnectionPtr conn, Buffer *buf);
  void Send(TcpConnectionPtr conn, const google::protobuf::Message& message);  // 传入参数用const引用
 private: 
  static constexpr int kHeaderLen = sizeof(int32_t);
  static constexpr int kTypenameLen = sizeof(int32_t);
  static constexpr int kMinMessageLen = kTypenameLen + 1; // typename 最小长度是1  protobuf可以为空 
  static constexpr int kMaxMessageLen = 1024 * 1024;

  void HandleError(TcpConnectionPtr conn, ErrorType error_type);
  MessagePtr ParseMessage(const char *buf, int len, ErrorType *error_type);
  google::protobuf::Message *CreateMessage(const std::string& type_name);

  ProtobufMessageCallBack message_call_back_;
};

#endif