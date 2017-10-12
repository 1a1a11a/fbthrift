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
#include "thrift/lib/cpp2/transport/rsocket/client/RSClientThriftChannel.h"

#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RPCSubscriber.h>

namespace apache {
namespace thrift {

using namespace rsocket;

std::unique_ptr<folly::IOBuf> RSClientThriftChannel::serializeMetadata(
    const RequestRpcMetadata& requestMetadata) {
  CompactProtocolWriter writer;
  folly::IOBufQueue queue;
  writer.setOutput(&queue);
  requestMetadata.write(&writer);
  return queue.move();
}

std::unique_ptr<ResponseRpcMetadata> RSClientThriftChannel::deserializeMetadata(
    const folly::IOBuf& buffer) {
  CompactProtocolReader reader;
  auto responseMetadata = std::make_unique<ResponseRpcMetadata>();
  reader.setInput(&buffer);
  responseMetadata->read(&reader);
  return responseMetadata;
}

RSClientThriftChannel::RSClientThriftChannel(
    std::shared_ptr<RSocketRequester> rsRequester)
    : rsRequester_(std::move(rsRequester)) {}

bool RSClientThriftChannel::supportsHeaders() const noexcept {
  return true;
}

void RSClientThriftChannel::sendThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> payload,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(metadata->__isset.kind);
  switch (metadata->kind) {
    case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
      sendSingleRequestResponse(
          std::move(metadata), std::move(payload), std::move(callback));
      break;
    case RpcKind::STREAMING_REQUEST_STREAMING_RESPONSE:
      channelRequest(std::move(metadata), std::move(payload));
      break;
    default:
      LOG(FATAL) << "not implemented";
  }
}

void RSClientThriftChannel::sendSingleRequestResponse(
    std::unique_ptr<RequestRpcMetadata> requestMetadata,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  DCHECK(requestMetadata);

  std::shared_ptr<ThriftClientCallback> spCallback{std::move(callback)};
  auto func = [spCallback](Payload payload) mutable {
    auto evb_ = spCallback->getEventBase();
    evb_->runInEventBaseThread([spCallback = std::move(spCallback),
                                payload = std::move(payload)]() mutable {
      spCallback->onThriftResponse(
          payload.metadata ? deserializeMetadata(*payload.metadata)
                           : std::make_unique<ResponseRpcMetadata>(),
          std::move(payload.data));
    });
  };

  auto err = [spCallback](folly::exception_wrapper ex) mutable {
    LOG(ERROR) << "This method should never be called: " << ex;

    // TODO: Inspect the cases where might we end up in this function.
    // 1- server closes the stream before all the messages are delievered.
  };

  auto singleObserver = yarpl::single::SingleObservers::create<Payload>(
      std::move(func), std::move(err));
  rsRequester_
      ->requestResponse(
          rsocket::Payload(std::move(buf), serializeMetadata(*requestMetadata)))
      ->subscribe(singleObserver);
}

void RSClientThriftChannel::channelRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<folly::IOBuf> payload) noexcept {
  auto input = yarpl::flowable::Flowables::fromPublisher<rsocket::Payload>(
      [this, initialBuf = std::move(payload), metadata = std::move(metadata)](
          yarpl::Reference<yarpl::flowable::Subscriber<rsocket::Payload>>
              subscriber) mutable {
        VLOG(3) << "Input is started to be consumed: "
                << initialBuf->cloneAsValue().moveToFbString().toStdString();
        outputPromise_.setValue(yarpl::make_ref<RPCSubscriber>(
            serializeMetadata(*metadata),
            std::move(initialBuf),
            std::move(subscriber)));
      });

  // Perform the rpc call
  auto result = rsRequester_->requestChannel(input);
  result
      ->map([](auto payload) -> std::unique_ptr<folly::IOBuf> {
        VLOG(3) << "Request channel: "
                << payload.data->cloneAsValue().moveToFbString().toStdString();

        // TODO - don't drop the headers
        return std::move(payload.data);
      })
      ->subscribe(input_);
}
} // namespace thrift
} // namespace apache
