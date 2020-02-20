#pragma once

#include <yaml-cpp/yaml.h>

#include <osmium/util/verbose_output.hpp>

#include <string>

class Config
{
public:
    explicit Config(std::string const &config_file,
                    osmium::VerboseOutput &vout);

    std::string const &db_connection() const noexcept;
    std::string const &replication_slot() const noexcept;
    std::string const &log_dir() const noexcept;
    std::string const &changes_dir() const noexcept;
    std::string const &run_dir() const noexcept;

private:
    YAML::Node m_config;

    std::string m_db_host{"localhost"};
    std::string m_db_port{"5432"};
    std::string m_db_dbname{"osm"};
    std::string m_db_user{"osm"};
    std::string m_db_password{"osm"};

    std::string m_db_connection{"host="};
    std::string m_replication_slot{"rs"};

    std::string m_log_dir{"/tmp"};
    std::string m_changes_dir{"/tmp"};
    std::string m_run_dir{"/tmp"};
}; // class Config
