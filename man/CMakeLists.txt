#-----------------------------------------------------------------------------
#
#  Generate man pages
#
#-----------------------------------------------------------------------------
message(STATUS "Looking for pandoc")
find_program(PANDOC pandoc)

function(add_man_page _section _name)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/man${_section})
    set(_output_file ${CMAKE_CURRENT_BINARY_DIR}/man${_section}/${_name}.${_section})
    install(FILES ${_output_file} DESTINATION share/man/man${_section})
    set(_source_file ${CMAKE_CURRENT_SOURCE_DIR}/${_name}.md)
    set(_dest_file ${CMAKE_CURRENT_BINARY_DIR}/source/${_name}.md)
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/common-options.md MAN_COMMON_OPTIONS)
    configure_file(${_source_file} ${_dest_file} @ONLY)
    string(TOUPPER ${_name} _name_upcase)
    add_custom_command(OUTPUT ${_output_file}
        COMMAND ${PANDOC}
            ${PANDOC_MAN_OPTIONS}
            --variable "title=${_name_upcase}"
            --variable "section=${_section}"
            -o ${_output_file}
            ${_dest_file}
        DEPENDS ${_source_file} manpage.template common-options.md
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building manpage ${_name}.${_section}"
        VERBATIM)
    set(ALL_MAN_PAGES "${ALL_MAN_PAGES};${_output_file}" PARENT_SCOPE)
endfunction()


if(PANDOC)
    message(STATUS "Looking for pandoc - found")
    message(STATUS "  Manual pages will be built")
    set(PANDOC_MAN_OPTIONS
        -s
        -t man
        --template ${CMAKE_CURRENT_SOURCE_DIR}/manpage.template
        --variable "description=osmdbt/${OSMDBT_VERSION}"
        --variable "version=${OSMDBT_VERSION}"
        --variable "author=${AUTHOR}"
    )

    add_man_page(1 osmdbt)
    add_man_page(1 osmdbt-catchup)
    add_man_page(1 osmdbt-create-diff)
    add_man_page(1 osmdbt-disable-replication)
    add_man_page(1 osmdbt-enable-replication)
    add_man_page(1 osmdbt-fake-log)
    add_man_page(1 osmdbt-get-log)
    add_man_page(1 osmdbt-testdb)
    add_man_page(5 osmdbt-state.txt)

    add_custom_target(man ALL DEPENDS ${ALL_MAN_PAGES})
else()
    message(STATUS "Looking for pandoc - not found")
    message(STATUS "  Manual pages will not be built")
endif()


