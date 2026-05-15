#pragma once

#ifdef GUINMOTION_ENABLE_QT

#include "guinmotion/app_core/project_service.hpp"
#include "guinmotion/operator_runtime/dynamic_plugin_host.hpp"
#include "guinmotion/operator_runtime/operator_registry.hpp"
#include "guinmotion/viewport/viewport_widget.hpp"

#include <QMainWindow>

#include <optional>
#include <string>

class QLabel;
class QListWidget;
class QPushButton;
class QStandardItemModel;
class QTreeView;

namespace guinmotion::app {

class MainWindow final : public QMainWindow {
 public:
  MainWindow();

 private:
  void register_builtin_operators();
  void load_external_plugins();
  void refresh_operator_panel();
  void rebuild_scene_tree();
  void sync_viewport();
  void update_properties_panel(const QString& text);
  void import_trajectory_xml();
  void import_point_cloud();
  void import_robot_model();
  void import_target_points();
  void run_selected_operator();
  void run_selected_simulation();
  void evaluate_selected_trajectory();

  [[nodiscard]] std::optional<std::string> selected_trajectory_id_from_tree() const;
  [[nodiscard]] std::string trajectory_id_for_operator_run() const;

  app_core::ProjectService service_;
  operator_runtime::OperatorRegistry operator_registry_;
  operator_runtime::DynamicPluginHost plugin_host_;

  QTreeView* tree_view_{nullptr};
  QStandardItemModel* tree_model_{nullptr};
  QLabel* properties_label_{nullptr};
  QListWidget* operator_list_{nullptr};
  QPushButton* run_operator_button_{nullptr};
  QPushButton* run_simulation_button_{nullptr};
  QPushButton* evaluate_trajectory_button_{nullptr};
  viewport::ViewportWidget* viewport_{nullptr};
};

}  // namespace guinmotion::app

#endif
