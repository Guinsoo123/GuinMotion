add_library(guinmotion_core
  src/project.cpp
  src/io/trajectory_xml.cpp
  src/io/point_cloud_file.cpp
  src/io/robot_model_urdf.cpp
  src/io/target_points_xml.cpp
)

add_library(GuinMotion::core ALIAS guinmotion_core)

target_include_directories(guinmotion_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(guinmotion_core PUBLIC cxx_std_20)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_libraries(guinmotion_core PUBLIC stdc++fs)
endif()

install(TARGETS guinmotion_core
  EXPORT GuinMotionTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
