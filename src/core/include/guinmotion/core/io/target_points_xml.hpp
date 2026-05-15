#pragma once

#include "guinmotion/core/io/import_result.hpp"

#include <filesystem>
#include <string_view>

namespace guinmotion::core::io {

/// Root: `<guinmotion_targets id="..." name="..." robot_model_id="...">`
/// Child: `<target id="..." name="..." tolerance="..." time_hint_seconds="...">`
///          `<pose x="..." y="..." z="..." qw="1" qx="0" qy="0" qz="0"/>`
///        `</target>`
ImportResult import_target_points_xml(std::string_view xml);
ImportResult import_target_points_xml_file(const std::filesystem::path& path);

}  // namespace guinmotion::core::io
