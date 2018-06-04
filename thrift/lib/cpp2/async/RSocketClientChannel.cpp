/*
 * Copyright 2015-present Facebook, Inc.
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

#include <thrift/lib/cpp2/async/RSocketClientChannel.h>

#include <rsocket/RSocketResponder.h>
#include <rsocket/framing/FramedDuplexConnection.h>
#include <rsocket/transports/tcp/TcpConnectionFactory.h>
#include <rsocket/transports/tcp/TcpDuplexConnection.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/transport/core/EnvelopeUtil.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <thrift/lib/cpp2/transport/rsocket/client/TakeFirst.h>
#include <thrift/lib/cpp2/transport/rsocket/gen-cpp2/Config_types.h>

using namespace apache::thrift::transport;
using namespace rsocket;
using namespace yarpl::single;
using namespace yarpl::flowable;

namespace apache {
namespace thrift {

namespace detail {
std::unique_ptr<folly::IOBuf> encodeSetupPayload(
    const RSocketSetupParameters& setupParams) {
  folly::IOBufQueue queue;
  // Put version information
  auto buf = folly::IOBuf::createCombined(sizeof(int32_t));
  queue.append(std::move(buf));
  folly::io::QueueAppender appender(&queue, /* do not grow */ 0);
  appender.writeBE<uint16_t>(RSocketClientChannel::kMajorVersion);
  appender.writeBE<uint16_t>(RSocketClientChannel::kMinorVersion);

  // Put encoded setup struct
  CompactProtocolWriter writer;
  writer.setOutput(&queue);
  setupParams.write(&writer);
  return queue.move();
}

void RSConnectionStatus::setCloseCallback(CloseCallback* ccb) {
  closeCallback_ = ccb;
}
bool RSConnectionStatus::isConnected() const {
  return isConnected_;
}
void RSConnectionStatus::onConnected() {
  isConnected_ = true;
}
void RSConnectionStatus::onDisconnected(const folly::exception_wrapper&) {
  closed();
}
void RSConnectionStatus::onClosed(const folly::exception_wrapper&) {
  closed();
}
void RSConnectionStatus::closed() {
  if (isConnected_) {
    isConnected_ = false;
    if (closeCallback_) {
      closeCallback_->channelClosed();
    }
  }
}
} // namespace detail

namespace {
class CountedSingleObserver : public SingleObserver<Payload> {
 public:
  CountedSingleObserver(
      std::unique_ptr<ThriftClientCallback> callback,
      detail::ChannelCounters* counters)
      : callback_(std::move(callback)), counters_(counters) {}

  virtual ~CountedSingleObserver() {
    if (counters_) {
      counters_->decPendingRequests();
    }
  }

 protected:
  void onSubscribe(std::shared_ptr<SingleSubscription>) override {
    // Note that we don't need the Subscription object.
    callback_->onThriftRequestSent();
  }

  void onSuccess(Payload payload) override {
    if (auto callback = std::move(callback_)) {
      callback->onThriftResponse(
          payload.metadata
              ? RSocketClientChannel::deserializeMetadata(*payload.metadata)
              : std::make_unique<ResponseRpcMetadata>(),
          std::move(payload.data));
    }
  }

  void onError(folly::exception_wrapper ew) override {
    if (auto callback = std::move(callback_)) {
      callback->onError(std::move(ew));
      // TODO: Inspect the cases where might we end up in this function.
      // 1- server closes the stream before all the messages are
      // delievered.
      // 2- time outs
    }
  }

 private:
  std::unique_ptr<ThriftClientCallback> callback_;
  detail::ChannelCounters* counters_;
};

} // namespace

const std::chrono::milliseconds RSocketClientChannel::kDefaultRpcTimeout =
    std::chrono::milliseconds(500);
const uint16_t RSocketClientChannel::kMajorVersion = 0;
const uint16_t RSocketClientChannel::kMinorVersion = 1;

