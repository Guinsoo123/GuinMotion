#include "guinmotion/app/main_window.hpp"

#include "guinmotion/core/project.hpp"
#include "guinmotion/sdk/builtin_operators.hpp"

#include <QAbstractItemView>

#include <QAction>
#include <QCoreApplication>
#include <QDockWidget>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardItem>
#include <QStatusBar>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace sdk = guinmotion::sdk;

namespace guinmotion::app {
namespace {

constexpr int kTreeKindRole = Qt::UserRole;
constexpr int kTreeIdRole = Qt::UserRole + 1;

void register_builtin_operators_impl(operator_runtime::OperatorRegistry& registry) {
  const auto plugin = operator_runtime::builtin_plugin_metadata();
  registry.register_operator(
      plugin,
      operator_runtime::OperatorMetadata{
          .id = "guinmotion.trajectory.duration_check",
          .name = "轨迹时长检查",
          .version = "0.1.0",
          .description = "校验路点 duration_seconds 与 time_seconds 的合理性。",
      });
  registry.register_operator(
      plugin,
      operator_runtime::OperatorMetadata{
          .id = "guinmotion.joint.limit_check",
          .name = "关节限位检查",
          .version = "0.1.0",
          .description = "根据机器人模型关节上下限校验关节角（弧度）。",
      });
}

[[nodiscard]] QString format_validation_row(const guinmotion::core::ValidationMessage& message) {
  using guinmotion::core::ValidationStatus;
  QString prefix;
  switch (message.status) {
    case ValidationStatus::Error:
      prefix = QStringLiteral("[错误] ");
      break;
    case ValidationStatus::Warning:
      prefix = QStringLiteral("[警告] ");
      break;
    case ValidationStatus::Valid:
      prefix = QStringLiteral("[通过] ");
      break;
    case ValidationStatus::Skipped:
      prefix = QStringLiteral("[跳过] ");
      break;
    default:
      prefix = QStringLiteral("[信息] ");
      break;
  }
  QString suffix;
  if (!message.related_object_id.empty()) {
    suffix = QStringLiteral("（关联：%1）").arg(QString::fromStdString(message.related_object_id));
  }
  return prefix + QString::fromStdString(message.message) + suffix;
}

}  // namespace

MainWindow::MainWindow() : service_(core::make_demo_project()) {
  setWindowTitle(QStringLiteral("GuinMotion 工作台"));
  resize(1280, 720);

  register_builtin_operators();

  auto* central = new QWidget(this);
  auto* central_layout = new QVBoxLayout(central);
  viewport_ = new viewport::ViewportWidget(central);
  central_layout->addWidget(viewport_, 1);
  setCentralWidget(central);

  tree_model_ = new QStandardItemModel(this);
  tree_view_ = new QTreeView(this);
  tree_view_->setModel(tree_model_);
  tree_view_->header()->hide();
  tree_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);

  auto* project_dock = new QDockWidget(QStringLiteral("项目"), this);
  project_dock->setWidget(tree_view_);
  addDockWidget(Qt::LeftDockWidgetArea, project_dock);

  properties_label_ = new QLabel(QStringLiteral("请在项目树中选择一项。"));
  properties_label_->setWordWrap(true);
  properties_label_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  auto* prop_dock = new QDockWidget(QStringLiteral("属性"), this);
  prop_dock->setWidget(properties_label_);
  addDockWidget(Qt::RightDockWidgetArea, prop_dock);

  operator_list_ = new QListWidget(this);
  auto* op_panel = new QWidget(this);
  auto* op_layout = new QVBoxLayout(op_panel);
  op_layout->addWidget(operator_list_, 1);
  run_operator_button_ = new QPushButton(QStringLiteral("对所选轨迹运行算子"), op_panel);
  op_layout->addWidget(run_operator_button_);
  auto* op_dock = new QDockWidget(QStringLiteral("算子"), this);
  op_dock->setWidget(op_panel);
  addDockWidget(Qt::BottomDockWidgetArea, op_dock);

  connect(run_operator_button_, &QPushButton::clicked, this, &MainWindow::run_selected_operator);

  load_external_plugins();

  auto* file_menu = menuBar()->addMenu(QStringLiteral("文件(&F)"));
  auto* import_traj = new QAction(QStringLiteral("导入轨迹 XML(&X)…"), this);
  connect(import_traj, &QAction::triggered, this, &MainWindow::import_trajectory_xml);
  file_menu->addAction(import_traj);
  auto* import_cloud = new QAction(QStringLiteral("导入点云(&C)…"), this);
  connect(import_cloud, &QAction::triggered, this, &MainWindow::import_point_cloud);
  file_menu->addAction(import_cloud);

