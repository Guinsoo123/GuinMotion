add_executable(guinmotion_one_stop_validation
  one_stop_validation.cpp
)

target_link_libraries(guinmotion_one_stop_validation
  PRIVATE
    GuinMotion::app_core
    GuinMotion::sdk_builtins
)

target_compile_features(guinmotion_one_stop_validation PRIVATE cxx_std_20)
target_compile_definitions(
  guinmotion_one_stop_validation
  PRIVATE
    "GUINMOTION_ONE_STOP_DATA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/data\""
)

if(GUINMOTION_BUILD_TESTS)
  add_test(
    NAME guinmotion_one_stop_validation
    COMMAND $<TARGET_FILE:guinmotion_one_stop_validation>
  )
endif()
