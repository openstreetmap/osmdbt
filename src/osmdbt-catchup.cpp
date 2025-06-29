
#include "config.hpp"
#include "db.hpp"
#include "exception.hpp"
#include "io.hpp"
#include "lsn.hpp"
#include "options.hpp"
#include "util.hpp"

#include <osmium/io/detail/read_write.hpp>
#include <osmium/util/verbose_output.hpp>

#include <filesystem>
#include <iostream>
#include <regex>
#include <string>

namespace {

lsn_type get_lsn(Config const &config)
{
    std::regex const re{
        R"(osm-repl-\d\d\d\d-\d\d-\d\dT\d\d:\d\d:\d\dZ-lsn-([0-9A-F]+-[0-9A-F]+)\.log)"};

    lsn_type lsn;

    std::filesystem::path const p{config.log_dir()};
    for (auto const &file : std::filesystem::directory_iterator(p)) {
        if (file.path().extension() == ".log") {
            std::string const fn = file.path().filename().string();
            std::cmatch m;
            bool const has_match = std::regex_match(fn.c_str(), m, re);
            if (has_match && m.size() == 2) {
                lsn_type const new_lsn{m.str(1)};
                if (new_lsn > lsn) {
                    lsn = new_lsn;
                }
            } else {
                throw std::runtime_error{"Invalid log file format: '" + fn +
                                         "'"};
            }
        }
    }

    return lsn;
}

class CatchupOptions : public Options
{
public:
    CatchupOptions()
    : Options("catchup", "Mark changes in the log file as done.")
    {}

    [[nodiscard]] lsn_type lsn() const noexcept { return m_lsn; }

private:
    void add_command_options(po::options_description &desc) override
    {
        po::options_description opts_cmd{"COMMAND OPTIONS"};

        // clang-format off
        opts_cmd.add_options()
            ("lsn,l", po::value<std::string>(), "LSN (Log Sequence Number)");
        // clang-format on

        desc.add(opts_cmd);
    }

    void check_command_options(po::variables_map const &vm) override
    {
        if (vm.count("lsn")) {
            m_lsn = lsn_type{vm["lsn"].as<std::string>()};
        }
    }

    lsn_type m_lsn;
}; // class CatchupOptions

bool app(osmium::VerboseOutput &vout, Config const &config,
         CatchupOptions const &options)
{
    // All commands writing log files and/or advancing the replication slot
    // use the same pid/lock file.
    PIDFile const pid_file{config.run_dir(), "osmdbt-log"};

    lsn_type const lsn = options.lsn() ? options.lsn() : get_lsn(config);
    if (!lsn) {
        vout << "No catching up to do.\n";
        vout << "Done.\n";
        return true;
    }

    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};

    pqxx::work txn{db};
    vout << "Database version: " << get_db_version(txn) << '\n';

    vout << "Catching up to " << lsn.str() << "...\n";
    catchup_to_lsn(txn, config.replication_slot(), lsn.str());

    txn.commit();

    vout << "Done.\n";

    return true;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
    CatchupOptions options;
    return app_wrapper(options, argc, argv);
}
