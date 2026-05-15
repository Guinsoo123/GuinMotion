#pragma once

#include "guinmotion/core/types.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace guinmotion::simulation {

struct SimulationResult {
  bool ok{false};
  core::SimulationTrace trace;
  std::vector<core::ValidationMessage> messages;
};

/// MuJoCo-backed trajectory executor. The public interface intentionally keeps MuJoCo
/// types out of core/app layers.
class MujocoSimulationEngine {
 public:
  [[nodiscard]] SimulationResult run_trajectory(
      const core::RobotModel& robot,
      const core::Trajectory& trajectory,
      const std::filesystem::path& model_path = {}) const;
};

[[nodiscard]] core::Pose estimate_tool_pose(
    const core::RobotModel& robot, const core::RobotState& state);

}  // namespace guinmotion::simulation
