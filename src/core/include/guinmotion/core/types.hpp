#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace guinmotion::core {

using ObjectId = std::string;

struct Vec3 {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

struct Color {
  float r{1.0F};
  float g{1.0F};
  float b{1.0F};
  float a{1.0F};
};

struct Transform {
  Vec3 translation{};
  // Quaternion order: w, x, y, z.
  double rotation[4]{1.0, 0.0, 0.0, 0.0};
};

struct MetadataEntry {
  std::string key;
  std::string value;
};

struct PointCloud {
  ObjectId id;
  std::string name;
  std::string frame{"world"};
  std::vector<Vec3> positions;
  std::vector<Color> colors;
  std::vector<Vec3> normals;
  std::vector<MetadataEntry> metadata;
};

struct PointAnnotation {
  ObjectId id;
  std::string label;
  std::string frame{"world"};
  Vec3 position{};
  Color color{};
  double confidence{1.0};
};

struct RobotState {
  ObjectId robot_model_id;
  std::vector<double> joint_positions_radians;
  std::vector<double> joint_velocities;
};

struct Waypoint {
  ObjectId id;
  std::string label;
  RobotState state;
  double time_seconds{0.0};
  double duration_seconds{0.0};
  Transform tool_pose{};
};

enum class InterpolationMode {
  Hold,
  LinearJoint,
  CubicJoint,
  CartesianLinear,
};

struct Trajectory {
  ObjectId id;
  std::string name;
  ObjectId robot_model_id;
  InterpolationMode interpolation{InterpolationMode::LinearJoint};
  std::vector<Waypoint> waypoints;
};

enum class JointType {
  Revolute,
  Prismatic,
  Fixed,
};

struct JointLimit {
  double lower{0.0};
  double upper{0.0};
  double velocity{0.0};
  double acceleration{0.0};
};

struct Joint {
  std::string name;
  JointType type{JointType::Revolute};
  JointLimit limit{};
};

struct Link {
  std::string name;
  std::string visual_geometry_uri;
  std::string collision_geometry_uri;
};

struct RobotModel {
  ObjectId id;
  std::string name;
  std::string base_frame{"robot_base"};
  std::string tool_frame{"tool"};
  std::vector<Joint> joints;
  std::vector<Link> links;
};

enum class ValidationStatus {
  Unknown,
  Valid,
  Warning,
  Error,
  Skipped,
};

struct ValidationMessage {
  ValidationStatus status{ValidationStatus::Unknown};
  std::string source;
  std::string message;
  ObjectId related_object_id;
};

struct Scene {
  std::vector<PointCloud> point_clouds;
  std::vector<PointAnnotation> points;
  std::vector<Trajectory> trajectories;
  std::vector<RobotModel> robot_models;
  std::vector<ValidationMessage> validation_messages;
};

}  // namespace guinmotion::core
