#include "guinmotion/app_core/project_service.hpp"

#include "guinmotion/core/io/point_cloud_file.hpp"
#include "guinmotion/core/io/robot_model_urdf.hpp"
#include "guinmotion/core/io/target_points_xml.hpp"
#include "guinmotion/core/io/trajectory_xml.hpp"
#include "guinmotion/simulation/mujoco_simulation.hpp"
#include "guinmotion/simulation/trajectory_evaluator.hpp"

#include <fstream>
#include <sstream>
#include <unordered_set>

namespace guinmotion::app_core {
namespace sim = guinmotion::simulation;
namespace {

[[nodiscard]] std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return {};
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

}  // namespace

ProjectService::ProjectService(core::Project project) : project_(std::move(project)) {}

void ProjectService::ensure_unique_trajectory_id(core::Trajectory& trajectory) {
  std::unordered_set<std::string> used;
  for (const auto& t : project_.scene().trajectories) {
    used.insert(t.id);
  }
  if (!used.contains(trajectory.id)) {
    return;
  }
  std::string base = trajectory.id;
  for (unsigned n = 1;; ++n) {
    const std::string candidate = base + "_import" + std::to_string(n);
    if (!used.contains(candidate)) {
      trajectory.id = candidate;
      return;
    }
  }
}

void ProjectService::ensure_unique_point_cloud_id(core::PointCloud& cloud) {
  std::unordered_set<std::string> used;
  for (const auto& c : project_.scene().point_clouds) {
    used.insert(c.id);
  }
  if (!used.contains(cloud.id)) {
    return;
  }
  std::string base = cloud.id;
  for (unsigned n = 1;; ++n) {
    const std::string candidate = base + "_import" + std::to_string(n);
    if (!used.contains(candidate)) {
      cloud.id = candidate;
      return;
    }
  }
}

void ProjectService::ensure_unique_robot_model_id(core::RobotModel& robot) {
  std::unordered_set<std::string> used;
  for (const auto& r : project_.scene().robot_models) {
    used.insert(r.id);
  }
  if (!used.contains(robot.id)) {
    return;
  }
  const std::string base = robot.id;
  for (unsigned n = 1;; ++n) {
    const std::string candidate = base + "_import" + std::to_string(n);
    if (!used.contains(candidate)) {
      robot.id = candidate;
      return;
    }
  }
}

void ProjectService::ensure_unique_target_point_set_id(core::TargetPointSet& targets) {
  std::unordered_set<std::string> used;
  for (const auto& t : project_.scene().target_point_sets) {
    used.insert(t.id);
  }
  if (!used.contains(targets.id)) {
    return;
  }
  const std::string base = targets.id;
  for (unsigned n = 1;; ++n) {
    const std::string candidate = base + "_import" + std::to_string(n);
    if (!used.contains(candidate)) {
      targets.id = candidate;
      return;
    }
  }
}

core::io::ImportResult ProjectService::import_trajectory_xml(std::string_view xml) {
  core::io::TrajectoryXmlImportContext ctx{&project_.scene()};
  auto result = core::io::import_trajectory_xml(xml, ctx);
  if (!result.ok || !result.trajectory) {
    return result;
  }
  auto traj = std::move(*result.trajectory);
  ensure_unique_trajectory_id(traj);
  project_.scene().trajectories.push_back(std::move(traj));
  project_.mark_scene_changed();
  result.trajectory.reset();
  return result;
}

core::io::ImportResult ProjectService::import_trajectory_xml_file(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    core::io::ImportResult r;
    r.message = "轨迹文件不存在：" + path.string();
    return r;
  }
  const std::string text = read_text_file(path);
  if (text.empty()) {
    core::io::ImportResult r;
    r.message = "轨迹文件为空或无法读取：" + path.string();
    return r;
  }
  auto r = import_trajectory_xml(text);
  if (r.ok) {
    r.message = "已从文件导入轨迹：" + path.string();
  }
  return r;
}

core::io::ImportResult ProjectService::import_point_cloud_file(
    const std::filesystem::path& path, const core::io::ImportLimits& limits) {
  auto result = core::io::import_point_cloud_file(path, limits);
  if (!result.ok || !result.point_cloud) {
    return result;
  }
  auto cloud = std::move(*result.point_cloud);
  if (cloud.name.empty()) {
    cloud.name = path.filename().string();
  }
  ensure_unique_point_cloud_id(cloud);
  project_.scene().point_clouds.push_back(std::move(cloud));
  project_.mark_scene_changed();
  result.point_cloud.reset();
  return result;
}

core::io::ImportResult ProjectService::import_robot_model_urdf_file(const std::filesystem::path& path) {
  auto result = core::io::import_robot_model_urdf_file(path);
  if (!result.ok || !result.robot_model) {
    return result;
  }
  auto robot = std::move(*result.robot_model);
  ensure_unique_robot_model_id(robot);
  if (robot.name.empty()) {
    robot.name = robot.id;
  }
  core::RobotInstance instance;
  instance.id = robot.id + "_instance";
  instance.robot_model_id = robot.id;
  instance.name = robot.name + " 实例";
  instance.state.robot_model_id = robot.id;
  instance.state.joint_positions_radians = robot.home_joint_positions_radians;
  project_.scene().robot_models.push_back(std::move(robot));
  project_.scene().robot_instances.push_back(std::move(instance));
  project_.mark_scene_changed();
  result.robot_model.reset();
  return result;
}

