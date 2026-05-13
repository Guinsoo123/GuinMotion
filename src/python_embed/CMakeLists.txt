find_package(Python3 3.8 COMPONENTS Interpreter Development REQUIRED)

find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
  include(FetchContent)
  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.13.6
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(pybind11)
endif()

add_library(guinmotion_python_embed STATIC
  src/embed_smoke.cpp
)
add_library(GuinMotion::python_embed ALIAS guinmotion_python_embed)

target_include_directories(guinmotion_python_embed
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(guinmotion_python_embed PUBLIC pybind11::embed)

target_compile_features(guinmotion_python_embed PUBLIC cxx_std_20)

target_compile_definitions(
  guinmotion_python_embed
  PRIVATE
    "GUINMOTION_EMBED_SMOKE_PY_DEFAULT=\"${CMAKE_SOURCE_DIR}/resources/python/embed_smoke.py\""
)

install(TARGETS guinmotion_python_embed
  EXPORT GuinMotionTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
