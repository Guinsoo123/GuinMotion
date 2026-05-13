#pragma once

#include "guinmotion/core/io/import_result.hpp"

#include <filesystem>
#include <string_view>

namespace guinmotion::core::io {

/// Reads the entire file. Format from extension: `.ply` (ASCII) or `.xyz` (three columns per line).
ImportResult import_point_cloud_file(const std::filesystem::path& path, const ImportLimits& limits = {});

/// Sniff: if data begins with "ply" treat as PLY ASCII, else XYZ three-column text.
ImportResult import_point_cloud_memory(std::string_view data, const ImportLimits& limits = {});

}  // namespace guinmotion::core::io
