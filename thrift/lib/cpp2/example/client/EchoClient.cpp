/*
 * Copyright 2004-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/example/if/gen-cpp2/Echo.h>

int main(int argc, char *argv[]) {
  FLAGS_logtostderr = 1;
  folly::init(&argc, &argv);
  folly::EventBase eventBase;

  try {
    // Create a client to the service
    auto socket = apache::thrift::async::TAsyncSocket::newSocket(
        &eventBase, "::1", 7778);
    auto connection = apache::thrift::HeaderClientChannel::newChannel(socket);
    auto client = std::make_unique<
        tutorials::chatroom::EchoAsyncClient>(std::move(connection));

    // Get an echo'd message
    std::string message = "Ping this back";
    std::string response;
    client->sync_echo(response, message);
    LOG(INFO) << response;
  } catch (apache::thrift::transport::TTransportException& ex) {
    LOG(ERROR) << "Request failed " << ex.what();
  }

  return 0;
}
