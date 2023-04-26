#include <iostream>
#include <bits/stdc++.h>
#include <google/protobuf/descriptor.h>
#include "base/logger.h"
#include "rpc.pb.h"
using namespace std;

int main() {
  rpc::RpcMessage message;
  cout << message.GetTypeName() << endl;
}