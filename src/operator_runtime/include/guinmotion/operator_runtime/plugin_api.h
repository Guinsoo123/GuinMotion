#pragma once

#include <stdint.h>

#if defined(_WIN32)
#  if defined(GUINMOTION_PLUGIN_BUILD)
#    define GUINMOTION_EXPORT __declspec(dllexport)
#  else
#    define GUINMOTION_EXPORT __declspec(dllimport)
#  endif
#else
#  define GUINMOTION_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GUINMOTION_SDK_ABI_VERSION 1u

typedef struct GuinMotionStringView {
  const char* data;
  uint64_t size;
} GuinMotionStringView;

typedef struct GuinMotionPluginMetadata {
  GuinMotionStringView id;
  GuinMotionStringView name;
  GuinMotionStringView version;
  uint32_t sdk_abi_version;
} GuinMotionPluginMetadata;

typedef struct GuinMotionOperatorMetadata {
  GuinMotionStringView id;
  GuinMotionStringView name;
  GuinMotionStringView version;
  GuinMotionStringView description;
} GuinMotionOperatorMetadata;

typedef struct GuinMotionPluginApi {
  uint32_t sdk_abi_version;
  int (*get_plugin_metadata)(GuinMotionPluginMetadata* out_metadata);
  uint64_t (*get_operator_count)(void);
  int (*get_operator_metadata)(uint64_t index, GuinMotionOperatorMetadata* out_metadata);
} GuinMotionPluginApi;

typedef int (*GuinMotionGetPluginApiFn)(uint32_t host_abi_version, GuinMotionPluginApi* out_api);

#ifdef __cplusplus
}
#endif
