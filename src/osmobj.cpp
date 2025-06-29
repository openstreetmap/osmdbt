
#include "osmobj.hpp"

#include <osmium/util/string.hpp>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

osmobj::osmobj(std::string const &obj, std::string const &version,
               std::string const &changeset, changeset_user_lookup *cucache)
{
    if (obj.size() < 2 || version.size() < 2 || changeset.size() < 2) {
        throw std::runtime_error{"Log file has wrong format: entry too short"};
    }

    m_type = osmium::char_to_item_type(obj[0]);
    if (m_type != osmium::item_type::node && m_type != osmium::item_type::way &&
        m_type != osmium::item_type::relation) {
        throw std::runtime_error{
            "Log file has wrong format: type must be 'n', 'w', or 'r'"};
    }
    m_id = std::strtoll(&obj[1], nullptr, 10);

    if (version[0] != 'v') {
        throw std::runtime_error{"Log file has wrong format: expected version"};
    }
    m_version = std::strtoll(&version[1], nullptr, 10);

    if (changeset[0] != 'c') {
        throw std::runtime_error{
            "Log file has wrong format: expected changeset"};
    }
    m_cid = std::strtoll(&changeset[1], nullptr, 10);

    if (cucache) {
        (*cucache)[m_cid] = {};
    }
}

void read_log(osmobjects &objects_todo, std::string const &dir_name,
              std::string const &file_name, changeset_user_lookup *cucache)
{
    std::ifstream logfile{dir_name + file_name};
    if (!logfile.is_open()) {
        throw std::system_error{errno, std::system_category(),
                                "Could not open log file '" + dir_name +
                                    file_name + "'"};
    }

    for (std::string line; std::getline(logfile, line);) {
        auto const parts = osmium::split_string(line, ' ');
        if (parts.size() < 3) {
            std::cerr << "Warning: Ignored log line due to wrong formatting: "
                      << line << '\n';
            continue;
        }

        if (parts[2] == "N") {
            if (parts.size() != 6) {
                std::cerr
                    << "Warning: Ignored log line due to wrong formatting: "
                    << line << '\n';
                continue;
            }
            objects_todo.add(parts[3], parts[4], parts[5], cucache);
        } else if (parts[2] == "X") {
            std::cerr << "Error found in logfile: " << line << '\n';
        }
    }
}

void osmobjects::sort()
{
    std::sort(m_objects(osmium::item_type::node).begin(),
              m_objects(osmium::item_type::node).end());
    std::sort(m_objects(osmium::item_type::way).begin(),
              m_objects(osmium::item_type::way).end());
    std::sort(m_objects(osmium::item_type::relation).begin(),
              m_objects(osmium::item_type::relation).end());
}
