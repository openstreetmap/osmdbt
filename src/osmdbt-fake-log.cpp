
#include "config.hpp"
#include "db.hpp"
#include "exception.hpp"
#include "io.hpp"
#include "options.hpp"
#include "util.hpp"

#include <osmium/osm/timestamp.hpp>
#include <osmium/util/verbose_output.hpp>

#include <algorithm>
#include <ctime>
#include <iostream>
#include <iterator>
#include <string>

class FakeLogOptions : public Options
{
public:
    FakeLogOptions()
    : Options("fake-log", "Create fake log file from recent changes.")
    {}

    osmium::Timestamp timestamp() const noexcept { return m_timestamp; }

private:
    void add_command_options(po::options_description &desc) override
    {
        po::options_description opts_cmd{"COMMAND OPTIONS"};

        // clang-format off
        opts_cmd.add_options()
            ("timestamp,t", po::value<std::string>(), "Changes from this timestamp will be in the log");
        // clang-format on

        desc.add(opts_cmd);
    }

    void check_command_options(
        boost::program_options::variables_map const &vm) override
    {
        if (vm.count("timestamp")) {
            m_timestamp = osmium::Timestamp{vm["timestamp"].as<std::string>()};
        } else {
            throw argument_error{"Missing '--timestamp=TIMESTAMP' or '-t "
                                 "TIMESTAMP' on command line"};
        }
    }

    osmium::Timestamp m_timestamp{};
}; // class FakeLogOptions

static std::size_t read_objects(pqxx::work &txn, std::string &data, char type,
                                char const *statement,
                                osmium::Timestamp timestamp)
{
    pqxx::result const result =
        txn.prepared(statement)(timestamp.to_iso()).exec();

    if (result.empty()) {
        return 0;
    }

    // log lines should fit in 50 bytes
    data.reserve(data.size() + result.size() * 50);

    for (auto const &row : result) {
        data += "0/0 0 N ";
        data += type;
        data += row[0].c_str();
        data += " v";
        data += row[1].c_str();
        data += " c";
        data += row[2].c_str();
        data += '\n';
    }

    return result.size();
}

bool app(osmium::VerboseOutput &vout, Config const &config,
         FakeLogOptions const &options)
{
    // Use the same pid file as osmdbt-get-log because we use it as lock file
    PIDFile pid_file{config.run_dir(), "osmdbt-get-log"};

    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};
    db.prepare("get-nodes",
               "SELECT node_id, version, changeset_id FROM nodes WHERE "
               "\"timestamp\" >= $1 ORDER BY node_id, version;");
    db.prepare("get-ways",
               "SELECT way_id, version, changeset_id FROM ways WHERE "
               "\"timestamp\" >= $1 ORDER BY way_id, version;");
    db.prepare("get-relations",
               "SELECT relation_id, version, changeset_id FROM relations WHERE "
               "\"timestamp\" >= $1 ORDER BY relation_id, version;");

    pqxx::work txn{db};
    vout << "Database version: " << get_db_version(txn) << '\n';

    vout << "Reading changes...\n";
    std::string data;

    auto count = read_objects(txn, data, 'n', "get-nodes", options.timestamp());
    count += read_objects(txn, data, 'w', "get-ways", options.timestamp());
    count += read_objects(txn, data, 'r', "get-relations", options.timestamp());

    txn.commit();

    if (count == 0) {
        vout << "No actual changes found.\n";
        vout << "Did not write log file.\n";
    } else {
        vout << "There are " << count << " entries in the replication log.\n";

        std::string const file_name =
            create_replication_log_name(options.timestamp().to_iso());
        vout << "Writing log to '" << config.log_dir() << file_name << "'...\n";

        write_data_to_file(data, config.log_dir(), file_name);
        vout << "Wrote and synced log.\n";
    }

    vout << "Done.\n";

    return count > 0;
}

int main(int argc, char *argv[])
{
    FakeLogOptions options;
    return app_wrapper(options, argc, argv);
}
