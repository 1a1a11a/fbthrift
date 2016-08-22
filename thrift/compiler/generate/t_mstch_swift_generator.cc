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

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>

class t_mstch_swift_generator : public t_mstch_generator {
 public:
  t_mstch_swift_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& option_string)
      : t_mstch_generator(program, "java/swift") {
    this->out_dir_base_ = "gen-swift";
  }

  void generate_program() override;

 protected:
  /*
   * Generate multiple Java items according to the given template. Writes
   * output to package_dir underneath the global output directory.
   */
  template <typename T>
  void generate_items(
      const std::string& tpl_path,
      const std::vector<T*>& items) {
    auto& tpl = this->get_template(tpl_path);
    for (const T* item : items) {
      auto package_dir =
          package_to_path(item->get_program()->get_namespace("java.swift"));
      auto filename = this->mangle_java_name(item->get_name(), true) + ".java";
      auto rendered_item =
          mstch::render(tpl, this->dump(*item), this->get_template_map());
      this->write_output(package_dir / filename, rendered_item);
    }
  }

  void generate_constants(const t_program& prog) {
    if (prog.get_consts().empty()) {
      // Only generate Constants.java if we actually have constants
      return;
    }
    auto package_dir = package_to_path(prog.get_namespace("java.swift"));
    auto rendered = mstch::render(
        this->get_template("Constants"),
        this->dump(prog),
        this->get_template_map());
    this->write_output(package_dir / "Constants.java", rendered);
  }

  mstch::map extend_program(const t_program& program) const override {
    // Sort constant members to match java swift generator
    auto constants = program.get_consts();
    std::sort(
        constants.begin(),
        constants.end(),
        [](const t_const* x, const t_const* y) {
          return std::less<std::string>{}(x->get_name(), y->get_name());
        });
    return mstch::map{
        {"javaPackage", program.get_namespace("java.swift")},
        {"sortedConstants", this->dump_vector(constants)},
    };
  }

  mstch::map extend_struct(const t_struct& strct) const override {
    mstch::map result{
        {"javaPackage", strct.get_program()->get_namespace("java.swift")},
    };
    this->add_java_names(result, strct.get_name());
    return result;
  }

  mstch::map extend_service(const t_service& service) const override {
    mstch::map result{
        {"javaPackage", service.get_program()->get_namespace("java.swift")},
    };
    this->add_java_names(result, service.get_name());
    return result;
  }

  mstch::map extend_function(const t_function& func) const override {
    mstch::map result{};
    this->add_java_names(result, func.get_name());
    return result;
  }

  mstch::map extend_field(const t_field& field) const override {
    mstch::map result{};
    this->add_java_names(result, field.get_name());
    return result;
  }

  mstch::map extend_enum(const t_enum& enm) const override {
    mstch::map result{
        {"javaPackage", enm.get_program()->get_namespace("java.swift")},
    };
    this->add_java_names(result, enm.get_name());
    return result;
  }

  mstch::map extend_enum_value(const t_enum_value& value) const override {
    mstch::map result{};
    this->add_java_names(result, value.get_name());
    return result;
  }

  /**
   * Extend types to have a field set if the corresponding Java type has
   * a primitive representation. We need this because these types are treated
   * differently when they are arguments to type constructors in Java.
   */
  mstch::map extend_type(const t_type& type) const override {
    return mstch::map{
        {"primitive?",
         type.is_void() || type.is_bool() || type.is_byte() || type.is_i16() ||
             type.is_i32() || type.is_i64() || type.is_double() ||
             type.is_float()},
    };
  }

  /** Static Helpers **/

  /**
   * Extends the map to contain elements for the java-mangled versions
   * of a Thrift identifier.
   * @modifies - map
   */
  static void add_java_names(mstch::map& map, const std::string& rawName) {
    map.emplace("javaName", mangle_java_name(rawName, false));
    map.emplace("javaCapitalName", mangle_java_name(rawName, true));
    map.emplace("javaConstantName", mangle_java_constant_name(rawName));
  }

  /**
   * Mangles an identifier for use in generated Java. Ported from
   * TemplateContextGenerator.java::mangleJavaName
   * from the java implementation of the swift generator.
   * http://tinyurl.com/z7vocup
   */
  static std::string mangle_java_name(const std::string& ref, bool capitalize) {
    std::ostringstream res;
    bool upcase = capitalize;
    bool acronym =
        ref.size() > 1 && std::isupper(ref[0]) && std::isupper(ref[1]);
    bool downcase = !capitalize && !acronym;
    for (typename std::string::size_type i = 0; i < ref.size(); ++i) {
      if (ref[i] == '_') {
        upcase = true;
        continue;
      } else {
        char ch = ref[i];
        ch = downcase ? std::tolower(ch) : ch;
        ch = upcase ? std::toupper(ch) : ch;
        res << ch;
        upcase = false;
        downcase = false;
      }
    }
    return res.str();
  }

  /**
   * Mangles an identifier for use in generated Java as a constant.
   * Ported from TemplateContextGenerator.java::mangleJavaConstantName
   * from the java implementation of the swift generator.
   * http://tinyurl.com/z7vocup
   */
  static std::string mangle_java_constant_name(const std::string& ref) {
    std::ostringstream res;
    bool lowercase = false;
    for (typename std::string::size_type i = 0; i < ref.size(); ++i) {
      char ch = ref[i];
      if (std::isupper(ch)) {
        if (lowercase) {
          res << '_';
        }
        res << static_cast<char>(std::toupper(ch));
        lowercase = false;
      } else if (std::islower(ch)) {
        res << static_cast<char>(std::toupper(ch));
        lowercase = true;
      } else {
        // Not a letter, just emit it
        res << ch;
      }
    }
    return res.str();
  }

  /**
   * Converts a java package string to the path containing the source for
   * that package. Example: "foo.bar.baz" -> "foo/bar/baz"
   */
  boost::filesystem::path package_to_path(std::string package) {
    if (package.empty()) {
      throw std::runtime_error{
          "No Java package specified and no default given"};
    }
    if (boost::algorithm::contains(package, "/")) {
      std::ostringstream err;
      err << "\"" << package << "\" is not a valid Java package name";
      throw std::runtime_error{err.str()};
    }
    boost::algorithm::replace_all(package, ".", "/");
    return boost::filesystem::path{package};
  }
};

void t_mstch_swift_generator::generate_program() {
  // disable mstch escaping
  mstch::config::escape = [](const std::string s) { return s; };

  // Load templates
  auto& templates = this->get_template_map();

  this->generate_items("Object", this->get_program()->get_objects());

  this->generate_items("Service", this->get_program()->get_services());

  this->generate_items("Enum", this->get_program()->get_enums());

  this->generate_constants(*this->get_program());
}

THRIFT_REGISTER_GENERATOR(mstch_swift, "Java Swift", "");
