
#include "config.hpp"

Config::Config(Options const &options, osmium::VerboseOutput &vout)
: m_config{YAML::LoadFile(options.config_file())}
{

    if (!m_config["database"]) {
        throw std::runtime_error{"Missing 'database' section in config file."};
    }
    if (!m_config["database"].IsMap()) {
        throw std::runtime_error{"'database' entry must be a Map."};
    }

    if (m_config["database"]["host"]) {
        m_db_host = m_config["database"]["host"].as<std::string>();
    }
    if (m_config["database"]["dbname"]) {
        m_db_dbname = m_config["database"]["dbname"].as<std::string>();
    }
    if (m_config["database"]["user"]) {
        m_db_user = m_config["database"]["user"].as<std::string>();
    }
    if (m_config["database"]["password"]) {
        m_db_password = m_config["database"]["password"].as<std::string>();
    }

    m_db_connection += m_db_host;
    m_db_connection += " dbname=";
    m_db_connection += m_db_dbname;
    m_db_connection += " user=";
    m_db_connection += m_db_user;
    m_db_connection += " password=";
    m_db_connection += m_db_password;

    if (m_config["database"]["replication_slot"]) {
        m_replication_slot =
            m_config["database"]["replication_slot"].as<std::string>();
    }

    if (m_config["dir"]) {
        m_dir = m_config["dir"].as<std::string>();
    }

    vout << "Config:\n";
    vout << "  Database:\n";
    vout << "    Host: " << m_db_host << '\n';
    vout << "    Name: " << m_db_dbname << '\n';
    vout << "    User: " << m_db_user << '\n';
    vout << "    Password: (not shown)\n";
    vout << "    Replication Slot: " << m_replication_slot << '\n';
    vout << "  Dir: " << m_dir << '\n';
}

std::string const &Config::db_connection() const { return m_db_connection; }

std::string const &Config::replication_slot() const
{
    return m_replication_slot;
}

std::string const &Config::dir() const { return m_dir; }
