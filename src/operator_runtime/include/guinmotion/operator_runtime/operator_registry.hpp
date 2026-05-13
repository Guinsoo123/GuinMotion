#pragma once

#include <string>
#include <vector>

namespace guinmotion::operator_runtime {

struct OperatorMetadata {
  std::string id;
  std::string name;
  std::string version;
  std::string description;
};

struct PluginMetadata {
  std::string id;
  std::string name;
  std::string version;
  unsigned int sdk_abi_version{0};
};

struct RegisteredOperator {
  PluginMetadata plugin;
  OperatorMetadata metadata;
};

class OperatorRegistry {
 public:
  void register_operator(PluginMetadata plugin, OperatorMetadata metadata);

  [[nodiscard]] const std::vector<RegisteredOperator>& operators() const noexcept;
  [[nodiscard]] std::vector<RegisteredOperator> find_by_plugin_id(const std::string& plugin_id) const;
  [[nodiscard]] bool empty() const noexcept;

 private:
  std::vector<RegisteredOperator> operators_;
};

PluginMetadata builtin_plugin_metadata();

}  // namespace guinmotion::operator_runtime