  connect(tree_view_, &QTreeView::clicked, this, [this](const QModelIndex& index) {
    if (!index.isValid()) {
      return;
    }
    const auto* item = tree_model_->itemFromIndex(index);
    if (item == nullptr) {
      return;
    }
    update_properties_panel(item->text());
  });

  rebuild_scene_tree();
  statusBar()->showMessage(
      QStringLiteral("场景版本：%1").arg(service_.project().scene_revision()));
}

void MainWindow::register_builtin_operators() {
  register_builtin_operators_impl(operator_registry_);
}

void MainWindow::refresh_operator_panel() {
  operator_list_->clear();
  for (const auto& op : operator_registry_.operators()) {
    auto* item = new QListWidgetItem(
        QString::fromStdString(op.metadata.id + " — " + op.metadata.name));
    item->setData(Qt::UserRole, QString::fromStdString(op.metadata.id));
    operator_list_->addItem(item);
  }
}

void MainWindow::load_external_plugins() {
  std::vector<std::string> errors;
  std::vector<std::filesystem::path> scan_dirs;
  std::unordered_set<std::string> seen_canonical_dirs;

  auto enqueue_plugin_dir = [&](const std::filesystem::path& dir) -> bool {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
      return false;
    }
    const std::string key = std::filesystem::weakly_canonical(dir, ec).string();
    if (!seen_canonical_dirs.insert(key).second) {
      return true;
    }
    scan_dirs.push_back(dir);
    return true;
  };

  if (const char* env = std::getenv("GUINMOTION_PLUGIN_DIR")) {
    const std::filesystem::path dir(env);
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
      errors.push_back(std::string("环境变量 GUINMOTION_PLUGIN_DIR 不是目录：") + env);
    } else {
      (void)enqueue_plugin_dir(dir);
    }
  }

  const std::filesystem::path app_plugins =
      std::filesystem::path(QCoreApplication::applicationDirPath().toStdString()) / "plugins";
  (void)enqueue_plugin_dir(app_plugins);

#ifdef GUINMOTION_BUILD_PLUGIN_DIR
  (void)enqueue_plugin_dir(GUINMOTION_BUILD_PLUGIN_DIR);
#endif

  for (const auto& dir : scan_dirs) {
    plugin_host_.load_plugins_from_directory(dir, operator_registry_, errors);
  }

  if (!errors.empty()) {
    QString message;
    for (std::size_t i = 0; i < errors.size(); ++i) {
      if (i > 0) {
        message += QStringLiteral(" | ");
      }
      message += QString::fromStdString(errors[i]);
    }
    statusBar()->showMessage(message, 15000);
  }

  refresh_operator_panel();
}

void MainWindow::sync_viewport() {
  viewport_->set_scene(service_.project().scene(), service_.project().scene_revision());
}

void MainWindow::rebuild_scene_tree() {
  tree_model_->clear();
  const auto& scene = service_.project().scene();

  auto* robots = new QStandardItem(QStringLiteral("机器人"));
  robots->setEditable(false);
  for (const auto& r : scene.robot_models) {
    auto* row = new QStandardItem(QString::fromStdString(r.id + " — " + r.name));
    row->setEditable(false);
    robots->appendRow(row);
  }
  tree_model_->appendRow(robots);

  auto* clouds = new QStandardItem(QStringLiteral("点云"));
  clouds->setEditable(false);
  for (const auto& c : scene.point_clouds) {
    auto* row = new QStandardItem(QString::fromStdString(c.id + " — " + c.name + "（" +
                                                          std::to_string(c.positions.size()) + " 点）"));
    row->setEditable(false);
    clouds->appendRow(row);
  }
  tree_model_->appendRow(clouds);

  auto* trajs = new QStandardItem(QStringLiteral("轨迹"));
  trajs->setEditable(false);
  for (const auto& t : scene.trajectories) {
    auto* row = new QStandardItem(QString::fromStdString(t.id + " — " + t.name + "（" +
                                                         std::to_string(t.waypoints.size()) + " 路点）"));
    row->setEditable(false);
    row->setData(QStringLiteral("trajectory"), kTreeKindRole);
    row->setData(QString::fromStdString(t.id), kTreeIdRole);
    trajs->appendRow(row);
  }
  tree_model_->appendRow(trajs);

  auto* val = new QStandardItem(QStringLiteral("校验"));
  val->setEditable(false);
  if (scene.validation_messages.empty()) {
    auto* row = new QStandardItem(QStringLiteral("（无消息）"));
    row->setEditable(false);
    val->appendRow(row);
  } else {
    for (const auto& m : scene.validation_messages) {
      auto* row = new QStandardItem(format_validation_row(m));
      row->setEditable(false);
      val->appendRow(row);
    }
  }
  tree_model_->appendRow(val);

  tree_view_->expandAll();
  sync_viewport();
}