core::io::ImportResult ProjectService::import_target_points_xml_file(const std::filesystem::path& path) {
  auto result = core::io::import_target_points_xml_file(path);
  if (!result.ok || !result.target_point_set) {
    return result;
  }
  auto targets = std::move(*result.target_point_set);
  ensure_unique_target_point_set_id(targets);
  project_.scene().target_point_sets.push_back(std::move(targets));
  project_.mark_scene_changed();
  result.target_point_set.reset();
  return result;
}

sdk::ExecutionResult ProjectService::run_operator(sdk::Operator& op, const std::string& trajectory_id) {
  sdk::ExecutionContext context;
  context.set_scene(&project_.scene());
  context.set_target_trajectory_id(trajectory_id);
  auto result = op.execute(context);
  for (auto& trajectory : result.trajectories) {
    ensure_unique_trajectory_id(trajectory);
    project_.scene().trajectories.push_back(std::move(trajectory));
  }
  for (auto& cloud : result.point_clouds) {
    ensure_unique_point_cloud_id(cloud);
    project_.scene().point_clouds.push_back(std::move(cloud));
  }
  for (const auto& message : result.validation_messages) {
    project_.scene().validation_messages.push_back(message);
  }
  project_.mark_scene_changed();
  return result;
}

core::SimulationTrace ProjectService::run_simulation(const std::string& trajectory_id) {
  const core::Trajectory* trajectory = nullptr;
  for (const auto& t : project_.scene().trajectories) {
    if ((trajectory_id.empty() && trajectory == nullptr) || t.id == trajectory_id) {
      trajectory = &t;
      if (!trajectory_id.empty()) {
        break;
      }
    }
  }
  if (trajectory == nullptr) {
    project_.scene().validation_messages.push_back(core::ValidationMessage{
        .status = core::ValidationStatus::Error,
        .source = "guinmotion.app_core.simulation",
        .message = "没有可仿真的轨迹。",
        .related_object_id = trajectory_id,
    });
    project_.mark_scene_changed();
    return {};
  }

  const core::RobotModel* robot = nullptr;
  for (const auto& r : project_.scene().robot_models) {
    if (r.id == trajectory->robot_model_id) {
      robot = &r;
      break;
    }
  }
  if (robot == nullptr) {
    project_.scene().validation_messages.push_back(core::ValidationMessage{
        .status = core::ValidationStatus::Error,
        .source = "guinmotion.app_core.simulation",
        .message = "未找到轨迹对应机器人模型：" + trajectory->robot_model_id,
        .related_object_id = trajectory->id,
    });
    project_.mark_scene_changed();
    return {};
  }

  sim::MujocoSimulationEngine engine;
  auto result = engine.run_trajectory(*robot, *trajectory, robot->source_path);
  for (const auto& msg : result.messages) {
    project_.scene().validation_messages.push_back(msg);
  }
  if (result.ok) {
    project_.scene().simulation_traces.push_back(result.trace);
  }
  project_.mark_scene_changed();
  return result.trace;
}

core::TrajectoryEvaluation ProjectService::evaluate_trajectory(
    const std::string& trajectory_id, const std::string& target_point_set_id) {
  const core::Trajectory* trajectory = nullptr;
  for (const auto& t : project_.scene().trajectories) {
    if ((trajectory_id.empty() && trajectory == nullptr) || t.id == trajectory_id) {
      trajectory = &t;
      if (!trajectory_id.empty()) {
        break;
      }
    }
  }
  const core::TargetPointSet* targets = nullptr;
  for (const auto& set : project_.scene().target_point_sets) {
    if ((target_point_set_id.empty() && targets == nullptr) || set.id == target_point_set_id) {
      targets = &set;
      if (!target_point_set_id.empty()) {
        break;
      }
    }
  }
  if (trajectory == nullptr || targets == nullptr) {
    core::TrajectoryEvaluation evaluation;
    evaluation.status = core::ValidationStatus::Error;
    evaluation.messages.push_back(core::ValidationMessage{
        .status = core::ValidationStatus::Error,
        .source = "guinmotion.app_core.evaluation",
        .message = "评价需要至少一条轨迹和一个目标点集。",
        .related_object_id = trajectory_id.empty() ? target_point_set_id : trajectory_id,
    });
    project_.scene().trajectory_evaluations.push_back(evaluation);
    project_.scene().validation_messages.insert(project_.scene().validation_messages.end(),
                                                evaluation.messages.begin(),
                                                evaluation.messages.end());
    project_.mark_scene_changed();
    return evaluation;
  }

  const core::RobotModel* robot = nullptr;
  for (const auto& r : project_.scene().robot_models) {
    if (r.id == trajectory->robot_model_id) {
      robot = &r;
      break;
    }
  }
  const core::SimulationTrace* trace = nullptr;
  for (const auto& tr : project_.scene().simulation_traces) {
    if (tr.trajectory_id == trajectory->id) {
      trace = &tr;
    }
  }
  core::TrajectoryEvaluation evaluation =
      robot == nullptr ? core::TrajectoryEvaluation{} :
                         sim::evaluate_trajectory(*robot, *trajectory, *targets, trace);
  if (robot == nullptr) {
    evaluation.status = core::ValidationStatus::Error;
    evaluation.messages.push_back(core::ValidationMessage{
        .status = core::ValidationStatus::Error,
        .source = "guinmotion.app_core.evaluation",
        .message = "未找到轨迹对应机器人模型：" + trajectory->robot_model_id,
        .related_object_id = trajectory->id,
    });
  }
  project_.scene().trajectory_evaluations.push_back(evaluation);
  project_.scene().validation_messages.insert(project_.scene().validation_messages.end(),
                                              evaluation.messages.begin(),
                                              evaluation.messages.end());
  project_.mark_scene_changed();
  return evaluation;
}

}  // namespace guinmotion::app_core
