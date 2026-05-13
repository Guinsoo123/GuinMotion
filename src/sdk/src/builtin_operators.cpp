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
        .name = "轨迹时长检查",
        .version = "0.1.0",
        .description = "校验路点 duration_seconds 与 time_seconds 单调性。",
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
      msg << "没有可检查的轨迹";
      if (!tid.empty()) {
        msg << "（未找到 id：" << tid << "）";
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
            .message = "duration_seconds 不是有限数值",
            .related_object_id = wp.id,
        });
      } else if (d < 0.0) {
        out.validation_messages.push_back(core::ValidationMessage{
            .status = core::ValidationStatus::Error,
            .source = kDurationOpId,
            .message = "duration_seconds 为负",
            .related_object_id = wp.id,
        });
      }

      const double t = wp.time_seconds;
      if (!std::isfinite(t)) {
        out.validation_messages.push_back(core::ValidationMessage{
            .status = core::ValidationStatus::Error,
            .source = kDurationOpId,
            .message = "time_seconds 不是有限数值",
            .related_object_id = wp.id,
        });
      } else if (t < previous_time) {
        out.validation_messages.push_back(core::ValidationMessage{
            .status = core::ValidationStatus::Warning,
            .source = kDurationOpId,
            .message = "time_seconds 相对上一路点变小（非单调）",
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
        .name = "关节限位检查",
        .version = "0.1.0",
        .description = "按 RobotModel 关节限位校验关节位置（转动关节为弧度）。",
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
      msg << "没有可检查的轨迹";
      if (!tid.empty()) {
        msg << "（未找到 id：" << tid << "）";
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
          .message = "未找到轨迹 robot_model_id 对应的机器人模型：" + trajectory->robot_model_id,
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
            .message = "关节数量 " + std::to_string(q.size()) + " 与机器人自由度 " + std::to_string(dof) +
                       " 不一致",
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
              .message = "关节 " + std::to_string(j) + " 的位置不是有限数值",
              .related_object_id = wp.id,
          });
          continue;
        }
        const auto& lim = robot->joints[j].limit;
        if (v < lim.lower || v > lim.upper) {
          out.validation_messages.push_back(core::ValidationMessage{
              .status = core::ValidationStatus::Error,
              .source = kJointLimitOpId,
              .message = "关节 " + robot->joints[j].name + " 超出限位 [" + std::to_string(lim.lower) + ", " +
                         std::to_string(lim.upper) + "]，当前值=" + std::to_string(v),
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
