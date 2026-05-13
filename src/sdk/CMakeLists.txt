add_library(guinmotion_sdk INTERFACE)
add_library(GuinMotion::sdk ALIAS guinmotion_sdk)

target_include_directories(guinmotion_sdk
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/operator_runtime/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/core/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(guinmotion_sdk INTERFACE cxx_std_20)

install(TARGETS guinmotion_sdk EXPORT GuinMotionTargets)
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
