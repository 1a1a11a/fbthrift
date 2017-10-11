/*
 * Copyright 2017-present Facebook, Inc.
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

#include <folly/io/IOBuf.h>
#include <glog/logging.h>
#include <stdint.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>
#include <memory>
#include <string>

namespace apache {
namespace thrift {

using folly::IOBuf;

/**
 * Manages per-RPC state.  There is one of these objects for each RPC.
 *
 * TODO: RSocket currently has a dependency to this class. We may want
 * to clean up our APIs to avoid the dependency to a ResponseChannel
 * object.
 */
class ThriftRequest : public ResponseChannel::Request {
 public:
  ThriftRequest(
      std::shared_ptr<ThriftChannelIf> channel,
      std::unique_ptr<RequestRpcMetadata> metadata)
      : channel_(channel),
        seqId_(metadata->seqId),
        kind_(metadata->kind),
        active_(true),
        reqContext_(&connContext_, &header_) {
    header_.setProtocolId(static_cast<int16_t>(metadata->protocol));
    header_.setSequenceNumber(metadata->seqId);
    if (metadata->__isset.clientTimeoutMs) {
      header_.setClientTimeout(
          std::chrono::milliseconds(metadata->clientTimeoutMs));
    }
    if (metadata->__isset.queueTimeoutMs) {
      header_.setClientQueueTimeout(
          std::chrono::milliseconds(metadata->queueTimeoutMs));
    }
    if (metadata->__isset.priority) {
      header_.setCallPriority(
          static_cast<concurrency::PRIORITY>(metadata->priority));
    }
    if (metadata->__isset.otherMetadata) {
      header_.setReadHeaders(std::move(metadata->otherMetadata));
    }
    reqContext_.setMessageBeginSize(0);
    reqContext_.setMethodName(metadata->name);
    reqContext_.setProtoSeqId(metadata->seqId);
  }

  bool isActive() override {
    return active_;
  }

  void cancel() override {
    active_ = false;
  }

  bool isOneway() override {
    return kind_ == RpcKind::SINGLE_REQUEST_NO_RESPONSE ||
        kind_ == RpcKind::STREAMING_REQUEST_NO_RESPONSE;
  }

  protocol::PROTOCOL_TYPES getProtoId() {
    return static_cast<protocol::PROTOCOL_TYPES>(header_.getProtocolId());
  }

  Cpp2RequestContext* getRequestContext() {
    return &reqContext_;
  }

  void sendReply(
      std::unique_ptr<folly::IOBuf>&& buf,
      apache::thrift::MessageChannel::SendCallback* /*cb*/ = nullptr) override {
    auto metadata = std::make_unique<ResponseRpcMetadata>();
    metadata->seqId = seqId_;
    metadata->__isset.seqId = true;
    auto header = reqContext_.getHeader();
    DCHECK(header);
    metadata->otherMetadata = header->releaseWriteHeaders();
    auto* eh = header->getExtraWriteHeaders();
    if (eh) {
      metadata->otherMetadata.insert(eh->begin(), eh->end());
    }
    // TODO: Do we have persistent headers to send server2client?
    // auto& pwh = getPersistentWriteHeaders();
    // metadata->otherMetadata.insert(pwh.begin(), pwh.end());
    // if (!metadata->otherMetadata.empty()) {
    //   metadata->__isset.otherMetadata = true;
    // }
    if (!metadata->otherMetadata.empty()) {
      metadata->__isset.otherMetadata = true;
    }
    channel_->sendThriftResponse(std::move(metadata), std::move(buf));
  }

  // TODO: Implement sendErrorWrapped
  void sendErrorWrapped(
      folly::exception_wrapper /* ex */,
      std::string /* exCode */,
      apache::thrift::MessageChannel::SendCallback* /*cb*/ = nullptr) override {
  }

  std::shared_ptr<ThriftChannelIf> getChannel() {
    return channel_;
  }

 private:
  std::shared_ptr<ThriftChannelIf> channel_;
  int32_t seqId_;
  RpcKind kind_;
  std::atomic<bool> active_;
  transport::THeader header_;
  Cpp2ConnContext connContext_;
  Cpp2RequestContext reqContext_;
};

} // namespace thrift
} // namespace apache
