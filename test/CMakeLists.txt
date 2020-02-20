#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  Tests
#
#-----------------------------------------------------------------------------

include_directories(include)
include_directories(../src)
include_directories(../include)

set(ALL_UNIT_TESTS
    t/test_util.cpp
    t/test_config.cpp
)

add_executable(unit_tests unit_tests.cpp ${ALL_UNIT_TESTS} ../src/config.cpp ../src/util.cpp)
target_link_libraries(unit_tests ${YAML_LIB})
add_test(NAME unit_tests COMMAND unit_tests WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

