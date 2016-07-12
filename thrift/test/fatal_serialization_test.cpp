/*
 * Copyright 2016 Facebook, Inc.
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

#include <thrift/test/gen-cpp2/simple_reflection_fatal.h>
#include <thrift/test/gen-cpp2/simple_reflection_types.tcc>
#include <thrift/test/gen-cpp2/simple_reflection_fatal_types.h>
#include <thrift/test/gen-cpp2/simple_reflection_fatal_struct.h>

#include <thrift/lib/cpp2/fatal/serializer.h>
#include <thrift/lib/cpp2/fatal/pretty_print.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/SimpleJSONProtocol.h>
#include <thrift/lib/cpp2/protocol/JSONProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/fatal/internal/test_helpers.h>

#include <gtest/gtest.h>
#include <folly/Memory.h>

#include <iomanip>

namespace test_cpp2 { namespace simple_cpp_reflection {

// TODO: undef this once union deserialization has a fix
// this basically disables testing unions, because unions are broken
// in the JSON readers when using the legacy deserializer
#define UNIONS_STILL_BROKEN

using namespace apache::thrift;

template <typename Reader, typename Writer, bool Printable>
struct RWPair {
  using reader = Reader;
  using writer = Writer;
  using printable = std::integral_constant<bool, Printable>;
};

using protocol_type_pairs = ::testing::Types<
  RWPair<SimpleJSONProtocolReader, SimpleJSONProtocolWriter, true>,
  RWPair<JSONProtocolReader, JSONProtocolWriter, true>,
  RWPair<BinaryProtocolReader, BinaryProtocolWriter, false>,
  RWPair<CompactProtocolReader, CompactProtocolWriter, false>
>;

template <bool printable>
void print_underlying(folly::IOBuf& buffer) {
  if(VLOG_IS_ON(5)) {
    auto range = buffer.coalesce();
    if(printable) {
      VLOG(5) << "buffer: "
        << std::string((const char*)range.data(), range.size());
    } else {
      std::ostringstream out;
      for(int i = 0; i < range.size(); i++) {
        out << std::setw(2) << std::setfill('0')
                << std::hex << (int)range.data()[i] << " ";
      }
      VLOG(5) << "buffer: " << out.str();
    }
  }
}

template <typename Pair>
struct TypedTestCommon : public ::testing::Test {
  typename Pair::reader reader;
  typename Pair::writer writer;
  folly::IOBufQueue buffer;
  std::unique_ptr<folly::IOBuf> underlying;

  TypedTestCommon() {
    this->writer.setOutput(&this->buffer, 1024);
  }

  void prep_read() {
    this->underlying = this->buffer.move();
    this->reader.setInput(this->underlying.get());
  }

  void debug_buffer() {
    print_underlying<Pair::printable::value>(*underlying);
  }
};

template <typename Pair>
struct StructTest : public TypedTestCommon<Pair> {};

template <typename Pair>
struct StructTestConcrete : public TypedTestCommon<Pair> {
  virtual void TestBody() { return; }
};

template <typename Pair>
struct CompareStructTest : public ::testing::Test {
  StructTestConcrete<Pair> st1, st2;

  void prep_read() {
    st1.prep_read();
    st2.prep_read();
  }

  void debug_buffer() {
    st1.debug_buffer();
    st2.debug_buffer();
  }
};

TYPED_TEST_CASE(StructTest, protocol_type_pairs);
TYPED_TEST_CASE(CompareStructTest, protocol_type_pairs);

void init_struct_1(struct1& a) {
  a.field0 = 10;
  a.field1 = "this is a string";
  a.field2 = enum1::field1;
  a.field3 = {{1, 2, 3}, {4, 5, 6, 7}};
  a.field4 = {1, 1, 2, 3, 4, 10, 4, 6};
  a.field5 = {{42, "<answer>"}, {55, "schwifty five"}};
  a.field6.nfield00 = 5.678;
  a.field6.nfield01 = 0x42;
  a.field7 = 0xCAFEBABEA4DAFACE;
  a.field8 = "this field isn't set";

  a.__isset.field1 = true;
  a.__isset.field2 = true;
  a.field6.__isset.nfield00 = true;
  a.field6.__isset.nfield01 = true;
  a.__isset.field7 = true;
}

TYPED_TEST(StructTest, test_serialization) {
  // test/reflection.thrift
  struct1 a, b;
  init_struct_1(a);

  EXPECT_EQ(a.field4, std::set<int32_t>({1, 2, 3, 4, 6, 10}));

  serializer_write(a, this->writer);
  this->prep_read();

  serializer_read(b, this->reader);

  EXPECT_EQ(a.field0, b.field0);
  EXPECT_EQ(a.field1, b.field1);
  EXPECT_EQ(a.field2, b.field2);
  EXPECT_EQ(a.field3, b.field3);
  EXPECT_EQ(a.field4, b.field4);
  EXPECT_EQ(a.field5, b.field5);

  EXPECT_EQ(a.field6.nfield00, b.field6.nfield00);
  EXPECT_EQ(a.field6.nfield01, b.field6.nfield01);
  EXPECT_EQ(a.field6, b.field6);
  EXPECT_EQ(a.field7, b.field7);
  EXPECT_EQ(a.field8, b.field8); // default fields are always written out

  EXPECT_EQ(true, b.__isset.field1);
  EXPECT_EQ(true, b.__isset.field2);
  EXPECT_EQ(true, b.__isset.field7);
  EXPECT_EQ(true, b.__isset.field8);
}

TYPED_TEST(StructTest, test_other_containers) {
  struct4 a, b;

  a.um_field = {{42, "answer"}, {5, "five"}};
  a.us_field = {7, 11, 13, 17, 13, 19, 11};
  a.deq_field = {10, 20, 30, 40};

  serializer_write(a, this->writer);
  this->prep_read();
  serializer_read(b, this->reader);

  EXPECT_EQ(true, b.__isset.um_field);
  EXPECT_EQ(true, b.__isset.us_field);
  EXPECT_EQ(true, b.__isset.deq_field);
  EXPECT_EQ(a.um_field, b.um_field);
  EXPECT_EQ(a.us_field, b.us_field);
  EXPECT_EQ(a.deq_field, b.deq_field);
}

TYPED_TEST(StructTest, test_blank_default_ref_field) {
  struct3 a, b;
  a.opt_nested = std::make_unique<smallstruct>();
  a.req_nested = std::make_unique<smallstruct>();

  a.opt_nested->f1 = 5;
  a.req_nested->f1 = 10;

  // ref fields, interesting enough, do not have an __isset,
  // but are xfered based on the pointer value (nullptr or not)

  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();
  serializer_read(b, this->reader);

  EXPECT_EQ(smallstruct(),   *(b.def_nested));
  EXPECT_EQ(*(a.opt_nested), *(b.opt_nested));
  EXPECT_EQ(*(a.req_nested), *(b.req_nested));
}

TYPED_TEST(StructTest, test_blank_optional_ref_field) {
  struct3 a, b;
  a.def_nested = std::make_unique<smallstruct>();
  a.req_nested = std::make_unique<smallstruct>();

  a.def_nested->f1 = 5;
  a.req_nested->f1 = 10;

  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();
  serializer_read(b, this->reader);

  // null optional fields are deserialized to nullptr
  EXPECT_EQ(*(a.def_nested), *(b.def_nested));
  EXPECT_EQ(nullptr,           b.opt_nested.get());
  EXPECT_EQ(*(a.req_nested), *(b.req_nested));
}

TYPED_TEST(StructTest, test_blank_required_ref_field) {
  struct3 a, b;
  a.def_nested = std::make_unique<smallstruct>();
  a.opt_nested = std::make_unique<smallstruct>();

  a.def_nested->f1 = 5;
  a.opt_nested->f1 = 10;

  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();
  serializer_read(b, this->reader);

  EXPECT_EQ(*(a.def_nested), *(b.def_nested));
  EXPECT_EQ(*(a.opt_nested), *(b.opt_nested));
  EXPECT_EQ(smallstruct(),   *(b.req_nested));
}

TYPED_TEST(CompareStructTest, test_struct_xfer) {
  struct1 a1, a2, b1, b2;
  init_struct_1(a1);
  init_struct_1(a2);
  const std::size_t legacy_write_xfer = a1.write(&this->st1.writer);
  const std::size_t new_write_xfer = serializer_write(a2, this->st2.writer);
  EXPECT_EQ(legacy_write_xfer, new_write_xfer);

  this->prep_read();
  this->debug_buffer();

  const std::size_t legacy_read_xfer = b1.read(&this->st1.reader);
  const std::size_t new_read_xfer    = serializer_read(b2, this->st2.reader);
  EXPECT_EQ(legacy_read_xfer, new_read_xfer);
  EXPECT_EQ(b1, b2);
}

#ifndef UNIONS_STILL_BROKEN
TYPED_TEST(CompareStructTest, test_union_xfer) {
  union1 a1, a2, b1, b2;
  a1.set_field_i64(0x1ABBADABAD00);
  a2.set_field_i64(0x1ABBADABAD00);
  const std::size_t lwx = a1.write(&this->st1.writer);
  const std::size_t nwx = serializer_write(a2, this->st2.writer);
  EXPECT_EQ(lwx, nwx);

  const std::size_t lrx = b1.read(&this->st1.reader);
  const std::size_t nrx = serializer_read(b2, this->st2.reader);
  EXPECT_EQ(lrx, nrx);
  EXPECT_EQ(b1, b2);
}
#endif /* UNIONS_STILL_BROKEN */

