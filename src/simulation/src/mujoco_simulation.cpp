#include "guinmotion/simulation/mujoco_simulation.hpp"

#include <algorithm>
#include <cmath>

#if defined(GUINMOTION_WITH_MUJOCO) && __has_include(<mujoco/mujoco.h>)
#include <mujoco/mujoco.h>
#define GUINMOTION_MUJOCO_HEADER_AVAILABLE 1
#else
#define GUINMOTION_MUJOCO_HEADER_AVAILABLE 0
#endif

namespace guinmotion::simulation {
namespace {

[[nodiscard]] core::ValidationMessage message(
    core::ValidationStatus status, std::string text, std::string related = {}) {
  return core::ValidationMessage{
      .status = status,
      .source = "guinmotion.simulation.mujoco",
      .message = std::move(text),
      .related_object_id = std::move(related),
  };
}

[[nodiscard]] core::RobotState interpolate_state(
    const core::Trajectory& trajectory, double time_seconds) {
  if (trajectory.waypoints.empty()) {
    return {};
  }
  if (trajectory.waypoints.size() == 1 || time_seconds <= trajectory.waypoints.front().time_seconds) {
    return trajectory.waypoints.front().state;
  }
  for (std::size_t i = 1; i < trajectory.waypoints.size(); ++i) {
    const auto& a = trajectory.waypoints[i - 1];
    const auto& b = trajectory.waypoints[i];
    if (time_seconds <= b.time_seconds) {
      const double span = std::max(b.time_seconds - a.time_seconds, 1e-9);
      const double u = std::clamp((time_seconds - a.time_seconds) / span, 0.0, 1.0);
      core::RobotState state;
      state.robot_model_id = trajectory.robot_model_id;
      const std::size_t n =
          std::min(a.state.joint_positions_radians.size(), b.state.joint_positions_radians.size());
      state.joint_positions_radians.resize(n);
      for (std::size_t j = 0; j < n; ++j) {
        state.joint_positions_radians[j] =
            a.state.joint_positions_radians[j] * (1.0 - u) + b.state.joint_positions_radians[j] * u;
      }
      return state;
    }
  }
  return trajectory.waypoints.back().state;
}

[[nodiscard]] bool has_tool_pose(const core::Waypoint& wp) {
  const auto& p = wp.tool_pose.translation;
  return std::abs(p.x) > 1e-9 || std::abs(p.y) > 1e-9 || std::abs(p.z) > 1e-9;
}

[[nodiscard]] core::Pose interpolate_tool_pose(
    const core::RobotModel& robot, const core::Trajectory& trajectory, double time_seconds) {
  if (trajectory.waypoints.empty()) {
    return {};
  }
  if (trajectory.waypoints.size() == 1 || time_seconds <= trajectory.waypoints.front().time_seconds) {
    if (has_tool_pose(trajectory.waypoints.front())) {
      core::Pose pose;
      pose.position = trajectory.waypoints.front().tool_pose.translation;
      for (int i = 0; i < 4; ++i) {
        pose.orientation[i] = trajectory.waypoints.front().tool_pose.rotation[i];
      }
      return pose;
    }
    return estimate_tool_pose(robot, trajectory.waypoints.front().state);
  }
  for (std::size_t i = 1; i < trajectory.waypoints.size(); ++i) {
    const auto& a = trajectory.waypoints[i - 1];
    const auto& b = trajectory.waypoints[i];
    if (time_seconds <= b.time_seconds) {
      if (has_tool_pose(a) && has_tool_pose(b)) {
        const double span = std::max(b.time_seconds - a.time_seconds, 1e-9);
        const double u = std::clamp((time_seconds - a.time_seconds) / span, 0.0, 1.0);
        core::Pose pose;
        pose.position.x = a.tool_pose.translation.x * (1.0 - u) + b.tool_pose.translation.x * u;
        pose.position.y = a.tool_pose.translation.y * (1.0 - u) + b.tool_pose.translation.y * u;
        pose.position.z = a.tool_pose.translation.z * (1.0 - u) + b.tool_pose.translation.z * u;
        for (int q = 0; q < 4; ++q) {
          pose.orientation[q] = a.tool_pose.rotation[q] * (1.0 - u) + b.tool_pose.rotation[q] * u;
        }
        return pose;
      }
      return estimate_tool_pose(robot, interpolate_state(trajectory, time_seconds));
    }
  }
  const auto& last = trajectory.waypoints.back();
  if (has_tool_pose(last)) {
    core::Pose pose;
    pose.position = last.tool_pose.translation;
    for (int i = 0; i < 4; ++i) {
      pose.orientation[i] = last.tool_pose.rotation[i];
    }
    return pose;
  }
  return estimate_tool_pose(robot, last.state);
}

}  // namespace

core::Pose estimate_tool_pose(const core::RobotModel& robot, const core::RobotState& state) {
  core::Pose pose;
  double heading = 0.0;
  double reach = 0.0;
  const std::size_t n = std::min(robot.joints.size(), state.joint_positions_radians.size());
  for (std::size_t i = 0; i < n; ++i) {
    const auto& joint = robot.joints[i];
    const double q = state.joint_positions_radians[i];
    heading += (joint.type == core::JointType::Prismatic) ? 0.0 : q;
    const double link_len = std::max(std::abs(joint.origin.translation.x) +
                                         std::abs(joint.origin.translation.y) +
                                         std::abs(joint.origin.translation.z),
                                     0.12);
    if (joint.type == core::JointType::Prismatic) {
      reach += q;
    } else {
      reach += link_len;
    }
  }
  pose.position.x = reach * std::cos(heading);
  pose.position.y = reach * std::sin(heading);
  pose.position.z = 0.05 * static_cast<double>(n);
  pose.orientation[0] = std::cos(heading * 0.5);
  pose.orientation[3] = std::sin(heading * 0.5);
  return pose;
}

SimulationResult MujocoSimulationEngine::run_trajectory(
    const core::RobotModel& robot,
    const core::Trajectory& trajectory,
    const std::filesystem::path& model_path) const {
  SimulationResult result;
  result.trace.id = trajectory.id + "_mujoco_trace";
  result.trace.name = trajectory.name + " MuJoCo Trace";
  result.trace.robot_model_id = robot.id;
  result.trace.trajectory_id = trajectory.id;

  if (trajectory.waypoints.empty()) {
    result.messages.push_back(message(core::ValidationStatus::Error, "轨迹为空，无法仿真。", trajectory.id));
    return result;
  }

#if GUINMOTION_MUJOCO_HEADER_AVAILABLE
  mjModel* model = nullptr;
  mjData* data = nullptr;
  char error[1024] = {0};
  if (!model_path.empty()) {
    model = mj_loadXML(model_path.string().c_str(), nullptr, error, sizeof(error));
    if (model == nullptr) {
      result.messages.push_back(message(core::ValidationStatus::Error,
                                        std::string("MuJoCo 加载模型失败：") + error,
                                        robot.id));
      return result;
    }
    data = mj_makeData(model);
    result.trace.used_mujoco = data != nullptr;
  }
#else
  result.messages.push_back(message(core::ValidationStatus::Warning,
                                    "构建环境未提供 MuJoCo 头文件，使用确定性运动学 trace 作为降级仿真。",
                                    trajectory.id));
#endif

  const double end_time = std::max(trajectory.waypoints.back().time_seconds, 1.0);
  const int sample_count = std::max(2, static_cast<int>(std::ceil(end_time / 0.05)) + 1);
  for (int i = 0; i < sample_count; ++i) {
    const double t = end_time * static_cast<double>(i) / static_cast<double>(sample_count - 1);
    auto state = interpolate_state(trajectory, t);

#if GUINMOTION_MUJOCO_HEADER_AVAILABLE
    if (model != nullptr && data != nullptr) {
      const int n = std::min<int>(model->nq, static_cast<int>(state.joint_positions_radians.size()));
      for (int j = 0; j < n; ++j) {
        data->qpos[j] = state.joint_positions_radians[static_cast<std::size_t>(j)];
      }
      mj_forward(model, data);
      mj_step(model, data);
    }
#endif

    result.trace.samples.push_back(core::SimulationSample{
        .time_seconds = t,
        .state = std::move(state),
        .tool_pose = interpolate_tool_pose(robot, trajectory, t),
    });
  }

#if GUINMOTION_MUJOCO_HEADER_AVAILABLE
  if (data != nullptr) {
    mj_deleteData(data);
  }
  if (model != nullptr) {
    mj_deleteModel(model);
  }
#endif

  result.ok = true;
  result.messages.push_back(message(core::ValidationStatus::Valid, "轨迹仿真 trace 已生成。", trajectory.id));
  return result;
}

}  // namespace guinmotion::simulation
