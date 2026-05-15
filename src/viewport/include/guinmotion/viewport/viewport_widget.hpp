#pragma once

#include "guinmotion/core/types.hpp"

#include <QMatrix4x4>
#include <QPoint>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <cstdint>
#include <vector>

namespace guinmotion::viewport {

/// RViz-style lightweight OpenGL viewport with independent point-cloud, trajectory,
/// target, robot-model, simulation-trace, grid and axis layers.
class ViewportWidget final : public QOpenGLWidget, protected QOpenGLFunctions {
 public:
  explicit ViewportWidget(QWidget* parent = nullptr);

  void set_scene(const core::Scene& scene, std::uint64_t scene_revision);
  void set_playback_time(double seconds);
  void set_layer_visible(int layer, bool visible);

 protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

 private:
  void setup_vertex_layout(QOpenGLVertexArrayObject& vao, QOpenGLBuffer& buffer);
  void ensure_program();
  void upload_if_dirty();
  void draw_layers(const QMatrix4x4& mvp);

  QOpenGLShaderProgram program_;
  QOpenGLBuffer cloud_buffer_{QOpenGLBuffer::VertexBuffer};
  QOpenGLBuffer traj_buffer_{QOpenGLBuffer::VertexBuffer};
  QOpenGLBuffer wp_buffer_{QOpenGLBuffer::VertexBuffer};
  QOpenGLBuffer robot_buffer_{QOpenGLBuffer::VertexBuffer};
  QOpenGLBuffer target_buffer_{QOpenGLBuffer::VertexBuffer};
  QOpenGLBuffer trace_buffer_{QOpenGLBuffer::VertexBuffer};
  QOpenGLBuffer grid_buffer_{QOpenGLBuffer::VertexBuffer};
  QOpenGLVertexArrayObject cloud_vao_;
  QOpenGLVertexArrayObject traj_vao_;
  QOpenGLVertexArrayObject wp_vao_;
  QOpenGLVertexArrayObject robot_vao_;
  QOpenGLVertexArrayObject target_vao_;
  QOpenGLVertexArrayObject trace_vao_;
  QOpenGLVertexArrayObject grid_vao_;

  std::vector<float> cloud_xyz_;
  std::vector<float> traj_xyz_;
  std::vector<float> wp_xyz_;
  std::vector<float> robot_xyz_;
  std::vector<float> target_xyz_;
  std::vector<float> trace_xyz_;
  std::vector<float> grid_xyz_;
  std::vector<int> traj_first_;
  std::vector<int> traj_count_;
  std::vector<int> trace_first_;
  std::vector<int> trace_count_;

  std::uint64_t last_revision_{0};
  bool geometry_dirty_{true};
  bool layer_visible_[7]{true, true, true, true, true, true, true};
  double playback_time_seconds_{0.0};
  float camera_yaw_{42.0F};
  float camera_pitch_{28.0F};
  float camera_distance_scale_{1.0F};
  QPoint last_mouse_pos_{};
  int width_{1};
  int height_{1};
};

}  // namespace guinmotion::viewport
