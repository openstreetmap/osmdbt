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

    std::string m_log_dir{"/tmp"};
    std::string m_changes_dir{"/tmp"};
    std::string m_run_dir{"/tmp"};

public:
    explicit Config(Options const &options, osmium::VerboseOutput &vout);

    std::string const &db_connection() const;

    std::string const &replication_slot() const;

    std::string const &log_dir() const;
    std::string const &changes_dir() const;
    std::string const &run_dir() const;

}; // class Config
