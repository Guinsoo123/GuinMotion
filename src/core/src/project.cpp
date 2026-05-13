#include "guinmotion/core/project.hpp"

#include <numeric>
#include <utility>

namespace guinmotion::core {

Project::Project(std::string id, std::string name) : id_(std::move(id)), name_(std::move(name)) {}

const std::string& Project::id() const noexcept {
  return id_;
}

const std::string& Project::name() const noexcept {
  return name_;
}

void Project::set_name(std::string name) {
  name_ = std::move(name);
}

Scene& Project::scene() noexcept {
  return scene_;
}

const Scene& Project::scene() const noexcept {
  return scene_;
}

ProjectSummary Project::summary() const {
  ProjectSummary summary;
  summary.robot_model_count = scene_.robot_models.size();
  summary.point_cloud_count = scene_.point_clouds.size();
  summary.point_count = scene_.points.size();
  summary.trajectory_count = scene_.trajectories.size();
  summary.validation_message_count = scene_.validation_messages.size();
  summary.waypoint_count = std::accumulate(
      scene_.trajectories.begin(),
      scene_.trajectories.end(),
      std::size_t{0},
      [](std::size_t total, const Trajectory& trajectory) {
        return total + trajectory.waypoints.size();
      });
  return summary;
}

Project make_demo_project() {
  Project project{"demo", "GuinMotion 演示项目"};

  RobotModel robot;
  robot.id = "demo_robot";
  robot.name = "演示六轴机器人";
  for (int i = 0; i < 6; ++i) {
    Joint joint;
    joint.name = "关节" + std::to_string(i + 1);
    joint.limit.lower = -3.141592653589793;
    joint.limit.upper = 3.141592653589793;
    joint.limit.velocity = 1.0;
    robot.joints.push_back(joint);
  }
  project.scene().robot_models.push_back(robot);

  Trajectory trajectory;
  trajectory.id = "demo_trajectory";
  trajectory.name = "演示关节轨迹";
  trajectory.robot_model_id = "demo_robot";
  for (int i = 0; i < 3; ++i) {
    Waypoint waypoint;
    waypoint.id = "waypoint_" + std::to_string(i + 1);
    waypoint.label = "路点 " + std::to_string(i + 1);
    waypoint.state.robot_model_id = "demo_robot";
    waypoint.state.joint_positions_radians = {
        0.1 * i, 0.2 * i, 0.3 * i, 0.1 * i, -0.2 * i, -0.1 * i};
    waypoint.duration_seconds = 1.0;
    trajectory.waypoints.push_back(waypoint);
  }
  project.scene().trajectories.push_back(trajectory);

  PointCloud cloud;
  cloud.id = "demo_cloud";
  cloud.name = "演示点云";
  cloud.positions = {{0.0, 0.0, 0.0}, {0.1, 0.0, 0.0}, {0.0, 0.1, 0.0}};
  project.scene().point_clouds.push_back(cloud);

  project.mark_scene_changed();
  return project;
}

}  // namespace guinmotion::core
