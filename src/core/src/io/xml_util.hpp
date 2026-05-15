#pragma once

#include "guinmotion/core/types.hpp"

#include <cctype>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace guinmotion::core::io::xml_util {

[[nodiscard]] inline std::string_view trim(std::string_view s) {
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
    s.remove_prefix(1);
  }
  while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
    s.remove_suffix(1);
  }
  return s;
}

[[nodiscard]] inline bool is_attr_name_start(std::string_view tag, std::size_t key_pos) {
  if (key_pos == 0) {
    return true;
  }
  const unsigned char prev = static_cast<unsigned char>(tag[key_pos - 1]);
  return std::isspace(prev) || prev == static_cast<unsigned char>('<');
}

[[nodiscard]] inline std::optional<std::string_view> attribute_value(
    std::string_view tag_open, std::string_view key) {
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

[[nodiscard]] inline std::string_view extract_tag_open(
    std::string_view xml, std::string_view tag, std::size_t from) {
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
    if (!(std::isspace(c) != 0 || c == static_cast<unsigned char>('>') ||
          c == static_cast<unsigned char>('/'))) {
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

[[nodiscard]] inline std::optional<std::string_view> extract_element(
    std::string_view xml, std::string_view tag, std::size_t from) {
  const auto open = extract_tag_open(xml, tag, from);
  if (open.empty()) {
    return std::nullopt;
  }
  if (open.size() >= 2 && open[open.size() - 2] == '/') {
    return open;
  }
  const std::string close_tag = "</" + std::string(tag) + ">";
  const std::size_t start = open.data() - xml.data();
  const auto end = xml.find(close_tag, start + open.size());
  if (end == std::string_view::npos) {
    return std::nullopt;
  }
  return xml.substr(start, end + close_tag.size() - start);
}

[[nodiscard]] inline bool parse_double(std::string_view tok, double& out) {
  tok = trim(tok);
  if (tok.empty()) {
    return false;
  }
  std::size_t idx = 0;
  try {
    out = std::stod(std::string(tok), &idx);
  } catch (...) {
    return false;
  }
  return idx == tok.size();
}

[[nodiscard]] inline std::vector<double> parse_doubles(std::string_view text) {
  std::vector<double> out;
  std::istringstream in{std::string(trim(text))};
  std::string tok;
  while (in >> tok) {
    double v = 0.0;
    if (!parse_double(tok, v)) {
      return {};
    }
    out.push_back(v);
  }
  return out;
}

[[nodiscard]] inline Vec3 parse_vec3_attr(std::string_view tag, std::string_view key, Vec3 fallback = {}) {
  if (auto value = attribute_value(tag, key)) {
    const auto values = parse_doubles(*value);
    if (values.size() == 3) {
      return Vec3{values[0], values[1], values[2]};
    }
  }
  return fallback;
}

}  // namespace guinmotion::core::io::xml_util
