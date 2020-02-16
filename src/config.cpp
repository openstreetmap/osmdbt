
#include "config.hpp"
#include "exception.hpp"

Config::Config(Options const &options, osmium::VerboseOutput &vout)
: m_config{YAML::LoadFile(options.config_file())}
{

    if (!m_config["database"]) {
        throw config_error{"Missing 'database' section."};
    }
    if (!m_config["database"].IsMap()) {
        throw config_error{"'database' entry must be a Map."};
    }

    if (m_config["database"]["host"]) {
        m_db_host = m_config["database"]["host"].as<std::string>();
    }
    if (m_config["database"]["port"]) {
        m_db_port = m_config["database"]["port"].as<std::string>();
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
    m_db_connection += " port=";
    m_db_connection += m_db_port;
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

    if (m_config["log_dir"]) {
        m_log_dir = m_config["log_dir"].as<std::string>();
    }

    if (m_config["changes_dir"]) {
        m_changes_dir = m_config["changes_dir"].as<std::string>();
    }

    if (m_config["run_dir"]) {
        m_changes_dir = m_config["run_dir"].as<std::string>();
    }

    vout << "Config:\n";
    vout << "  Database:\n";
    vout << "    Host: " << m_db_host << '\n';
    vout << "    Port: " << m_db_port << '\n';
    vout << "    Name: " << m_db_dbname << '\n';
    vout << "    User: " << m_db_user << '\n';
    vout << "    Password: (not shown)\n";
    vout << "    Replication Slot: " << m_replication_slot << '\n';
    vout << "  Directory for log files: " << m_log_dir << '\n';
    vout << "  Directory for change files: " << m_changes_dir << '\n';
    vout << "  Directory for run files: " << m_run_dir << '\n';
}

std::string const &Config::db_connection() const { return m_db_connection; }

std::string const &Config::replication_slot() const
{
    return m_replication_slot;
}

std::string const &Config::log_dir() const { return m_log_dir; }

std::string const &Config::changes_dir() const { return m_changes_dir; }

std::string const &Config::run_dir() const { return m_run_dir; }