namespace {
  const std::array<uint8_t, 5> test_buffer{{0xBA, 0xDB, 0xEE, 0xF0, 0x42}};
  const folly::ByteRange test_range(test_buffer.begin(), test_buffer.end());
  const folly::StringPiece test_string(test_range);

  const std::array<uint8_t, 6> test_buffer2
    {{0xFA, 0xCE, 0xB0, 0x01, 0x10, 0x0C}};
  const folly::ByteRange test_range2(test_buffer2.begin(), test_buffer2.end());
  const folly::StringPiece test_string2(test_range2);
}

TYPED_TEST(StructTest, test_binary_containers) {
  struct5 a, b;

  a.def_field = test_string.str();
  a.iobuf_field = folly::IOBuf::wrapBufferAsValue(test_range);
  a.iobufptr_field = folly::IOBuf::wrapBuffer(test_range2);

  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();

  serializer_read(b, this->reader);

  EXPECT_EQ(true, b.__isset.def_field);
  EXPECT_EQ(true, b.__isset.iobuf_field);
  EXPECT_EQ(true, b.__isset.iobufptr_field);
  EXPECT_EQ(a.def_field, b.def_field);

  EXPECT_EQ(test_range, b.iobuf_field.coalesce());
  EXPECT_EQ(test_range2, b.iobufptr_field->coalesce());
}

