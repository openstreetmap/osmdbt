
#include "config.hpp"
#include "exception.hpp"

#include <osmium/util/verbose_output.hpp>

#include <cassert>
#include <cerrno>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

static void set_config(YAML::Node const &node, std::string &config)
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

static void build_conn_str(std::string &str, char const *key,
                           std::string const &val)
{
    if (val.empty()) {
        return;
    }

    if (!str.empty()) {
        str += ' ';
    }
    str += key;
    str += '=';
    str += val;
}

static YAML::Node load_config_file(std::string const &config_file)
{
    std::ifstream stream{config_file};
    if (!stream.is_open()) {
        throw std::system_error{errno, std::system_category(),
                                "Config error: Could not open config file '" +
                                    config_file + "'"};
    }

    std::string const data((std::istreambuf_iterator<char>(stream)),
                           std::istreambuf_iterator<char>());

    return YAML::Load(data);
}

static void set_dir(YAML::Node const &config, std::string *var)
{
    assert(var);
    if (!config) {
        return;
    }

    *var = config.as<std::string>();
    if (var->back() != '/') {
        var->append("/");
    }
}

Config::Config(std::string const &config_file, osmium::VerboseOutput &vout)
: m_config{load_config_file(config_file)}
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

    set_dir(m_config["log_dir"], &m_log_dir);
    set_dir(m_config["changes_dir"], &m_changes_dir);
    set_dir(m_config["tmp_dir"], &m_tmp_dir);
    set_dir(m_config["run_dir"], &m_run_dir);

    build_conn_str(m_db_connection, "host", m_db_host);
    build_conn_str(m_db_connection, "port", m_db_port);
    build_conn_str(m_db_connection, "dbname", m_db_dbname);
    build_conn_str(m_db_connection, "user", m_db_user);
    build_conn_str(m_db_connection, "password", m_db_password);

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
    vout << "  Directory for tmp files: " << m_tmp_dir << '\n';
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

std::string const &Config::tmp_dir() const noexcept { return m_tmp_dir; }

std::string const &Config::run_dir() const noexcept { return m_run_dir; }
