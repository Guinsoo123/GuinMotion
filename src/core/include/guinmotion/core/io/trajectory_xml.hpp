#pragma once

#include "guinmotion/core/io/import_result.hpp"

#include <string_view>

namespace guinmotion::core::io {

/// When set, `robot_model_id` must exist and waypoint joint counts must match the model.
struct TrajectoryXmlImportContext {
  const Scene* scene{nullptr};
};

/// Minimal GuinMotion interchange schema (UTF-8, no namespaces).
///
/// Root: `<guinmotion_trajectory id="..." name="..." robot_model_id="..."
///         interpolation="linear_joint|hold|cubic_joint|cartesian_linear">`
/// Child: `<waypoint id="..." label="..." time_seconds="..." duration_seconds="...">`
///         `<joints>j0 j1 ...</joints>` (radians, whitespace-separated)
///         `</waypoint>`
/// `</guinmotion_trajectory>`
ImportResult import_trajectory_xml(std::string_view xml, const TrajectoryXmlImportContext& ctx = {});

}  // namespace guinmotion::core::io
