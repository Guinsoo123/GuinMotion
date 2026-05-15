#pragma once

#include "guinmotion/core/io/import_result.hpp"
#include "guinmotion/core/project.hpp"
#include "guinmotion/sdk/operator.hpp"

#include <filesystem>
#include <string_view>

namespace guinmotion::app_core {

/// Owns a `Project` and coordinates imports (trajectory XML, point cloud files).
class ProjectService {
 public:
  explicit ProjectService(core::Project project);

  [[nodiscard]] core::Project& project() noexcept { return project_; }
  [[nodiscard]] const core::Project& project() const noexcept { return project_; }

  [[nodiscard]] core::io::ImportResult import_trajectory_xml(std::string_view xml);
  [[nodiscard]] core::io::ImportResult import_trajectory_xml_file(const std::filesystem::path& path);
  [[nodiscard]] core::io::ImportResult import_point_cloud_file(
      const std::filesystem::path& path, const core::io::ImportLimits& limits = {});
  [[nodiscard]] core::io::ImportResult import_robot_model_urdf_file(const std::filesystem::path& path);
  [[nodiscard]] core::io::ImportResult import_target_points_xml_file(const std::filesystem::path& path);

  /// Runs `op` against `trajectory_id` (empty id: first trajectory). Appends `validation_messages` to the scene
  /// and bumps `scene_revision`.
  [[nodiscard]] sdk::ExecutionResult run_operator(sdk::Operator& op, const std::string& trajectory_id);
  [[nodiscard]] core::SimulationTrace run_simulation(const std::string& trajectory_id);
  [[nodiscard]] core::TrajectoryEvaluation evaluate_trajectory(
      const std::string& trajectory_id, const std::string& target_point_set_id);

 private:
  core::Project project_;

  void ensure_unique_trajectory_id(core::Trajectory& trajectory);
  void ensure_unique_point_cloud_id(core::PointCloud& cloud);
  void ensure_unique_robot_model_id(core::RobotModel& robot);
  void ensure_unique_target_point_set_id(core::TargetPointSet& targets);
};

}  // namespace guinmotion::app_core
