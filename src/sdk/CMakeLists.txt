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

add_library(guinmotion_sdk_builtins STATIC
  src/builtin_operators.cpp
)
add_library(GuinMotion::sdk_builtins ALIAS guinmotion_sdk_builtins)

target_include_directories(guinmotion_sdk_builtins
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(guinmotion_sdk_builtins PUBLIC GuinMotion::sdk)
target_compile_features(guinmotion_sdk_builtins PUBLIC cxx_std_20)

install(TARGETS guinmotion_sdk guinmotion_sdk_builtins EXPORT GuinMotionTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
