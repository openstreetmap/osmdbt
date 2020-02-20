
#include "config.hpp"
#include "exception.hpp"

static void set_config(YAML::Node const &node, std::string& config)
{
    if (!node) {
        return;
    }

    if (node.IsNull()) {
        config = "";
    } else {
        config = node.as<std::string>();
    }
}

Config::Config(std::string const &config_file, osmium::VerboseOutput &vout)
: m_config{YAML::LoadFile(config_file)}
{
    if (m_config["database"]) {
        if (!m_config["database"].IsMap()) {
            throw config_error{"'database' entry must be a Map."};
        }

        set_config(m_config["database"]["host"], m_db_host);
        set_config(m_config["database"]["port"], m_db_port);
        set_config(m_config["database"]["dbname"], m_db_dbname);
        set_config(m_config["database"]["user"], m_db_user);
        set_config(m_config["database"]["password"], m_db_password);
        set_config(m_config["database"]["replication_slot"],
                   m_replication_slot);
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

    m_db_connection += m_db_host;
    m_db_connection += " port=";
    m_db_connection += m_db_port;
    m_db_connection += " dbname=";
    m_db_connection += m_db_dbname;
    m_db_connection += " user=";
    m_db_connection += m_db_user;
    m_db_connection += " password=";
    m_db_connection += m_db_password;

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

std::string const &Config::db_connection() const noexcept
{
    return m_db_connection;
}

std::string const &Config::replication_slot() const noexcept
{
    return m_replication_slot;
}

std::string const &Config::log_dir() const noexcept { return m_log_dir; }

std::string const &Config::changes_dir() const noexcept
{
    return m_changes_dir;
}

std::string const &Config::run_dir() const noexcept { return m_run_dir; }
