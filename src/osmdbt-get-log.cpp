
#include "config.hpp"
#include "db.hpp"
#include "exception.hpp"
#include "io.hpp"
#include "options.hpp"
#include "util.hpp"

#include <osmium/util/verbose_output.hpp>

#include <algorithm>
#include <ctime>
#include <iostream>
#include <iterator>
#include <string>

class GetLogOptions : public Options
{
public:
    GetLogOptions()
    : Options("get-log", "Write changes from replication slot to log file.")
    {}

    bool catchup() const noexcept { return m_catchup; }

    uint32_t max_changes() const noexcept { return m_max_changes; }

private:
    void add_command_options(po::options_description &desc) override
    {
        po::options_description opts_cmd{"COMMAND OPTIONS"};

        // clang-format off
        opts_cmd.add_options()
            ("catchup", "Commit changes when they have been logged successfully")
            ("max-changes,m", po::value<uint32_t>(), "Maximum number of changes (default: no limit)");
        // clang-format on

        desc.add(opts_cmd);
    }

    void check_command_options(
        boost::program_options::variables_map const &vm) override
    {
        if (vm.count("catchup")) {
            m_catchup = true;
        }
        if (vm.count("max-changes")) {
            m_max_changes = vm["max-changes"].as<uint32_t>();
        }
    }

    std::uint32_t m_max_changes = 0;
    bool m_catchup = false;
}; // class GetLogOptions

bool app(osmium::VerboseOutput &vout, Config const &config,
         GetLogOptions const &options)
{
    // All commands writing log files and/or advancing the replication slot
    // use the same pid/lock file.
    PIDFile pid_file{config.run_dir(), "osmdbt-log"};

    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};

    std::string select{"SELECT * FROM pg_logical_slot_peek_changes($1, NULL, "};
    if (options.max_changes() > 0) {
        vout << "Reading up to " << options.max_changes()
             << " changes (change with --max-changes)\n";
        select += std::to_string(options.max_changes());
    } else {
        vout << "Reading any number of changes (change with --max-changes)\n";
        select += "NULL";
    }
    select += ");";

    db.prepare("peek", select);

    std::string lsn;

    {
        pqxx::read_transaction txn{db};
        vout << "Database version: " << get_db_version(txn) << '\n';

        vout << "Reading replication log...\n";
        pqxx::result const result =
#if PQXX_VERSION_MAJOR >= 6
            txn.exec_prepared("peek", config.replication_slot());
#else
            txn.prepared("peek")(config.replication_slot()).exec();
#endif

        if (result.empty()) {
            vout << "No changes found.\n";
            vout << "Did not write log file.\n";
            txn.commit();
            vout << "Done.\n";
            return true;
        }

        vout << "There are " << result.size()
             << " entries in the replication log.\n";

        std::string data;
        data.reserve(result.size() * 50); // log lines should fit in 50 bytes

        bool has_actual_data = false;
        for (auto const &row : result) {
            char const *const message = row[2].c_str();

            data.append(row[0].c_str());
            data += ' ';
            data.append(row[1].c_str());
            data += ' ';
            data.append(message);
            data += '\n';

            if (message[0] == 'C') {
                lsn = row[0].c_str();
            } else if (message[0] == 'N') {
                has_actual_data = true;
            }
        }

        vout << "LSN is " << lsn << '\n';

        if (has_actual_data) {
            std::string lsn_dash{"lsn-"};
            std::transform(lsn.cbegin(), lsn.cend(),
                           std::back_inserter(lsn_dash),
                           [](char c) { return c == '/' ? '-' : c; });

            std::string const file_name = create_replication_log_name(lsn_dash);
            vout << "Writing log to '" << config.log_dir() << file_name
                 << "'...\n";

            write_data_to_file(data, config.log_dir(), file_name);
            vout << "Wrote and synced log.\n";
        } else {
            vout << "No actual changes found.\n";
            vout << "Did not write log file.\n";
        }
    }

    if (options.catchup()) {
        vout << "Catching up to " << lsn << "...\n";
        pqxx::work txn{db};
        catchup_to_lsn(txn, config.replication_slot(), lsn_type{lsn});
        txn.commit();
    } else {
        vout << "Not catching up (use --catchup if you want this).\n";
    }

    vout << "Done.\n";

    return true;
}

int main(int argc, char *argv[])
{
    GetLogOptions options;
    return app_wrapper(options, argc, argv);
}
