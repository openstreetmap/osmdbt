#pragma once

#include "options.hpp"

#include <yaml-cpp/yaml.h>

#include <osmium/util/verbose_output.hpp>

#include <string>

class Config
{

    YAML::Node m_config;

    std::string m_db_host{"localhost"};
    std::string m_db_dbname{"osm"};
    std::string m_db_user{"osm"};
    std::string m_db_password{"osm"};

    std::string m_db_connection{"host="};
    std::string m_replication_slot;

public:
    explicit Config(Options const &options, osmium::VerboseOutput &vout);

    std::string const &db_connection() const;

    std::string const &replication_slot() const;

}; // class Config