RSocketClientChannel::Ptr RSocketClientChannel::newChannel(
    async::TAsyncTransport::UniquePtr transport,
    bool isSecure) {
  return RSocketClientChannel::Ptr(
      new RSocketClientChannel(std::move(transport), isSecure));
}

std::unique_ptr<folly::IOBuf> RSocketClientChannel::serializeMetadata(
    const RequestRpcMetadata& requestMetadata) {
  CompactProtocolWriter writer;
  folly::IOBufQueue queue;
  writer.setOutput(&queue);
  requestMetadata.write(&writer);
  return queue.move();
}

std::unique_ptr<ResponseRpcMetadata> RSocketClientChannel::deserializeMetadata(
    const folly::IOBuf& buffer) {
  CompactProtocolReader reader;
  auto responseMetadata = std::make_unique<ResponseRpcMetadata>();
  reader.setInput(&buffer);
  responseMetadata->read(&reader);
  return responseMetadata;
}

RSocketClientChannel::RSocketClientChannel(
    apache::thrift::async::TAsyncTransport::UniquePtr socket,
    bool isSecure)
    : evb_(socket->getEventBase()),
      isSecure_(isSecure),
      connectionStatus_(std::make_shared<detail::RSConnectionStatus>()),
      channelCounters_([&]() {
        if (isDetachable()) {
          notifyDetachable();
        }
      }) {
  stateMachine_ = std::make_shared<RSocketStateMachine>(
      std::make_shared<RSocketResponder>(),
      nullptr,
      RSocketMode::CLIENT,
      nullptr,
      connectionStatus_,
      ResumeManager::makeEmpty(),
      nullptr);

  rsocket::SetupParameters rsocketSetupParams;
  rsocketSetupParams.resumable = false;

  apache::thrift::RSocketSetupParameters thriftSetupParams;
  rsocketSetupParams.payload.metadata =
      detail::encodeSetupPayload(thriftSetupParams);

  auto&& conn =
      TcpConnectionFactory::createDuplexConnectionFromSocket(std::move(socket));
  DCHECK(!conn->isFramed());
  auto&& framedConn = std::make_unique<FramedDuplexConnection>(
      std::move(conn), rsocketSetupParams.protocolVersion);
  auto transport = std::make_shared<FrameTransportImpl>(std::move(framedConn));

  stateMachine_->connectClient(
      std::move(transport), std::move(rsocketSetupParams));
}

RSocketClientChannel::~RSocketClientChannel() {
  connectionStatus_->setCloseCallback(nullptr);
  if (stateMachine_) {
    closeNow();
  }
}

void RSocketClientChannel::setProtocolId(uint16_t protocolId) {
  protocolId_ = protocolId;
}

void RSocketClientChannel::setHTTPHost(const std::string& host) {
  httpHost_ = host;
}

void RSocketClientChannel::setHTTPUrl(const std::string& url) {
  httpUrl_ = url;
}

uint32_t RSocketClientChannel::sendRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<THeader> header) {
  sendThriftRequest(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header));
  return 0;
}

uint32_t RSocketClientChannel::sendOnewayRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<THeader> header) {
  sendThriftRequest(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_NO_RESPONSE,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header));
  return ResponseChannel::ONEWAY_REQUEST_ID;
}

uint32_t RSocketClientChannel::sendStreamRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header) {
  sendThriftRequest(
      rpcOptions,
      RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header));
  return 0;
}

