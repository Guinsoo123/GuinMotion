#include "guinmotion/core/io/point_cloud_file.hpp"

#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

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

[[nodiscard]] std::string read_all_stream(std::istream& in) {
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

[[nodiscard]] bool starts_with(std::string_view s, std::string_view p) {
  return s.size() >= p.size() && s.substr(0, p.size()) == p;
}

[[nodiscard]] bool starts_with_ci(std::string_view hay, std::string_view needle) {
  if (hay.size() < needle.size()) {
    return false;
  }
  for (std::size_t i = 0; i < needle.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(hay[i])) !=
        std::tolower(static_cast<unsigned char>(needle[i]))) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] ImportResult fail(std::string msg) {
  ImportResult r;
  r.ok = false;
  r.message = std::move(msg);
  return r;
}

[[nodiscard]] ImportResult import_ply_ascii(std::string_view data, const ImportLimits& limits) {
  std::istringstream in{std::string(data)};
  std::string line;
  if (!std::getline(in, line) || !starts_with(trim(line), "ply")) {
    return fail("PLY：缺少文件头 ply。");
  }
  bool format_ascii = false;
  std::optional<std::size_t> vertex_count;
  while (std::getline(in, line)) {
    const auto t = trim(line);
    if (t == "end_header") {
      break;
    }
    if (starts_with_ci(t, "format")) {
      if (t.find("ascii") != std::string::npos) {
        format_ascii = true;
      } else if (t.find("binary") != std::string::npos) {
        return fail("PLY：暂不支持二进制格式（请使用 ASCII PLY）。");
      }
    }
    if (starts_with_ci(t, "element vertex")) {
      std::istringstream ls{std::string(t)};
      std::string el;
      std::string vert;
      std::size_t n = 0;
      if (!(ls >> el >> vert >> n)) {
        return fail("PLY：element vertex 行无效。");
      }
      vertex_count = n;
    }
  }
  if (!format_ascii) {
    return fail("PLY：仅支持 ASCII 格式 1.0。");
  }
  if (!vertex_count || *vertex_count == 0) {
    return fail("PLY：缺少顶点数量或顶点数为 0。");
  }
  if (*vertex_count > limits.max_point_cloud_vertices) {
    return fail("PLY：顶点数超过导入上限 max_point_cloud_vertices。");
  }

  PointCloud cloud;
  cloud.id = "imported_cloud";
  cloud.name = "导入的点云";
  cloud.metadata.push_back({"source", "ply_ascii"});
  cloud.positions.reserve(*vertex_count);

  for (std::size_t i = 0; i < *vertex_count; ++i) {
    if (!std::getline(in, line)) {
      return fail("PLY：顶点数据意外结束。");
    }
    std::istringstream ls{std::string(trim(line))};
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!(ls >> x >> y >> z)) {
      return fail("PLY：顶点行无效（至少需要 x y z）。");
    }
    cloud.positions.push_back(Vec3{x, y, z});
  }

  ImportResult okr;
  okr.ok = true;
  okr.message = "成功";
  okr.point_cloud = std::move(cloud);
  return okr;
}

[[nodiscard]] ImportResult import_xyz_text(std::string_view data, const ImportLimits& limits) {
  PointCloud cloud;
  cloud.id = "imported_cloud";
  cloud.name = "导入的点云";
  cloud.metadata.push_back({"source", "xyz_text"});

  std::istringstream in{std::string(data)};
  std::string line;
  while (std::getline(in, line)) {
    const auto t = trim(line);
    if (t.empty() || t[0] == '#') {
      continue;
    }
    std::istringstream ls{std::string(t)};
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!(ls >> x >> y >> z)) {
      return fail("XYZ：每行需要三个浮点数（x y z）。");
    }
    cloud.positions.push_back(Vec3{x, y, z});
    if (cloud.positions.size() > limits.max_point_cloud_vertices) {
      return fail("XYZ：顶点数超过导入上限 max_point_cloud_vertices。");
    }
  }
  if (cloud.positions.empty()) {
    return fail("XYZ：未读取到任何点。");
  }
  ImportResult okr;
  okr.ok = true;
  okr.message = "成功";
  okr.point_cloud = std::move(cloud);
  return okr;
}

}  // namespace

ImportResult import_point_cloud_file(const std::filesystem::path& path, const ImportLimits& limits) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    return fail("无法打开文件：" + path.string());
  }
  const std::string data = read_all_stream(f);
  const auto ext = path.extension().string();
  if (ext == ".ply" || ext == ".PLY") {
    return import_ply_ascii(data, limits);
  }
  return import_xyz_text(data, limits);
}

ImportResult import_point_cloud_memory(std::string_view data, const ImportLimits& limits) {
  const auto t = trim(data);
  if (starts_with_ci(t, "ply")) {
    return import_ply_ascii(t, limits);
  }
  return import_xyz_text(t, limits);
}

}  // namespace guinmotion::core::io
