set(GUINMOTION_LOCAL_PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/plugins")
file(MAKE_DIRECTORY "${GUINMOTION_LOCAL_PLUGIN_OUTPUT_DIR}")

add_library(guinmotion_minimal_operator MODULE
  minimal_operator.cpp
)

target_link_libraries(guinmotion_minimal_operator
  PRIVATE
    GuinMotion::sdk
)

target_compile_definitions(guinmotion_minimal_operator
  PRIVATE
    GUINMOTION_PLUGIN_BUILD=1
)

set_target_properties(guinmotion_minimal_operator PROPERTIES
  PREFIX ""
  OUTPUT_NAME "minimal_operator"
  LIBRARY_OUTPUT_DIRECTORY "${GUINMOTION_LOCAL_PLUGIN_OUTPUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${GUINMOTION_LOCAL_PLUGIN_OUTPUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY "${GUINMOTION_LOCAL_PLUGIN_OUTPUT_DIR}"
)

install(TARGETS guinmotion_minimal_operator
  LIBRARY DESTINATION "${CMAKE_INSTALL_DATADIR}/guinmotion/plugins/minimal_operator"
  RUNTIME DESTINATION "${CMAKE_INSTALL_DATADIR}/guinmotion/plugins/minimal_operator"
)

install(FILES plugin.json
  DESTINATION "${CMAKE_INSTALL_DATADIR}/guinmotion/plugins/minimal_operator"
)