std::unique_ptr<RequestRpcMetadata>
RSocketClientChannel::createRequestRpcMetadata(
    RpcOptions& rpcOptions,
    RpcKind kind,
    apache::thrift::ProtocolId protocolId,
    THeader* header) {
  auto metadata = std::make_unique<RequestRpcMetadata>();
  metadata->protocol = protocolId;
  metadata->__isset.protocol = true;
  metadata->kind = kind;
  metadata->__isset.kind = true;
  if (!httpHost_.empty()) {
    metadata->host = httpHost_;
    metadata->__isset.host = true;
  }
  if (!httpUrl_.empty()) {
    metadata->url = httpUrl_;
    metadata->__isset.url = true;
  }
  if (rpcOptions.getTimeout() > std::chrono::milliseconds(0)) {
    metadata->clientTimeoutMs = rpcOptions.getTimeout().count();
  } else {
    metadata->clientTimeoutMs = kDefaultRpcTimeout.count();
  }
  metadata->__isset.clientTimeoutMs = true;
  if (rpcOptions.getQueueTimeout() > std::chrono::milliseconds(0)) {
    metadata->queueTimeoutMs = rpcOptions.getQueueTimeout().count();
    metadata->__isset.queueTimeoutMs = true;
  }
  if (rpcOptions.getPriority() < concurrency::N_PRIORITIES) {
    metadata->priority = static_cast<RpcPriority>(rpcOptions.getPriority());
    metadata->__isset.priority = true;
  }
  metadata->otherMetadata = header->releaseWriteHeaders();
  auto* eh = header->getExtraWriteHeaders();
  if (eh) {
    metadata->otherMetadata.insert(eh->begin(), eh->end());
  }
  auto& pwh = getPersistentWriteHeaders();
  metadata->otherMetadata.insert(pwh.begin(), pwh.end());
  if (!metadata->otherMetadata.empty()) {
    metadata->__isset.otherMetadata = true;
  }
  return metadata;
}

void RSocketClientChannel::sendThriftRequest(
    RpcOptions& rpcOptions,
    RpcKind kind,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<THeader> header) noexcept {
  DestructorGuard dg(this);

  cb->context_ = folly::RequestContext::saveContext();
  auto metadata = createRequestRpcMetadata(
      rpcOptions,
      kind,
      static_cast<apache::thrift::ProtocolId>(protocolId_),
      header.get());

  if (!EnvelopeUtil::stripEnvelope(metadata.get(), buf) ||
      !(metadata->kind == RpcKind::SINGLE_REQUEST_NO_RESPONSE ||
        metadata->kind == RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE ||
        metadata->kind == RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE)) {
    folly::RequestContextScopeGuard rctx(cb->context_);
    cb->requestError(ClientReceiveState(
        folly::make_exception_wrapper<TTransportException>(
            TTransportException::CORRUPTED_DATA,
            "Unexpected problem stripping envelope"),
        std::move(ctx),
        isSecurityActive()));
    return;
  }
  metadata->seqId = 0;
  metadata->__isset.seqId = true;
  DCHECK(metadata->__isset.kind);

  if (!connectionStatus_->isConnected()) {
    folly::RequestContextScopeGuard rctx(cb->context_);
    cb->requestError(ClientReceiveState(
        folly::make_exception_wrapper<TTransportException>(
            TTransportException::NOT_OPEN, "Connection is not open"),
        std::move(ctx),
        isSecurityActive()));
    return;
  }

  if (!channelCounters_.incPendingRequests()) {
    LOG_EVERY_N(ERROR, 100)
        << "max number of pending requests is exceeded x100";

    TTransportException ex(
        TTransportException::NETWORK_ERROR,
        "Too many active requests on connection");
    // Might be able to create another transaction soon
    ex.setOptions(TTransportException::CHANNEL_IS_VALID);

    folly::RequestContextScopeGuard rctx(cb->context_);
    cb->requestError(
        ClientReceiveState(std::move(ex), std::move(ctx), isSecurityActive()));
    return;
  }

  switch (metadata->kind) {
    case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
      sendSingleRequestNoResponse(
          std::move(metadata), std::move(ctx), std::move(buf), std::move(cb));
      break;
    case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
      sendSingleRequestSingleResponse(
          std::move(metadata), std::move(ctx), std::move(buf), std::move(cb));
      break;
    case RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE:
      sendSingleRequestStreamResponse(
          rpcOptions,
          std::move(metadata),
          std::move(ctx),
          std::move(buf),
          std::move(cb));
      break;
    default:
      folly::assume_unreachable();
  }
}

uint16_t RSocketClientChannel::getProtocolId() {
  return protocolId_;
}

