// Copyright 2026 Maik Knof
// SPDX-License-Identifier: Apache-2.0

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <ament_index_cpp/get_resource.hpp>
#include <ament_index_cpp/get_resources.hpp>
#include <class_loader/class_loader.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/node_factory.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cxxabi.h>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include "filter_component_base/ros/filter_component_base.hpp"

namespace
{

using FilterComponentBase = filter_component_base::ros::FilterComponentBase;
using ComponentResource = std::pair<std::string, std::string>;

struct CliOptions
{
  bool all{false};
  bool pretty{true};
  bool include_parameters{true};
  std::vector<std::string> packages;
};

struct FilterExport
{
  std::string package;
  std::string filter;
  std::string kind{"filter"};
  std::string chain_data_type;
};

struct ErrorEntry
{
  std::string package;
  std::string filter;
  std::string component_class;
  std::string message;
};

std::string usage()
{
  return R"(Inspect installed C++ filter components and print JSON metadata.

Usage:
  inspect_filter_components --all [options]
  inspect_filter_components --packages-select PKG [PKG ...] [options]
  inspect_filter_components PKG [PKG ...] [options]

Selection:
  --all
      Inspect every installed package that exports C++ filter components through
      package.xml <filter_component filter="..."> entries.

  --packages-select PKG [PKG ...]
      Inspect only the listed packages. Like colcon, package names are read until
      the next option. Positional package names are also accepted.

  PKG [PKG ...]
      Direct package input, equivalent to --packages-select.

Options:
  --no-parameters
      Omit declared ROS parameters from the JSON output.

  --compact
      Emit compact single-line JSON instead of pretty-printed JSON.

  -h, --help
      Show this help text.

Notes:
  The tool uses package.xml only to identify which filters a package exports.
  Port descriptors, shared-memory key descriptors, and parameter descriptors are
  read by loading each rclcpp component, constructing it, inspecting the
  FilterComponentBase object, and destroying it without configuring or activating
  the lifecycle node.
)";
}

bool startsWith(const std::string & value, const std::string & prefix)
{
  return value.rfind(prefix, 0) == 0;
}

void debugLog(const std::string & message)
{
  if (std::getenv("FILTER_COMPONENT_INSPECT_DEBUG") != nullptr) {
    std::cerr << "[inspect_filter_components] " << message << "\n";
  }
}

class StdoutSilencer
{
public:
  StdoutSilencer()
  {
    std::fflush(stdout);
    saved_stdout_ = dup(STDOUT_FILENO);
    const int null_fd = open("/dev/null", O_WRONLY);
    if (saved_stdout_ >= 0 && null_fd >= 0) {
      active_ = dup2(null_fd, STDOUT_FILENO) >= 0;
    }
    if (null_fd >= 0) {
      close(null_fd);
    }
  }

  ~StdoutSilencer()
  {
    restore();
  }

  void restore()
  {
    if (active_) {
      std::fflush(stdout);
      dup2(saved_stdout_, STDOUT_FILENO);
      active_ = false;
    }
    if (saved_stdout_ >= 0) {
      close(saved_stdout_);
      saved_stdout_ = -1;
    }
  }

private:
  int saved_stdout_{-1};
  bool active_{false};
};

CliOptions parseArgs(int argc, char ** argv)
{
  CliOptions options;
  for (int index = 1; index < argc; ++index) {
    const std::string arg = argv[index];
    if (arg == "-h" || arg == "--help") {
      std::cout << usage();
      std::exit(0);
    }
    if (arg == "--all") {
      options.all = true;
      continue;
    }
    if (arg == "--compact") {
      options.pretty = false;
      continue;
    }
    if (arg == "--no-parameters") {
      options.include_parameters = false;
      continue;
    }
    if (arg == "--packages-select") {
      while (index + 1 < argc) {
        const std::string next = argv[index + 1];
        if (startsWith(next, "-")) {
          break;
        }
        options.packages.push_back(next);
        ++index;
      }
      if (options.packages.empty()) {
        throw std::runtime_error("--packages-select requires at least one package name");
      }
      continue;
    }
    if (startsWith(arg, "-")) {
      throw std::runtime_error("Unknown option: " + arg);
    }
    options.packages.push_back(arg);
  }

  std::sort(options.packages.begin(), options.packages.end());
  options.packages.erase(std::unique(options.packages.begin(), options.packages.end()), options.packages.end());
  if (!options.all && options.packages.empty()) {
    throw std::runtime_error("No packages selected. Use --all, --packages-select, or positional package names.");
  }
  if (options.all && !options.packages.empty()) {
    throw std::runtime_error("--all cannot be combined with package selections");
  }
  return options;
}

