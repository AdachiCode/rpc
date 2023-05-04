#!/bin/bash

set -e

protoc rpc.proto --cpp_out=./
protoc rpc_service.proto --cpp_out=./

rm -rf build
mkdir build
cd build
cmake .. && make 