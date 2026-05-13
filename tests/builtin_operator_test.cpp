#include "guinmotion/core/project.hpp"
#include "guinmotion/sdk/builtin_operators.hpp"
#include "guinmotion/sdk/operator.hpp"

#include <cassert>
#include <cstdlib>
#include <string>

namespace {

void test_duration_negative_reports_error() {
  auto project = guinmotion::core::make_demo_project();
  assert(!project.scene().trajectories.empty());
  project.scene().trajectories.front().waypoints.front().duration_seconds = -0.5;

  auto op = guinmotion::sdk::make_builtin_operator("guinmotion.trajectory.duration_check");
  assert(op != nullptr);

  guinmotion::sdk::ExecutionContext ctx;
  ctx.set_scene(&project.scene());
  ctx.set_target_trajectory_id(project.scene().trajectories.front().id);
  const auto result = op->execute(ctx);
  assert(!result.validation_messages.empty());
  assert(result.validation_messages.front().status == guinmotion::core::ValidationStatus::Error);
}

void test_joint_out_of_limit_reports_error() {
  auto project = guinmotion::core::make_demo_project();
  assert(!project.scene().trajectories.empty());
  auto& wp = project.scene().trajectories.front().waypoints.front();
  if (wp.state.joint_positions_radians.empty()) {
    std::abort();
  }
  wp.state.joint_positions_radians[0] = 99.0;

  auto op = guinmotion::sdk::make_builtin_operator("guinmotion.joint.limit_check");
  assert(op != nullptr);

  guinmotion::sdk::ExecutionContext ctx;
  ctx.set_scene(&project.scene());
  ctx.set_target_trajectory_id(project.scene().trajectories.front().id);
  const auto result = op->execute(ctx);
  assert(!result.validation_messages.empty());
  assert(result.validation_messages.front().status == guinmotion::core::ValidationStatus::Error);
}

}  // namespace

int main() {
  test_duration_negative_reports_error();
  test_joint_out_of_limit_reports_error();
  return 0;
}
