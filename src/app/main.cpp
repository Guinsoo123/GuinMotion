#include "guinmotion/core/project.hpp"
#include "guinmotion/operator_runtime/operator_registry.hpp"

#ifdef GUINMOTION_ENABLE_QT
#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QString>
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
          .description = "Validate basic waypoint duration fields.",
      });
  registry.register_operator(
      plugin,
      guinmotion::operator_runtime::OperatorMetadata{
          .id = "guinmotion.joint.limit_check",
          .name = "Joint Limit Check",
          .version = "0.1.0",
          .description = "Placeholder for robot joint limit validation.",
      });
}

std::string build_summary_text() {
  auto project = guinmotion::core::make_demo_project();
  guinmotion::operator_runtime::OperatorRegistry registry;
  register_builtin_operators(registry);

  const auto summary = project.summary();
  std::string text;
  text += "GuinMotion " + project.name() + "\n";
  text += "Robots: " + std::to_string(summary.robot_model_count) + "\n";
  text += "Point clouds: " + std::to_string(summary.point_cloud_count) + "\n";
  text += "Trajectories: " + std::to_string(summary.trajectory_count) + "\n";
  text += "Waypoints: " + std::to_string(summary.waypoint_count) + "\n";
  text += "Registered operators: " + std::to_string(registry.operators().size()) + "\n";
  return text;
}

}  // namespace

int main(int argc, char** argv) {
#ifdef GUINMOTION_ENABLE_QT
  QApplication app(argc, argv);
  QMainWindow window;
  window.setWindowTitle("GuinMotion");
  auto* label = new QLabel(QString::fromStdString(build_summary_text()));
  label->setMargin(24);
  window.setCentralWidget(label);
  window.resize(720, 420);
  window.show();
  return app.exec();
#else
  (void)argc;
  (void)argv;
  std::cout << build_summary_text();
  return 0;
#endif
}
