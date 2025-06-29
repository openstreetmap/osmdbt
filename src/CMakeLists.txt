#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_definitions(${OSMIUM_WARNING_OPTIONS})

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(SYSTEM ${OSMIUM_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR} ${PROTOZERO_INCLUDE_DIR})

set(COMMON_SRCS config.cpp io.cpp options.cpp util.cpp ${PROJECT_BINARY_DIR}/src/version.cpp)

set(COMMON_LIBS ${Boost_PROGRAM_OPTIONS_LIBRARY} ${YAML_LIB})

add_executable(osmdbt-catchup osmdbt-catchup.cpp db.cpp lsn.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-catchup ${PQXX_LIB} ${COMMON_LIBS})
install(TARGETS osmdbt-catchup DESTINATION bin)

add_executable(osmdbt-create-diff osmdbt-create-diff.cpp db.cpp lsn.cpp osmobj.cpp state.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-create-diff ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${PQXX_LIB} ${COMMON_LIBS})
set_pthread_on_target(osmdbt-create-diff)
install(TARGETS osmdbt-create-diff DESTINATION bin)

add_executable(osmdbt-disable-replication osmdbt-disable-replication.cpp db.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-disable-replication ${PQXX_LIB} ${COMMON_LIBS})
install(TARGETS osmdbt-disable-replication DESTINATION bin)

add_executable(osmdbt-enable-replication osmdbt-enable-replication.cpp db.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-enable-replication ${PQXX_LIB} ${COMMON_LIBS})
install(TARGETS osmdbt-enable-replication DESTINATION bin)

add_executable(osmdbt-get-log osmdbt-get-log.cpp db.cpp lsn.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-get-log ${PQXX_LIB} ${COMMON_LIBS})
set_pthread_on_target(osmdbt-get-log)
install(TARGETS osmdbt-get-log DESTINATION bin)

add_executable(osmdbt-fake-log osmdbt-fake-log.cpp db.cpp osmobj.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-fake-log ${PQXX_LIB} ${COMMON_LIBS})
set_pthread_on_target(osmdbt-fake-log)
install(TARGETS osmdbt-fake-log DESTINATION bin)

add_executable(osmdbt-testdb osmdbt-testdb.cpp db.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-testdb ${PQXX_LIB} ${COMMON_LIBS})
install(TARGETS osmdbt-testdb DESTINATION bin)