std::string readFile(const std::string & path)
{
  std::ifstream stream(path);
  if (!stream) {
    throw std::runtime_error("Failed to open " + path);
  }
  std::ostringstream buffer;
  buffer << stream.rdbuf();
  return buffer.str();
}

std::map<std::string, std::string> parseAttributes(const std::string & text)
{
  std::map<std::string, std::string> attributes;
  const std::regex attr_regex{R"ATTR(([A-Za-z_][A-Za-z0-9_]*)\s*=\s*"([^"]*)")ATTR"};
  for (auto iter = std::sregex_iterator(text.begin(), text.end(), attr_regex);
    iter != std::sregex_iterator();
    ++iter)
  {
    attributes[(*iter)[1].str()] = (*iter)[2].str();
  }
  return attributes;
}

std::vector<FilterExport> packageFilterExports(const std::string & package)
{
  const auto share_dir = ament_index_cpp::get_package_share_directory(package);
  const auto xml = readFile(share_dir + "/package.xml");
  const std::regex tag_regex{R"(<filter_component\s+([^>]*)/?>)"};
  std::vector<FilterExport> filters;
  for (auto iter = std::sregex_iterator(xml.begin(), xml.end(), tag_regex);
    iter != std::sregex_iterator();
    ++iter)
  {
    const auto attributes = parseAttributes((*iter)[1].str());
    const auto filter_iter = attributes.find("filter");
    if (filter_iter == attributes.end() || filter_iter->second.empty()) {
      continue;
    }
    const auto kind_iter = attributes.find("kind");
    const auto chain_iter = attributes.find("chain_data_type");
    filters.push_back(
      FilterExport{
        package,
        filter_iter->second,
        kind_iter == attributes.end() || kind_iter->second.empty() ? "filter" : kind_iter->second,
        chain_iter == attributes.end() ? "" : chain_iter->second});
  }
  return filters;
}

std::vector<std::string> allFilterPackages()
{
  std::vector<std::string> packages;
  for (const auto & [package, prefix] : ament_index_cpp::get_resources("packages")) {
    (void)prefix;
    try {
      if (!packageFilterExports(package).empty()) {
        packages.push_back(package);
      }
    } catch (const std::exception &) {
      continue;
    }
  }
  std::sort(packages.begin(), packages.end());
  return packages;
}

std::string jsonEscape(const std::string & value)
{
  std::ostringstream out;
  for (const auto ch : value) {
    switch (ch) {
      case '\\':
        out << "\\\\";
        break;
      case '"':
        out << "\\\"";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (static_cast<unsigned char>(ch) < 0x20U) {
          out << "\\u00";
          const char * digits = "0123456789ABCDEF";
          out << digits[(static_cast<unsigned char>(ch) >> 4U) & 0x0FU];
          out << digits[static_cast<unsigned char>(ch) & 0x0FU];
        } else {
          out << ch;
        }
        break;
    }
  }
  return out.str();
}

std::string quoted(const std::string & value)
{
  return "\"" + jsonEscape(value) + "\"";
}

std::string demangle(const std::string & name)
{
  int status = 0;
  std::unique_ptr<char, void (*)(void *)> demangled{
    abi::__cxa_demangle(name.c_str(), nullptr, nullptr, &status),
    std::free};
  return status == 0 && demangled ? std::string{demangled.get()} : name;
}

std::string displayTypeName(const std::string & name)
{
  return name.empty() ? name : demangle(name);
}

std::string accessToString(FilterComponentBase::ShmAccess access)
{
  return access == FilterComponentBase::ShmAccess::ReadWrite ? "rw" : "r";
}

std::string joinPortString(const std::vector<FilterComponentBase::PortDescriptor> & ports)
{
  std::ostringstream out;
  for (size_t index = 0; index < ports.size(); ++index) {
    if (index > 0U) {
      out << ",";
    }
    out << ports[index].name << ":" << displayTypeName(ports[index].type_name);
  }
  return out.str();
}

std::string joinTypeString(const std::vector<FilterComponentBase::PortDescriptor> & ports)
{
  std::ostringstream out;
  for (size_t index = 0; index < ports.size(); ++index) {
    if (index > 0U) {
      out << ",";
    }
    out << displayTypeName(ports[index].type_name);
  }
  return out.str();
}

std::string indent(size_t level, bool pretty)
{
  return pretty ? std::string(level * 2U, ' ') : "";
}

std::string newline(bool pretty)
{
  return pretty ? "\n" : "";
}

class JsonWriter
{
public:
  explicit JsonWriter(bool pretty)
  : pretty_(pretty)
  {
  }

  void keyValue(std::ostream & out, const std::string & key, const std::string & value, bool comma, size_t level) const
  {
    out << indent(level, pretty_) << quoted(key) << ":" << (pretty_ ? " " : "") << quoted(value);
    if (comma) {
      out << ",";
    }
    out << newline(pretty_);
  }

  void portArray(
    std::ostream & out,
    const std::string & key,
    const std::vector<FilterComponentBase::PortDescriptor> & ports,
    bool comma,
    size_t level) const
  {
    out << indent(level, pretty_) << quoted(key) << ":" << (pretty_ ? " " : "") << "[" << newline(pretty_);
    for (size_t index = 0; index < ports.size(); ++index) {
      const auto & port = ports[index];
      out << indent(level + 1U, pretty_) << "{" << newline(pretty_);
      keyValue(out, "name", port.name, true, level + 2U);
      keyValue(out, "type", displayTypeName(port.type_name), true, level + 2U);
      keyValue(out, "description", port.description, false, level + 2U);
      out << indent(level + 1U, pretty_) << "}";
      if (index + 1U < ports.size()) {
        out << ",";
      }
      out << newline(pretty_);
    }
    out << indent(level, pretty_) << "]";
    if (comma) {
      out << ",";
    }
    out << newline(pretty_);
  }

  void shmArray(
    std::ostream & out,
    const std::vector<FilterComponentBase::ShmKeyDescriptor> & keys,
    bool comma,
    size_t level) const
  {
    out << indent(level, pretty_) << quoted("shm_keys") << ":" << (pretty_ ? " " : "") << "[" << newline(pretty_);
    for (size_t index = 0; index < keys.size(); ++index) {
      const auto & key = keys[index];
      out << indent(level + 1U, pretty_) << "{" << newline(pretty_);
      keyValue(out, "name", key.name, true, level + 2U);
      keyValue(out, "type", key.type_name, true, level + 2U);
      keyValue(out, "access", accessToString(key.access), true, level + 2U);
      keyValue(out, "description", key.description, false, level + 2U);
      out << indent(level + 1U, pretty_) << "}";
      if (index + 1U < keys.size()) {
        out << ",";
      }
      out << newline(pretty_);
    }
    out << indent(level, pretty_) << "]";
    if (comma) {
      out << ",";
    }
    out << newline(pretty_);
  }

  void parameterObject(std::ostream & out, const FilterComponentBase & node, bool comma, size_t level) const
  {
    auto names = node.list_parameters({}, 10).names;
    std::sort(names.begin(), names.end());
    out << indent(level, pretty_) << quoted("parameters") << ":" << (pretty_ ? " " : "") << "{" << newline(pretty_);
    for (size_t index = 0; index < names.size(); ++index) {
      const auto & name = names[index];
      const auto descriptor = node.describe_parameter(name);
      rclcpp::Parameter parameter;
      (void)node.get_parameter(name, parameter);
      out << indent(level + 1U, pretty_) << quoted(name) << ":" << (pretty_ ? " " : "") << "{" << newline(pretty_);
      keyValue(out, "type", parameter.get_type_name(), true, level + 2U);
      keyValue(out, "default", parameter.value_to_string(), true, level + 2U);
      keyValue(out, "description", descriptor.description, false, level + 2U);
      out << indent(level + 1U, pretty_) << "}";
      if (index + 1U < names.size()) {
        out << ",";
      }
      out << newline(pretty_);
    }
    out << indent(level, pretty_) << "}";
    if (comma) {
      out << ",";
    }
    out << newline(pretty_);
  }

private:
  bool pretty_;
};

std::string filterNameFromClass(const std::string & component_class)
{
  const auto scope = component_class.rfind("::");
  auto name = scope == std::string::npos ? component_class : component_class.substr(scope + 2U);
  const std::string suffix = "Component";
  if (name.size() > suffix.size() && name.substr(name.size() - suffix.size()) == suffix) {
    name.resize(name.size() - suffix.size());
  }
  return name;
}

std::map<std::string, ComponentResource> componentResourcesByFilter(const std::string & package)
{
  std::string content;
  std::string prefix;
  if (!ament_index_cpp::get_resource("rclcpp_components", package, content, &prefix)) {
    throw std::runtime_error("Package has no rclcpp_components resource");
  }

  std::map<std::string, ComponentResource> resources;
  std::istringstream lines{content};
  std::string line;
  while (std::getline(lines, line)) {
    if (line.empty()) {
      continue;
    }
    const auto separator = line.find(';');
    if (separator == std::string::npos) {
      continue;
    }
    const auto component_class = line.substr(0, separator);
    const auto library_path = line.substr(separator + 1U);
    resources[filterNameFromClass(component_class)] = {component_class, prefix + "/" + library_path};
  }
  return resources;
}

void writeFilterJson(
  std::ostream & out,
  const FilterExport & filter_export,
  const ComponentResource & resource,
  const FilterComponentBase & filter,
  const CliOptions & options,
  const JsonWriter & writer,
  size_t level)
{
  out << indent(level, options.pretty) << "{" << newline(options.pretty);
  writer.keyValue(out, "package", filter_export.package, true, level + 1U);
  writer.keyValue(out, "filter", filter_export.filter, true, level + 1U);
  writer.keyValue(out, "component_class", resource.first, true, level + 1U);
  writer.keyValue(out, "kind", filter_export.kind, true, level + 1U);
  writer.keyValue(out, "chain_data_type", filter_export.chain_data_type, true, level + 1U);
  writer.keyValue(out, "input_type", joinTypeString(filter.inputPortDescriptors()), true, level + 1U);
  writer.keyValue(out, "output_type", joinTypeString(filter.outputPortDescriptors()), true, level + 1U);
  writer.keyValue(out, "input_ports", joinPortString(filter.inputPortDescriptors()), true, level + 1U);
  writer.keyValue(out, "output_ports", joinPortString(filter.outputPortDescriptors()), true, level + 1U);
  writer.portArray(out, "inputs", filter.inputPortDescriptors(), true, level + 1U);
  writer.portArray(out, "outputs", filter.outputPortDescriptors(), true, level + 1U);
  writer.shmArray(out, filter.shmKeyDescriptors(), options.include_parameters, level + 1U);
  if (options.include_parameters) {
    writer.parameterObject(out, filter, false, level + 1U);
  }
  out << indent(level, options.pretty) << "}";
}

std::string sanitizedNodeName(const std::string & package, const std::string & filter)
{
  std::string name = "inspect_" + package + "_" + filter;
  for (auto & ch : name) {
    if (!std::isalnum(static_cast<unsigned char>(ch)) && ch != '_') {
      ch = '_';
    }
  }
  return name;
}

void writeErrorJson(std::ostream & out, const ErrorEntry & error, bool comma, bool pretty, size_t level)
{
  JsonWriter writer{pretty};
  out << indent(level, pretty) << "{" << newline(pretty);
  writer.keyValue(out, "package", error.package, true, level + 1U);
  writer.keyValue(out, "filter", error.filter, true, level + 1U);
  writer.keyValue(out, "component_class", error.component_class, true, level + 1U);
  writer.keyValue(out, "message", error.message, false, level + 1U);
  out << indent(level, pretty) << "}";
  if (comma) {
    out << ",";
  }
  out << newline(pretty);
}

}  // namespace

int main(int argc, char ** argv)
{
  CliOptions options;
  try {
    options = parseArgs(argc, argv);
  } catch (const std::exception & exception) {
    std::cerr << "error: " << exception.what() << "\n\n" << usage();
    return 2;
  }

  if (std::getenv("ROS_LOG_DIR") == nullptr) {
    setenv("ROS_LOG_DIR", "/tmp", 0);
  }
  StdoutSilencer stdout_silencer;
  rclcpp::init(argc, argv);
  std::vector<std::pair<FilterExport, ComponentResource>> selected;
  std::vector<ErrorEntry> errors;

  try {
    const auto packages = options.all ? allFilterPackages() : options.packages;
    for (const auto & package : packages) {
      std::vector<FilterExport> exports;
      try {
        exports = packageFilterExports(package);
      } catch (const std::exception & exception) {
        errors.push_back(ErrorEntry{package, "", "", exception.what()});
        continue;
      }
      if (exports.empty()) {
        errors.push_back(ErrorEntry{package, "", "", "Package exports no C++ filter components"});
        continue;
      }

      std::map<std::string, ComponentResource> resources;
      try {
        resources = componentResourcesByFilter(package);
      } catch (const std::exception & exception) {
        errors.push_back(ErrorEntry{package, "", "", exception.what()});
        continue;
      }

      for (const auto & filter_export : exports) {
        const auto resource_iter = resources.find(filter_export.filter);
        if (resource_iter == resources.end()) {
          errors.push_back(
            ErrorEntry{
              package,
              filter_export.filter,
              package + "::" + filter_export.filter + "Component",
              "No matching rclcpp component resource"});
          continue;
        }
        selected.push_back({filter_export, resource_iter->second});
      }
    }

    JsonWriter writer{options.pretty};
    std::ostringstream output;
    output << "{" << newline(options.pretty);
    output << indent(1U, options.pretty) << quoted("filters") << ":" << (options.pretty ? " " : "") << "[" <<
      newline(options.pretty);

    size_t written_filters = 0U;
    for (const auto & [filter_export, resource] : selected) {
      try {
        debugLog("loading " + resource.second);
        class_loader::ClassLoader loader{resource.second};
        debugLog("creating factory for " + resource.first);
        auto factory = loader.createInstance<rclcpp_components::NodeFactory>(
          "rclcpp_components::NodeFactoryTemplate<" + resource.first + ">");
        debugLog("creating node instance for " + resource.first);
        auto node_options = rclcpp::NodeOptions{}
          .start_parameter_services(false)
          .start_parameter_event_publisher(false)
          .arguments({"--ros-args", "-r", "__node:=" + sanitizedNodeName(filter_export.package, filter_export.filter)});
        auto wrapper = factory->create_node_instance(node_options);
        debugLog("casting node instance for " + resource.first);
        auto filter = std::static_pointer_cast<FilterComponentBase>(wrapper.get_node_instance());
        debugLog("writing metadata for " + resource.first);
        if (written_filters > 0U) {
          output << "," << newline(options.pretty);
        }
        writeFilterJson(output, filter_export, resource, *filter, options, writer, 2U);
        debugLog("finished metadata for " + resource.first);
        ++written_filters;
      } catch (const std::exception & exception) {
        errors.push_back(ErrorEntry{filter_export.package, filter_export.filter, resource.first, exception.what()});
      }
    }

    output << newline(options.pretty) << indent(1U, options.pretty) << "]," << newline(options.pretty);
    output << indent(1U, options.pretty) << quoted("errors") << ":" << (options.pretty ? " " : "") << "[" <<
      newline(options.pretty);
    for (size_t index = 0; index < errors.size(); ++index) {
      writeErrorJson(output, errors[index], index + 1U < errors.size(), options.pretty, 2U);
    }
    output << indent(1U, options.pretty) << "]" << newline(options.pretty);
    output << "}" << newline(options.pretty);
    stdout_silencer.restore();
    std::cout << output.str();
  } catch (const std::exception & exception) {
    rclcpp::shutdown();
    stdout_silencer.restore();
    std::cerr << "error: " << exception.what() << "\n";
    return 1;
  }

  rclcpp::shutdown();
  return errors.empty() ? 0 : 1;
}
