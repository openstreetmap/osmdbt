#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  osmdbt -- PostgreSQL Plugin
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

set(PG_CONFIG "pg_config" CACHE PATH "path to pg_config program")

message(STATUS "pg_config is at ${PG_CONFIG}")

execute_process(COMMAND ${PG_CONFIG} --pkglibdir         OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE PG_PKGLIBDIR)
execute_process(COMMAND ${PG_CONFIG} --pkgincludedir     OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE PG_PKGINCLUDEDIR)
execute_process(COMMAND ${PG_CONFIG} --includedir-server OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE PG_INCLUDEDIR_SERVER)
execute_process(COMMAND ${PG_CONFIG} --cppflags          OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE PG_CPPFLAGS)
execute_process(COMMAND ${PG_CONFIG} --cflags            OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE PG_CFLAGS)
execute_process(COMMAND ${PG_CONFIG} --cflags_sl         OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE PG_CFLAGS_SL)
execute_process(COMMAND ${PG_CONFIG} --ldflags           OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE PG_LDFLAGS)

set(LIBRARY_INSTALL_PATH ${PKGLIBDIR})

message(STATUS "PKGLIBDIR         = ${PG_PKGLIBDIR}")
message(STATUS "PKGINCLUDEDIR     = ${PG_PKGINCLUDEDIR}")
message(STATUS "INCLUDEDIR_SERVER = ${PG_INCLUDEDIR_SERVER}")
message(STATUS "CPPFLAGS          = ${PG_CPPFLAGS}")
message(STATUS "CFLAGS            = ${PG_CFLAGS}")
message(STATUS "CFLAGS_SL         = ${PG_CFLAGS_SL}")
message(STATUS "LDFLAGS           = ${PG_LDFLAGS}")

add_definitions(${PG_CFLAGS} ${PG_CFLAGS_SL} ${PG_CPPFLAGS})

add_library(osm-logical SHARED osm-logical.c)
set_target_properties(osm-logical PROPERTIES PREFIX "")
target_include_directories(osm-logical PRIVATE ${PG_INCLUDEDIR_SERVER} ${PG_PKGINCLUDEDIR}/internal)

install(TARGETS osm-logical LIBRARY DESTINATION ${PG_PKGLIBDIR})

