#include "guinmotion/operator_runtime/plugin_api.h"

#include <cstring>

namespace {

GuinMotionStringView view(const char* value) {
  return GuinMotionStringView{value, static_cast<uint64_t>(std::strlen(value))};
}

int get_plugin_metadata(GuinMotionPluginMetadata* out_metadata) {
  if (out_metadata == nullptr) {
    return -1;
  }
  out_metadata->id = view("guinmotion.example.minimal");
  out_metadata->name = view("最小化示例算子");
  out_metadata->version = view("0.1.0");
  out_metadata->sdk_abi_version = GUINMOTION_SDK_ABI_VERSION;
  return 0;
}

uint64_t get_operator_count() {
  return 1;
}

int get_operator_metadata(uint64_t index, GuinMotionOperatorMetadata* out_metadata) {
  if (out_metadata == nullptr || index != 0) {
    return -1;
  }
  out_metadata->id = view("guinmotion.example.minimal.echo");
  out_metadata->name = view("最小化回显");
  out_metadata->version = view("0.1.0");
  out_metadata->description = view("用于验证 C ABI 动态加载的示例插件。");
  return 0;
}

}  // namespace

extern "C" GUINMOTION_EXPORT int guinmotion_get_plugin_api(
    uint32_t host_abi_version,
    GuinMotionPluginApi* out_api) {
  if (out_api == nullptr || host_abi_version != GUINMOTION_SDK_ABI_VERSION) {
    return -1;
  }

  out_api->sdk_abi_version = GUINMOTION_SDK_ABI_VERSION;
  out_api->get_plugin_metadata = &get_plugin_metadata;
  out_api->get_operator_count = &get_operator_count;
  out_api->get_operator_metadata = &get_operator_metadata;
  return 0;
}
