#include "guinmotion/core/io/target_points_xml.hpp"

#include "xml_util.hpp"

#include <fstream>
#include <sstream>

namespace guinmotion::core::io {
namespace {

[[nodiscard]] ImportResult fail(std::string message) {
  ImportResult r;
  r.message = std::move(message);
  return r;
}

[[nodiscard]] std::string read_text_file(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return {};
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

}  // namespace

ImportResult import_target_points_xml(std::string_view xml) {
  namespace xu = xml_util;
  const auto root = xu::extract_tag_open(xml, "guinmotion_targets", 0);
  if (root.empty()) {
    return fail("缺少根元素 <guinmotion_targets>。");
  }

  TargetPointSet set;
  if (auto v = xu::attribute_value(root, "id")) {
    set.id = std::string(*v);
  } else {
    set.id = "targets";
  }
  if (auto v = xu::attribute_value(root, "name")) {
    set.name = std::string(*v);
  } else {
    set.name = set.id;
  }
  if (auto v = xu::attribute_value(root, "robot_model_id")) {
    set.robot_model_id = std::string(*v);
  }

  const auto close = xml.find("</guinmotion_targets>");
  if (close == std::string_view::npos) {
    return fail("缺少闭合标签 </guinmotion_targets>。");
  }
  const std::size_t body_begin = root.data() - xml.data() + root.size();
  const std::string_view body = xml.substr(body_begin, close - body_begin);

  std::size_t search = 0;
  while (search < body.size()) {
    const auto target_elem = xu::extract_element(body, "target", search);
    if (!target_elem) {
      break;
    }
    const auto target_open = xu::extract_tag_open(*target_elem, "target", 0);
    TargetPoint target;
    if (auto v = xu::attribute_value(target_open, "id")) {
      target.id = std::string(*v);
    } else {
      return fail("每个 <target> 都需要 id 属性。");
    }
    if (auto v = xu::attribute_value(target_open, "name")) {
      target.name = std::string(*v);
    } else {
      target.name = target.id;
    }
    if (auto v = xu::attribute_value(target_open, "tolerance")) {
      if (!xu::parse_double(*v, target.tolerance_m) || target.tolerance_m < 0.0) {
        return fail("目标点「" + target.id + "」的 tolerance 无效。");
      }
    }
    if (auto v = xu::attribute_value(target_open, "time_hint_seconds")) {
      if (!xu::parse_double(*v, target.time_hint_seconds)) {
        return fail("目标点「" + target.id + "」的 time_hint_seconds 无效。");
      }
    }

    const auto pose = xu::extract_tag_open(*target_elem, "pose", 0);
    if (pose.empty()) {
      return fail("目标点「" + target.id + "」缺少 <pose>。");
    }
    auto read = [&](std::string_view key, double& out) -> bool {
      if (auto v = xu::attribute_value(pose, key)) {
        return xu::parse_double(*v, out);
      }
      return true;
    };
    if (!read("x", target.pose.position.x) || !read("y", target.pose.position.y) ||
        !read("z", target.pose.position.z) || !read("qw", target.pose.orientation[0]) ||
        !read("qx", target.pose.orientation[1]) || !read("qy", target.pose.orientation[2]) ||
        !read("qz", target.pose.orientation[3])) {
      return fail("目标点「" + target.id + "」的 pose 数值无效。");
    }

    set.targets.push_back(std::move(target));
    search = target_elem->data() - body.data() + target_elem->size();
  }

  if (set.targets.empty()) {
    return fail("目标点文件中没有任何 <target>。");
  }

  ImportResult r;
  r.ok = true;
  r.message = "成功";
  r.target_point_set = std::move(set);
  return r;
}

ImportResult import_target_points_xml_file(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    return fail("目标点文件不存在：" + path.string());
  }
  const auto text = read_text_file(path);
  if (text.empty()) {
    return fail("目标点文件为空或无法读取：" + path.string());
  }
  auto r = import_target_points_xml(text);
  if (r.ok) {
    r.message = "已从文件导入目标点：" + path.string();
  }
  return r;
}

}  // namespace guinmotion::core::io
