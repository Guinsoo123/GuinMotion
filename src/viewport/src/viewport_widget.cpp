#include "guinmotion/viewport/viewport_widget.hpp"
#include "guinmotion/viewport/joint_placeholder.hpp"

#include <QMatrix4x4>
#include <QOpenGLShader>
#include <QVector3D>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace guinmotion::viewport {
namespace {

constexpr char kVs[] = R"GLSL(
#version 330 core
layout(location = 0) in vec3 position;
uniform mat4 mvp;
uniform int layer;
void main() {
  gl_Position = mvp * vec4(position, 1.0);
  if (layer == 0) {
    gl_PointSize = 3.0;
  } else if (layer == 2) {
    gl_PointSize = 10.0;
  } else {
    gl_PointSize = 1.0;
  }
}
)GLSL";

constexpr char kFs[] = R"GLSL(
#version 330 core
uniform int layer;
out vec4 frag_color;
void main() {
  if (layer == 0) {
    frag_color = vec4(0.65, 0.82, 1.0, 1.0);
  } else if (layer == 1) {
    frag_color = vec4(1.0, 0.92, 0.25, 1.0);
  } else {
    frag_color = vec4(1.0, 0.35, 0.12, 1.0);
  }
}
)GLSL";

[[nodiscard]] QMatrix4x4 compute_mvp(
    int viewport_w, int viewport_h, const QVector3D& center, float radius) {
  QMatrix4x4 proj;
  const float aspect =
      viewport_h > 0 ? static_cast<float>(viewport_w) / static_cast<float>(viewport_h) : 1.0F;
  proj.perspective(45.0F, aspect, 0.05F, 500.0F);

  const float dist = std::max(radius * 2.2F, 0.8F);
  const QVector3D eye = center + QVector3D(dist * 0.85F, dist * 0.55F, dist * 0.9F);
  QMatrix4x4 view;
  view.lookAt(eye, center, QVector3D(0.0F, 1.0F, 0.0F));

  return proj * view;
}

}  // namespace

ViewportWidget::ViewportWidget(QWidget* parent) : QOpenGLWidget(parent) {
  setMinimumSize(400, 300);
}

void ViewportWidget::set_scene(const core::Scene& scene, const std::uint64_t scene_revision) {
  (void)scene_revision;
  last_revision_ = scene_revision;

  cloud_xyz_.clear();
  for (const auto& cloud : scene.point_clouds) {
    for (const auto& p : cloud.positions) {
      cloud_xyz_.push_back(static_cast<float>(p.x));
      cloud_xyz_.push_back(static_cast<float>(p.y));
      cloud_xyz_.push_back(static_cast<float>(p.z));
    }
  }

  traj_xyz_.clear();
  traj_first_.clear();
  traj_count_.clear();
  wp_xyz_.clear();

  for (const auto& tr : scene.trajectories) {
    if (tr.waypoints.empty()) {
      continue;
    }
    const int first = static_cast<int>(traj_xyz_.size() / 3);
    for (const auto& wp : tr.waypoints) {
      const auto p = joint_placeholder_scaled_xyz(wp.state);
      const QVector3D v(p[0], p[1], p[2]);
      traj_xyz_.push_back(v.x());
      traj_xyz_.push_back(v.y());
      traj_xyz_.push_back(v.z());
    }
    traj_first_.push_back(first);
    traj_count_.push_back(static_cast<int>(tr.waypoints.size()));

    for (const auto& wp : tr.waypoints) {
      const auto p = joint_placeholder_scaled_xyz(wp.state);
      const QVector3D v(p[0], p[1], p[2]);
      wp_xyz_.push_back(v.x());
      wp_xyz_.push_back(v.y());
      wp_xyz_.push_back(v.z());
    }
  }

  geometry_dirty_ = true;
  update();
}

void ViewportWidget::initializeGL() {
  initializeOpenGLFunctions();
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClearColor(0.12F, 0.12F, 0.14F, 1.0F);
  glEnable(GL_PROGRAM_POINT_SIZE);

  ensure_program();

  cloud_vao_.create();
  cloud_buffer_.create();
  traj_vao_.create();
  traj_buffer_.create();
  wp_vao_.create();
  wp_buffer_.create();

  cloud_vao_.bind();
  cloud_buffer_.bind();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  cloud_buffer_.release();
  cloud_vao_.release();

  traj_vao_.bind();
  traj_buffer_.bind();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  traj_buffer_.release();
  traj_vao_.release();

  wp_vao_.bind();
  wp_buffer_.bind();
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  wp_buffer_.release();
  wp_vao_.release();
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
    if (data.empty()) {
      buf.allocate(nullptr, 0);
    } else {
      buf.allocate(data.data(), static_cast<int>(data.size() * sizeof(float)));
    }
    buf.release();
  };

  upload(cloud_buffer_, cloud_xyz_);
  upload(traj_buffer_, traj_xyz_);
  upload(wp_buffer_, wp_xyz_);
}

void ViewportWidget::draw_layers(const QMatrix4x4& mvp) {
  if (!program_.isLinked()) {
    return;
  }
  if (!program_.bind()) {
    return;
  }
  program_.setUniformValue("mvp", mvp);

  if (!cloud_xyz_.empty()) {
    program_.setUniformValue("layer", 0);
    cloud_vao_.bind();
    cloud_buffer_.bind();
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(cloud_xyz_.size() / 3));
    cloud_buffer_.release();
    cloud_vao_.release();
  }

  if (!traj_xyz_.empty()) {
    program_.setUniformValue("layer", 1);
    traj_vao_.bind();
    traj_buffer_.bind();
    for (std::size_t i = 0; i < traj_first_.size(); ++i) {
      glDrawArrays(
          GL_LINE_STRIP,
          traj_first_[i],
          traj_count_[i]);
    }
    traj_buffer_.release();
    traj_vao_.release();
  }

  if (!wp_xyz_.empty()) {
    program_.setUniformValue("layer", 2);
    wp_vao_.bind();
    wp_buffer_.bind();
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(wp_xyz_.size() / 3));
    wp_buffer_.release();
    wp_vao_.release();
  }

  program_.release();
}

void ViewportWidget::paintGL() {
  makeCurrent();
  upload_if_dirty();

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  QVector3D center(0.0F, 0.0F, 0.0F);
  float radius = 0.5F;

  auto expand = [&](const std::vector<float>& buf) {
    for (std::size_t i = 0; i + 2 < buf.size(); i += 3) {
      const QVector3D v(buf[i], buf[i + 1], buf[i + 2]);
      center += v;
    }
  };

  const std::size_t npts =
      (cloud_xyz_.size() + traj_xyz_.size() + wp_xyz_.size()) / 3;
  if (npts > 0) {
    center = QVector3D(0, 0, 0);
    expand(cloud_xyz_);
    expand(traj_xyz_);
    expand(wp_xyz_);
    center /= static_cast<float>(npts);

    auto max_r = [&](const std::vector<float>& buf) {
      for (std::size_t i = 0; i + 2 < buf.size(); i += 3) {
        const QVector3D v(buf[i], buf[i + 1], buf[i + 2]);
        radius = std::max(radius, (v - center).length());
      }
    };
    max_r(cloud_xyz_);
    max_r(traj_xyz_);
    max_r(wp_xyz_);
  }

  const QMatrix4x4 mvp = compute_mvp(width_, height_, center, radius);
  draw_layers(mvp);
}

}  // namespace guinmotion::viewport
