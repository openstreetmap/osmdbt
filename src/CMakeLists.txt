#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

set(COMMON_SRCS config.cpp db.cpp options.cpp ${PROJECT_BINARY_DIR}/src/version.cpp)

set(COMMON_LIBS ${Boost_LIBRARIES} ${PQXX_LIB} ${PQ_LIB} ${YAML_LIB})

add_executable(osmdbt-catchup osmdbt-catchup.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-catchup ${COMMON_LIBS})
install(TARGETS osmdbt-catchup DESTINATION bin)

add_executable(osmdbt-create-diff osmdbt-create-diff.cpp io.cpp ${COMMON_SRCS} util.cpp)
target_link_libraries(osmdbt-create-diff ${OSMIUM_LIBRARIES} ${COMMON_LIBS})
set_pthread_on_target(osmdbt-create-diff)
install(TARGETS osmdbt-create-diff DESTINATION bin)

add_executable(osmdbt-disable-replication osmdbt-disable-replication.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-disable-replication ${COMMON_LIBS})
install(TARGETS osmdbt-disable-replication DESTINATION bin)

add_executable(osmdbt-enable-replication osmdbt-enable-replication.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-enable-replication ${COMMON_LIBS})
install(TARGETS osmdbt-enable-replication DESTINATION bin)

add_executable(osmdbt-peek osmdbt-peek.cpp io.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-peek ${COMMON_LIBS})
set_pthread_on_target(osmdbt-peek)
install(TARGETS osmdbt-peek DESTINATION bin)

add_executable(osmdbt-testdb osmdbt-testdb.cpp ${COMMON_SRCS})
target_link_libraries(osmdbt-testdb ${COMMON_LIBS})
install(TARGETS osmdbt-testdb DESTINATION bin)

