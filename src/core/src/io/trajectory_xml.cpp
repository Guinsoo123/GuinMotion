#include "guinmotion/core/io/trajectory_xml.hpp"

#include <cctype>
#include <cmath>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace guinmotion::core::io {
namespace {

[[nodiscard]] std::string_view trim(std::string_view s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.remove_prefix(1);
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.remove_suffix(1);
  }
  return s;
}

[[nodiscard]] bool is_attr_name_start(std::string_view tag, std::size_t key_pos) {
  if (key_pos == 0) {
    return true;
  }
  const unsigned char prev = static_cast<unsigned char>(tag[key_pos - 1]);
  return std::isspace(prev) || prev == static_cast<unsigned char>('<');
}

[[nodiscard]] std::optional<std::string_view> attribute_value(std::string_view tag_open, std::string_view key) {
  const std::string pat = std::string(key) + "=";
  std::size_t pos = 0;
  while (pos < tag_open.size()) {
    const auto found = tag_open.find(pat, pos);
    if (found == std::string_view::npos) {
      return std::nullopt;
    }
    if (!is_attr_name_start(tag_open, found)) {
      pos = found + 1;
      continue;
    }
    const std::size_t after_eq = found + pat.size();
    if (after_eq >= tag_open.size()) {
      return std::nullopt;
    }
    const char q = tag_open[after_eq];
    if (q != '"' && q != '\'') {
      pos = found + 1;
      continue;
    }
    const std::size_t start = after_eq + 1;
    const auto end = tag_open.find(q, start);
    if (end == std::string_view::npos) {
      return std::nullopt;
    }
    return tag_open.substr(start, end - start);
  }
  return std::nullopt;
}

[[nodiscard]] bool parse_double_loose(std::string_view tok, double& out) {
  tok = trim(tok);
  if (tok.empty()) {
    return false;
  }
  std::size_t idx = 0;
  out = std::stod(std::string(tok), &idx);
  return idx == tok.size();
}

[[nodiscard]] bool parse_double_token(std::string_view tok, double& out) {
  return parse_double_loose(tok, out);
}

[[nodiscard]] std::vector<double> parse_joint_list(std::string_view inner) {
  std::vector<double> joints;
  std::istringstream iss{std::string(trim(inner))};
  std::string tok;
  while (iss >> tok) {
    double v = 0.0;
    if (!parse_double_loose(tok, v)) {
      return {};
    }
    joints.push_back(v);
  }
  return joints;
}

[[nodiscard]] InterpolationMode parse_interpolation(std::string_view s) {
  static const std::unordered_map<std::string, InterpolationMode> kMap{
      {"hold", InterpolationMode::Hold},
      {"linear_joint", InterpolationMode::LinearJoint},
      {"cubic_joint", InterpolationMode::CubicJoint},
      {"cartesian_linear", InterpolationMode::CartesianLinear},
  };
  std::string key;
  key.reserve(s.size());
  for (unsigned char c : s) {
    key.push_back(static_cast<char>(std::tolower(c)));
  }
  auto it = kMap.find(key);
  if (it == kMap.end()) {
    return InterpolationMode::LinearJoint;
  }
  return it->second;
}

