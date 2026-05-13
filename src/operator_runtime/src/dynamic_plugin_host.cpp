#include "guinmotion/operator_runtime/dynamic_plugin_host.hpp"

#include "guinmotion/operator_runtime/operator_registry.hpp"
#include "guinmotion/operator_runtime/plugin_api.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <utility>

#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

namespace guinmotion::operator_runtime {
namespace {

#if defined(_WIN32)
void* open_module(const std::filesystem::path& path) {
  return reinterpret_cast<void*>(LoadLibraryW(path.c_str()));
}

void close_module(void* handle) {
  if (handle != nullptr) {
    FreeLibrary(reinterpret_cast<HMODULE>(handle));
  }
}

GuinMotionGetPluginApiFn resolve_get_api(void* handle) {
  auto* module = reinterpret_cast<HMODULE>(handle);
  return reinterpret_cast<GuinMotionGetPluginApiFn>(GetProcAddress(module, "guinmotion_get_plugin_api"));
}
#else
void* open_module(const std::filesystem::path& path) {
  return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
}

void close_module(void* handle) {
  if (handle != nullptr) {
    dlclose(handle);
  }
}

GuinMotionGetPluginApiFn resolve_get_api(void* handle) {
  return reinterpret_cast<GuinMotionGetPluginApiFn>(dlsym(handle, "guinmotion_get_plugin_api"));
}
#endif

std::string last_dynamic_loader_error() {
#if defined(_WIN32)
  const DWORD code = GetLastError();
  if (code == 0) {
    return "unknown error";
  }
  return "Windows error " + std::to_string(code);
#else
  const char* message = dlerror();
  return message != nullptr ? std::string(message) : std::string("unknown error");
#endif
}

std::string c_abi_string_view_to_string(const GuinMotionStringView& view) {
  if (view.data == nullptr || view.size == 0) {
    return {};
  }
  return {view.data, static_cast<std::size_t>(view.size)};
}

bool is_plugin_extension(const std::filesystem::path& path) {
  const auto ext = path.extension().string();
  std::string lower = ext;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return lower == ".so" || lower == ".dylib" || lower == ".dll";
}

}  // namespace

DynamicPluginHost::~DynamicPluginHost() {
  for (void* handle : modules_) {
    close_module(handle);
  }
  modules_.clear();
}

bool DynamicPluginHost::load_plugin_file(const std::filesystem::path& path, OperatorRegistry& registry,
                                         std::string& error_message) {
#if !defined(_WIN32)
  dlerror();
#endif
  void* handle = open_module(path);
  if (handle == nullptr) {
    error_message = "failed to load library: " + last_dynamic_loader_error();
    return false;
  }

  const auto release = [&]() {
    close_module(handle);
  };

  GuinMotionGetPluginApiFn get_api = resolve_get_api(handle);
  if (get_api == nullptr) {
    error_message = "missing symbol guinmotion_get_plugin_api";
    release();
    return false;
  }

  GuinMotionPluginApi api{};
  if (get_api(GUINMOTION_SDK_ABI_VERSION, &api) != 0) {
    error_message = "guinmotion_get_plugin_api rejected host ABI (or invalid plugin)";
    release();
    return false;
  }

  if (api.sdk_abi_version != GUINMOTION_SDK_ABI_VERSION) {
    error_message = "plugin reports incompatible sdk_abi_version " + std::to_string(api.sdk_abi_version) +
                    " (host expects " + std::to_string(GUINMOTION_SDK_ABI_VERSION) + ")";
    release();
    return false;
  }

  if (api.get_plugin_metadata == nullptr || api.get_operator_count == nullptr ||
      api.get_operator_metadata == nullptr) {
    error_message = "plugin API table contains null function pointers";
    release();
    return false;
  }

  GuinMotionPluginMetadata plugin_meta{};
  if (api.get_plugin_metadata(&plugin_meta) != 0) {
    error_message = "get_plugin_metadata failed";
    release();
    return false;
  }

  if (plugin_meta.sdk_abi_version != GUINMOTION_SDK_ABI_VERSION) {
    error_message = "plugin metadata sdk_abi_version mismatch";
    release();
    return false;
  }

  const PluginMetadata plugin_cpp{
      .id = std::string(c_abi_string_view_to_string(plugin_meta.id)),
      .name = std::string(c_abi_string_view_to_string(plugin_meta.name)),
      .version = std::string(c_abi_string_view_to_string(plugin_meta.version)),
      .sdk_abi_version = plugin_meta.sdk_abi_version,
  };

  const uint64_t operator_count = api.get_operator_count();
  std::vector<OperatorMetadata> pending;
  pending.reserve(static_cast<std::size_t>(operator_count));

  for (uint64_t i = 0; i < operator_count; ++i) {
    GuinMotionOperatorMetadata op_meta{};
    if (api.get_operator_metadata(i, &op_meta) != 0) {
      error_message = "get_operator_metadata failed at index " + std::to_string(i);
      release();
      return false;
    }
    pending.push_back(OperatorMetadata{
        .id = std::string(c_abi_string_view_to_string(op_meta.id)),
        .name = std::string(c_abi_string_view_to_string(op_meta.name)),
        .version = std::string(c_abi_string_view_to_string(op_meta.version)),
        .description = std::string(c_abi_string_view_to_string(op_meta.description)),
    });
  }

  for (const auto& meta : pending) {
    registry.register_operator(plugin_cpp, meta);
  }

  modules_.push_back(handle);
  return true;
}

void DynamicPluginHost::load_plugins_from_directory(const std::filesystem::path& dir,
                                                    OperatorRegistry& registry,
                                                    std::vector<std::string>& error_messages) {
  std::error_code ec;
  if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) {
    return;
  }

  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
    if (ec) {
      error_messages.push_back("directory iteration failed: " + ec.message());
      return;
    }
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto& p = entry.path();
    if (!is_plugin_extension(p)) {
      continue;
    }
    files.push_back(p);
  }
  std::sort(files.begin(), files.end());

  for (const auto& p : files) {
    std::string err;
    if (!load_plugin_file(p, registry, err)) {
      error_messages.push_back(p.filename().string() + ": " + err);
    }
  }
}

}  // namespace guinmotion::operator_runtime
