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
    t/test-config.cpp
    t/test-lsn.cpp
    t/test-osmobj.cpp
    t/test-state.cpp
    t/test-util.cpp
)

add_test(NAME unit-test-setup COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/some-state.txt)
set_tests_properties(unit-test-setup PROPERTIES FIXTURES_SETUP UnitTest)

add_executable(unit-tests unit-tests.cpp ${ALL_UNIT_TESTS}
               ../src/config.cpp ../src/lsn.cpp ../src/io.cpp ../src/osmobj.cpp
               ../src/state.cpp ../src/util.cpp)
add_definitions(-DTEST_DIR="${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(unit-tests ${PQXX_LIB} ${YAML_LIB})
add_test(NAME unit-tests COMMAND unit-tests WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
set_tests_properties(unit-tests PROPERTIES FIXTURES_REQUIRED UnitTest)

function(add_pg_test _tname)
    add_test(NAME ${_tname} COMMAND pg_virtualenv -o wal_level=logical ${CMAKE_CURRENT_SOURCE_DIR}/scripts/${_tname}.sh)
    set_tests_properties(${_tname} PROPERTIES ENVIRONMENT
                         "TEST=${_tname};TESTDIR=${CMAKE_CURRENT_BINARY_DIR}/${_tname};SRCDIR=${CMAKE_CURRENT_SOURCE_DIR}/scripts")
endfunction()

add_pg_test(01-get-log)
add_pg_test(02-create-diff)
add_pg_test(03-fake-log)
add_pg_test(04-log-pid-fail)
add_pg_test(05-create-diff-state)
add_pg_test(06-create-diff-missing-state)
add_pg_test(07-cmdline)

