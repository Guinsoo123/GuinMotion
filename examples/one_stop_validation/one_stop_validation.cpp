#include "guinmotion/app_core/project_service.hpp"
#include "guinmotion/core/project.hpp"
#include "guinmotion/sdk/builtin_operators.hpp"

#include <filesystem>
#include <iostream>

namespace {

[[nodiscard]] std::filesystem::path data_dir(int argc, char** argv) {
  if (argc > 1) {
    return std::filesystem::path(argv[1]);
  }
  return std::filesystem::path(GUINMOTION_ONE_STOP_DATA_DIR);
}

void print_import(const char* label, const guinmotion::core::io::ImportResult& result) {
  std::cout << label << ": " << (result.ok ? "OK" : "FAIL") << " - " << result.message << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  namespace core = guinmotion::core;
  namespace app_core = guinmotion::app_core;
  namespace sdk = guinmotion::sdk;

  const auto dir = data_dir(argc, argv);
  app_core::ProjectService service(core::Project{"one_stop_validation", "One Stop Validation"});

  const auto robot = service.import_robot_model_urdf_file(dir / "demo_robot.urdf");
  print_import("import model", robot);
  const auto targets = service.import_target_points_xml_file(dir / "targets.xml");
  print_import("import targets", targets);
  const auto cloud = service.import_point_cloud_file(dir / "scene.xyz");
  print_import("import point cloud", cloud);
  if (!robot.ok || !targets.ok || !cloud.ok) {
    return 2;
  }

  auto planner = sdk::make_builtin_operator("guinmotion.examples.target_demo_planner");
  if (!planner) {
    std::cerr << "planner operator missing\n";
    return 3;
  }
  const auto plan_result = service.run_operator(*planner, {});
  std::cout << "run planner: generated " << plan_result.trajectories.size() << " trajectory\n";
  if (service.project().scene().trajectories.empty()) {
    std::cerr << "no trajectory generated\n";
    return 4;
  }

  const auto trajectory_id = service.project().scene().trajectories.back().id;
  const auto trace = service.run_simulation(trajectory_id);
  std::cout << "run simulation: " << trace.samples.size() << " samples\n";
  const auto evaluation = service.evaluate_trajectory(trajectory_id, {});

  const bool pass = evaluation.status == core::ValidationStatus::Valid;
  std::cout << "evaluation: " << (pass ? "PASS" : "FAIL")
            << " max_position_error_m=" << evaluation.max_position_error_m << "\n";
  for (const auto& msg : evaluation.messages) {
    std::cout << " - " << msg.message << "\n";
  }
  return pass ? 0 : 5;
}