[[nodiscard]] std::optional<std::size_t> find_robot_joint_count(const Scene& scene, std::string_view robot_id) {
  for (const auto& r : scene.robot_models) {
    if (r.id == robot_id) {
      return r.joints.size();
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::string_view extract_tag_open(std::string_view xml, std::string_view tag, std::size_t from) {
  const std::string open = "<" + std::string(tag);
  std::size_t pos = from;
  while (pos < xml.size()) {
    pos = xml.find(open, pos);
    if (pos == std::string_view::npos) {
      return {};
    }
    const std::size_t after = pos + open.size();
    if (after >= xml.size()) {
      return {};
    }
    const unsigned char c = static_cast<unsigned char>(xml[after]);
    if (!(std::isspace(c) != 0 || c == static_cast<unsigned char>('>') || c == static_cast<unsigned char>('/'))) {
      pos = pos + 1;
      continue;
    }
    const auto close = xml.find('>', pos);
    if (close == std::string_view::npos) {
      return {};
    }
    return xml.substr(pos, close - pos + 1);
  }
  return {};
}

[[nodiscard]] std::optional<std::string_view> extract_inner(
    std::string_view xml, std::string_view open_tag, std::string_view close_tag, std::size_t from) {
  auto pos = xml.find(open_tag, from);
  if (pos == std::string_view::npos) {
    return std::nullopt;
  }
  pos += open_tag.size();
  const auto end = xml.find(close_tag, pos);
  if (end == std::string_view::npos) {
    return std::nullopt;
  }
  return xml.substr(pos, end - pos);
}

}  // namespace

ImportResult import_trajectory_xml(std::string_view xml, const TrajectoryXmlImportContext& ctx) {
  ImportResult result;
  const auto root_open = extract_tag_open(xml, "guinmotion_trajectory", 0);
  if (root_open.empty()) {
    result.message = "Missing <guinmotion_trajectory> root element.";
    return result;
  }

  Trajectory trajectory;
  if (auto v = attribute_value(root_open, "id")) {
    trajectory.id = std::string(*v);
  } else {
    result.message = "Trajectory attribute 'id' is required.";
    return result;
  }
  if (auto v = attribute_value(root_open, "name")) {
    trajectory.name = std::string(*v);
  } else {
    trajectory.name = trajectory.id;
  }
  if (auto v = attribute_value(root_open, "robot_model_id")) {
    trajectory.robot_model_id = std::string(*v);
  } else {
    result.message = "Trajectory attribute 'robot_model_id' is required.";
    return result;
  }
  if (auto v = attribute_value(root_open, "interpolation")) {
    trajectory.interpolation = parse_interpolation(*v);
  }

  if (ctx.scene != nullptr) {
    const auto jc = find_robot_joint_count(*ctx.scene, trajectory.robot_model_id);
    if (!jc) {
      result.message = "robot_model_id '" + trajectory.robot_model_id + "' not found in scene.";
      return result;
    }
  }

  const auto root_close_pos = xml.find("</guinmotion_trajectory>");
  if (root_close_pos == std::string_view::npos) {
    result.message = "Missing </guinmotion_trajectory> closing tag.";
    return result;
  }
  const std::size_t body_begin = root_open.data() - xml.data() + root_open.size();
  const std::string_view body = xml.substr(body_begin, root_close_pos - body_begin);

  std::optional<std::size_t> expected_joints;
  if (ctx.scene != nullptr) {
    expected_joints = find_robot_joint_count(*ctx.scene, trajectory.robot_model_id);
  }

  std::size_t search = 0;
  double last_time = -std::numeric_limits<double>::infinity();

  while (search < body.size()) {
    const auto w_open = extract_tag_open(body, "waypoint", search);
    if (w_open.empty()) {
      break;
    }

    Waypoint wp;
    if (auto v = attribute_value(w_open, "id")) {
      wp.id = std::string(*v);
    } else {
      result.message = "Each <waypoint> requires an 'id' attribute.";
      return result;
    }
    if (auto v = attribute_value(w_open, "label")) {
      wp.label = std::string(*v);
    }
    if (auto v = attribute_value(w_open, "time_seconds")) {
      double t = 0.0;
      if (!parse_double_token(*v, t)) {
        result.message = "Invalid time_seconds on waypoint '" + wp.id + "'.";
        return result;
      }
      wp.time_seconds = t;
      if (t < last_time - 1e-9) {
        result.message = "time_seconds must be non-decreasing (waypoint '" + wp.id + "').";
        return result;
      }
      last_time = t;
    }
    if (auto v = attribute_value(w_open, "duration_seconds")) {
      double d = 0.0;
      if (!parse_double_token(*v, d)) {
        result.message = "Invalid duration_seconds on waypoint '" + wp.id + "'.";
        return result;
      }
      if (d < 0.0) {
        result.message = "duration_seconds must be non-negative (waypoint '" + wp.id + "').";
        return result;
      }
      wp.duration_seconds = d;
    }

    const std::size_t inner_from = w_open.data() - body.data() + w_open.size();
    const auto joints_inner = extract_inner(body, "<joints>", "</joints>", inner_from);
    if (!joints_inner) {
      result.message = "Missing <joints> for waypoint '" + wp.id + "'.";
      return result;
    }
    auto joints = parse_joint_list(*joints_inner);
    if (joints.empty()) {
      result.message = "Waypoint '" + wp.id + "' has empty or invalid <joints> list.";
      return result;
    }
    if (expected_joints && joints.size() != *expected_joints) {
      result.message = "Waypoint '" + wp.id + "' joint count " + std::to_string(joints.size()) +
                     " does not match robot model '" + trajectory.robot_model_id + "' (" +
                     std::to_string(*expected_joints) + " joints).";
      return result;
    }

    wp.state.robot_model_id = trajectory.robot_model_id;
    wp.state.joint_positions_radians = std::move(joints);

    const auto w_close = body.find("</waypoint>", inner_from);
    if (w_close == std::string_view::npos) {
      result.message = "Missing </waypoint> after '" + wp.id + "'.";
      return result;
    }
    search = w_close + std::string_view("</waypoint>").size();

    trajectory.waypoints.push_back(std::move(wp));
  }

  if (trajectory.waypoints.empty()) {
    result.message = "Trajectory contains no waypoints.";
    return result;
  }

  result.ok = true;
  result.message = "OK";
  result.trajectory = std::move(trajectory);
  return result;
}

}  // namespace guinmotion::core::io
