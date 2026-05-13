#include "guinmotion/python_embed/embed_smoke.hpp"

#include <pybind11/embed.h>

#include <cstdlib>
#include <filesystem>
#include <utility>

namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(guinmotion_sdk, m) {
  (void)m;
}

namespace guinmotion::python_embed {
namespace {

[[nodiscard]] std::optional<std::filesystem::path> resolve_script_path(
    const std::optional<std::filesystem::path>& explicit_path) {
  if (explicit_path.has_value() && !explicit_path->empty()) {
    return explicit_path;
  }
  if (const char* env = std::getenv("GUINMOTION_PYTHON_SMOKE_SCRIPT")) {
    return std::filesystem::path(env);
  }
#ifdef GUINMOTION_EMBED_SMOKE_PY_DEFAULT
  return std::filesystem::path(GUINMOTION_EMBED_SMOKE_PY_DEFAULT);
#else
  return std::nullopt;
#endif
}

}  // namespace

void run_embedded_python_smoke(std::ostream& out,
                               const std::optional<std::filesystem::path>& script_path) {
  py::scoped_interpreter guard{};
  (void)py::module_::import("guinmotion_sdk");
  out << "[python_embed] interpreter started; imported guinmotion_sdk\n";

  const auto resolved = resolve_script_path(script_path);
  if (!resolved.has_value() || resolved->empty()) {
    out << "[python_embed] no script path configured (optional)\n";
    return;
  }

  std::error_code ec;
  if (!std::filesystem::exists(*resolved, ec)) {
    out << "[python_embed] script path does not exist: " << resolved->string() << '\n';
    return;
  }

  try {
    py::eval_file(resolved->string(), py::globals());
    out << "[python_embed] eval_file ok: " << resolved->string() << '\n';
  } catch (const py::error_already_set& e) {
    out << "[python_embed] eval_file failed: " << e.what() << '\n';
  }
}

}  // namespace guinmotion::python_embed
