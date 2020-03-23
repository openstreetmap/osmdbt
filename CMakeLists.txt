#-----------------------------------------------------------------------------
#
#  CMake Config
#
#  osmdbt
#
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#-----------------------------------------------------------------------------
#
#  Project version
#
#-----------------------------------------------------------------------------

project(osmdbt VERSION 0.1)

set(AUTHOR "Jochen Topf <jochen@topf.org>")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


#-----------------------------------------------------------------------------
#
#  Find external dependencies
#
#-----------------------------------------------------------------------------

find_package(Boost 1.55.0 REQUIRED COMPONENTS filesystem program_options)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

find_package(Osmium 2.14.2 REQUIRED COMPONENTS xml)
include_directories(SYSTEM ${OSMIUM_INCLUDE_DIRS})

find_library(PQXX_LIB pqxx)
if(PQXX_LIB STREQUAL "PQXX_LIB-NOTFOUND")
    message(FATAL_ERROR "Missing libpqxx")
endif()

# workaround as per https://github.com/jtv/libpqxx/issues/93
add_definitions(-DPQXX_HIDE_EXP_OPTIONAL)

find_library(YAML_LIB yaml-cpp)
if(YAML_LIB STREQUAL "YAML-NOTFOUND")
    message(FATAL_ERROR "Missing yaml-cpp")
endif()

# This can be set to something like "-v11" to force testing with a specific
# PostgreSQL version.
set(PG_VIRTUALENV_VERSION "" CACHE STRING "Version parameter for pg_virtualenv")


#-----------------------------------------------------------------------------
#
#  Compiler and Linker flags
#
#-----------------------------------------------------------------------------
add_definitions(${OSMIUM_WARNING_OPTIONS})


#-----------------------------------------------------------------------------
#
#  Optional "iwyu" target to check headers
#  https://include-what-you-use.org/
#
#-----------------------------------------------------------------------------
find_program(IWYU_TOOL NAMES iwyu_tool iwyu_tool.py)

if(IWYU_TOOL)
    message(STATUS "Looking for iwyu_tool.py - found")
    add_custom_target(iwyu ${IWYU_TOOL} -p ${CMAKE_BINARY_DIR} --
                      --no_fwd_decls
                      --quoted_includes_first
                      --mapping_file=${CMAKE_SOURCE_DIR}/iwyu.imp)
else()
    message(STATUS "Looking for iwyu_tool.py - not found")
    message(STATUS "  Make target 'iwyu' will not be available")
endif()


#-----------------------------------------------------------------------------
#
#  Optional "cppcheck" target that checks C++ code
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for cppcheck")
find_program(CPPCHECK cppcheck)

if(CPPCHECK)
    message(STATUS "Looking for cppcheck - found")
    set(CPPCHECK_OPTIONS --enable=all)

    # cpp doesn't find system includes for some reason, suppress that report
    set(CPPCHECK_OPTIONS ${CPPCHECK_OPTIONS} --suppress=missingIncludeSystem)

    file(GLOB ALL_CODE src/*.cpp)

    set(CPPCHECK_FILES ${ALL_CODE})

    add_custom_target(cppcheck
        ${CPPCHECK}
        --std=c++11 ${CPPCHECK_OPTIONS}
        ${CPPCHECK_FILES}
    )
else()
    message(STATUS "Looking for cppcheck - not found")
    message(STATUS "  Build target 'cppcheck' will not be available.")
endif(CPPCHECK)


#-----------------------------------------------------------------------------
#
#  Optional "clang-tidy" target
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for clang-tidy")
find_program(CLANG_TIDY NAMES clang-tidy clang-tidy-10 clang-tidy-9 clang-tidy-8 clang-tidy-7)

if(CLANG_TIDY)
    message(STATUS "Looking for clang-tidy - found ${CLANG_TIDY}")

    file(GLOB CT_CHECK_FILES src/*.cpp)

    add_custom_target(clang-tidy
        ${CLANG_TIDY}
        -p ${CMAKE_BINARY_DIR}
        ${CT_CHECK_FILES}
    )
else()
    message(STATUS "Looking for clang-tidy - not found")
    message(STATUS "  Build target 'clang-tidy' will not be available.")
endif()


#-----------------------------------------------------------------------------
#
#  Version
#
#-----------------------------------------------------------------------------

find_package(Git)

if(GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" describe --tags --dirty=-changed
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    OUTPUT_VARIABLE VERSION_FROM_GIT
                    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(VERSION_FROM_GIT)
        set(VERSION_FROM_GIT " (${VERSION_FROM_GIT})")
    endif()
endif()

configure_file(
    ${PROJECT_SOURCE_DIR}/src/version.cpp.in
    ${PROJECT_BINARY_DIR}/src/version.cpp
)


#-----------------------------------------------------------------------------
#
#  Tests
#
#-----------------------------------------------------------------------------

enable_testing()
add_subdirectory(test)


#-----------------------------------------------------------------------------

add_subdirectory(man)
add_subdirectory(src)


#-----------------------------------------------------------------------------
