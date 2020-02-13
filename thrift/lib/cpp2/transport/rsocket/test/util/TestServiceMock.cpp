/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/transport/rsocket/test/util/TestServiceMock.h>

#include <rsocket/Payload.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>

namespace testutil {
namespace testservice {

using namespace apache::thrift;
using namespace yarpl::flowable;

class LeakDetector {
 public:
  class InternalClass {};

  LeakDetector()
      : internal_(std::make_shared<testing::StrictMock<InternalClass>>()) {
    ++instanceCount();
  }

  LeakDetector(const LeakDetector& oth) : internal_(oth.internal_) {
    ++instanceCount();
  }

  LeakDetector& operator=(const LeakDetector& oth) {
    internal_ = oth.internal_;
    return *this;
  }

  virtual ~LeakDetector() {
    --instanceCount();
  }

  std::shared_ptr<testing::StrictMock<InternalClass>> internal_;

  static int32_t getInstanceCount() {
    return instanceCount();
  }

 protected:
  static std::atomic_int& instanceCount() {
    static std::atomic_int instanceCount{0};
    return instanceCount;
  }
};

int32_t TestServiceMock::echo(int32_t value) {
  return value;
}

ServerStream<int32_t> TestServiceMock::range(int32_t from, int32_t to) {
  auto [stream, publisher] = ServerStream<int32_t>::createPublisher();
  for (; from < to; ++from) {
    publisher.next(from);
  }
  std::move(publisher).complete();
  return std::move(stream);
}

ServerStream<int32_t>
TestServiceMock::slowRange(int32_t from, int32_t to, int32_t millis) {
  auto [stream, publisher] = ServerStream<int32_t>::createPublisher();
  auto eb = folly::getEventBase();
  std::shared_ptr<std::function<void(decltype(publisher), int32_t)>> schedule =
      std::make_shared<std::function<void(decltype(publisher), int32_t)>>();
  *schedule =
      [=,
       schedule =
           std::weak_ptr<std::function<void(decltype(publisher), int32_t)>>(
               schedule)](auto publisher, int32_t from) {
        publisher.next(from);
        if (++from < to) {
          folly::futures::sleep(std::chrono::milliseconds(millis))
              .via(eb)
              .thenValue([=,
                          publisher = std::move(publisher),
                          schedule = schedule.lock()](auto) mutable {
                (*schedule)(std::move(publisher), from);
              });
        } else {
          std::move(publisher).complete();
        }
      };
  folly::futures::sleep(std::chrono::milliseconds(millis))
      .via(eb)
      .thenValue([=, publisher = std::move(publisher)](auto) mutable {
        (*schedule)(std::move(publisher), from);
      });
  return std::move(stream);
}

ServerStream<int32_t> TestServiceMock::slowCancellation() {
  class Slow {
   public:
    ~Slow() {
      /* sleep override */
      std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
  };
#if FOLLY_HAS_COROUTINES
  return folly::coro::co_invoke(
      [slow = std::make_unique<Slow>()]()
          -> folly::coro::AsyncGenerator<int32_t&&> {
        LOG(FATAL) << "Should not be called";
      });
#else
  return ServerStream<int32_t>::createEmpty();
#endif
}

ResponseAndServerStream<int32_t, int32_t> TestServiceMock::leakCheck(
    int32_t from,
    int32_t to) {
#if FOLLY_HAS_COROUTINES
  auto stream = folly::coro::co_invoke([
    =,
    detector = LeakDetector()
  ]() -> folly::coro::AsyncGenerator<int32_t&&> {
    for (int i = from; i < to; ++i) {
      co_yield std::move(i);
    }
  });
#else
  auto stream = ServerStream<int32_t>::createEmpty();
#endif
  return {LeakDetector::getInstanceCount(), std::move(stream)};
}

ResponseAndServerStream<int32_t, int32_t>
TestServiceMock::leakCheckWithSleep(int32_t from, int32_t to, int32_t sleepMs) {
  std::this_thread::sleep_for(std::chrono::milliseconds{sleepMs});
  return leakCheck(from, to);
}

int32_t TestServiceMock::instanceCount() {
  return LeakDetector::getInstanceCount();
}

ServerStream<Message> TestServiceMock::returnNullptr() {
  return ServerStream<Message>::createEmpty();
}

ResponseAndServerStream<int, Message> TestServiceMock::throwError() {
  throw Error();
}

apache::thrift::ResponseAndServerStream<int32_t, int32_t>
TestServiceMock::sleepWithResponse(int32_t timeMs) {
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::milliseconds(timeMs));
  return {1,
          toStream(
              Flowable<>::range(1, 1)->map([](auto i) { return (int32_t)i; }),
              &executor_)};
}

apache::thrift::ServerStream<int32_t> TestServiceMock::sleepWithoutResponse(
    int32_t timeMs) {
  return std::move(sleepWithResponse(timeMs).stream);
}

apache::thrift::ResponseAndServerStream<int32_t, int32_t>
TestServiceMock::streamServerSlow() {
#if FOLLY_HAS_COROUTINES
  auto stream = folly::coro::co_invoke([
    =,
    detector = LeakDetector()
  ]() -> folly::coro::AsyncGenerator<int32_t&&> {
    co_await folly::futures::sleep(std::chrono::milliseconds(1000));
    while (true) {
      co_yield 1;
    }
  });
#else
  auto stream = ServerStream<int32_t>::createEmpty();
#endif
  return {1, std::move(stream)};
}

void TestServiceMock::sendMessage(
    int32_t messageId,
    bool complete,
    bool error) {
  if (!messages_) {
    throw std::runtime_error("First call registerToMessages");
  }
  if (messageId > 0) {
    messages_->next(messageId);
  }
  if (complete) {
    std::move(*messages_).complete();
    messages_.reset();
  } else if (error) {
    std::move(*messages_).complete({std::runtime_error("error")});
    messages_.reset();
  }
}

apache::thrift::ServerStream<int32_t> TestServiceMock::registerToMessages() {
  auto streamAndPublisher = ServerStream<int32_t>::createPublisher();
  messages_ = std::make_unique<apache::thrift::ServerStreamPublisher<int32_t>>(
      std::move(streamAndPublisher.second));
  return std::move(streamAndPublisher.first);
}

apache::thrift::ServerStream<Message> TestServiceMock::streamThrows(
    int32_t whichEx) {
  if (whichEx == 0) {
    SecondEx ex;
    ex.set_errCode(0);
    throw ex;
  }

  auto streamAndPublisher = ServerStream<Message>::createPublisher();
  if (whichEx == 1) {
    FirstEx ex;
    ex.set_errMsg("FirstEx");
    ex.set_errCode(1);
    std::move(streamAndPublisher.second).complete(ex);
  } else if (whichEx == 2) {
    SecondEx ex;
    ex.set_errCode(2);
    std::move(streamAndPublisher.second).complete(ex);
  } else {
    std::move(streamAndPublisher.second)
        .complete(std::runtime_error("random error"));
  }
  return std::move(streamAndPublisher.first);
}

apache::thrift::ResponseAndServerStream<int32_t, Message>
TestServiceMock::responseAndStreamThrows(int32_t whichEx) {
  return {1, streamThrows(whichEx)};
}

apache::thrift::ServerStream<int32_t> TestServiceMock::requestWithBlob(
    std::unique_ptr<folly::IOBuf>) {
  return apache::thrift::ServerStream<int32_t>::createEmpty();
}

} // namespace testservice
} // namespace testutil
