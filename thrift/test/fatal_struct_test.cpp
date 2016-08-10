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

#include <thrift/test/gen-cpp2/reflection_fatal_struct.h>

#include <thrift/lib/cpp2/fatal/internal/test_helpers.h>

#include <gtest/gtest.h>

FATAL_S(struct1s, "struct1");
FATAL_S(field0s, "field0");
FATAL_S(field1s, "field1");
FATAL_S(field2s, "field2");
FATAL_S(field3s, "field3");
FATAL_S(field4s, "field4");
FATAL_S(field5s, "field5");
FATAL_S(fieldAs, "fieldA");
FATAL_S(fieldBs, "fieldB");
FATAL_S(fieldCs, "fieldC");
FATAL_S(fieldDs, "fieldD");
FATAL_S(fieldEs, "fieldE");
FATAL_S(fieldFs, "fieldF");
FATAL_S(fieldGs, "fieldG");

template <apache::thrift::field_id_t Id>
using field_id = std::integral_constant<apache::thrift::field_id_t, Id>;

template <apache::thrift::optionality Optionality>
using required = std::integral_constant<
  apache::thrift::optionality,
  Optionality
>;

namespace test_cpp2 {
namespace cpp_reflection {

TEST(fatal_struct, struct1_sanity_check) {
  using traits = apache::thrift::reflect_struct<struct1>;

  EXPECT_SAME<struct1, traits::type>();
  EXPECT_SAME<struct1s, traits::name>();
  EXPECT_SAME<reflection_tags::module, traits::module>();

  EXPECT_SAME<traits, apache::thrift::try_reflect_struct<struct1, void>>();
  EXPECT_SAME<void, apache::thrift::try_reflect_struct<int, void>>();

  EXPECT_SAME<field0s, traits::names::field0>();
  EXPECT_SAME<field1s, traits::names::field1>();
  EXPECT_SAME<field2s, traits::names::field2>();
  EXPECT_SAME<field3s, traits::names::field3>();
  EXPECT_SAME<field4s, traits::names::field4>();
  EXPECT_SAME<field5s, traits::names::field5>();

  EXPECT_SAME<
    fatal::map<
      fatal::pair<field0s, std::int32_t>,
      fatal::pair<field1s, std::string>,
      fatal::pair<field2s, enum1>,
      fatal::pair<field3s, enum2>,
      fatal::pair<field4s, union1>,
      fatal::pair<field5s, union2>
    >,
    traits::types
  >();

  EXPECT_SAME<
    fatal::list<
      field0s,
      field1s,
      field2s,
      field3s,
      field4s,
      field5s
    >,
    fatal::map_keys<traits::members>
  >();

  struct1 pod;
  pod.field0 = 19;
  pod.field1 = "hello";
  pod.field2 = enum1::field2;
  pod.field3 = enum2::field1_2;
  pod.field4.set_ud(5.6);
  pod.field5.set_us_2("world");

  EXPECT_EQ(
    pod.field0,
    (fatal::map_get<traits::members, traits::names::field0>::getter::ref(pod))
  );
  EXPECT_EQ(
    pod.field1,
    (fatal::map_get<traits::members, traits::names::field1>::getter::ref(pod))
  );
  EXPECT_EQ(
    pod.field2,
    (fatal::map_get<traits::members, traits::names::field2>::getter::ref(pod))
  );
  EXPECT_EQ(
    pod.field3,
    (fatal::map_get<traits::members, traits::names::field3>::getter::ref(pod))
  );
  EXPECT_EQ(
    pod.field4,
    (fatal::map_get<traits::members, traits::names::field4>::getter::ref(pod))
  );
  EXPECT_EQ(
    pod.field5,
    (fatal::map_get<traits::members, traits::names::field5>::getter::ref(pod))
  );

  fatal::map_get<traits::members, traits::names::field0>::getter::ref(pod)
    = 98;
  EXPECT_EQ(98, pod.field0);

  fatal::map_get<traits::members, traits::names::field1>::getter::ref(pod)
    = "test";
  EXPECT_EQ("test", pod.field1);

  fatal::map_get<traits::members, traits::names::field2>::getter::ref(pod)
    = enum1::field0;
  EXPECT_EQ(enum1::field0, pod.field2);

  fatal::map_get<traits::members, traits::names::field3>::getter::ref(pod)
    = enum2::field2_2;
  EXPECT_EQ(enum2::field2_2, pod.field3);

  fatal::map_get<traits::members, traits::names::field4>::getter::ref(pod)
    .set_ui(56);
  EXPECT_EQ(union1::Type::ui, pod.field4.getType());
  EXPECT_EQ(56, pod.field4.get_ui());

  fatal::map_get<traits::members, traits::names::field5>::getter::ref(pod)
    .set_ue_2(enum1::field1);
  EXPECT_EQ(union2::Type::ue_2, pod.field5.getType());
  EXPECT_EQ(enum1::field1, pod.field5.get_ue_2());

  EXPECT_SAME<field0s, fatal::map_get<traits::members, field0s>::name>();
  EXPECT_SAME<field1s, fatal::map_get<traits::members, field1s>::name>();
  EXPECT_SAME<field2s, fatal::map_get<traits::members, field2s>::name>();
  EXPECT_SAME<field3s, fatal::map_get<traits::members, field3s>::name>();
  EXPECT_SAME<field4s, fatal::map_get<traits::members, field4s>::name>();
  EXPECT_SAME<field5s, fatal::map_get<traits::members, field5s>::name>();

  EXPECT_SAME<std::int32_t, fatal::map_get<traits::members, field0s>::type>();
  EXPECT_SAME<std::string, fatal::map_get<traits::members, field1s>::type>();
  EXPECT_SAME<enum1, fatal::map_get<traits::members, field2s>::type>();
  EXPECT_SAME<enum2, fatal::map_get<traits::members, field3s>::type>();
  EXPECT_SAME<union1, fatal::map_get<traits::members, field4s>::type>();
  EXPECT_SAME<union2, fatal::map_get<traits::members, field5s>::type>();

  EXPECT_SAME<field_id<1>, fatal::map_get<traits::members, field0s>::id>();
  EXPECT_SAME<field_id<2>, fatal::map_get<traits::members, field1s>::id>();
  EXPECT_SAME<field_id<3>, fatal::map_get<traits::members, field2s>::id>();
  EXPECT_SAME<field_id<4>, fatal::map_get<traits::members, field3s>::id>();
  EXPECT_SAME<field_id<5>, fatal::map_get<traits::members, field4s>::id>();
  EXPECT_SAME<field_id<6>, fatal::map_get<traits::members, field5s>::id>();

  EXPECT_SAME<
    required<apache::thrift::optionality::required>,
    fatal::map_get<traits::members, field0s>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::optional>,
    fatal::map_get<traits::members, field1s>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::required_of_writer>,
    fatal::map_get<traits::members, field2s>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::required>,
    fatal::map_get<traits::members, field3s>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::optional>,
    fatal::map_get<traits::members, field4s>::optional
  >();
  EXPECT_SAME<
    required<apache::thrift::optionality::required_of_writer>,
    fatal::map_get<traits::members, field5s>::optional
  >();

  EXPECT_EQ(
    98,
    (fatal::map_get<traits::members, traits::names::field0>::getter::ref(pod))
  );
  EXPECT_EQ(
    "test",
    (fatal::map_get<traits::members, traits::names::field1>::getter::ref(pod))
  );
  EXPECT_EQ(
    enum1::field0,
    (fatal::map_get<traits::members, traits::names::field2>::getter::ref(pod))
  );
  EXPECT_EQ(
    enum2::field2_2,
    (fatal::map_get<traits::members, traits::names::field3>::getter::ref(pod))
  );
  EXPECT_EQ(
    union1::Type::ui,
    (fatal::map_get<traits::members, traits::names::field4>::getter::ref(pod)
      .getType())
  );
  EXPECT_EQ(
    56,
    (fatal::map_get<traits::members, traits::names::field4>::getter::ref(pod)
      .get_ui())
  );
  EXPECT_EQ(
    union2::Type::ue_2,
    (fatal::map_get<traits::members, traits::names::field5>::getter::ref(pod)
      .getType())
  );
  EXPECT_EQ(
    enum1::field1,
    (fatal::map_get<traits::members, traits::names::field5>::getter::ref(pod)
      .get_ue_2())
  );

  EXPECT_SAME<
    apache::thrift::type_class::integral,
    fatal::map_get<traits::members, field0s>::type_class
  >();
  EXPECT_SAME<
    apache::thrift::type_class::string,
    fatal::map_get<traits::members, field1s>::type_class
  >();
  EXPECT_SAME<
    apache::thrift::type_class::enumeration,
    fatal::map_get<traits::members, field2s>::type_class
  >();
  EXPECT_SAME<
    apache::thrift::type_class::enumeration,
    fatal::map_get<traits::members, field3s>::type_class
  >();
  EXPECT_SAME<
    apache::thrift::type_class::variant,
    fatal::map_get<traits::members, field4s>::type_class
  >();
  EXPECT_SAME<
    apache::thrift::type_class::variant,
    fatal::map_get<traits::members, field5s>::type_class
  >();

  EXPECT_SAME<
    std::int32_t,
    decltype(
      std::declval<fatal::map_get<traits::members, field0s>::pod<>>().field0
    )
  >();
  EXPECT_SAME<
    std::string,
    decltype(
      std::declval<fatal::map_get<traits::members, field1s>::pod<>>().field1
    )
  >();
  EXPECT_SAME<
    enum1,
    decltype(
      std::declval<fatal::map_get<traits::members, field2s>::pod<>>().field2
    )
  >();
  EXPECT_SAME<
    enum2,
    decltype(
      std::declval<fatal::map_get<traits::members, field3s>::pod<>>().field3
    )
  >();
  EXPECT_SAME<
    union1,
    decltype(
      std::declval<fatal::map_get<traits::members, field4s>::pod<>>().field4
    )
  >();
  EXPECT_SAME<
    union2,
    decltype(
      std::declval<fatal::map_get<traits::members, field5s>::pod<>>().field5
    )
  >();

  EXPECT_SAME<
    bool,
    decltype(
      std::declval<fatal::map_get<traits::members, field0s>::pod<bool>>().field0
    )
  >();
  EXPECT_SAME<
    bool,
    decltype(
      std::declval<fatal::map_get<traits::members, field1s>::pod<bool>>().field1
    )
  >();
  EXPECT_SAME<
    bool,
    decltype(
      std::declval<fatal::map_get<traits::members, field2s>::pod<bool>>().field2
    )
  >();
  EXPECT_SAME<
    bool,
    decltype(
      std::declval<fatal::map_get<traits::members, field3s>::pod<bool>>().field3
    )
  >();
  EXPECT_SAME<
    bool,
    decltype(
      std::declval<fatal::map_get<traits::members, field4s>::pod<bool>>().field4
    )
  >();
  EXPECT_SAME<
    bool,
    decltype(
      std::declval<fatal::map_get<traits::members, field5s>::pod<bool>>().field5
    )
  >();

  EXPECT_SAME<
    traits::member<>::field0,
    fatal::map_get<traits::members, traits::names::field0>
  >();
  EXPECT_SAME<
    traits::member<>::field1,
    fatal::map_get<traits::members, traits::names::field1>
  >();
  EXPECT_SAME<
    traits::member<>::field2,
    fatal::map_get<traits::members, traits::names::field2>
  >();
  EXPECT_SAME<
    traits::member<>::field3,
    fatal::map_get<traits::members, traits::names::field3>
  >();
  EXPECT_SAME<
    traits::member<>::field4,
    fatal::map_get<traits::members, traits::names::field4>
  >();
  EXPECT_SAME<
    traits::member<>::field5,
    fatal::map_get<traits::members, traits::names::field5>
  >();
}

FATAL_S(structB_annotation1k, "multi_line_annotation");
FATAL_S(structB_annotation1v, "line one\nline two");
FATAL_S(structB_annotation2k, "some.annotation");
FATAL_S(structB_annotation2v, "this is its value");
FATAL_S(structB_annotation3k, "some.other.annotation");
FATAL_S(structB_annotation3v, "this is its other value");

TEST(fatal_struct, annotations) {
  EXPECT_SAME<
    fatal::map<>,
    apache::thrift::reflect_struct<struct1>::annotations::map
  >();

  EXPECT_SAME<
    fatal::map<>,
    apache::thrift::reflect_struct<struct2>::annotations::map
  >();

  EXPECT_SAME<
    fatal::map<>,
    apache::thrift::reflect_struct<struct3>::annotations::map
  >();

  EXPECT_SAME<
    fatal::map<>,
    apache::thrift::reflect_struct<structA>::annotations::map
  >();

  using structB_annotations = apache::thrift::reflect_struct<structB>
    ::annotations;

  EXPECT_SAME<
    structB_annotation1k,
    structB_annotations::keys::multi_line_annotation
  >();
  EXPECT_SAME<
    structB_annotation1v,
    structB_annotations::values::multi_line_annotation
  >();
  EXPECT_SAME<
    structB_annotation2k,
    structB_annotations::keys::some_annotation
  >();
  EXPECT_SAME<
    structB_annotation2v,
    structB_annotations::values::some_annotation
  >();
  EXPECT_SAME<
    structB_annotation3k,
    structB_annotations::keys::some_other_annotation
  >();
  EXPECT_SAME<
    structB_annotation3v,
    structB_annotations::values::some_other_annotation
  >();
  EXPECT_SAME<
    fatal::map<
      fatal::pair<structB_annotation1k, structB_annotation1v>,
      fatal::pair<structB_annotation2k, structB_annotation2v>,
      fatal::pair<structB_annotation3k, structB_annotation3v>
    >,
    structB_annotations::map
  >();

  EXPECT_SAME<
    fatal::map<>,
    apache::thrift::reflect_struct<structC>::annotations::map
  >();
}

FATAL_S(structBd_annotation1k, "another.annotation");
FATAL_S(structBd_annotation1v, "another value");
FATAL_S(structBd_annotation2k, "some.annotation");
FATAL_S(structBd_annotation2v, "some value");

TEST(fatal_struct, member_annotations) {
  using info = apache::thrift::reflect_struct<structB>;

  EXPECT_SAME<fatal::map<>, info::members_annotations::c::map>();
  EXPECT_SAME<
    fatal::map<>,
    fatal::map_get<info::members, info::names::c>::annotations::map
  >();

  using annotations_d = fatal::map_get<info::members, info::names::d>
    ::annotations;
  using expected_d_map = fatal::map<
    fatal::pair<structBd_annotation1k, structBd_annotation1v>,
    fatal::pair<structBd_annotation2k, structBd_annotation2v>
  >;

  EXPECT_SAME<expected_d_map, info::members_annotations::d::map>();
  EXPECT_SAME<expected_d_map, annotations_d::map>();

  EXPECT_SAME<
    structBd_annotation1k,
    info::members_annotations::d::keys::another_annotation
  >();
  EXPECT_SAME<
    structBd_annotation1v,
    info::members_annotations::d::values::another_annotation
  >();
  EXPECT_SAME<
    structBd_annotation2k,
    info::members_annotations::d::keys::some_annotation
  >();
  EXPECT_SAME<
    structBd_annotation2v,
    info::members_annotations::d::values::some_annotation
  >();

  EXPECT_SAME<
    structBd_annotation1k,
    annotations_d::keys::another_annotation
  >();
  EXPECT_SAME<
    structBd_annotation1v,
    annotations_d::values::another_annotation
  >();
  EXPECT_SAME<
    structBd_annotation2k,
    annotations_d::keys::some_annotation
  >();
  EXPECT_SAME<
    structBd_annotation2v,
    annotations_d::values::some_annotation
  >();
}

TEST(fatal_struct, set_methods) {
  using info = apache::thrift::reflect_struct<struct4>;
  using req_field = fatal::map_get<info::members, info::names::field0>;
  using opt_field = fatal::map_get<info::members, info::names::field1>;
  using def_field = fatal::map_get<info::members, info::names::field2>;

  using ref_field = fatal::map_get<info::members, info::names::field3>;

  struct4 a;
  EXPECT_EQ(true, req_field::is_set(a));
  req_field::mark_set(a);
  EXPECT_EQ(true, req_field::is_set(a));
  // can't test that req_field::unmark_set doesn't compile,
  // but trust me, it shoudln't
  // req_field::unmark_set(a);

  EXPECT_EQ(false, opt_field::is_set(a));
  opt_field::mark_set(a);
  EXPECT_EQ(true, opt_field::is_set(a));
  opt_field::unmark_set(a);
  EXPECT_EQ(false, opt_field::is_set(a));

  EXPECT_EQ(false, def_field::is_set(a));
  def_field::mark_set(a);
  EXPECT_EQ(true, def_field::is_set(a));
  def_field::unmark_set(a);
  EXPECT_EQ(false, def_field::is_set(a));

  EXPECT_EQ(true, ref_field::is_set(a));
  ref_field::mark_set(a);
  EXPECT_EQ(true, req_field::is_set(a));
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {
