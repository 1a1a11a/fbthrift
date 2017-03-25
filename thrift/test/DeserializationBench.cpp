/*
 * Copyright 2014-present Facebook, Inc.
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
#include <folly/Benchmark.h>
#include <folly/Random.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/test/gen-cpp2/DeserializationBench_types.h>
#include <gflags/gflags.h>

folly::Random::DefaultGenerator rng_(12345);
const int32_t kNumOfInserts = 250;

std::vector<int32_t> getRandomVector() {
  std::vector<int32_t> v;
  for (int i = 0; i < kNumOfInserts; ++i) {
    v.push_back(i);
  }
  return v;
}

std::set<int32_t> getRandomSet() {
  std::set<int32_t> s;
  for (int i = 0; i < kNumOfInserts; ++i) {
    s.insert(i);
  }
  return s;
}

std::map<int32_t, std::string> getRandomMap() {
  std::map<int32_t, std::string> m;
  for (int i = 0; i < kNumOfInserts; ++i) {
    m.emplace(i, std::to_string(i));
  }
  return m;
}

void buildRandomStructA(cpp2::StructA& obj) {
  obj.fieldA = folly::Random::rand32(rng_) % 2;
  obj.fieldB = folly::Random::rand32(rng_);
  obj.fieldC = std::to_string(folly::Random::rand32(rng_));
  obj.fieldD = getRandomVector();
  obj.fieldE = getRandomSet();
  obj.fieldF = getRandomMap();

  for (int32_t i = 0; i < kNumOfInserts; ++i) {
    std::vector<std::vector<int32_t>> g1;
    std::set<std::set<int32_t>> h1;
    std::vector<std::set<int32_t>> j1;
    std::set<std::vector<int32_t>> j2;
    for (int32_t j = 0; j < kNumOfInserts; ++j) {
      g1.push_back(getRandomVector());
      h1.insert(getRandomSet());
      j1.push_back(getRandomSet());
      j2.insert(getRandomVector());
    }
    obj.fieldG.push_back(g1);
    obj.fieldH.insert(h1);
    obj.fieldI.emplace(getRandomMap(), getRandomMap());
    obj.fieldJ.emplace(j1, j2);
  }
}

/*
============================================================================
thrift/test/DeserializationBench.cpp            relative  time/iter  iters/s
============================================================================
Deserialization                                               1.36s  734.51m
============================================================================
*/
BENCHMARK(Deserialization, iters) {
  for (size_t i = 0; i < iters; ++i) {
    // Stop time during object construction
    folly::BenchmarkSuspender braces;
      cpp2::StructA obj;
      cpp2::StructA obj2;
      apache::thrift::CompactProtocolWriter writer;
      apache::thrift::CompactProtocolReader reader;

      buildRandomStructA(obj);

      // Prepare Serialization
      size_t bufSize = obj.serializedSize(&writer);
      folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
      writer.setOutput(&queue, bufSize);

      // Serialize
      obj.write(&writer);

      // Prepare Deserialization
      bufSize = queue.chainLength();
      auto buf = queue.move();
      reader.setInput(buf.get());
    braces.dismiss();

    // Deserialize
    obj2.read(&reader);
  }
}

int main(int /*argc*/, char** /*argv*/) {
  folly::runBenchmarks();
}
