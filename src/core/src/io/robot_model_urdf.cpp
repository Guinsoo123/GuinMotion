#include "guinmotion/core/io/robot_model_urdf.hpp"

#include "xml_util.hpp"

#include <cmath>
#include <fstream>
#include <sstream>
#include <unordered_map>

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

[[nodiscard]] double deg_to_rad(double v) {
  return v * 3.14159265358979323846 / 180.0;
}

[[nodiscard]] Transform parse_origin(std::string_view xml) {
  namespace xu = xml_util;
  Transform t;
  const auto origin = xu::extract_tag_open(xml, "origin", 0);
  if (origin.empty()) {
    return t;
  }
  t.translation = xu::parse_vec3_attr(origin, "xyz");
  if (auto rpy_attr = xu::attribute_value(origin, "rpy")) {
    const auto rpy = xu::parse_doubles(*rpy_attr);
    if (rpy.size() == 3) {
      const double cr = std::cos(rpy[0] * 0.5);
      const double sr = std::sin(rpy[0] * 0.5);
      const double cp = std::cos(rpy[1] * 0.5);
      const double sp = std::sin(rpy[1] * 0.5);
      const double cy = std::cos(rpy[2] * 0.5);
      const double sy = std::sin(rpy[2] * 0.5);
      t.rotation[0] = cr * cp * cy + sr * sp * sy;
      t.rotation[1] = sr * cp * cy - cr * sp * sy;
      t.rotation[2] = cr * sp * cy + sr * cp * sy;
      t.rotation[3] = cr * cp * sy - sr * sp * cy;
    }
  }
  return t;
}

[[nodiscard]] JointType parse_joint_type(std::string_view type) {
  if (type == "fixed") {
    return JointType::Fixed;
  }
  if (type == "prismatic") {
    return JointType::Prismatic;
  }
  if (type == "continuous") {
    return JointType::Continuous;
  }
  return JointType::Revolute;
}

[[nodiscard]] Color parse_rgba(std::string_view text, Color fallback = {}) {
  const auto values = xml_util::parse_doubles(text);
  if (values.size() == 4) {
    return Color{static_cast<float>(values[0]), static_cast<float>(values[1]),
                 static_cast<float>(values[2]), static_cast<float>(values[3])};
  }
  return fallback;
}

[[nodiscard]] VisualGeometry parse_visual(std::string_view visual_xml,
                                          const std::unordered_map<std::string, Color>& materials) {
  namespace xu = xml_util;
  VisualGeometry visual;
  visual.origin = parse_origin(visual_xml);

  const auto material_elem = xu::extract_element(visual_xml, "material", 0);
  if (material_elem) {
    const auto material_open = xu::extract_tag_open(*material_elem, "material", 0);
    if (auto name = xu::attribute_value(material_open, "name")) {
      visual.material_name = std::string(*name);
      if (auto it = materials.find(visual.material_name); it != materials.end()) {
        visual.color = it->second;
      }
    }
    const auto color_open = xu::extract_tag_open(*material_elem, "color", 0);
    if (!color_open.empty()) {
      if (auto rgba = xu::attribute_value(color_open, "rgba")) {
        visual.color = parse_rgba(*rgba, visual.color);
      }
    }
  }

  const auto geometry = xu::extract_element(visual_xml, "geometry", 0);
  if (!geometry) {
    return visual;
  }
  if (const auto mesh = xu::extract_tag_open(*geometry, "mesh", 0); !mesh.empty()) {
    visual.type = GeometryType::Mesh;
    if (auto filename = xu::attribute_value(mesh, "filename")) {
      visual.mesh_uri = std::string(*filename);
    }
  } else if (const auto box = xu::extract_tag_open(*geometry, "box", 0); !box.empty()) {
    visual.type = GeometryType::Box;
    visual.size = xu::parse_vec3_attr(box, "size", visual.size);
  } else if (const auto sphere = xu::extract_tag_open(*geometry, "sphere", 0); !sphere.empty()) {
    visual.type = GeometryType::Sphere;
    if (auto radius = xu::attribute_value(sphere, "radius")) {
      (void)xu::parse_double(*radius, visual.radius);
    }
  } else if (const auto cylinder = xu::extract_tag_open(*geometry, "cylinder", 0); !cylinder.empty()) {
    visual.type = GeometryType::Cylinder;
    if (auto radius = xu::attribute_value(cylinder, "radius")) {
      (void)xu::parse_double(*radius, visual.radius);
    }
    if (auto length = xu::attribute_value(cylinder, "length")) {
      (void)xu::parse_double(*length, visual.length);
    }
  }
  return visual;
}

}  // namespace

