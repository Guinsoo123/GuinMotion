#include "guinmotion/core/io/point_cloud_file.hpp"
#include "guinmotion/core/io/trajectory_xml.hpp"
#include "guinmotion/core/types.hpp"

#include <cstdlib>
#include <iostream>

namespace {

int g_failures = 0;

void check(bool cond, const char* expr, const char* file, int line) {
  if (!cond) {
    std::cerr << file << ":" << line << " check failed: " << expr << "\n";
    ++g_failures;
  }
}

#define CHECK(cond) check((cond), #cond, __FILE__, __LINE__)

}  // namespace

int main() {
  using guinmotion::core::InterpolationMode;
  using guinmotion::core::Joint;
  using guinmotion::core::RobotModel;
  using guinmotion::core::Scene;
  using guinmotion::core::io::TrajectoryXmlImportContext;
  using guinmotion::core::io::import_point_cloud_memory;
  using guinmotion::core::io::import_trajectory_xml;

  const char* kXmlOk = R"(
<guinmotion_trajectory id="t1" name="Test" robot_model_id="r1" interpolation="linear_joint">
  <waypoint id="w1" duration_seconds="1" time_seconds="0">
    <joints>0 0.1 0.2 0.3</joints>
  </waypoint>
  <waypoint id="w2" duration_seconds="1" time_seconds="1">
    <joints>0.1 0.2 0.3 0.4</joints>
  </waypoint>
</guinmotion_trajectory>
)";

  Scene scene;
  RobotModel robot;
  robot.id = "r1";
  for (int i = 0; i < 4; ++i) {
    Joint j;
    j.name = "j" + std::to_string(i);
    robot.joints.push_back(j);
  }
  scene.robot_models.push_back(robot);

  TrajectoryXmlImportContext ctx{&scene};
  auto tr = import_trajectory_xml(kXmlOk, ctx);
  CHECK(tr.ok);
  CHECK(tr.trajectory.has_value());
  CHECK(tr.trajectory->waypoints.size() == 2);
  CHECK(tr.trajectory->interpolation == InterpolationMode::LinearJoint);

  const char* kXmlBadTime = R"(
<guinmotion_trajectory id="t2" name="Bad" robot_model_id="r1">
  <waypoint id="a" time_seconds="2" duration_seconds="0"><joints>0 0 0 0</joints></waypoint>
  <waypoint id="b" time_seconds="1" duration_seconds="0"><joints>0 0 0 0</joints></waypoint>
</guinmotion_trajectory>
)";
  auto bad = import_trajectory_xml(kXmlBadTime, ctx);
  CHECK(!bad.ok);

  const char* kXmlWrongJoints = R"(
<guinmotion_trajectory id="t3" name="J" robot_model_id="r1">
  <waypoint id="a" duration_seconds="0"><joints>0 0 0</joints></waypoint>
</guinmotion_trajectory>
)";
  auto wj = import_trajectory_xml(kXmlWrongJoints, ctx);
  CHECK(!wj.ok);

  const char* kPly = R"(ply
format ascii 1.0
element vertex 2
property float x
property float y
property float z
end_header
0 0 0
0.5 1.0 -0.25
)";
  auto ply = import_point_cloud_memory(kPly);
  CHECK(ply.ok);
  CHECK(ply.point_cloud->positions.size() == 2);

  const char* kXyz = R"(# header
0 0 0
1 2 3
)";
  auto xyz = import_point_cloud_memory(kXyz);
  CHECK(xyz.ok);
  CHECK(xyz.point_cloud->positions.size() == 2);

  auto no_scene = import_trajectory_xml(kXmlOk, {});
  CHECK(no_scene.ok);
  CHECK(no_scene.trajectory->waypoints[0].state.joint_positions_radians.size() == 4);

  return g_failures == 0 ? 0 : 1;
}
