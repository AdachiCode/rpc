#include <arpa/inet.h>
#include <string>
#include "base/logger.h"
#include "protobuf_codec.h"

ProtobufCodec::ProtobufCodec(const ProtobufMessageCallBack& cb) : message_call_back_(cb) {}

void ProtobufCodec::Send(TcpConnectionPtr conn, const google::protobuf::Message& message) {
  const std::string& type_name = message.GetTypeName();
  int32_t namelen = static_cast<int32_t>(type_name.size());
  std::string protobuf;
  message.SerializeToString(&protobuf);
  int32_t len = static_cast<int32_t>(sizeof(namelen) + type_name.size() + protobuf.size());  
  // 不能用 to_string 或者 sprintf 因为是 二进制格式 而不是 文本格式
  int32_t net_bytes = htonl(len);
  std::string send_message = std::string(reinterpret_cast<char *>(&net_bytes), kHeaderLen);
  net_bytes = htonl(namelen);
  send_message += std::string(reinterpret_cast<char *>(&net_bytes), kTypenameLen);
  send_message += type_name;
  send_message += protobuf;
  assert(send_message.size() == len + kHeaderLen);
  conn->Send(std::move(send_message));
}

void ProtobufCodec::OnMessage(TcpConnectionPtr conn, Buffer *buf) {
  while (buf->readable_bytes() >= kHeaderLen) {
    int32_t len = ntohl(*reinterpret_cast<int32_t *>(buf->peek()));
    if (len < kMinMessageLen || len > kMaxMessageLen) {
      HandleError(conn, kInvalidLength);
      break;
    }
    if (buf->readable_bytes() < kHeaderLen + len) {
      break;
    } else {
      ErrorType error_type = kNoError;
      MessagePtr message = ParseMessage(buf->peek() + kHeaderLen, len, &error_type);
      if (message && error_type == kNoError) {
        buf->Retrieve(kHeaderLen + len);
        message_call_back_(conn, message);
      } else {
        HandleError(conn, error_type);
        break;
      }
    }
  }
}

void ProtobufCodec::HandleError(TcpConnectionPtr conn, ErrorType error_type) {
  LOG_ERROR << "ProtobufCodec::HandleError() - error = " << error_type;
  conn->ShutDown(); 
}

google::protobuf::Message *ProtobufCodec::CreateMessage(const std::string& type_name) {
  google::protobuf::Message* message = nullptr;
  const google::protobuf::Descriptor* descriptor = 
    google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
  if (descriptor) {
    const google::protobuf::Message* prototype =
      google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
    if (prototype) {
      message = prototype->New(); // 堆内存 用shared_ptr管理
    }
  }
  return message;
}

MessagePtr ProtobufCodec::ParseMessage(const char *buf, int len, ErrorType *error_type) {
  MessagePtr messgae;
  int32_t namelen = ntohl(*reinterpret_cast<const int32_t *>(buf));
  if (namelen < 1 || namelen > len - kTypenameLen) {
    *error_type = kInvalidNameLen;
  } else {
    std::string type_name(buf + kTypenameLen, namelen);
    messgae.reset(CreateMessage(type_name));
    if (!messgae) {
      *error_type = kUnKnownMessageType; 
    } else {
      if (!messgae->ParseFromArray(buf + kTypenameLen + namelen, len - kTypenameLen - namelen)) {
        *error_type = kParseProtobufError;
      }
    }
  }
  return messgae;
}