#include "guinmotion/app_core/project_service.hpp"
#include "guinmotion/core/project.hpp"
#include "guinmotion/operator_runtime/operator_registry.hpp"

#ifdef GUINMOTION_ENABLE_PYTHON
#include "guinmotion/python_embed/embed_smoke.hpp"

#include <filesystem>
#include <optional>
#endif

#ifdef GUINMOTION_ENABLE_QT
#include "guinmotion/app/main_window.hpp"
#include <QApplication>
#include <QSurfaceFormat>
#endif

#include <iostream>
#include <string>

namespace {

void register_builtin_operators(guinmotion::operator_runtime::OperatorRegistry& registry) {
  const auto plugin = guinmotion::operator_runtime::builtin_plugin_metadata();
  registry.register_operator(
      plugin,
      guinmotion::operator_runtime::OperatorMetadata{
          .id = "guinmotion.trajectory.duration_check",
          .name = "Trajectory Duration Check",
          .version = "0.1.0",
          .description = "Validate waypoint durations and time ordering.",
      });
  registry.register_operator(
      plugin,
      guinmotion::operator_runtime::OperatorMetadata{
          .id = "guinmotion.joint.limit_check",
          .name = "Joint Limit Check",
          .version = "0.1.0",
          .description = "Validate joint positions against robot model limits.",
      });
}

std::string build_summary_text() {
  guinmotion::app_core::ProjectService service{guinmotion::core::make_demo_project()};
  guinmotion::operator_runtime::OperatorRegistry registry;
  register_builtin_operators(registry);

  const char* kExtraTrajectory = R"(
<guinmotion_trajectory id="t_import" name="Imported Sample" robot_model_id="demo_robot" interpolation="linear_joint">
  <waypoint id="wi1" duration_seconds="0.5" time_seconds="0">
    <joints>0 0 0 0 0 0</joints>
  </waypoint>
</guinmotion_trajectory>
)";
  (void)service.import_trajectory_xml(kExtraTrajectory);

  const auto& project = service.project();
  const auto summary = project.summary();
  std::string text;
  text += "GuinMotion " + project.name() + "\n";
  text += "Scene revision: " + std::to_string(project.scene_revision()) + "\n";
  text += "Robots: " + std::to_string(summary.robot_model_count) + "\n";
  text += "Point clouds: " + std::to_string(summary.point_cloud_count) + "\n";
  text += "Trajectories: " + std::to_string(summary.trajectory_count) + "\n";
  text += "Waypoints: " + std::to_string(summary.waypoint_count) + "\n";
  text += "Registered operators: " + std::to_string(registry.operators().size()) + "\n";
  return text;
}

#ifdef GUINMOTION_ENABLE_PYTHON
[[nodiscard]] std::optional<std::filesystem::path> python_smoke_script_from_argv(int argc, char** argv) {
  if (argc <= 1) {
    return std::nullopt;
  }
  const std::filesystem::path candidate(argv[1]);
  if (candidate.extension() == ".py") {
    return candidate;
  }
  return std::nullopt;
}
#endif

}  // namespace

int main(int argc, char** argv) {
#ifdef GUINMOTION_ENABLE_QT
  QSurfaceFormat format;
  format.setRenderableType(QSurfaceFormat::OpenGL);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setMajorVersion(3);
  format.setMinorVersion(3);
  format.setDepthBufferSize(24);
  QSurfaceFormat::setDefaultFormat(format);

  QApplication app(argc, argv);
  guinmotion::app::MainWindow window;
  window.show();
  return app.exec();
#else
  std::cout << build_summary_text() << std::flush;
#ifdef GUINMOTION_ENABLE_PYTHON
  guinmotion::python_embed::run_embedded_python_smoke(std::cout, python_smoke_script_from_argv(argc, argv));
#endif
  return 0;
#endif
}
