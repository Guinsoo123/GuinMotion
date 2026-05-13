#pragma once

#include <filesystem>
#include <optional>
#include <ostream>

namespace guinmotion::python_embed {

/// Initializes an embedded interpreter, imports empty module `guinmotion_sdk`, then optionally runs a
/// `.py` file (first existing path wins): explicit `script_path`, `GUINMOTION_PYTHON_SMOKE_SCRIPT`, or
/// the built-in `resources/python/embed_smoke.py` when present. Writes human-readable lines to `out`.
void run_embedded_python_smoke(std::ostream& out,
                               const std::optional<std::filesystem::path>& script_path = std::nullopt);

}  // namespace guinmotion::python_embed