TYPED_TEST(StructTest, test_workaround_binary) {
  struct5_workaround a, b;
  a.def_field = test_string.str();
  a.iobuf_field = folly::IOBuf::wrapBufferAsValue(test_range2);

  serializer_write(a, this->writer);
  this->prep_read();
  serializer_read(b, this->reader);

  EXPECT_EQ(true, b.__isset.def_field);
  EXPECT_EQ(true, b.__isset.iobuf_field);
  EXPECT_EQ(test_string.str(), b.def_field);
  EXPECT_EQ(test_range2, b.iobuf_field.coalesce());
}

template <typename Pair>
class UnionTest : public TypedTestCommon<Pair> {
protected:
  union1 a, b;

  void xfer() {
    serializer_write(this->a, this->writer);
    this->prep_read();

    print_underlying<Pair::printable::value>(*this->underlying);

    serializer_read(b, this->reader);
    EXPECT_EQ(this->b.getType(), this->a.getType());
  }
};

#ifndef UNIONS_STILL_BROKEN
TYPED_TEST_CASE(UnionTest, protocol_type_pairs);

TYPED_TEST(UnionTest, can_read_union_i64s) {
  this->a.set_field_i64(0xFACEB00CFACEDEAD);
  this->xfer();
  EXPECT_EQ(this->b.get_field_i64(), this->a.get_field_i64());
}
TYPED_TEST(UnionTest, can_read_strings) {
  this->a.set_field_string("test string? oh my!");
  this->xfer();
  EXPECT_EQ(this->b.get_field_string(), this->a.get_field_string());
}
TYPED_TEST(UnionTest, can_read_refstrings) {
  this->a.set_field_string_ref("also reference strings!");
  this->xfer();
  EXPECT_EQ(
    *(this->b.get_field_string_ref().get()),
    *(this->a.get_field_string_ref().get()));
}
TYPED_TEST(UnionTest, can_read_iobufs) {
  this->a.set_field_binary(test_string.str());
  this->xfer();
  EXPECT_EQ(test_string.str(), this->b.get_field_binary());
}
TYPED_TEST(UnionTest, can_read_nestedstructs) {
  smallstruct nested;
  nested.f1 = 6;
  this->a.set_field_smallstruct(nested);
  this->xfer();
  EXPECT_EQ(6, this->b.get_field_smallstruct()->f1);
}
#endif // UNIONS_STILL_BROKEN

template <typename Pair>
class BinaryInContainersTest : public TypedTestCommon<Pair> {
protected:
  struct5_listworkaround a, b;

