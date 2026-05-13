add_executable(guinmotion_app
  main.cpp
)

set_target_properties(guinmotion_app PROPERTIES
  OUTPUT_NAME GuinMotion
)

target_link_libraries(guinmotion_app
  PRIVATE
    GuinMotion::core
    GuinMotion::operator_runtime
)

target_compile_features(guinmotion_app PRIVATE cxx_std_20)

if(GUINMOTION_ENABLE_QT)
  find_package(Qt6 REQUIRED COMPONENTS Widgets)
  target_compile_definitions(guinmotion_app PRIVATE GUINMOTION_ENABLE_QT=1)
  target_link_libraries(guinmotion_app PRIVATE Qt6::Widgets)
endif()

install(TARGETS guinmotion_app
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  BUNDLE DESTINATION "."
)
