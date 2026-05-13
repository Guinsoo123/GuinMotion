#pragma once

#include "guinmotion/core/types.hpp"
#include "guinmotion/operator_runtime/operator_registry.hpp"
#include "guinmotion/operator_runtime/plugin_api.h"

#include <string>
#include <vector>

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

 private:
  bool cancel_requested_{false};
};

class Operator {
 public:
  virtual ~Operator() = default;

  [[nodiscard]] virtual OperatorMetadata metadata() const = 0;
  [[nodiscard]] virtual ExecutionResult execute(ExecutionContext& context) = 0;
};

}  // namespace guinmotion::sdk
