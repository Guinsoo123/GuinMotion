add_library(guinmotion_operator_runtime
  src/operator_registry.cpp
)

add_library(GuinMotion::operator_runtime ALIAS guinmotion_operator_runtime)

target_include_directories(guinmotion_operator_runtime
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(guinmotion_operator_runtime PUBLIC cxx_std_20)

install(TARGETS guinmotion_operator_runtime
  EXPORT GuinMotionTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
