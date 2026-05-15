#pragma once

#include "guinmotion/core/types.hpp"
#include "guinmotion/operator_runtime/operator_registry.hpp"
#include "guinmotion/operator_runtime/plugin_api.h"

#include <string>
#include <vector>

namespace guinmotion::core {
class Scene;
}

namespace guinmotion::sdk {

using core::PointCloud;
using core::RobotModel;
using core::RobotState;
using core::Trajectory;
using core::ValidationMessage;
using operator_runtime::OperatorMetadata;

struct ExecutionLog {
  std::string level;
  std::string message;
};

struct ExecutionResult {
  std::vector<Trajectory> trajectories;
  std::vector<PointCloud> point_clouds;
  std::vector<ValidationMessage> validation_messages;
  std::vector<ExecutionLog> logs;
};

class ExecutionContext {
 public:
  void request_cancel() noexcept { cancel_requested_ = true; }
  [[nodiscard]] bool cancel_requested() const noexcept { return cancel_requested_; }

  /// Scene under validation / execution (may be null for headless unit tests that inject data later).
  void set_scene(core::Scene* scene) noexcept { scene_ = scene; }
  [[nodiscard]] core::Scene* scene() const noexcept { return scene_; }

  /// Empty string means: use the first trajectory in the scene, if any.
  void set_target_trajectory_id(std::string trajectory_id) { target_trajectory_id_ = std::move(trajectory_id); }
  [[nodiscard]] const std::string& target_trajectory_id() const noexcept { return target_trajectory_id_; }

  /// Empty string means: use the first target point set in the scene, if any.
  void set_target_point_set_id(std::string target_point_set_id) {
    target_point_set_id_ = std::move(target_point_set_id);
  }
  [[nodiscard]] const std::string& target_point_set_id() const noexcept { return target_point_set_id_; }

 private:
  bool cancel_requested_{false};
  core::Scene* scene_{nullptr};
  std::string target_trajectory_id_{};
  std::string target_point_set_id_{};
};

class Operator {
 public:
  virtual ~Operator() = default;

  [[nodiscard]] virtual OperatorMetadata metadata() const = 0;
  [[nodiscard]] virtual ExecutionResult execute(ExecutionContext& context) = 0;
};

}  // namespace guinmotion::sdk
