#pragma once

#include "guinmotion/core/io/import_result.hpp"

#include <filesystem>
#include <string_view>

namespace guinmotion::core::io {

/// Imports a practical URDF subset into GuinMotion's robot model:
/// robot/link/joint origin/axis/limit and visual geometry material metadata.
ImportResult import_robot_model_urdf(std::string_view xml, const std::filesystem::path& source_path = {});
ImportResult import_robot_model_urdf_file(const std::filesystem::path& path);

}  // namespace guinmotion::core::io
