#include "guinmotion/app_core/project_service.hpp"

#include "guinmotion/core/io/point_cloud_file.hpp"
#include "guinmotion/core/io/trajectory_xml.hpp"

#include <fstream>
#include <sstream>
#include <unordered_set>

namespace guinmotion::app_core {
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

sdk::ExecutionResult ProjectService::run_operator(sdk::Operator& op, const std::string& trajectory_id) {
  sdk::ExecutionContext context;
  context.set_scene(&project_.scene());
  context.set_target_trajectory_id(trajectory_id);
  auto result = op.execute(context);
  for (const auto& message : result.validation_messages) {
    project_.scene().validation_messages.push_back(message);
  }
  project_.mark_scene_changed();
  return result;
}

}  // namespace guinmotion::app_core