void RSocketClientChannel::sendSingleRequestNoResponse(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<RequestCallback> cb) noexcept {
  auto callback = new RSocketClientChannel::OnewayCallback(
      std::move(cb), std::move(ctx), isSecurityActive());

  callback->sendQueued();

  stateMachine_->fireAndForget(
      Payload(std::move(buf), serializeMetadata(*metadata)));

  callback->messageSent();

  channelCounters_.decPendingRequests();
}

void RSocketClientChannel::sendSingleRequestSingleResponse(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<RequestCallback> cb) noexcept {
  auto callback = std::make_unique<ThriftClientCallback>(
      evb_,
      std::move(cb),
      std::move(ctx),
      isSecurityActive(),
      protocolId_,
      std::chrono::milliseconds(metadata->clientTimeoutMs));

  auto singleObserver = std::make_shared<CountedSingleObserver>(
      std::move(callback), &channelCounters_);

  // As we send clientTimeoutMs, queueTimeoutMs and priority values using
  // RequestRpcMetadata object, there is no need for RSocket to put them to
  // metadata->otherMetadata map.
  stateMachine_->requestResponse(
      Payload(std::move(buf), serializeMetadata(*metadata)),
      std::move(singleObserver));
}

void RSocketClientChannel::sendSingleRequestStreamResponse(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<RequestCallback> cb) noexcept {
  auto callback = std::make_shared<ThriftClientCallback>(
      evb_,
      std::move(cb),
      std::move(ctx),
      isSecurityActive(),
      protocolId_,
      std::chrono::milliseconds(metadata->clientTimeoutMs));

  auto takeFirst = std::make_shared<TakeFirst>(
      // onRequestSent
      [callback]() mutable {
        auto cb = std::exchange(callback, nullptr);
        cb->onThriftRequestSent();
      },
      // onResponse
      [this, callback, chunkTimeout = rpcOptions.getChunkTimeout()](
          std::pair<
              rsocket::Payload,
              std::shared_ptr<Flowable<std::unique_ptr<IOBuf>>>>
              result) mutable {
        auto cb = std::exchange(callback, nullptr);
        auto flowable = std::move(result.second);
        if (chunkTimeout.count() > 0) {
          flowable = flowable->timeout(
              *evb_,
              std::chrono::milliseconds(chunkTimeout),
              std::chrono::milliseconds(chunkTimeout),
              []() {
                return transport::TTransportException(
                    transport::TTransportException::TTransportExceptionType::
                        TIMED_OUT);
              });
        }
        cb->onThriftResponse(
            result.first.metadata ? deserializeMetadata(*result.first.metadata)
                                  : std::make_unique<ResponseRpcMetadata>(),
            std::move(result.first.data),
            toStream(std::move(flowable), evb_));
      },
      // onError
      [this, callback](folly::exception_wrapper ew) mutable {
        channelCounters_.decPendingRequests();
        auto cb = std::exchange(callback, nullptr);
        cb->onError(std::move(ew));
      },
      // onTerminal
      [this]() { channelCounters_.decPendingRequests(); });

  callback->setTimedOut([tfw = std::weak_ptr<TakeFirst>(takeFirst)]() {
    if (auto tfs = tfw.lock()) {
      tfs->cancel();
    }
  });

  stateMachine_->requestStream(
      Payload(std::move(buf), serializeMetadata(*metadata)),
      std::move(takeFirst));
}

void RSocketClientChannel::setMaxPendingRequests(uint32_t count) {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  channelCounters_.setMaxPendingRequests(count);
}

folly::EventBase* RSocketClientChannel::getEventBase() const {
  return evb_;
}

apache::thrift::async::TAsyncTransport* FOLLY_NULLABLE
RSocketClientChannel::getTransport() {
  DCHECK(!evb_ || evb_->isInEventBaseThread());
  if (stateMachine_) {
    DuplexConnection* connection = stateMachine_->getConnection();
    if (!connection) {
      LOG_EVERY_N(ERROR, 100)
          << "Connection is already closed. May be protocol mismatch x 100";
      stateMachine_.reset();
      return nullptr;
    }
    if (auto framedConnection =
            dynamic_cast<FramedDuplexConnection*>(connection)) {
      connection = framedConnection->getConnection();
    }
    auto* tcpConnection = dynamic_cast<TcpDuplexConnection*>(connection);
    CHECK_NOTNULL(tcpConnection);
    return dynamic_cast<apache::thrift::async::TAsyncTransport*>(
        tcpConnection->getTransport());
  }
  return nullptr;
}

