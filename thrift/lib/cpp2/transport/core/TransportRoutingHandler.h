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

#include <vector>
#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncTransport.h>
#include <wangle/acceptor/Acceptor.h>
#include <wangle/acceptor/TransportInfo.h>

namespace apache {
namespace thrift {

/*
 * This interface is used by ThriftServer to route the
 * socket to different Transports.
 */
class TransportRoutingHandler {
 public:
  TransportRoutingHandler() = default;
  virtual ~TransportRoutingHandler() = default;
  TransportRoutingHandler(const TransportRoutingHandler&) = delete;

  virtual bool canAcceptConnection(const std::vector<uint8_t>& bytes) = 0;
  virtual void handleConnection(
    folly::AsyncTransportWrapper::UniquePtr sock,
    folly::SocketAddress* peerAddress,
    wangle::TransportInfo const& tinfo,
    wangle::Acceptor* acceptor) = 0;
};

} // namspace thrift
} // namespace apache
