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
    t/test-util.cpp
    t/test-config.cpp
)

add_executable(unit-tests unit-tests.cpp ${ALL_UNIT_TESTS} ../src/config.cpp ../src/util.cpp)
target_link_libraries(unit-tests ${YAML_LIB})
add_test(NAME unit-tests COMMAND unit-tests WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_test(NAME db-init COMMAND ${PROJECT_SOURCE_DIR}/test/db/init.sh)

add_test(NAME db-test COMMAND osmdbt-testdb -c test-config.yaml)
set_tests_properties(db-test PROPERTIES DEPENDS db-init)

add_test(NAME db-enable COMMAND osmdbt-enable-replication -c test-config.yaml)
set_tests_properties(db-enable PROPERTIES DEPENDS db-test)

add_test(NAME db-get-log-1 COMMAND osmdbt-get-log -c test-config.yaml)
set_tests_properties(db-get-log-1 PROPERTIES DEPENDS db-enable WILL_FAIL 1)

add_test(NAME db-data COMMAND ${PROJECT_SOURCE_DIR}/test/db/create-objects.sh)
set_tests_properties(db-data PROPERTIES DEPENDS db-get-log-1)

add_test(NAME db-get-log-2 COMMAND osmdbt-get-log -c test-config.yaml --catchup)
set_tests_properties(db-get-log-2 PROPERTIES DEPENDS db-data)

add_test(NAME db-check-log COMMAND ${PROJECT_SOURCE_DIR}/test/db/check-log.sh)
set_tests_properties(db-check-log PROPERTIES DEPENDS db-get-log-2)

add_test(NAME db-disable COMMAND osmdbt-disable-replication -c test-config.yaml)
set_tests_properties(db-disable PROPERTIES DEPENDS db-check-log)

