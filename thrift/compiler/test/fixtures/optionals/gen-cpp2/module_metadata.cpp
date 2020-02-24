/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include <thrift/lib/cpp2/gen/module_metadata_cpp.h>
#include "thrift/compiler/test/fixtures/optionals/gen-cpp2/module_metadata.h"

namespace apache {
namespace thrift {
namespace detail {
namespace md {
using ThriftMetadata = ::apache::thrift::metadata::ThriftMetadata;
using ThriftPrimitiveType = ::apache::thrift::metadata::ThriftPrimitiveType;
using ThriftType = ::apache::thrift::metadata::ThriftType;
using ThriftService = ::apache::thrift::metadata::ThriftService;
using ThriftServiceContext = ::apache::thrift::metadata::ThriftServiceContext;
using ThriftFunctionGenerator = void (*)(ThriftMetadata&, ThriftService&);

void EnumMetadata<::cpp2::Animal>::gen(ThriftMetadata& metadata) {
  auto res = metadata.enums.emplace("module.Animal", ::apache::thrift::metadata::ThriftEnum{});
  if (!res.second) {
    return;
  }
  ::apache::thrift::metadata::ThriftEnum& enum_metadata = res.first->second;
  enum_metadata.name = "module.Animal";
  for (const auto& p : ::cpp2::_Animal_VALUES_TO_NAMES) {
    enum_metadata.elements.emplace(static_cast<int32_t>(p.first), p.second) ;
  }
}

void StructMetadata<::cpp2::Color>::gen(ThriftMetadata& metadata) {
  auto res = metadata.structs.emplace("module.Color", ::apache::thrift::metadata::ThriftStruct{});
  if (!res.second) {
    return;
  }
  ::apache::thrift::metadata::ThriftStruct& module_Color = res.first->second;
  module_Color.name = "module.Color";
  module_Color.is_union = false;
  static const std::tuple<int32_t, const char*, bool, std::unique_ptr<MetadataTypeInterface>>
  module_Color_fields[] = {
    std::make_tuple(1, "red", false, std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_DOUBLE_TYPE)),
    std::make_tuple(2, "green", false, std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_DOUBLE_TYPE)),
    std::make_tuple(3, "blue", false, std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_DOUBLE_TYPE)),
    std::make_tuple(4, "alpha", false, std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_DOUBLE_TYPE)),
  };
  for (const auto& f : module_Color_fields) {
    ::apache::thrift::metadata::ThriftField field;
    field.id = std::get<0>(f);
    field.name = std::get<1>(f);
    field.is_optional = std::get<2>(f);
    std::get<3>(f)->initialize(field.type);
    module_Color.fields.push_back(std::move(field));
  }
}
void StructMetadata<::cpp2::Vehicle>::gen(ThriftMetadata& metadata) {
  auto res = metadata.structs.emplace("module.Vehicle", ::apache::thrift::metadata::ThriftStruct{});
  if (!res.second) {
    return;
  }
  ::apache::thrift::metadata::ThriftStruct& module_Vehicle = res.first->second;
  module_Vehicle.name = "module.Vehicle";
  module_Vehicle.is_union = false;
  static const std::tuple<int32_t, const char*, bool, std::unique_ptr<MetadataTypeInterface>>
  module_Vehicle_fields[] = {
    std::make_tuple(1, "color", false, std::make_unique<Struct< ::cpp2::Color>>("module.Color", metadata)),
    std::make_tuple(2, "licensePlate", true, std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_STRING_TYPE)),
    std::make_tuple(3, "description", true, std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_STRING_TYPE)),
    std::make_tuple(4, "name", true, std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_STRING_TYPE)),
    std::make_tuple(5, "hasAC", true, std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_BOOL_TYPE)),
  };
  for (const auto& f : module_Vehicle_fields) {
    ::apache::thrift::metadata::ThriftField field;
    field.id = std::get<0>(f);
    field.name = std::get<1>(f);
    field.is_optional = std::get<2>(f);
    std::get<3>(f)->initialize(field.type);
    module_Vehicle.fields.push_back(std::move(field));
  }
}
void StructMetadata<::cpp2::Person>::gen(ThriftMetadata& metadata) {
  auto res = metadata.structs.emplace("module.Person", ::apache::thrift::metadata::ThriftStruct{});
  if (!res.second) {
    return;
  }
  ::apache::thrift::metadata::ThriftStruct& module_Person = res.first->second;
  module_Person.name = "module.Person";
  module_Person.is_union = false;
  static const std::tuple<int32_t, const char*, bool, std::unique_ptr<MetadataTypeInterface>>
  module_Person_fields[] = {
    std::make_tuple(1, "id", false, std::make_unique<Typedef>("module.PersonID", std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_I64_TYPE))),
    std::make_tuple(2, "name", false, std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_STRING_TYPE)),
    std::make_tuple(3, "age", true, std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_I16_TYPE)),
    std::make_tuple(4, "address", true, std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_STRING_TYPE)),
    std::make_tuple(5, "favoriteColor", true, std::make_unique<Struct< ::cpp2::Color>>("module.Color", metadata)),
    std::make_tuple(6, "friends", true, std::make_unique<Set>(std::make_unique<Typedef>("module.PersonID", std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_I64_TYPE)))),
    std::make_tuple(7, "bestFriend", true, std::make_unique<Typedef>("module.PersonID", std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_I64_TYPE))),
    std::make_tuple(8, "petNames", true, std::make_unique<Map>(std::make_unique<Enum< ::cpp2::Animal>>("module.Animal", metadata), std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_STRING_TYPE))),
    std::make_tuple(9, "afraidOfAnimal", true, std::make_unique<Enum< ::cpp2::Animal>>("module.Animal", metadata)),
    std::make_tuple(10, "vehicles", true, std::make_unique<List>(std::make_unique<Struct< ::cpp2::Vehicle>>("module.Vehicle", metadata))),
  };
  for (const auto& f : module_Person_fields) {
    ::apache::thrift::metadata::ThriftField field;
    field.id = std::get<0>(f);
    field.name = std::get<1>(f);
    field.is_optional = std::get<2>(f);
    std::get<3>(f)->initialize(field.type);
    module_Person.fields.push_back(std::move(field));
  }
}

} // namespace md
} // namespace detail
} // namespace thrift
} // namespace apache