void MainWindow::update_properties_panel(const QString& text) {
  properties_label_->setText(text);
}

void MainWindow::import_trajectory_xml() {
  const QString path = QFileDialog::getOpenFileName(
      this, QStringLiteral("导入轨迹 XML"), {}, QStringLiteral("XML 文件 (*.xml)"));
  if (path.isEmpty()) {
    return;
  }
  const auto result = service_.import_trajectory_xml_file(path.toStdString());
  if (!result.ok) {
    statusBar()->showMessage(QString::fromStdString(result.message), 8000);
    QMessageBox::warning(this, QStringLiteral("导入失败"), QString::fromStdString(result.message));
    return;
  }
  rebuild_scene_tree();
  statusBar()->showMessage(
      QString::fromStdString(result.message) +
          QStringLiteral(" | 场景版本：%1").arg(service_.project().scene_revision()),
      8000);
}

void MainWindow::import_point_cloud() {
  const QString path = QFileDialog::getOpenFileName(
      this,
      QStringLiteral("导入点云"),
      {},
      QStringLiteral("点云 (*.ply *.xyz);;所有文件 (*)"));
  if (path.isEmpty()) {
    return;
  }
  const auto result = service_.import_point_cloud_file(path.toStdString());
  if (!result.ok) {
    statusBar()->showMessage(QString::fromStdString(result.message), 8000);
    QMessageBox::warning(this, QStringLiteral("导入失败"), QString::fromStdString(result.message));
    return;
  }
  rebuild_scene_tree();
  statusBar()->showMessage(
      QStringLiteral("已导入点云 | 场景版本：%1")
          .arg(service_.project().scene_revision()),
      8000);
}

std::optional<std::string> MainWindow::selected_trajectory_id_from_tree() const {
  for (QModelIndex idx = tree_view_->currentIndex(); idx.isValid(); idx = idx.parent()) {
    QStandardItem* item = tree_model_->itemFromIndex(idx);
    if (item == nullptr) {
      continue;
    }
    if (item->data(kTreeKindRole).toString() == QLatin1String("trajectory")) {
      return item->data(kTreeIdRole).toString().toStdString();
    }
  }
  return std::nullopt;
}

std::string MainWindow::trajectory_id_for_operator_run() const {
  if (const auto selected = selected_trajectory_id_from_tree()) {
    return *selected;
  }
  const auto& trajectories = service_.project().scene().trajectories;
  if (!trajectories.empty()) {
    return trajectories.front().id;
  }
  return {};
}

void MainWindow::run_selected_operator() {
  QListWidgetItem* item = operator_list_->currentItem();
  if (item == nullptr) {
    QMessageBox::information(this, QStringLiteral("算子"),
                             QStringLiteral("请先在列表中选择一个算子。"));
    return;
  }
  const QString op_id = item->data(Qt::UserRole).toString();
  if (op_id.isEmpty()) {
    QMessageBox::warning(this, QStringLiteral("算子"),
                         QStringLiteral("无法解析算子 ID。"));
    return;
  }
  std::unique_ptr<sdk::Operator> op = sdk::make_builtin_operator(op_id.toStdString());
  if (op == nullptr) {
    statusBar()->showMessage(
        QStringLiteral("算子「%1」不是内置实现（插件执行尚未接入）。").arg(op_id),
        8000);
    return;
  }
  const std::string trajectory_id = trajectory_id_for_operator_run();
  if (trajectory_id.empty()) {
    QMessageBox::information(this, QStringLiteral("算子"),
                             QStringLiteral("请先添加或导入一条轨迹，再运行校验类算子。"));
    return;
  }
  (void)service_.run_operator(*op, trajectory_id);
  rebuild_scene_tree();
  statusBar()->showMessage(QStringLiteral("已在轨迹 %2 上运行算子 %1 | 场景版本 %3")
                               .arg(op_id)
                               .arg(QString::fromStdString(trajectory_id))
                               .arg(service_.project().scene_revision()),
                           8000);
}

}  // namespace guinmotion::app
