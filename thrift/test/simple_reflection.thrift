namespace cpp test_cpp1.simple_cpp_reflection
namespace cpp2 test_cpp2.simple_cpp_reflection

cpp_include "<deque>"

typedef binary (cpp2.type = "folly::IOBuf") IOBuf
typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr

enum enum1 {
  field0,
  field1,
  field2
}

struct nested1 {
  1: optional float nfield00;
  2: optional byte nfield01;
}

struct smallstruct {
  1: i32 f1;
}

struct struct1 {
  1: required i32 field0
  2: optional string field1
  3: optional enum1 field2
  4: required list<list<i32>> field3
  5: required set<i32> field4
  6: required map<i32, string> field5
  7: required nested1 field6
  8: i64 field7 # generate writer req/reader opt field
  9: string field8 # default requiredness type field
}

struct struct2 {
  1: required string req_string
  2: optional string opt_string
  3: string def_string
}

union union1 {
  1: i64 field_i64;
  2: string field_string;
  66: list<i64> field_list_i64;
  99: list<string> field_list_string;
  5: string field_string_ref (cpp2.ref = "true")
  999: binary field_binary
}

struct struct3 {
    1: optional smallstruct opt_nested (cpp.ref = "true", cpp2.ref = "true")
    2: smallstruct def_nested (cpp.ref = "true", cpp2.ref = "true")
    3: required smallstruct req_nested (cpp.ref = "true", cpp2.ref = "true")
}

typedef map<i32, string> (cpp2.template = "std::unordered_map") unordered_map
typedef set<i32> (cpp2.template = "std::unordered_set") unordered_set
typedef list<i32> (cpp2.template = "std::deque") deque

struct struct4 {
  1: unordered_map um_field
  2: unordered_set us_field
  3: deque deq_field
}

struct struct5 {
  1: binary def_field
  2: IOBuf iobuf_field
  3: IOBufPtr iobufptr_field
}

struct struct5_workaround {
  1: binary def_field
  2: IOBuf iobuf_field
}

struct struct5_listworkaround {
  1: list<binary> binary_list_field
  2: map<i32, binary> binary_map_field1
}
