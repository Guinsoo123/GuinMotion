add_library(guinmotion_app_core
  src/project_service.cpp
)

add_library(GuinMotion::app_core ALIAS guinmotion_app_core)

target_include_directories(guinmotion_app_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(guinmotion_app_core
  PUBLIC
    GuinMotion::core
    GuinMotion::sdk
    guinmotion_sdk_builtins
)

target_compile_features(guinmotion_app_core PUBLIC cxx_std_20)

install(TARGETS guinmotion_app_core
  EXPORT GuinMotionTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
