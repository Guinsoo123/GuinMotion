#include "guinmotion/simulation/trajectory_evaluator.hpp"

#include "guinmotion/simulation/mujoco_simulation.hpp"

#include <algorithm>
#include <cmath>

namespace guinmotion::simulation {
namespace {

[[nodiscard]] double dist(const core::Vec3& a, const core::Vec3& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  const double dz = a.z - b.z;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void add(core::TrajectoryEvaluation& evaluation,
         core::ValidationStatus status,
         std::string message,
         std::string related) {
  evaluation.messages.push_back(core::ValidationMessage{
      .status = status,
      .source = "guinmotion.evaluation.trajectory",
      .message = std::move(message),
      .related_object_id = std::move(related),
  });
}

}  // namespace

core::TrajectoryEvaluation evaluate_trajectory(
    const core::RobotModel& robot,
    const core::Trajectory& trajectory,
    const core::TargetPointSet& targets,
    const core::SimulationTrace* trace) {
  core::TrajectoryEvaluation evaluation;
  evaluation.id = trajectory.id + "_evaluation";
  evaluation.trajectory_id = trajectory.id;
  evaluation.target_point_set_id = targets.id;
  evaluation.status = core::ValidationStatus::Valid;

  if (trajectory.waypoints.empty()) {
    evaluation.status = core::ValidationStatus::Error;
    add(evaluation, core::ValidationStatus::Error, "轨迹为空。", trajectory.id);
    return evaluation;
  }
  if (targets.targets.empty()) {
    evaluation.status = core::ValidationStatus::Error;
    add(evaluation, core::ValidationStatus::Error, "目标点集为空。", targets.id);
    return evaluation;
  }
  if (trace == nullptr || trace->samples.empty()) {
    evaluation.status = core::ValidationStatus::Warning;
    add(evaluation, core::ValidationStatus::Warning, "没有可用仿真 trace，使用路点估计末端位姿评价。", trajectory.id);
  }

  const std::size_t checks = std::min(targets.targets.size(), trajectory.waypoints.size());
  for (std::size_t i = 0; i < checks; ++i) {
    const auto& target = targets.targets[i];
    core::Pose pose;
    if (trace != nullptr && !trace->samples.empty()) {
      const auto closest = std::min_element(
          trace->samples.begin(),
          trace->samples.end(),
          [&](const core::SimulationSample& a, const core::SimulationSample& b) {
            return std::abs(a.time_seconds - target.time_hint_seconds) <
                   std::abs(b.time_seconds - target.time_hint_seconds);
          });
      pose = closest->tool_pose;
    } else {
      pose = estimate_tool_pose(robot, trajectory.waypoints[i].state);
    }
    const double err = dist(pose.position, target.pose.position);
    evaluation.max_position_error_m = std::max(evaluation.max_position_error_m, err);
    if (err > target.tolerance_m) {
      evaluation.status = core::ValidationStatus::Error;
      add(evaluation,
          core::ValidationStatus::Error,
          "目标点 " + target.id + " 到达误差 " + std::to_string(err) +
              " m 超过容差 " + std::to_string(target.tolerance_m) + " m。",
          target.id);
    }
  }

  for (const auto& wp : trajectory.waypoints) {
    if (wp.state.joint_positions_radians.size() != robot.joints.size()) {
      evaluation.status = core::ValidationStatus::Error;
      add(evaluation, core::ValidationStatus::Error, "路点关节数量与机器人模型不一致。", wp.id);
      continue;
    }
    for (std::size_t j = 0; j < robot.joints.size(); ++j) {
      const auto& joint = robot.joints[j];
      const double q = wp.state.joint_positions_radians[j];
      if (joint.type != core::JointType::Continuous && (q < joint.limit.lower || q > joint.limit.upper)) {
        evaluation.status = core::ValidationStatus::Error;
        add(evaluation, core::ValidationStatus::Error, "关节 " + joint.name + " 超出限位。", wp.id);
      }
    }
  }

  if (evaluation.messages.empty()) {
    add(evaluation, core::ValidationStatus::Valid, "轨迹评价通过。", trajectory.id);
  }
  return evaluation;
}

}  // namespace guinmotion::simulation
