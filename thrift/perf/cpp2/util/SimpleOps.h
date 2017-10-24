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

#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/perf/cpp2/if/gen-cpp2/ApiBase_types.h>
#include <thrift/perf/cpp2/util/QPSStats.h>

using apache::thrift::ClientReceiveState;
using apache::thrift::RequestCallback;
using facebook::thrift::benchmarks::QPSStats;
using facebook::thrift::benchmarks::TwoInts;

template <typename AsyncClient>
class Noop {
 public:
  Noop(QPSStats* stats) : stats_(stats) {
    stats_->registerCounter(op_name_);
  }
  ~Noop() = default;

  void async(AsyncClient* client, std::unique_ptr<RequestCallback> cb) {
    client->noop(std::move(cb));
  }

  void asyncReceived(AsyncClient* client, ClientReceiveState&& rstate) {
    stats_->add(op_name_);
    client->recv_noop(rstate);
  }

 private:
  QPSStats* stats_;
  std::string op_name_ = "noop";
};

template <typename AsyncClient>
class Sum {
 public:
  Sum(QPSStats* stats) : stats_(stats) {
    // TODO: Perform different additions per call and verify correctness
    stats_->registerCounter(op_name_);
    request_.x = 0;
    request_.__isset.x = true;
    request_.y = 0;
    request_.__isset.y = true;
  }
  ~Sum() = default;

  void async(AsyncClient* client, std::unique_ptr<RequestCallback> cb) {
    client->sum(std::move(cb), request_);
  }

  void asyncReceived(AsyncClient* client, ClientReceiveState&& rstate) {
    stats_->add(op_name_);
    client->recv_sum(response_, rstate);
  }

 private:
  QPSStats* stats_;
  std::string op_name_ = "sum";
  TwoInts request_;
  TwoInts response_;
};