ImportResult import_robot_model_urdf(std::string_view xml, const std::filesystem::path& source_path) {
  namespace xu = xml_util;
  const auto root = xu::extract_tag_open(xml, "robot", 0);
  if (root.empty()) {
    const auto mujoco = xu::extract_tag_open(xml, "mujoco", 0);
    if (mujoco.empty()) {
      return fail("缺少 URDF 根元素 <robot> 或 MJCF 根元素 <mujoco>。");
    }
    RobotModel robot;
    robot.source_path = source_path.string();
    if (auto model = xu::attribute_value(mujoco, "model")) {
      robot.id = std::string(*model);
      robot.name = std::string(*model);
    } else {
      robot.id = source_path.stem().empty() ? "mujoco_model" : source_path.stem().string();
      robot.name = robot.id;
    }
    robot.base_frame = "world";
    robot.tool_frame = "tool";
    robot.links.push_back(Link{.name = "world"});
    ImportResult r;
    r.ok = true;
    r.message = "成功";
    r.robot_model = std::move(robot);
    return r;
  }

  RobotModel robot;
  robot.source_path = source_path.string();
  if (auto name = xu::attribute_value(root, "name")) {
    robot.id = std::string(*name);
    robot.name = std::string(*name);
  } else {
    robot.id = source_path.stem().empty() ? "robot" : source_path.stem().string();
    robot.name = robot.id;
  }

  std::unordered_map<std::string, Color> materials;
  std::size_t search = 0;
  while (search < xml.size()) {
    const auto mat = xu::extract_element(xml, "material", search);
    if (!mat) {
      break;
    }
    const auto open = xu::extract_tag_open(*mat, "material", 0);
    if (auto name = xu::attribute_value(open, "name")) {
      const auto color = xu::extract_tag_open(*mat, "color", 0);
      if (!color.empty()) {
        if (auto rgba = xu::attribute_value(color, "rgba")) {
          materials[std::string(*name)] = parse_rgba(*rgba);
        }
      }
    }
    search = mat->data() - xml.data() + mat->size();
  }

  search = 0;
  while (search < xml.size()) {
    const auto link_elem = xu::extract_element(xml, "link", search);
    if (!link_elem) {
      break;
    }
    const auto link_open = xu::extract_tag_open(*link_elem, "link", 0);
    Link link;
    if (auto name = xu::attribute_value(link_open, "name")) {
      link.name = std::string(*name);
    } else {
      return fail("URDF link 缺少 name。");
    }
    std::size_t vsearch = 0;
    while (vsearch < link_elem->size()) {
      const auto visual_elem = xu::extract_element(*link_elem, "visual", vsearch);
      if (!visual_elem) {
        break;
      }
      auto visual = parse_visual(*visual_elem, materials);
      if (visual.type == GeometryType::Mesh) {
        link.visual_geometry_uri = visual.mesh_uri;
      }
      link.visuals.push_back(std::move(visual));
      vsearch = visual_elem->data() - link_elem->data() + visual_elem->size();
    }
    robot.links.push_back(std::move(link));
    search = link_elem->data() - xml.data() + link_elem->size();
  }

  search = 0;
  while (search < xml.size()) {
    const auto joint_elem = xu::extract_element(xml, "joint", search);
    if (!joint_elem) {
      break;
    }
    const auto joint_open = xu::extract_tag_open(*joint_elem, "joint", 0);
    Joint joint;
    if (auto name = xu::attribute_value(joint_open, "name")) {
      joint.name = std::string(*name);
    } else {
      return fail("URDF joint 缺少 name。");
    }
    if (auto type = xu::attribute_value(joint_open, "type")) {
      joint.type = parse_joint_type(*type);
    }
    joint.origin = parse_origin(*joint_elem);
    const auto parent = xu::extract_tag_open(*joint_elem, "parent", 0);
    const auto child = xu::extract_tag_open(*joint_elem, "child", 0);
    if (auto link = xu::attribute_value(parent, "link")) {
      joint.parent_link = std::string(*link);
    }
    if (auto link = xu::attribute_value(child, "link")) {
      joint.child_link = std::string(*link);
    }
    const auto axis = xu::extract_tag_open(*joint_elem, "axis", 0);
    if (!axis.empty()) {
      joint.axis = xu::parse_vec3_attr(axis, "xyz", joint.axis);
    }
    const auto limit = xu::extract_tag_open(*joint_elem, "limit", 0);
    if (!limit.empty()) {
      if (auto lower = xu::attribute_value(limit, "lower")) {
        (void)xu::parse_double(*lower, joint.limit.lower);
      }
      if (auto upper = xu::attribute_value(limit, "upper")) {
        (void)xu::parse_double(*upper, joint.limit.upper);
      }
      if (auto velocity = xu::attribute_value(limit, "velocity")) {
        (void)xu::parse_double(*velocity, joint.limit.velocity);
      }
      if (auto effort = xu::attribute_value(limit, "effort")) {
        double v = 0.0;
        if (xu::parse_double(*effort, v)) {
          joint.limit.acceleration = v;
        }
      }
    } else if (joint.type == JointType::Continuous) {
      joint.limit.lower = -3.14159265358979323846;
      joint.limit.upper = 3.14159265358979323846;
    }
    robot.joints.push_back(std::move(joint));
    search = joint_elem->data() - xml.data() + joint_elem->size();
  }

  if (robot.links.empty()) {
    return fail("URDF 中没有 link。");
  }
  robot.home_joint_positions_radians.assign(robot.joints.size(), 0.0);
  if (!robot.links.empty()) {
    robot.base_frame = robot.links.front().name;
    robot.tool_frame = robot.links.back().name;
  }

  ImportResult r;
  r.ok = true;
  r.message = "成功";
  r.robot_model = std::move(robot);
  return r;
}

ImportResult import_robot_model_urdf_file(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    return fail("URDF 文件不存在：" + path.string());
  }
  const auto text = read_text_file(path);
  if (text.empty()) {
    return fail("URDF 文件为空或无法读取：" + path.string());
  }
  auto r = import_robot_model_urdf(text, path);
  if (r.ok) {
    r.message = "已从文件导入机器人模型：" + path.string();
  }
  return r;
}

}  // namespace guinmotion::core::io
