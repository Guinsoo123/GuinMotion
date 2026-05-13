#include "guinmotion/core/types.hpp"
#include "guinmotion/viewport/joint_placeholder.hpp"

#include <cmath>
#include <cstdlib>

namespace {

void expect_near(float a, float b) {
  if (std::fabs(a - b) > 1e-5F) {
    std::abort();
  }
}

}  // namespace

int main() {
  guinmotion::core::RobotState state;
  state.robot_model_id = "demo_robot";
  state.joint_positions_radians = {1.0, 2.0, 3.0};

  const auto p = guinmotion::viewport::joint_placeholder_scaled_xyz(state);
  expect_near(p[0], 0.35F);
  expect_near(p[1], 0.70F);
  expect_near(p[2], 1.05F);

  state.joint_positions_radians = {};
  const auto q = guinmotion::viewport::joint_placeholder_scaled_xyz(state);
  expect_near(q[0], 0.0F);
  expect_near(q[1], 0.0F);
  expect_near(q[2], 0.0F);

  return 0;
}
