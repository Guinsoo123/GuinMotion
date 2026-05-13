add_executable(guinmotion_app
  main.cpp
)

set(GUINMOTION_APP_EXTRA_SOURCES "")
if(GUINMOTION_ENABLE_QT)
  list(APPEND GUINMOTION_APP_EXTRA_SOURCES main_window.cpp)
endif()
target_sources(guinmotion_app PRIVATE ${GUINMOTION_APP_EXTRA_SOURCES})

set_target_properties(guinmotion_app PROPERTIES
  OUTPUT_NAME GuinMotion
)

target_link_libraries(guinmotion_app
  PRIVATE
    GuinMotion::app_core
    GuinMotion::operator_runtime
)

target_include_directories(guinmotion_app
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(guinmotion_app PRIVATE cxx_std_20)

if(GUINMOTION_ENABLE_PYTHON)
  target_compile_definitions(guinmotion_app PRIVATE GUINMOTION_ENABLE_PYTHON=1)
  target_link_libraries(guinmotion_app PRIVATE guinmotion_python_embed)
endif()

if(GUINMOTION_ENABLE_QT)
  find_package(Qt6 REQUIRED COMPONENTS Widgets)
  target_compile_definitions(guinmotion_app PRIVATE GUINMOTION_ENABLE_QT=1)
  target_compile_definitions(
    guinmotion_app
    PRIVATE
      "GUINMOTION_BUILD_PLUGIN_DIR=\"${CMAKE_BINARY_DIR}/bin/plugins\""
  )
  target_link_libraries(guinmotion_app PRIVATE Qt6::Widgets GuinMotion::viewport)
  if(GUINMOTION_BUILD_EXAMPLES)
    add_dependencies(guinmotion_app guinmotion_minimal_operator)
  endif()
endif()

install(TARGETS guinmotion_app
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  BUNDLE DESTINATION "."
)
