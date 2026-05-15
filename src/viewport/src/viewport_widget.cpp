#include "guinmotion/viewport/viewport_widget.hpp"

#include <QMouseEvent>
#include <QOpenGLShader>
#include <QVector3D>
#include <QWheelEvent>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace guinmotion::viewport {
namespace {

constexpr int kLayerCloud = 0;
constexpr int kLayerTrajectory = 1;
constexpr int kLayerWaypoint = 2;
constexpr int kLayerRobot = 3;
constexpr int kLayerTarget = 4;
constexpr int kLayerTrace = 5;
constexpr int kLayerGrid = 6;

constexpr char kVs[] = R"GLSL(
#version 330 core
layout(location = 0) in vec3 position;
uniform mat4 mvp;
uniform int layer;
void main() {
  gl_Position = mvp * vec4(position, 1.0);
  if (layer == 0) gl_PointSize = 3.0;
  else if (layer == 2 || layer == 4) gl_PointSize = 10.0;
  else gl_PointSize = 1.0;
}
)GLSL";

constexpr char kFs[] = R"GLSL(
#version 330 core
uniform int layer;
out vec4 frag_color;
void main() {
  if (layer == 0) frag_color = vec4(0.55, 0.78, 1.0, 1.0);
  else if (layer == 1) frag_color = vec4(1.0, 0.86, 0.22, 1.0);
  else if (layer == 2) frag_color = vec4(1.0, 0.35, 0.12, 1.0);
  else if (layer == 3) frag_color = vec4(0.76, 0.78, 0.82, 1.0);
  else if (layer == 4) frag_color = vec4(0.25, 1.0, 0.62, 1.0);
  else if (layer == 5) frag_color = vec4(0.9, 0.45, 1.0, 1.0);
  else frag_color = vec4(0.35, 0.38, 0.42, 1.0);
}
)GLSL";

void push(std::vector<float>& out, const core::Vec3& p) {
  out.push_back(static_cast<float>(p.x));
  out.push_back(static_cast<float>(p.y));
  out.push_back(static_cast<float>(p.z));
}

void push(std::vector<float>& out, float x, float y, float z) {
  out.push_back(x);
  out.push_back(y);
  out.push_back(z);
}

[[nodiscard]] core::Vec3 state_tool_position(
    const core::Scene& scene, const core::Trajectory& trajectory, const core::RobotState& state) {
  for (const auto& robot : scene.robot_models) {
    if (robot.id != trajectory.robot_model_id) {
      continue;
    }
    double heading = 0.0;
    double reach = 0.0;
    const std::size_t n = std::min(robot.joints.size(), state.joint_positions_radians.size());
    for (std::size_t i = 0; i < n; ++i) {
      const auto& joint = robot.joints[i];
      const double q = state.joint_positions_radians[i];
      heading += joint.type == core::JointType::Prismatic ? 0.0 : q;
      const double len = std::max(std::abs(joint.origin.translation.x) +
                                      std::abs(joint.origin.translation.y) +
                                      std::abs(joint.origin.translation.z),
                                  0.12);
      reach += joint.type == core::JointType::Prismatic ? q : len;
    }
    return core::Vec3{reach * std::cos(heading), reach * std::sin(heading), 0.05 * n};
  }
  return {0.0, 0.0, 0.0};
}

[[nodiscard]] core::Vec3 waypoint_position(
    const core::Scene& scene, const core::Trajectory& trajectory, const core::Waypoint& wp) {
  const auto& p = wp.tool_pose.translation;
  if (std::abs(p.x) > 1e-9 || std::abs(p.y) > 1e-9 || std::abs(p.z) > 1e-9) {
    return p;
  }
  return state_tool_position(scene, trajectory, wp.state);
}

