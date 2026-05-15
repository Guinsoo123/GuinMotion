#include "guinmotion/sdk/builtin_operators.hpp"

#include "guinmotion/sdk/operator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace guinmotion::sdk {
namespace {

constexpr const char* kDurationOpId = "guinmotion.trajectory.duration_check";
constexpr const char* kJointLimitOpId = "guinmotion.joint.limit_check";
constexpr const char* kTargetDemoOpId = "guinmotion.examples.target_demo_planner";

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

class TargetDemoPlannerOperator final : public Operator {
 public:
  [[nodiscard]] OperatorMetadata metadata() const override {
    return OperatorMetadata{
        .id = kTargetDemoOpId,
        .name = "目标点演示轨迹生成",
        .version = "0.1.0",
        .description = "从导入的目标点集生成一条用于端到端验收的示例关节轨迹。",
    };
  }

  ExecutionResult execute(ExecutionContext& context) override {
    ExecutionResult out;
    const auto* scene = context.scene();
    if (scene == nullptr || scene->target_point_sets.empty()) {
      out.validation_messages.push_back(core::ValidationMessage{
          .status = core::ValidationStatus::Error,
          .source = kTargetDemoOpId,
          .message = "没有可用于生成轨迹的目标点集。",
      });
      return out;
    }
    const core::TargetPointSet* target_set = nullptr;
    for (const auto& set : scene->target_point_sets) {
      if ((context.target_point_set_id().empty() && target_set == nullptr) ||
          set.id == context.target_point_set_id()) {
        target_set = &set;
        if (!context.target_point_set_id().empty()) {
          break;
        }
      }
    }
    if (target_set == nullptr) {
      out.validation_messages.push_back(core::ValidationMessage{
          .status = core::ValidationStatus::Error,
          .source = kTargetDemoOpId,
          .message = "未找到指定目标点集：" + context.target_point_set_id(),
      });
      return out;
    }
    const auto& targets = *target_set;
    const core::RobotModel* robot = nullptr;
    for (const auto& r : scene->robot_models) {
      if ((!targets.robot_model_id.empty() && r.id == targets.robot_model_id) ||
          (targets.robot_model_id.empty() && robot == nullptr)) {
        robot = &r;
        if (!targets.robot_model_id.empty()) {
          break;
        }
      }
    }
    if (robot == nullptr) {
      out.validation_messages.push_back(core::ValidationMessage{
          .status = core::ValidationStatus::Error,
          .source = kTargetDemoOpId,
          .message = "没有可用于目标点轨迹生成的机器人模型。",
          .related_object_id = targets.id,
      });
      return out;
    }

    core::Trajectory trajectory;
    trajectory.id = targets.id + "_demo_trajectory";
    trajectory.name = targets.name + " 演示轨迹";
    trajectory.robot_model_id = robot->id;
    trajectory.interpolation = core::InterpolationMode::LinearJoint;
    for (std::size_t i = 0; i < targets.targets.size(); ++i) {
      const auto& target = targets.targets[i];
      core::Waypoint wp;
      wp.id = target.id + "_wp";
      wp.label = target.name;
      wp.time_seconds = target.time_hint_seconds > 0.0 ? target.time_hint_seconds : static_cast<double>(i);
      wp.duration_seconds = i == 0 ? 0.0 : 1.0;
      wp.tool_pose.translation = target.pose.position;
      for (int q = 0; q < 4; ++q) {
        wp.tool_pose.rotation[q] = target.pose.orientation[q];
      }
      wp.state.robot_model_id = robot->id;
      wp.state.joint_positions_radians.assign(robot->joints.size(), 0.0);
      if (!wp.state.joint_positions_radians.empty()) {
        wp.state.joint_positions_radians[0] = std::atan2(target.pose.position.y, target.pose.position.x);
      }
      if (wp.state.joint_positions_radians.size() > 1) {
        const double planar = std::hypot(target.pose.position.x, target.pose.position.y);
        wp.state.joint_positions_radians[1] = std::clamp(planar - 0.24, -0.6, 0.6);
      }
      if (wp.state.joint_positions_radians.size() > 2) {
        wp.state.joint_positions_radians[2] = std::clamp(target.pose.position.z, -0.6, 0.6);
      }
      trajectory.waypoints.push_back(std::move(wp));
    }
    out.trajectories.push_back(std::move(trajectory));
    out.validation_messages.push_back(core::ValidationMessage{
        .status = core::ValidationStatus::Valid,
        .source = kTargetDemoOpId,
        .message = "已根据目标点生成演示轨迹。",
        .related_object_id = targets.id,
    });
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
  if (operator_id == kTargetDemoOpId) {
    return std::make_unique<TargetDemoPlannerOperator>();
  }
  return nullptr;
}

}  // namespace guinmotion::sdk
