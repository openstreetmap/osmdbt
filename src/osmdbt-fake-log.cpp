
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
#include <tuple>
#include <utility>

using id_version_type =
    std::pair<osmium::object_id_type, osmium::object_version_type>;

class FakeLogOptions : public Options
{
public:
    FakeLogOptions()
    : Options("fake-log", "Create fake log file from recent changes.")
    {}

    [[nodiscard]] std::vector<std::string> const &
    log_file_names() const noexcept
    {
        return m_log_file_names;
    }

    [[nodiscard]] osmium::Timestamp timestamp() const noexcept
    {
        return m_timestamp;
    }

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

    void check_command_options(po::variables_map const &vm) override
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

class log_entry
{
    std::time_t m_time;
    osmium::object_id_type m_id;
    osmium::object_version_type m_version;
    osmium::changeset_id_type m_cid;
    osmium::item_type m_type;

public:
    log_entry(std::time_t time, osmium::item_type type,
              osmium::object_id_type id, osmium::object_version_type version,
              osmium::changeset_id_type cid)
    : m_time(time), m_id(id), m_version(version), m_cid(cid), m_type(type)
    {}

    [[nodiscard]] std::time_t time() const noexcept { return m_time; }

    [[nodiscard]] osmium::Timestamp timestamp() const noexcept
    {
        return {m_time};
    }

    void append_to(std::string &data) const
    {
        data += "0/0 0 N ";
        data += osmium::item_type_to_char(m_type);
        data += std::to_string(m_id);
        data += " v";
        data += std::to_string(m_version);
        data += " c";
        data += std::to_string(m_cid);
        data += '\n';
    }

    using entry_tuple =
        std::tuple<std::time_t, osmium::item_type, osmium::object_id_type,
                   osmium::object_version_type>;

    friend bool operator<(log_entry const &a, log_entry const &b) noexcept
    {
        return entry_tuple{a.m_time, a.m_type, a.m_id, a.m_version} <
               entry_tuple{b.m_time, b.m_type, b.m_id, b.m_version};
    }
};

static void read_objects(pqxx::dbtransaction &txn,
                         std::vector<log_entry> &entries,
                         osmium::Timestamp timestamp, osmium::item_type type,
                         std::set<id_version_type> const &objects_done)
{
    pqxx::result const result =
        txn.exec_prepared(osmium::item_type_to_name(type), timestamp.to_iso());

    entries.reserve(entries.size() + result.size());

    for (auto const &row : result) {
        auto const id = row[1].as<osmium::object_id_type>();
        auto const version = row[2].as<osmium::object_version_type>();
        if (!objects_done.count(std::make_pair(id, version))) {
            entries.emplace_back(row[0].as<std::time_t>(), type, id, version,
                                 row[3].as<osmium::changeset_id_type>());
        }
    }
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
    vout << "Start from timestamp " << options.timestamp() << '\n';

    // All commands writing log files and/or advancing the replication slot
    // use the same pid/lock file.
    PIDFile pid_file{config.run_dir(), "osmdbt-log"};

    auto const objects_done =
        read_log_files(config.log_dir(), options.log_file_names());

    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};
    db.prepare(
        "node",
        "SELECT EXTRACT(EPOCH FROM date_trunc('minute', \"timestamp\"))::int AS ts,"
        "       node_id, version, changeset_id"
        "  FROM nodes WHERE \"timestamp\" >= $1"
        "    ORDER BY \"timestamp\", node_id, version;");
    db.prepare(
        "way",
        "SELECT EXTRACT(EPOCH FROM date_trunc('minute', \"timestamp\"))::int AS ts,"
        "       way_id, version, changeset_id"
        "  FROM ways WHERE \"timestamp\" >= $1"
        "    ORDER BY \"timestamp\", way_id, version;");
    db.prepare(
        "relation",
        "SELECT EXTRACT(EPOCH FROM date_trunc('minute', \"timestamp\"))::int AS ts,"
        "       relation_id, version, changeset_id"
        "  FROM relations WHERE \"timestamp\" >= $1"
        "    ORDER BY \"timestamp\", relation_id, version;");

    pqxx::read_transaction txn{db};
    vout << "Database version: " << get_db_version(txn) << '\n';

    vout << "Reading changes...\n";

    std::vector<log_entry> entries;
    read_objects(txn, entries, options.timestamp(), osmium::item_type::node,
                 objects_done(osmium::item_type::node));
    read_objects(txn, entries, options.timestamp(), osmium::item_type::way,
                 objects_done(osmium::item_type::way));
    read_objects(txn, entries, options.timestamp(), osmium::item_type::relation,
                 objects_done(osmium::item_type::relation));

    txn.commit();

    if (entries.empty()) {
        vout << "No actual changes found.\n";
        vout << "Did not write log file.\n";
        vout << "Done.\n";
        return false;
    }

    vout << "There are " << entries.size() << " changes.\n";

    std::sort(entries.begin(), entries.end());

    std::time_t last = 0;
    std::string file_name;
    std::string data;
    for (auto const &entry : entries) {
        if (last != entry.time()) {
            if (last != 0) {
                write_data_to_file(data, config.log_dir(), file_name);
                data.clear();
            }
            file_name = create_replication_log_name(entry.timestamp().to_iso());
            last = entry.time();
            vout << "Writing log to '" << config.log_dir() << file_name
                 << "'...\n";
        }
        entry.append_to(data);
    }
    write_data_to_file(data, config.log_dir(), file_name);

    vout << "Wrote and synced logs.\n";
    vout << "Done.\n";

    return true;
}

int main(int argc, char *argv[])
{
    FakeLogOptions options;
    return app_wrapper(options, argc, argv);
}
