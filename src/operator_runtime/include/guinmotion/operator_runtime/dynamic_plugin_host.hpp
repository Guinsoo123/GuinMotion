#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace guinmotion::operator_runtime {

class OperatorRegistry;

/// Owns dynamically loaded plugin modules (dlopen / LoadLibrary) and registers their operators.
class DynamicPluginHost final {
 public:
  DynamicPluginHost() = default;
  ~DynamicPluginHost();

  DynamicPluginHost(const DynamicPluginHost&) = delete;
  DynamicPluginHost& operator=(const DynamicPluginHost&) = delete;
  DynamicPluginHost(DynamicPluginHost&&) = delete;
  DynamicPluginHost& operator=(DynamicPluginHost&&) = delete;

  /// Loads one plugin shared library. On failure the library is not retained.
  [[nodiscard]] bool load_plugin_file(const std::filesystem::path& path, OperatorRegistry& registry,
                                      std::string& error_message);

  /// Tries every regular file with a known plugin extension in `dir`. Appends one message per failed file.
  void load_plugins_from_directory(const std::filesystem::path& dir, OperatorRegistry& registry,
                                   std::vector<std::string>& error_messages);

  [[nodiscard]] std::size_t loaded_module_count() const noexcept { return modules_.size(); }

 private:
  std::vector<void*> modules_{};
};

}  // namespace guinmotion::operator_runtime
