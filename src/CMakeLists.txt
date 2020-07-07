#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_definitions(${OSMIUM_WARNING_OPTIONS})

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
include_directories(SYSTEM ${OSMIUM_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIR})

set(COMMON_SRCS config.cpp db.cpp io.cpp lsn.cpp options.cpp util.cpp
                ${PROJECT_BINARY_DIR}/src/version.cpp)

set(COMMON_LIBS ${Boost_PROGRAM_OPTIONS_LIBRARY} ${PQXX_LIB} ${YAML_LIB})

add_executable(osmdbt-catchup osmdbt-catchup.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-catchup ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${COMMON_LIBS})
install(TARGETS osmdbt-catchup DESTINATION bin)

add_executable(osmdbt-create-diff osmdbt-create-diff.cpp osmobj.cpp state.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-create-diff ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${COMMON_LIBS})
set_pthread_on_target(osmdbt-create-diff)
install(TARGETS osmdbt-create-diff DESTINATION bin)

add_executable(osmdbt-disable-replication osmdbt-disable-replication.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-disable-replication ${COMMON_LIBS})
install(TARGETS osmdbt-disable-replication DESTINATION bin)

add_executable(osmdbt-enable-replication osmdbt-enable-replication.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-enable-replication ${COMMON_LIBS})
install(TARGETS osmdbt-enable-replication DESTINATION bin)

add_executable(osmdbt-get-log osmdbt-get-log.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-get-log ${COMMON_LIBS})
set_pthread_on_target(osmdbt-get-log)
install(TARGETS osmdbt-get-log DESTINATION bin)

add_executable(osmdbt-fake-log osmdbt-fake-log.cpp osmobj.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-fake-log ${COMMON_LIBS})
set_pthread_on_target(osmdbt-fake-log)
install(TARGETS osmdbt-fake-log DESTINATION bin)

add_executable(osmdbt-testdb osmdbt-testdb.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-testdb ${COMMON_LIBS})
install(TARGETS osmdbt-testdb DESTINATION bin)

