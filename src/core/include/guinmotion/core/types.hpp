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

struct Pose {
  Vec3 position{};
  // Quaternion order: w, x, y, z.
  double orientation[4]{1.0, 0.0, 0.0, 0.0};
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
  Continuous,
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
  std::string parent_link;
  std::string child_link;
  Transform origin{};
  Vec3 axis{0.0, 0.0, 1.0};
  JointLimit limit{};
};

enum class GeometryType {
  None,
  Box,
  Sphere,
  Cylinder,
  Mesh,
};

struct VisualGeometry {
  GeometryType type{GeometryType::None};
  Vec3 size{0.05, 0.05, 0.05};
  double radius{0.025};
  double length{0.1};
  std::string mesh_uri;
  Transform origin{};
  std::string material_name;
  Color color{0.72F, 0.74F, 0.78F, 1.0F};
};

struct Link {
  std::string name;
  std::string visual_geometry_uri;
  std::string collision_geometry_uri;
  std::vector<VisualGeometry> visuals;
};

struct RobotModel {
  ObjectId id;
  std::string name;
  std::string source_path;
  std::string base_frame{"robot_base"};
  std::string tool_frame{"tool"};
  std::vector<Joint> joints;
  std::vector<Link> links;
  std::vector<double> home_joint_positions_radians;
};

struct RobotInstance {
  ObjectId id;
  ObjectId robot_model_id;
  std::string name;
  Transform world_from_base{};
  RobotState state;
};

struct TargetPoint {
  ObjectId id;
  std::string name;
  Pose pose{};
  double tolerance_m{0.01};
  double time_hint_seconds{0.0};
};

struct TargetPointSet {
  ObjectId id;
  std::string name;
  ObjectId robot_model_id;
  std::vector<TargetPoint> targets;
};

struct SimulationSample {
  double time_seconds{0.0};
  RobotState state;
  Pose tool_pose{};
};

struct SimulationTrace {
  ObjectId id;
  std::string name;
  ObjectId robot_model_id;
  ObjectId trajectory_id;
  bool used_mujoco{false};
  std::vector<SimulationSample> samples;
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

struct TrajectoryEvaluation {
  ObjectId id;
  ObjectId trajectory_id;
  ObjectId target_point_set_id;
  ValidationStatus status{ValidationStatus::Unknown};
  double max_position_error_m{0.0};
  std::vector<ValidationMessage> messages;
};

struct Scene {
  std::vector<PointCloud> point_clouds;
  std::vector<PointAnnotation> points;
  std::vector<TargetPointSet> target_point_sets;
  std::vector<Trajectory> trajectories;
  std::vector<RobotModel> robot_models;
  std::vector<RobotInstance> robot_instances;
  std::vector<SimulationTrace> simulation_traces;
  std::vector<TrajectoryEvaluation> trajectory_evaluations;
  std::vector<ValidationMessage> validation_messages;
};

}  // namespace guinmotion::core
