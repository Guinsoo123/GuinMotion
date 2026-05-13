#pragma once

#include "guinmotion/core/types.hpp"

#include <array>

namespace guinmotion::viewport {

/// Maps the first three joint angles (radians) to a placeholder XYZ used before real FK is available.
/// Keep in sync with historical `ViewportWidget` behaviour (scale 0.35 per joint).
[[nodiscard]] inline std::array<float, 3> joint_placeholder_scaled_xyz(const core::RobotState& state) noexcept {
  constexpr float k = 0.35F;
  const auto& j = state.joint_positions_radians;
  const float x = j.size() > 0 ? static_cast<float>(j[0]) * k : 0.0F;
  const float y = j.size() > 1 ? static_cast<float>(j[1]) * k : 0.0F;
  const float z = j.size() > 2 ? static_cast<float>(j[2]) * k : 0.0F;
  return {x, y, z};
}

}  // namespace guinmotion::viewport
