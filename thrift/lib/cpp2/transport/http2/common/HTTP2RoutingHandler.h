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

#pragma once

#include <thrift/lib/cpp2/transport/core/TransportRoutingHandler.h>

#include <proxygen/httpserver/HTTPServerAcceptor.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <proxygen/lib/http/session/SimpleController.h>

namespace apache {
namespace thrift {

/*
 * This handler is used to determine if a client is talking HTTP2 and
 * routes creates the handler to route the socket to Proxygen
 */
class HTTP2RoutingHandler : public TransportRoutingHandler {
 public:
  HTTP2RoutingHandler() = default;
  virtual ~HTTP2RoutingHandler() = default;
  HTTP2RoutingHandler(const HTTP2RoutingHandler&) = delete;

  bool canAcceptConnection(const std::vector<uint8_t>& bytes) override;
  void handleConnection(
      folly::AsyncTransportWrapper::UniquePtr sock,
      folly::SocketAddress* peerAddress,
      wangle::TransportInfo const& tinfo) override;

  void setConnectionManager(
      wangle::ConnectionManager* connectionManager) override {
    connectionManager_ = connectionManager;
  }

 private:
  wangle::ConnectionManager* connectionManager_;
  std::unique_ptr<proxygen::HTTPServerOptions> options_;
  std::unique_ptr<proxygen::SimpleController> controller_;
  std::unique_ptr<proxygen::HTTPServerAcceptor> serverAcceptor_;
};

} // namspace thrift
} // namespace apache
