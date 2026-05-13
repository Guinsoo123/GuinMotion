#include "guinmotion/operator_runtime/operator_registry.hpp"

#include <algorithm>
#include <utility>

namespace guinmotion::operator_runtime {

void OperatorRegistry::register_operator(PluginMetadata plugin, OperatorMetadata metadata) {
  operators_.push_back(RegisteredOperator{std::move(plugin), std::move(metadata)});
}

const std::vector<RegisteredOperator>& OperatorRegistry::operators() const noexcept {
  return operators_;
}

std::vector<RegisteredOperator> OperatorRegistry::find_by_plugin_id(const std::string& plugin_id) const {
  std::vector<RegisteredOperator> result;
  std::copy_if(
      operators_.begin(),
      operators_.end(),
      std::back_inserter(result),
      [&plugin_id](const RegisteredOperator& item) {
        return item.plugin.id == plugin_id;
      });
  return result;
}

bool OperatorRegistry::empty() const noexcept {
  return operators_.empty();
}

PluginMetadata builtin_plugin_metadata() {
  return PluginMetadata{
      .id = "guinmotion.builtin",
      .name = "GuinMotion 内置算子",
      .version = "0.1.0",
      .sdk_abi_version = 1U,
  };
}

}  // namespace guinmotion::operator_runtime
