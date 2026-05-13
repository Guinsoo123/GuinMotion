#pragma once

#include <memory>
#include <string>

namespace guinmotion::sdk {

class Operator;

/// Returns nullptr for unknown ids (e.g. third-party plugin operators).
[[nodiscard]] std::unique_ptr<Operator> make_builtin_operator(const std::string& operator_id);

}  // namespace guinmotion::sdk
