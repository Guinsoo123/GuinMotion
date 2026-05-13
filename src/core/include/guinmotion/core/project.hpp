#pragma once

#include "guinmotion/core/types.hpp"

#include <cstddef>
#include <string>

namespace guinmotion::core {

struct ProjectSummary {
  std::size_t robot_model_count{0};
  std::size_t point_cloud_count{0};
  std::size_t point_count{0};
  std::size_t trajectory_count{0};
  std::size_t waypoint_count{0};
  std::size_t validation_message_count{0};
};

class Project {
 public:
  Project(std::string id, std::string name);

  [[nodiscard]] const std::string& id() const noexcept;
  [[nodiscard]] const std::string& name() const noexcept;
  void set_name(std::string name);

  [[nodiscard]] Scene& scene() noexcept;
  [[nodiscard]] const Scene& scene() const noexcept;

  [[nodiscard]] ProjectSummary summary() const;

 private:
  std::string id_;
  std::string name_;
  Scene scene_;
};

Project make_demo_project();

}  // namespace guinmotion::core
