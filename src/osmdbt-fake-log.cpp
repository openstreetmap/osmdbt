
#include "config.hpp"
#include "db.hpp"
#include "exception.hpp"
#include "io.hpp"
#include "options.hpp"
#include "osmobj.hpp"
#include "util.hpp"

#include <osmium/index/nwr_array.hpp>
#include <osmium/osm/timestamp.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/util/verbose_output.hpp>

#include <algorithm>
#include <ctime>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <utility>

using id_version_type =
    std::pair<osmium::object_id_type, osmium::object_version_type>;

class FakeLogOptions : public Options
{
public:
    FakeLogOptions()
    : Options("fake-log", "Create fake log file from recent changes.")
    {}

    std::vector<std::string> const &log_file_names() const noexcept
    {
        return m_log_file_names;
    }

    osmium::Timestamp timestamp() const noexcept { return m_timestamp; }

private:
    void add_command_options(po::options_description &desc) override
    {
        po::options_description opts_cmd{"COMMAND OPTIONS"};

        // clang-format off
        opts_cmd.add_options()
            ("timestamp,t", po::value<std::string>(), "Changes at or after this timestamp will be in the log")
            ("log,l", po::value<std::vector<std::string>>(), "Remove entries found in this log file");
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

        if (vm.count("log")) {
            m_log_file_names = vm["log"].as<std::vector<std::string>>();
        }
    }

    std::vector<std::string> m_log_file_names;
    osmium::Timestamp m_timestamp{};

}; // class FakeLogOptions

static std::size_t
read_objects(pqxx::dbtransaction &txn, std::string &data,
             osmium::Timestamp timestamp, osmium::item_type type,
             osmium::nwr_array<std::set<id_version_type>> const &objects_done)
{
    pqxx::result const result =
#if PQXX_VERSION_MAJOR >= 6
        txn.exec_prepared(osmium::item_type_to_name(type), timestamp.to_iso());
#else
        txn.prepared(osmium::item_type_to_name(type))(timestamp.to_iso())
            .exec();
#endif

    if (result.empty()) {
        return 0;
    }

    // log lines should fit in 50 bytes
    data.reserve(data.size() + result.size() * 50);

    std::size_t count = 0;
    for (auto const &row : result) {
        auto const p = std::make_pair(row[0].as<osmium::object_id_type>(),
                                      row[1].as<osmium::object_version_type>());
        if (!objects_done(type).count(p)) {
            data += "0/0 0 N ";
            data += osmium::item_type_to_char(type);
            data += row[0].c_str();
            data += " v";
            data += row[1].c_str();
            data += " c";
            data += row[2].c_str();
            data += '\n';
            ++count;
        }
    }

    return count;
}

static osmium::nwr_array<std::set<id_version_type>>
read_log_files(std::string const &log_dir,
               std::vector<std::string> const &log_names)
{
    osmium::nwr_array<std::set<id_version_type>> objects_done;

    for (auto const &log : log_names) {
        osmobjects objects;
        read_log(objects, log[0] == '/' ? "" : log_dir, log);
        for (auto const &obj : objects.nodes()) {
            objects_done(osmium::item_type::node)
                .emplace(obj.id(), obj.version());
        }
        for (auto const &obj : objects.ways()) {
            objects_done(osmium::item_type::way)
                .emplace(obj.id(), obj.version());
        }
        for (auto const &obj : objects.relations()) {
            objects_done(osmium::item_type::relation)
                .emplace(obj.id(), obj.version());
        }
    }

    return objects_done;
}

bool app(osmium::VerboseOutput &vout, Config const &config,
         FakeLogOptions const &options)
{
    // All commands writing log files and/or advancing the replication slot
    // use the same pid/lock file.
    PIDFile pid_file{config.run_dir(), "osmdbt-log"};

    auto const objects_done =
        read_log_files(config.log_dir(), options.log_file_names());

    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};
    db.prepare("node", "SELECT node_id, version, changeset_id FROM nodes WHERE "
                       "\"timestamp\" >= $1 ORDER BY node_id, version;");
    db.prepare("way", "SELECT way_id, version, changeset_id FROM ways WHERE "
                      "\"timestamp\" >= $1 ORDER BY way_id, version;");
    db.prepare("relation",
               "SELECT relation_id, version, changeset_id FROM relations WHERE "
               "\"timestamp\" >= $1 ORDER BY relation_id, version;");

    pqxx::read_transaction txn{db};
    vout << "Database version: " << get_db_version(txn) << '\n';

    vout << "Reading changes...\n";
    std::string data;

    auto count = read_objects(txn, data, options.timestamp(),
                              osmium::item_type::node, objects_done);
    count += read_objects(txn, data, options.timestamp(),
                          osmium::item_type::way, objects_done);
    count += read_objects(txn, data, options.timestamp(),
                          osmium::item_type::relation, objects_done);

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
