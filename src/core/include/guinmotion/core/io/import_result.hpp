#pragma once

#include "guinmotion/core/types.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string>

namespace guinmotion::core::io {

struct ImportLimits {
  /// Hard cap to avoid accidental multi-gigabyte loads in the first milestone.
  std::size_t max_point_cloud_vertices{5'000'000};
};

struct ImportResult {
  bool ok{false};
  std::string message;
  std::optional<Trajectory> trajectory;
  std::optional<PointCloud> point_cloud;
  std::optional<RobotModel> robot_model;
  std::optional<TargetPointSet> target_point_set;
};

}  // namespace guinmotion::core::io
