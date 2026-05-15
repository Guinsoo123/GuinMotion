#include "guinmotion/core/types.hpp"
#include "guinmotion/simulation/mujoco_simulation.hpp"
#include "guinmotion/simulation/trajectory_evaluator.hpp"

#include <cassert>
#include <string>

int main() {
  guinmotion::core::RobotModel robot;
  robot.id = "r";
  for (int i = 0; i < 2; ++i) {
    guinmotion::core::Joint joint;
    joint.name = "j" + std::to_string(i);
    joint.limit.lower = -3.14;
    joint.limit.upper = 3.14;
    robot.joints.push_back(joint);
  }

  guinmotion::core::Trajectory trajectory;
  trajectory.id = "t";
  trajectory.robot_model_id = "r";
  for (int i = 0; i < 2; ++i) {
    guinmotion::core::Waypoint wp;
    wp.id = "w" + std::to_string(i);
    wp.time_seconds = static_cast<double>(i);
    wp.state.robot_model_id = "r";
    wp.state.joint_positions_radians = {0.0, 0.0};
    wp.tool_pose.translation = {0.1 * i, 0.0, 0.1};
    trajectory.waypoints.push_back(wp);
  }

  guinmotion::core::TargetPointSet targets;
  targets.id = "targets";
  targets.robot_model_id = "r";
  for (int i = 0; i < 2; ++i) {
    guinmotion::core::TargetPoint target;
    target.id = "p" + std::to_string(i);
    target.time_hint_seconds = static_cast<double>(i);
    target.tolerance_m = 0.02;
    target.pose.position = {0.1 * i, 0.0, 0.1};
    targets.targets.push_back(target);
  }

  guinmotion::simulation::MujocoSimulationEngine engine;
  const auto sim = engine.run_trajectory(robot, trajectory);
  assert(sim.ok);
  assert(!sim.trace.samples.empty());

  const auto evaluation =
      guinmotion::simulation::evaluate_trajectory(robot, trajectory, targets, &sim.trace);
  assert(evaluation.status == guinmotion::core::ValidationStatus::Valid);
  return 0;
}