[[nodiscard]] QMatrix4x4 compute_mvp(
    int viewport_w,
    int viewport_h,
    const QVector3D& center,
    float radius,
    float yaw_deg,
    float pitch_deg,
    float distance_scale) {
  QMatrix4x4 proj;
  const float aspect =
      viewport_h > 0 ? static_cast<float>(viewport_w) / static_cast<float>(viewport_h) : 1.0F;
  proj.perspective(45.0F, aspect, 0.05F, 1000.0F);

  const float dist = std::max(radius * 2.5F * distance_scale, 0.8F);
  const float yaw = yaw_deg * 3.14159265F / 180.0F;
  const float pitch = std::clamp(pitch_deg, -85.0F, 85.0F) * 3.14159265F / 180.0F;
  const QVector3D dir(std::cos(pitch) * std::cos(yaw), std::sin(pitch),
                      std::cos(pitch) * std::sin(yaw));
  QMatrix4x4 view;
  view.lookAt(center + dir * dist, center, QVector3D(0.0F, 0.0F, 1.0F));
  return proj * view;
}

void setup(QOpenGLVertexArrayObject& vao, QOpenGLBuffer& buffer) {
  vao.create();
  buffer.create();
  vao.bind();
  buffer.bind();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  buffer.release();
  vao.release();
}

}  // namespace

ViewportWidget::ViewportWidget(QWidget* parent) : QOpenGLWidget(parent) {
  setMinimumSize(400, 300);
}

void ViewportWidget::set_layer_visible(int layer, bool visible) {
  if (layer < 0 || layer >= 7) {
    return;
  }
  layer_visible_[layer] = visible;
  update();
}

void ViewportWidget::set_playback_time(double seconds) {
  playback_time_seconds_ = std::max(0.0, seconds);
  update();
}

void ViewportWidget::set_scene(const core::Scene& scene, const std::uint64_t scene_revision) {
  last_revision_ = scene_revision;
  cloud_xyz_.clear();
  traj_xyz_.clear();
  wp_xyz_.clear();
  robot_xyz_.clear();
  target_xyz_.clear();
  trace_xyz_.clear();
  grid_xyz_.clear();
  traj_first_.clear();
  traj_count_.clear();
  trace_first_.clear();
  trace_count_.clear();

  for (int i = -10; i <= 10; ++i) {
    push(grid_xyz_, static_cast<float>(i) * 0.1F, -1.0F, 0.0F);
    push(grid_xyz_, static_cast<float>(i) * 0.1F, 1.0F, 0.0F);
    push(grid_xyz_, -1.0F, static_cast<float>(i) * 0.1F, 0.0F);
    push(grid_xyz_, 1.0F, static_cast<float>(i) * 0.1F, 0.0F);
  }
  push(grid_xyz_, 0.0F, 0.0F, 0.0F);
  push(grid_xyz_, 0.35F, 0.0F, 0.0F);
  push(grid_xyz_, 0.0F, 0.0F, 0.0F);
  push(grid_xyz_, 0.0F, 0.35F, 0.0F);
  push(grid_xyz_, 0.0F, 0.0F, 0.0F);
  push(grid_xyz_, 0.0F, 0.0F, 0.35F);

  for (const auto& cloud : scene.point_clouds) {
    for (const auto& p : cloud.positions) {
      push(cloud_xyz_, p);
    }
  }

  for (const auto& set : scene.target_point_sets) {
    for (const auto& target : set.targets) {
      push(target_xyz_, target.pose.position);
    }
  }

  for (const auto& robot : scene.robot_models) {
    core::Vec3 cursor{0.0, 0.0, 0.0};
    for (const auto& joint : robot.joints) {
      core::Vec3 next{cursor.x + joint.origin.translation.x,
                      cursor.y + joint.origin.translation.y,
                      cursor.z + joint.origin.translation.z};
      if (std::abs(next.x - cursor.x) + std::abs(next.y - cursor.y) + std::abs(next.z - cursor.z) < 1e-9) {
        next.x += 0.12;
      }
      push(robot_xyz_, cursor);
      push(robot_xyz_, next);
      cursor = next;
    }
    if (robot.joints.empty()) {
      push(robot_xyz_, 0.0F, 0.0F, 0.0F);
      push(robot_xyz_, 0.25F, 0.0F, 0.0F);
    }
  }

  for (const auto& tr : scene.trajectories) {
    if (tr.waypoints.empty()) {
      continue;
    }
    const int first = static_cast<int>(traj_xyz_.size() / 3);
    for (const auto& wp : tr.waypoints) {
      const auto p = waypoint_position(scene, tr, wp);
      push(traj_xyz_, p);
      push(wp_xyz_, p);
    }
    traj_first_.push_back(first);
    traj_count_.push_back(static_cast<int>(tr.waypoints.size()));
  }

  for (const auto& trace : scene.simulation_traces) {
    if (trace.samples.empty()) {
      continue;
    }
    const int first = static_cast<int>(trace_xyz_.size() / 3);
    for (const auto& sample : trace.samples) {
      push(trace_xyz_, sample.tool_pose.position);
    }
    trace_first_.push_back(first);
    trace_count_.push_back(static_cast<int>(trace.samples.size()));
  }

  geometry_dirty_ = true;
  update();
}