bool RSocketClientChannel::good() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  auto const socket = getTransport();
  return socket && socket->good();
}

ClientChannel::SaturationStatus RSocketClientChannel::getSaturationStatus() {
  DCHECK(evb_ && evb_->isInEventBaseThread());
  return ClientChannel::SaturationStatus(
      channelCounters_.getPendingRequests(),
      channelCounters_.getMaxPendingRequests());
}

void RSocketClientChannel::attachEventBase(folly::EventBase* evb) {
  DCHECK(evb->isInEventBaseThread());
  auto transport = getTransport();
  if (transport) {
    transport->attachEventBase(evb);
  }
  evb_ = evb;
}

void RSocketClientChannel::detachEventBase() {
  DCHECK(isDetachable());
  auto transport = getTransport();
  if (transport) {
    transport->detachEventBase();
  }
  evb_ = nullptr;
}

bool RSocketClientChannel::isDetachable() {
  DCHECK(!evb_ || evb_->isInEventBaseThread());
  auto transport = getTransport();
  bool result = evb_ == nullptr || transport == nullptr ||
      stateMachine_ == nullptr ||
      (channelCounters_.getPendingRequests() == 0 && transport->isDetachable());
  return result;
}

bool RSocketClientChannel::isSecurityActive() {
  return isSecure_;
}

uint32_t RSocketClientChannel::getTimeout() {
  // TODO: Need to inspect this functionality for RSocket
  return timeout_.count();
}

void RSocketClientChannel::setTimeout(uint32_t timeoutMs) {
  // TODO: Need to inspect this functionality for RSocket
  timeout_ = std::chrono::milliseconds(timeoutMs);
}

void RSocketClientChannel::closeNow() {
  DCHECK(!evb_ || evb_->isInEventBaseThread());
  if (stateMachine_) {
    if (!evb_) {
      // Add current event base instead of the missing one.
      LOG(ERROR) << "Closing RSocketClientChannel with missing EventBase";
      attachEventBase(folly::EventBaseManager::get()->getEventBase());
    }
    stateMachine_->close(
        folly::exception_wrapper(), StreamCompletionSignal::SOCKET_CLOSED);
    stateMachine_.reset();
  }
}

CLIENT_TYPE RSocketClientChannel::getClientType() {
  // TODO: Should we use this value?
  return THRIFT_HTTP_CLIENT_TYPE;
}

void RSocketClientChannel::setCloseCallback(CloseCallback* cb) {
  connectionStatus_->setCloseCallback(cb);
}

// ChannelCounters' functions
namespace detail {

static constexpr uint32_t kMaxPendingRequests =
    std::numeric_limits<uint32_t>::max();
ChannelCounters::ChannelCounters(folly::Function<void()> onDetachable)
    : maxPendingRequests_(kMaxPendingRequests),
      onDetachable_(std::move(onDetachable)) {}

void ChannelCounters::setMaxPendingRequests(uint32_t count) {
  maxPendingRequests_ = count;
}

uint32_t ChannelCounters::getMaxPendingRequests() {
  return maxPendingRequests_;
}

uint32_t ChannelCounters::getPendingRequests() {
  return pendingRequests_;
}

bool ChannelCounters::incPendingRequests() {
  if (pendingRequests_ >= maxPendingRequests_) {
    return false;
  }
  ++pendingRequests_;
  return true;
}

void ChannelCounters::decPendingRequests() {
  --pendingRequests_;
  if (pendingRequests_ == 0) {
    onDetachable_();
  }
}
} // namespace detail

} // namespace thrift
} // namespace apache
