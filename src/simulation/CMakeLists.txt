add_library(guinmotion_simulation
  src/mujoco_simulation.cpp
  src/trajectory_evaluator.cpp
)

add_library(GuinMotion::simulation ALIAS guinmotion_simulation)

target_include_directories(guinmotion_simulation
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(guinmotion_simulation
  PUBLIC
    GuinMotion::core
)

target_compile_features(guinmotion_simulation PUBLIC cxx_std_20)

if(TARGET mujoco::mujoco)
  target_compile_definitions(guinmotion_simulation PRIVATE GUINMOTION_WITH_MUJOCO=1)
  target_link_libraries(guinmotion_simulation PRIVATE mujoco::mujoco)
elseif(TARGET mujoco)
  target_compile_definitions(guinmotion_simulation PRIVATE GUINMOTION_WITH_MUJOCO=1)
  target_link_libraries(guinmotion_simulation PRIVATE mujoco)
endif()

install(TARGETS guinmotion_simulation
  EXPORT GuinMotionTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