void ViewportWidget::initializeGL() {
  initializeOpenGLFunctions();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClearColor(0.10F, 0.11F, 0.12F, 1.0F);
  glEnable(GL_PROGRAM_POINT_SIZE);
  ensure_program();

  setup(cloud_vao_, cloud_buffer_);
  setup(traj_vao_, traj_buffer_);
  setup(wp_vao_, wp_buffer_);
  setup(robot_vao_, robot_buffer_);
  setup(target_vao_, target_buffer_);
  setup(trace_vao_, trace_buffer_);
  setup(grid_vao_, grid_buffer_);
}

void ViewportWidget::ensure_program() {
  if (program_.isLinked()) {
    return;
  }
  program_.addCacheableShaderFromSourceCode(QOpenGLShader::Vertex, kVs);
  program_.addCacheableShaderFromSourceCode(QOpenGLShader::Fragment, kFs);
  if (!program_.link()) {
    qWarning("GuinMotion ViewportWidget：着色器程序链接失败：%s", qPrintable(program_.log()));
  }
}

void ViewportWidget::resizeGL(const int w, const int h) {
  width_ = std::max(w, 1);
  height_ = std::max(h, 1);
  glViewport(0, 0, width_, height_);
}

void ViewportWidget::upload_if_dirty() {
  if (!geometry_dirty_) {
    return;
  }
  geometry_dirty_ = false;
  auto upload = [](QOpenGLBuffer& buf, const std::vector<float>& data) {
    buf.bind();
    buf.allocate(data.empty() ? nullptr : data.data(), static_cast<int>(data.size() * sizeof(float)));
    buf.release();
  };
  upload(cloud_buffer_, cloud_xyz_);
  upload(traj_buffer_, traj_xyz_);
  upload(wp_buffer_, wp_xyz_);
  upload(robot_buffer_, robot_xyz_);
  upload(target_buffer_, target_xyz_);
  upload(trace_buffer_, trace_xyz_);
  upload(grid_buffer_, grid_xyz_);
}

