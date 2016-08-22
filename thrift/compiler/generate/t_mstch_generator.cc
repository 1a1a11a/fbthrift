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

#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/common.h>
#include <thrift/compiler/generate/t_generator.h>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <mstch/mstch.hpp>

t_mstch_generator::t_mstch_generator(
    t_program* program,
    boost::filesystem::path template_prefix)
    : t_generator(program), template_dir_(g_template_dir) {
  if (this->template_dir_ == "") {
    std::string s = "Must set template directory when using mstch generator";
    throw std::runtime_error{s};
  }

  this->gen_template_map(template_prefix);
}

mstch::map t_mstch_generator::dump(const t_program& program) const {
  mstch::map result{
      {"name", program.get_name()},
      {"path", program.get_path()},
      {"outPath", program.get_out_path()},
      {"namespace", program.get_namespace()},
      {"includePrefix", program.get_include_prefix()},
      {"structs", this->dump_vector(program.get_objects())},
      {"enums", this->dump_vector(program.get_enums())},
      {"services", this->dump_vector(program.get_services())},
      {"typedefs", this->dump_vector(program.get_typedefs())},
      {"constants", this->dump_vector(program.get_consts())},
  };

  mstch::map extension = this->extend_program(program);
  result.insert(extension.begin(), extension.end());
  return this->prepend_prefix("program", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_struct& strct, bool shallow) const {
  mstch::map result{
      {"name", strct.get_name()},
      {"fields",
       shallow ? static_cast<mstch::node>(false)
               : this->dump_vector(strct.get_members())},
      {"exception?", strct.is_xception()},
      {"union?", strct.is_union()},
      {"plain?", !strct.is_xception() && !strct.is_union()},
  };

  mstch::map extension = this->extend_struct(strct);
  result.insert(extension.begin(), extension.end());
  return this->prepend_prefix("struct", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_field& field) const {
  auto req = field.get_req();
  mstch::map result{
      {"name", field.get_name()},
      {"key", std::to_string(field.get_key())},
      {"type", this->dump(*field.get_type())},
      {"required?", req == t_field::e_req::T_REQUIRED},
      {"optional?", req == t_field::e_req::T_OPTIONAL},
      {"optInReqOut?", req == t_field::e_req::T_OPT_IN_REQ_OUT},
  };

  mstch::map extension = this->extend_field(field);
  result.insert(extension.begin(), extension.end());
  return this->prepend_prefix("field", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_type& type) const {
  mstch::map result{
      {"name", type.get_name()},

      {"void?", type.is_void()},
      {"string?", type.is_string() && !type.is_binary()},
      {"binary?", type.is_string() && type.is_binary()},
      {"bool?", type.is_bool()},
      {"byte?", type.is_byte()},
      {"i16?", type.is_i16()},
      {"i32?", type.is_i32()},
      {"i64?", type.is_i64()},
      {"double?", type.is_double()},
      {"float?", type.is_float()},
      {"struct?", type.is_struct() || type.is_xception()},
      {"enum?", type.is_enum()},
      {"stream?", type.is_stream()},
      {"service?", type.is_service()},
      {"list?", type.is_list()},
      {"set?", type.is_set()},
      {"map?", type.is_map()},
      {"typedef?", type.is_typedef()},
  };

  if (type.is_struct() || type.is_xception()) {
    // Shallow dump the struct
    result.emplace(
        "struct", this->dump(dynamic_cast<const t_struct&>(type), true));
  } else if (type.is_enum()) {
    result.emplace("enum", this->dump(dynamic_cast<const t_enum&>(type)));
  } else if (type.is_service()) {
    result.emplace("service", this->dump(dynamic_cast<const t_service&>(type)));
  } else if (type.is_list()) {
    result.emplace(
        "listElemType",
        this->dump(*dynamic_cast<const t_list&>(type).get_elem_type()));
  } else if (type.is_stream()) {
    result.emplace(
        "streamElemType",
        this->dump(*dynamic_cast<const t_stream&>(type).get_elem_type()));
  } else if (type.is_set()) {
    result.emplace(
        "setElemType",
        this->dump(*dynamic_cast<const t_set&>(type).get_elem_type()));
  } else if (type.is_map()) {
    result.emplace(
        "keyType",
        this->dump(*dynamic_cast<const t_map&>(type).get_key_type()));
    result.emplace(
        "valueType",
        this->dump(*dynamic_cast<const t_map&>(type).get_val_type()));
  } else if (type.is_typedef()) {
    result.emplace(
        "typedefType",
        this->dump(*dynamic_cast<const t_typedef&>(type).get_type()));
  }

  mstch::map extension = this->extend_type(type);
  result.insert(extension.begin(), extension.end());
  return this->prepend_prefix("type", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_enum& enm) const {
  mstch::map result{
      {"name", enm.get_name()},
      {"values", this->dump_vector(enm.get_constants())},
  };

  mstch::map extension = this->extend_enum(enm);
  result.insert(extension.begin(), extension.end());
  return this->prepend_prefix("enum", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_enum_value& val) const {
  mstch::map result{
      {"name", val.get_name()}, {"value", std::to_string(val.get_value())},
  };

  mstch::map extension = this->extend_enum_value(val);
  result.insert(extension.begin(), extension.end());
  return this->prepend_prefix("value", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_service& service) const {
  t_service* extends = service.get_extends();
  mstch::map result{
      {"name", service.get_name()},
      {"functions", this->dump_vector(service.get_functions())},
      {"extends?", extends != nullptr},
      {"extends",
       extends ? static_cast<mstch::node>(this->dump(*extends)) : false},
  };

  mstch::map extension = this->extend_service(service);
  result.insert(extension.begin(), extension.end());
  return this->prepend_prefix("service", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_function& function) const {
  mstch::map result{
      {"name", function.get_name()},
      {"oneway?", function.is_oneway()},
      {"returnType", this->dump(*function.get_returntype())},
      {"exceptions", this->dump(*function.get_xceptions())},
      {"args", this->dump_vector(function.get_arglist()->get_members())}};

  mstch::map extension = this->extend_function(function);
  result.insert(extension.begin(), extension.end());
  return this->prepend_prefix("function", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_const& cnst) const {
  mstch::map result{
      {"type", this->dump(*cnst.get_type())},
      {"name", cnst.get_name()},
      {"value", this->dump(*cnst.get_value())},
  };

  mstch::map extension = this->extend_const(cnst);
  result.insert(extension.begin(), extension.end());
  return this->prepend_prefix("constant", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_const_value& value) const {
  using cv = t_const_value::t_const_value_type;
  const cv type = value.get_type();
  mstch::map result{
      {"double?", type == cv::CV_DOUBLE},
      {"integer?", type == cv::CV_INTEGER},
      {"string?", type == cv::CV_STRING},
      {"map?", type == cv::CV_MAP},
      {"list?", type == cv::CV_LIST},
  };

  switch (type) {
    case cv::CV_DOUBLE:
      result.emplace("doubleValue", std::to_string(value.get_double()));
      result.emplace("nonzero?", value.get_double() != 0.0);
      break;
    case cv::CV_INTEGER:
      result.emplace("integerValue", std::to_string(value.get_integer()));
      result.emplace("nonzero?", value.get_integer() != 0);
      break;
    case cv::CV_STRING:
      result.emplace("stringValue", value.get_string());
      break;
    case cv::CV_MAP:
      result.emplace("mapElements", this->dump_vector(value.get_map()));
      break;
    case cv::CV_LIST:
      result.emplace("listElements", this->dump_vector(value.get_list()));
      break;
    default:
      std::ostringstream err;
      err << "Unhandled t_const_value_type " << value.get_type();
      throw std::domain_error{err.str()};
  }

  mstch::map extension = this->extend_const_value(value);
  result.insert(extension.begin(), extension.end());
  return this->prepend_prefix("value", std::move(result));
}

mstch::map t_mstch_generator::dump(
    const std::map<t_const_value*, t_const_value*>::value_type& pair) const {
  mstch::map result{
      {"key", this->dump(*pair.first)}, {"value", this->dump(*pair.second)},
  };
  mstch::map extension = this->extend_const_value_map_elem(pair);
  result.insert(extension.begin(), extension.end());
  return this->prepend_prefix("element", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_typedef& typdef) const {
  mstch::map result{
      {"symbolic", typdef.get_symbolic()}, {"type", typdef.get_type()},
  };

  mstch::map extension = this->extend_typedef(typdef);
  result.insert(extension.begin(), extension.end());
  return this->prepend_prefix("typedef", std::move(result));
}

// Extenders, by default do no extending

mstch::map t_mstch_generator::extend_program(const t_program&) const {
  return {};
}

mstch::map t_mstch_generator::extend_struct(const t_struct&) const {
  return {};
}

mstch::map t_mstch_generator::extend_field(const t_field&) const {
  return {};
}

mstch::map t_mstch_generator::extend_type(const t_type&) const {
  return {};
}

mstch::map t_mstch_generator::extend_enum(const t_enum&) const {
  return {};
}

mstch::map t_mstch_generator::extend_enum_value(const t_enum_value&) const {
  return {};
}

mstch::map t_mstch_generator::extend_service(const t_service&) const {
  return {};
}

mstch::map t_mstch_generator::extend_function(const t_function&) const {
  return {};
}

mstch::map t_mstch_generator::extend_typedef(const t_typedef&) const {
  return {};
}

mstch::map t_mstch_generator::extend_const(const t_const&) const {
  return {};
}

mstch::map t_mstch_generator::extend_const_value(const t_const_value&) const {
  return {};
}

mstch::map t_mstch_generator::extend_const_value_map_elem(
    const std::map<t_const_value*, t_const_value*>::value_type&) const {
  return {};
}

void t_mstch_generator::gen_template_map(
    const boost::filesystem::path& prefix) {
  auto template_dir = this->template_dir_ / prefix;
  this->template_map_ = {};
  for (auto& elem : boost::filesystem::directory_iterator(template_dir)) {
    if (boost::filesystem::is_regular_file(elem.path()) &&
        boost::filesystem::extension(elem.path()) == ".mustache") {
      std::ifstream ifs{elem.path().string()};
      auto tpl = std::string{std::istreambuf_iterator<char>(ifs),
                             std::istreambuf_iterator<char>()};

      this->template_map_.emplace(elem.path().stem().string(), std::move(tpl));
    }
  }
}
