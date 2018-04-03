/*
 * Copyright 2018-present Facebook, Inc.
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

#include <folly/io/async/ScopedEventBaseThread.h>

#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/async/SemiStream.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <thrift/lib/cpp2/transport/rsocket/client/TakeFirst.h>
#include <thrift/lib/cpp2/transport/rsocket/test/util/gen-cpp2/StreamService.h>
#include <yarpl/flowable/TestSubscriber.h>

#include <gmock/gmock.h>

namespace apache {
namespace thrift {

using namespace yarpl::flowable;
using namespace testutil::testservice;

namespace {
/// Construct a pipeline with a test subscriber against the supplied
/// flowable.  Return the items that were sent to the subscriber.  If some
/// exception was sent, the exception is thrown.
template <typename T>
std::vector<T> run(
    std::shared_ptr<Flowable<T>> flowable,
    int64_t requestCount = 100) {
  auto subscriber = std::make_shared<TestSubscriber<T>>(requestCount);
  flowable->subscribe(subscriber);
  subscriber->awaitTerminalEvent(std::chrono::seconds(1));
  return std::move(subscriber->values());
}
} // namespace

TEST(YarplStreamImplTest, Basic) {
  folly::ScopedEventBaseThread evbThread;

  auto flowable = Flowable<>::justN({12, 34, 56, 98});
  auto stream = toStream(std::move(flowable), evbThread.getEventBase());
  auto stream2 = std::move(stream).map([&](int x) {
    EXPECT_TRUE(evbThread.getEventBase()->inRunningEventBaseThread());
    return x * 2;
  });

  auto flowable2 = toFlowable(std::move(stream2));

  EXPECT_EQ(run(flowable2), std::vector<int>({12 * 2, 34 * 2, 56 * 2, 98 * 2}));
}

TEST(YarplStreamImplTest, SemiStream) {
  folly::ScopedEventBaseThread evbThread;
  folly::ScopedEventBaseThread evbThread2;

  auto flowable = Flowable<>::justN({12, 34, 56, 98});
  auto stream = toStream(std::move(flowable), evbThread.getEventBase());
  SemiStream<int> stream2 = std::move(stream).map([&](int x) {
    EXPECT_TRUE(evbThread.getEventBase()->inRunningEventBaseThread());
    return x * 2;
  });
  auto streamString = std::move(stream2).map([&](int x) {
    EXPECT_TRUE(evbThread2.getEventBase()->inRunningEventBaseThread());
    return folly::to<std::string>(x);
  });
  auto flowableString =
      toFlowable(std::move(streamString).via(evbThread2.getEventBase()));

  EXPECT_EQ(
      run(flowableString),
      std::vector<std::string>({"24", "68", "112", "196"}));
}

TEST(YarplStreamImplTest, EncodeDecode) {
  folly::ScopedEventBaseThread evbThread;

  Message mIn;
  mIn.set_message("Test Message");
  mIn.__isset.message = true;
  mIn.set_timestamp(2015);
  mIn.__isset.timestamp = true;

  auto flowable = Flowable<>::justN({mIn});
  auto inStream = toStream(std::move(flowable), evbThread.getEventBase());

  using PResult =
      ThriftPresult<true, FieldData<0, protocol::T_STRUCT, Message*>>;

  // Encode in the thread of the flowable, namely at the user's thread
  auto encodedStream =
      detail::ap::encode_stream<CompactProtocolWriter, PResult>(
          std::move(inStream))
          .map([](folly::IOBufQueue&& in) mutable { return in.move(); });

  // No event base is involved, as this is a defered decoding
  auto decodedStream =
      detail::ap::decode_stream<CompactProtocolReader, PResult, Message>(
          SemiStream<std::unique_ptr<folly::IOBuf>>(std::move(encodedStream)));

  // Actual decode operation happens at the given user thread
  auto flowableOut =
      toFlowable(std::move(decodedStream).via(evbThread.getEventBase()));

  auto subscriber = std::make_shared<TestSubscriber<Message>>(1);
  flowableOut->subscribe(subscriber);

  subscriber->awaitTerminalEvent(std::chrono::seconds(1));

  Message mOut = std::move(subscriber->values()[0]);
  ASSERT_STREQ(mIn.get_message().c_str(), mOut.get_message().c_str());
  ASSERT_EQ(mIn.get_timestamp(), mOut.get_timestamp());
}

TEST(FlowableTest, TakeFirstNormal) {
  folly::ScopedEventBaseThread evbThread;

  auto a = Flowable<>::range(1, 1);
  auto b = Flowable<>::range(2, 1);
  auto combined = a->concatWith(b);

  folly::Baton<> baton;
  auto takeFirst = std::make_shared<TakeFirst<int64_t>>(std::move(combined));
  takeFirst->get()
      .via(evbThread.getEventBase())
      .then(
          [&baton](
              std::pair<int64_t, std::shared_ptr<Flowable<int64_t>>>&& result) {
            EXPECT_EQ(1, result.first);
            EXPECT_EQ(run(result.second), std::vector<int64_t>({2}));
            baton.post();
          });

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));
}

TEST(FlowableTest, TakeFirstDontSubscribe) {
  folly::ScopedEventBaseThread evbThread;

  auto a = Flowable<>::range(1, 1);
  auto b = Flowable<>::range(2, 1);
  auto combined = a->concatWith(b);

  folly::Baton<> baton;
  auto takeFirst = std::make_shared<TakeFirst<int64_t>>(std::move(combined));
  takeFirst->get()
      .via(evbThread.getEventBase())
      .then(
          [&baton](
              std::pair<int64_t, std::shared_ptr<Flowable<int64_t>>>&& result) {
            EXPECT_EQ(1, result.first);
            // Do not subscribe to the `result.second`
            baton.post();
          });

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));
}

TEST(FlowableTest, TakeFirstNoResponse) {
  folly::ScopedEventBaseThread evbThread;
  folly::Baton<> baton;
  auto takeFirst =
      std::make_shared<TakeFirst<int64_t>>(Flowable<int64_t>::never());
  bool timedOut = false;
  takeFirst->get()
      .via(evbThread.getEventBase())
      .then([&baton](std::pair<int64_t, std::shared_ptr<Flowable<int64_t>>>&&) {
        baton.post();
      })
      .onError([&timedOut, &baton](folly::exception_wrapper) {
        timedOut = true;
        baton.post();
      });

  ASSERT_FALSE(baton.timed_wait(std::chrono::seconds(1)));

  // Timed out, so cancel the TakeFirst
  baton.reset();
  takeFirst->cancel();

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));
  EXPECT_TRUE(timedOut);
}

TEST(FlowableTest, TakeFirstErrorResponse) {
  folly::ScopedEventBaseThread evbThread;
  folly::Baton<> baton;
  auto takeFirst = std::make_shared<TakeFirst<int64_t>>(
      Flowable<int64_t>::error(std::runtime_error("error")));
  takeFirst->get()
      .via(evbThread.getEventBase())
      .then([](auto&&) { ASSERT(false); })
      .onError([&baton](folly::exception_wrapper ew) {
        ASSERT_STREQ(ew.what().c_str(), "std::runtime_error: error");
        baton.post();
      });

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));
}

TEST(FlowableTest, TakeFirstStreamError) {
  folly::ScopedEventBaseThread evbThread;
  folly::Baton<> baton;

  auto first = Flowable<int64_t>::just(1);
  auto error = Flowable<int64_t>::error(std::runtime_error("error"));
  auto combined = first->concatWith(error);
  auto takeFirst = std::make_shared<TakeFirst<int64_t>>(combined);
  takeFirst->get()
      .via(evbThread.getEventBase())
      .then(
          [&baton](
              std::pair<int64_t, std::shared_ptr<Flowable<int64_t>>>&& result) {
            auto subscriber = std::make_shared<TestSubscriber<int64_t>>(1);
            result.second->subscribe(subscriber);

            EXPECT_TRUE(subscriber->isError());
            EXPECT_EQ(subscriber->getErrorMsg(), "error");
            baton.post();
          });

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));
}

TEST(FlowableTest, TakeFirstMultiSubscribe) {
  folly::ScopedEventBaseThread evbThread;

  auto a = Flowable<>::range(1, 1);
  auto b = Flowable<>::range(2, 1);
  auto combined = a->concatWith(b);

  // First subscribe
  folly::Baton<> baton;
  auto takeFirst = std::make_shared<TakeFirst<int64_t>>(combined);
  takeFirst->get()
      .via(evbThread.getEventBase())
      .then(
          [&baton](
              std::pair<int64_t, std::shared_ptr<Flowable<int64_t>>>&& result) {
            EXPECT_EQ(1, result.first);
            EXPECT_EQ(run(result.second), std::vector<int64_t>({2}));
            baton.post();
          });

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));

  // Second subscribe
  baton.reset();
  takeFirst = std::make_shared<TakeFirst<int64_t>>(combined);
  takeFirst->get()
      .via(evbThread.getEventBase())
      .then(
          [&baton](
              std::pair<int64_t, std::shared_ptr<Flowable<int64_t>>>&& result) {
            EXPECT_EQ(1, result.first);
            EXPECT_EQ(run(result.second), std::vector<int64_t>({2}));
            baton.post();
          });

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));
}

TEST(FlowableTest, TakeFirstMultiSubscribeInner) {
  folly::ScopedEventBaseThread evbThread;

  auto a = Flowable<>::range(1, 1);
  auto b = Flowable<>::range(2, 1);
  auto combined = a->concatWith(b);

  // First subscribe
  folly::Baton<> baton;
  auto takeFirst = std::make_shared<TakeFirst<int64_t>>(combined);
  takeFirst->get()
      .via(evbThread.getEventBase())
      .then(
          [&baton](
              std::pair<int64_t, std::shared_ptr<Flowable<int64_t>>>&& result) {
            EXPECT_EQ(1, result.first);
            EXPECT_EQ(run(result.second), std::vector<int64_t>({2}));

            // Second subscribe
            auto subscriber = std::make_shared<TestSubscriber<int64_t>>(10);
            EXPECT_THROW(
                result.second->subscribe(subscriber), std::logic_error);

            baton.post();
          });

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));
}

TEST(FlowableTest, TakeFirstMultiGet) {
  folly::ScopedEventBaseThread evbThread;

  auto a = Flowable<>::range(1, 1);
  auto b = Flowable<>::range(2, 1);
  auto combined = a->concatWith(b);

  // First `get` call
  folly::Baton<> baton;
  auto takeFirst = std::make_shared<TakeFirst<int64_t>>(combined);
  takeFirst->get()
      .via(evbThread.getEventBase())
      .then(
          [&baton](
              std::pair<int64_t, std::shared_ptr<Flowable<int64_t>>>&& result) {
            EXPECT_EQ(1, result.first);
            EXPECT_EQ(run(result.second), std::vector<int64_t>({2}));
            baton.post();
          });

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));

  // Second `get` call
  EXPECT_THROW(takeFirst->get(), std::logic_error);
}

} // namespace thrift
} // namespace apache
