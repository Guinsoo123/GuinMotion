#include "guinmotion/sdk/builtin_operators.hpp"

#include "guinmotion/sdk/operator.hpp"

#include <cmath>
#include <limits>
#include <sstream>

namespace guinmotion::sdk {
namespace {

constexpr const char* kDurationOpId = "guinmotion.trajectory.duration_check";
constexpr const char* kJointLimitOpId = "guinmotion.joint.limit_check";

[[nodiscard]] const core::Trajectory* pick_trajectory(const core::Scene* scene,
                                                        const std::string& trajectory_id) {
  if (scene == nullptr) {
    return nullptr;
  }
  if (!trajectory_id.empty()) {
    for (const auto& t : scene->trajectories) {
      if (t.id == trajectory_id) {
        return &t;
      }
    }
    return nullptr;
  }
  if (scene->trajectories.empty()) {
    return nullptr;
  }
  return &scene->trajectories.front();
}

[[nodiscard]] const core::RobotModel* find_robot(const core::Scene* scene, const std::string& robot_id) {
  if (scene == nullptr) {
    return nullptr;
  }
  for (const auto& r : scene->robot_models) {
    if (r.id == robot_id) {
      return &r;
    }
  }
  return nullptr;
}

class TrajectoryDurationCheckOperator final : public Operator {
 public:
  [[nodiscard]] OperatorMetadata metadata() const override {
    return OperatorMetadata{
        .id = kDurationOpId,
        .name = "Trajectory Duration Check",
        .version = "0.1.0",
        .description = "Validate waypoint duration_seconds and time_seconds monotonicity.",
    };
  }

  ExecutionResult execute(ExecutionContext& context) override {
    ExecutionResult out;
    const auto* scene = context.scene();
    const auto* trajectory =
        pick_trajectory(scene, context.target_trajectory_id());
    if (trajectory == nullptr) {
      const std::string tid = context.target_trajectory_id();
      std::ostringstream msg;
      msg << "No trajectory to check";
      if (!tid.empty()) {
        msg << " (id not found: " << tid << ")";
      }
      out.validation_messages.push_back(core::ValidationMessage{
          .status = core::ValidationStatus::Error,
          .source = kDurationOpId,
          .message = msg.str(),
          .related_object_id = tid,
      });
      return out;
    }

    double previous_time = -std::numeric_limits<double>::infinity();
    for (const auto& wp : trajectory->waypoints) {
      const double d = wp.duration_seconds;
      if (!std::isfinite(d)) {
        out.validation_messages.push_back(core::ValidationMessage{
            .status = core::ValidationStatus::Error,
            .source = kDurationOpId,
            .message = "duration_seconds is not finite",
            .related_object_id = wp.id,
        });
      } else if (d < 0.0) {
        out.validation_messages.push_back(core::ValidationMessage{
            .status = core::ValidationStatus::Error,
            .source = kDurationOpId,
            .message = "duration_seconds is negative",
            .related_object_id = wp.id,
        });
      }

      const double t = wp.time_seconds;
      if (!std::isfinite(t)) {
        out.validation_messages.push_back(core::ValidationMessage{
            .status = core::ValidationStatus::Error,
            .source = kDurationOpId,
            .message = "time_seconds is not finite",
            .related_object_id = wp.id,
        });
      } else if (t < previous_time) {
        out.validation_messages.push_back(core::ValidationMessage{
            .status = core::ValidationStatus::Warning,
            .source = kDurationOpId,
            .message = "time_seconds decreased relative to previous waypoint",
            .related_object_id = wp.id,
        });
      }
      previous_time = t;
    }

    return out;
  }
};

class JointLimitCheckOperator final : public Operator {
 public:
  [[nodiscard]] OperatorMetadata metadata() const override {
    return OperatorMetadata{
        .id = kJointLimitOpId,
        .name = "Joint Limit Check",
        .version = "0.1.0",
        .description = "Validate joint positions against RobotModel limits (radians for revolute).",
    };
  }

  ExecutionResult execute(ExecutionContext& context) override {
    ExecutionResult out;
    const auto* scene = context.scene();
    const auto* trajectory =
        pick_trajectory(scene, context.target_trajectory_id());
    if (trajectory == nullptr) {
      const std::string tid = context.target_trajectory_id();
      std::ostringstream msg;
      msg << "No trajectory to check";
      if (!tid.empty()) {
        msg << " (id not found: " << tid << ")";
      }
      out.validation_messages.push_back(core::ValidationMessage{
          .status = core::ValidationStatus::Error,
          .source = kJointLimitOpId,
          .message = msg.str(),
          .related_object_id = tid,
      });
      return out;
    }

    const auto* robot = find_robot(scene, trajectory->robot_model_id);
    if (robot == nullptr) {
      out.validation_messages.push_back(core::ValidationMessage{
          .status = core::ValidationStatus::Error,
          .source = kJointLimitOpId,
          .message = "Robot model not found for trajectory.robot_model_id: " + trajectory->robot_model_id,
          .related_object_id = trajectory->id,
      });
      return out;
    }

    const std::size_t dof = robot->joints.size();
    for (const auto& wp : trajectory->waypoints) {
      const auto& q = wp.state.joint_positions_radians;
      if (q.size() != dof) {
        out.validation_messages.push_back(core::ValidationMessage{
            .status = core::ValidationStatus::Error,
            .source = kJointLimitOpId,
            .message = "Joint count " + std::to_string(q.size()) + " does not match robot DOF " +
                       std::to_string(dof),
            .related_object_id = wp.id,
        });
        continue;
      }
      for (std::size_t j = 0; j < dof; ++j) {
        const double v = q[j];
        if (!std::isfinite(v)) {
          out.validation_messages.push_back(core::ValidationMessage{
              .status = core::ValidationStatus::Error,
              .source = kJointLimitOpId,
              .message = "Joint " + std::to_string(j) + " position is not finite",
              .related_object_id = wp.id,
          });
          continue;
        }
        const auto& lim = robot->joints[j].limit;
        if (v < lim.lower || v > lim.upper) {
          out.validation_messages.push_back(core::ValidationMessage{
              .status = core::ValidationStatus::Error,
              .source = kJointLimitOpId,
              .message = "Joint " + robot->joints[j].name + " out of limits [" + std::to_string(lim.lower) +
                         ", " + std::to_string(lim.upper) + "], value=" + std::to_string(v),
              .related_object_id = wp.id,
          });
        }
      }
    }

    return out;
  }
};

}  // namespace

std::unique_ptr<Operator> make_builtin_operator(const std::string& operator_id) {
  if (operator_id == kDurationOpId) {
    return std::make_unique<TrajectoryDurationCheckOperator>();
  }
  if (operator_id == kJointLimitOpId) {
    return std::make_unique<JointLimitCheckOperator>();
  }
  return nullptr;
}

}  // namespace guinmotion::sdk