void ViewportWidget::draw_layers(const QMatrix4x4& mvp) {
  if (!program_.isLinked() || !program_.bind()) {
    return;
  }
  program_.setUniformValue("mvp", mvp);

  auto draw_points = [&](int layer, QOpenGLVertexArrayObject& vao, QOpenGLBuffer& buf,
                         const std::vector<float>& data) {
    if (!layer_visible_[layer] || data.empty()) {
      return;
    }
    program_.setUniformValue("layer", layer);
    vao.bind();
    buf.bind();
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(data.size() / 3));
    buf.release();
    vao.release();
  };
  auto draw_lines = [&](int layer, QOpenGLVertexArrayObject& vao, QOpenGLBuffer& buf,
                        const std::vector<float>& data) {
    if (!layer_visible_[layer] || data.empty()) {
      return;
    }
    program_.setUniformValue("layer", layer);
    vao.bind();
    buf.bind();
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(data.size() / 3));
    buf.release();
    vao.release();
  };

  draw_lines(kLayerGrid, grid_vao_, grid_buffer_, grid_xyz_);
  draw_points(kLayerCloud, cloud_vao_, cloud_buffer_, cloud_xyz_);
  draw_lines(kLayerRobot, robot_vao_, robot_buffer_, robot_xyz_);
  draw_points(kLayerTarget, target_vao_, target_buffer_, target_xyz_);

  if (layer_visible_[kLayerTrajectory] && !traj_xyz_.empty()) {
    program_.setUniformValue("layer", kLayerTrajectory);
    traj_vao_.bind();
    traj_buffer_.bind();
    for (std::size_t i = 0; i < traj_first_.size(); ++i) {
      glDrawArrays(GL_LINE_STRIP, traj_first_[i], traj_count_[i]);
    }
    traj_buffer_.release();
    traj_vao_.release();
  }
  draw_points(kLayerWaypoint, wp_vao_, wp_buffer_, wp_xyz_);

  if (layer_visible_[kLayerTrace] && !trace_xyz_.empty()) {
    program_.setUniformValue("layer", kLayerTrace);
    trace_vao_.bind();
    trace_buffer_.bind();
    for (std::size_t i = 0; i < trace_first_.size(); ++i) {
      glDrawArrays(GL_LINE_STRIP, trace_first_[i], trace_count_[i]);
    }
    trace_buffer_.release();
    trace_vao_.release();
  }
  program_.release();
}

void ViewportWidget::paintGL() {
  makeCurrent();
  upload_if_dirty();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  QVector3D center(0.0F, 0.0F, 0.0F);
  float radius = 0.5F;
  std::size_t npts = 0;
  auto expand = [&](const std::vector<float>& buf) {
    for (std::size_t i = 0; i + 2 < buf.size(); i += 3) {
      center += QVector3D(buf[i], buf[i + 1], buf[i + 2]);
      ++npts;
    }
  };
  expand(cloud_xyz_);
  expand(traj_xyz_);
  expand(robot_xyz_);
  expand(target_xyz_);
  expand(trace_xyz_);
  if (npts > 0) {
    center /= static_cast<float>(npts);
    auto max_r = [&](const std::vector<float>& buf) {
      for (std::size_t i = 0; i + 2 < buf.size(); i += 3) {
        radius = std::max(radius, (QVector3D(buf[i], buf[i + 1], buf[i + 2]) - center).length());
      }
    };
    max_r(cloud_xyz_);
    max_r(traj_xyz_);
    max_r(robot_xyz_);
    max_r(target_xyz_);
    max_r(trace_xyz_);
  }

  const QMatrix4x4 mvp =
      compute_mvp(width_, height_, center, radius, camera_yaw_, camera_pitch_, camera_distance_scale_);
  draw_layers(mvp);
}

void ViewportWidget::mousePressEvent(QMouseEvent* event) {
  last_mouse_pos_ = event->pos();
}

void ViewportWidget::mouseMoveEvent(QMouseEvent* event) {
  const QPoint delta = event->pos() - last_mouse_pos_;
  last_mouse_pos_ = event->pos();
  if (event->buttons() & Qt::LeftButton) {
    camera_yaw_ += static_cast<float>(delta.x()) * 0.4F;
    camera_pitch_ += static_cast<float>(delta.y()) * 0.35F;
    update();
  }
}

void ViewportWidget::wheelEvent(QWheelEvent* event) {
  const float ticks = static_cast<float>(event->angleDelta().y()) / 120.0F;
  camera_distance_scale_ = std::clamp(camera_distance_scale_ * std::pow(0.88F, ticks), 0.25F, 8.0F);
  update();
}

}  // namespace guinmotion::viewport
