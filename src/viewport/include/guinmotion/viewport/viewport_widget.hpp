#pragma once

#include "guinmotion/core/types.hpp"

#include <QMatrix4x4>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <cstdint>
#include <vector>

namespace guinmotion::viewport {

/// Minimal OpenGL viewport: point clouds (`GL_POINTS`), trajectory polylines (`GL_LINE_STRIP`),
/// waypoint markers (`GL_POINTS` with larger raster size in shader).
///
/// Joint-space trajectories are mapped to R³ using the first three joint angles scaled
/// (visual placeholder until forward kinematics / URDF exist).
class ViewportWidget final : public QOpenGLWidget, protected QOpenGLFunctions {
 public:
  explicit ViewportWidget(QWidget* parent = nullptr);

  void set_scene(const core::Scene& scene, std::uint64_t scene_revision);

 protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

 private:
  void ensure_program();
  void upload_if_dirty();
  void draw_layers(const QMatrix4x4& mvp);

  QOpenGLShaderProgram program_;
  QOpenGLBuffer cloud_buffer_{QOpenGLBuffer::VertexBuffer};
  QOpenGLBuffer traj_buffer_{QOpenGLBuffer::VertexBuffer};
  QOpenGLBuffer wp_buffer_{QOpenGLBuffer::VertexBuffer};
  QOpenGLVertexArrayObject cloud_vao_;
  QOpenGLVertexArrayObject traj_vao_;
  QOpenGLVertexArrayObject wp_vao_;

  std::vector<float> cloud_xyz_;
  std::vector<float> traj_xyz_;
  std::vector<float> wp_xyz_;
  std::vector<int> traj_first_;
  std::vector<int> traj_count_;

  std::uint64_t last_revision_{0};
  bool geometry_dirty_{true};
  int width_{1};
  int height_{1};
};

}  // namespace guinmotion::viewport
