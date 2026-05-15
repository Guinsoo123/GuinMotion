cmake_minimum_required(VERSION 3.22)

project(
  GuinMotion
  VERSION 0.1.0
  DESCRIPTION "Robot motion-control algorithm development and visualization workbench"
  LANGUAGES CXX
)

option(GUINMOTION_BUILD_EXAMPLES "Build example operator plugins" ON)
option(GUINMOTION_BUILD_TESTS "Build unit tests" ON)
option(GUINMOTION_ENABLE_QT "Enable Qt desktop UI target when Qt6 is available" OFF)
option(GUINMOTION_ENABLE_PYTHON "Enable embedded Python operator runtime placeholders" OFF)
option(GUINMOTION_REQUIRE_ROBOTICS_DEPS "Require MuJoCo, tinyxml2 and Assimp at configure time" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(GNUInstallDirs)

if(GUINMOTION_REQUIRE_ROBOTICS_DEPS)
  find_package(mujoco CONFIG REQUIRED)
  find_package(tinyxml2 CONFIG REQUIRED)
  find_package(assimp CONFIG REQUIRED)
else()
  find_package(mujoco CONFIG QUIET)
  find_package(tinyxml2 CONFIG QUIET)
  find_package(assimp CONFIG QUIET)
endif()

add_subdirectory(src/core)
add_subdirectory(src/operator_runtime)
add_subdirectory(src/sdk)
add_subdirectory(src/simulation)
add_subdirectory(src/app_core)

if(GUINMOTION_ENABLE_PYTHON)
  add_subdirectory(src/python_embed)
endif()

if(GUINMOTION_ENABLE_QT)
  add_subdirectory(src/viewport)
endif()

add_subdirectory(src/app)

if(GUINMOTION_BUILD_EXAMPLES)
  add_subdirectory(src/plugins/examples/minimal_operator)
endif()

if(GUINMOTION_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(GUINMOTION_BUILD_EXAMPLES)
  add_subdirectory(examples/one_stop_validation)
endif()

install(DIRECTORY resources/ DESTINATION "${CMAKE_INSTALL_DATADIR}/guinmotion/resources")
install(DIRECTORY design/ DESTINATION "${CMAKE_INSTALL_DATADIR}/guinmotion/design")

set(CPACK_PACKAGE_NAME "GuinMotion")
set(CPACK_PACKAGE_VENDOR "guinsoo")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "GuinMotion")

if(APPLE)
  set(CPACK_GENERATOR "DragNDrop;TGZ")
elseif(UNIX)
  set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