  void xfer() {
    serializer_write(this->a, this->writer);
    this->prep_read();
    serializer_read(this->b, this->reader);
  }
};
TYPED_TEST_CASE(BinaryInContainersTest, protocol_type_pairs);

TYPED_TEST(BinaryInContainersTest, lists_of_binary_fields_work) {
  this->a.binary_list_field = {test_string.str()};
  this->a.binary_map_field1 = {
    {5,     test_string.str()},
    {-9999, test_string2.str()}};

  this->xfer();

  EXPECT_EQ(
    std::vector<std::string>({test_string.str()}),
    this->b.binary_list_field);
}

struct SimpleJsonTest : public ::testing::Test {
  SimpleJSONProtocolReader reader;
  std::unique_ptr<folly::IOBuf> underlying;

  void set_input(std::string&& str) {
    underlying = folly::IOBuf::copyBuffer(str);
    reader.setInput(underlying.get());

    if(VLOG_IS_ON(5)) {
      auto range = underlying->coalesce();
      VLOG(5) << "buffer: "
        << std::string((const char*)range.data(), range.size());
    }
  }
};

TEST_F(SimpleJsonTest, throws_on_unset_required_value) {
  set_input("{}");
  try {
    struct2 a;
    serializer_read(a, reader);
    EXPECT_TRUE(false) << "didn't throw!";
  }
  catch(TProtocolException& e) {
    EXPECT_EQ(TProtocolException::MISSING_REQUIRED_FIELD, e.getType());
  }
}

// wrap in quotes
#define Q(val) "\"" val "\""
// emit key/value json pair
#define KV(key, value) "\"" key "\":" value
// emit key/value json pair, where value is a string
#define KVS(key, value) KV(key, Q(value))

TEST_F(SimpleJsonTest, handles_unset_default_member) {
  set_input("{" KVS("req_string", "required") "}");
  struct2 a;
  serializer_read(a, reader);
  EXPECT_TRUE(false == a.__isset.opt_string); // gcc bug?
  EXPECT_TRUE(false == a.__isset.def_string);
  EXPECT_EQ("required", a.req_string);
  EXPECT_EQ("", a.opt_string);
  EXPECT_EQ("", a.def_string);
}
TEST_F(SimpleJsonTest, sets_opt_members) {
  set_input("{"
    KVS("req_string","required")","
    KVS("opt_string","optional")
  "}");
  struct2 a;
  serializer_read(a, reader);
  EXPECT_TRUE(true == a.__isset.opt_string); // gcc bug?
  EXPECT_TRUE(false == a.__isset.def_string);
  EXPECT_EQ("required", a.req_string);
  EXPECT_EQ("optional", a.opt_string);
  EXPECT_EQ("", a.def_string);
}
TEST_F(SimpleJsonTest, sets_def_members) {
  set_input("{"
    KVS("req_string","required")","
    KVS("def_string", "default")
  "}");
  struct2 a;
  serializer_read(a, reader);
  EXPECT_TRUE(false == a.__isset.opt_string);
  EXPECT_EQ(true,  a.__isset.def_string);
  EXPECT_EQ("required", a.req_string);
  EXPECT_EQ("", a.opt_string);
  EXPECT_EQ("default", a.def_string);
}
TEST_F(SimpleJsonTest, throws_on_missing_required_ref) {
  set_input("{"
    KV("opt_nested", "{"
      KV("f1", "10")
    "}")","
    KV("def_nested", "{"
      KV("f1", "5")
    "}")
  "}");

  struct3 a;

  try {
    serializer_read(a, reader);
    EXPECT_TRUE(false) << "didn't throw!";
  }
  catch(TProtocolException& e) {
    EXPECT_EQ(TProtocolException::MISSING_REQUIRED_FIELD, e.getType());
  }
}
TEST_F(SimpleJsonTest, doesnt_throw_when_req_field_present) {
  set_input("{"
    KV("opt_nested", "{"
      KV("f1", "10")
    "}")","
    KV("def_nested", "{"
      KV("f1", "5")
    "}")","
    KV("req_nested", "{"
      KV("f1", "15")
    "}")
  "}");

  struct3 a;
  serializer_read(a, reader);
  EXPECT_EQ(10, a.opt_nested->f1);
  EXPECT_EQ(5, a.def_nested->f1);
  EXPECT_EQ(15, a.req_nested->f1);
}
#undef KV
} } /* namespace cpp_reflection::test_cpp2 */
