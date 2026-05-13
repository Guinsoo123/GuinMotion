add_library(guinmotion_viewport
  src/viewport_widget.cpp
)

add_library(GuinMotion::viewport ALIAS guinmotion_viewport)

find_package(Qt6 REQUIRED COMPONENTS Gui OpenGLWidgets OpenGL)

target_include_directories(guinmotion_viewport
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(guinmotion_viewport
  PUBLIC
    GuinMotion::core
    Qt6::Gui
    Qt6::OpenGLWidgets
    Qt6::OpenGL
)

target_compile_features(guinmotion_viewport PUBLIC cxx_std_20)

install(TARGETS guinmotion_viewport
  EXPORT GuinMotionTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
