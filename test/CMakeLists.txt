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
)

add_executable(unit_tests unit_tests.cpp ${ALL_UNIT_TESTS} ../src/util.cpp)
#target_link_libraries(unit_tests ${Boost_LIBRARIES} ${OSMIUM_LIBRARIES})
#set_pthread_on_target(unit_tests)
add_test(NAME unit_tests COMMAND unit_tests WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

