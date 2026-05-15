#pragma once

#include "guinmotion/core/types.hpp"

namespace guinmotion::simulation {

[[nodiscard]] core::TrajectoryEvaluation evaluate_trajectory(
    const core::RobotModel& robot,
    const core::Trajectory& trajectory,
    const core::TargetPointSet& targets,
    const core::SimulationTrace* trace = nullptr);

}  // namespace guinmotion::simulation
