
#include "config.hpp"
#include "db.hpp"
#include "exception.hpp"
#include "io.hpp"
#include "options.hpp"
#include "osmobj.hpp"
#include "util.hpp"
#include "version.hpp"

#include <osmium/io/gzip_compression.hpp>
#include <osmium/io/xml_output.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/string.hpp>
#include <osmium/util/verbose_output.hpp>

#include <cstddef>
#include <string>
#include <utility>

class CreateDiffOptions : public Options
{
public:
    CreateDiffOptions()
    : Options("create-diff", "Create replication diff files from log file.")
    {}

    std::string const &log_file_name() const noexcept
    {
        return m_log_file_name;
    }

private:
    void add_command_options(po::options_description &desc) override
    {
        po::options_description opts_cmd{"COMMAND OPTIONS"};

        // clang-format off
        opts_cmd.add_options()
            ("log-file,f", po::value<std::string>(), "Log file name (required)");
        // clang-format on

        desc.add(opts_cmd);
    }

    void check_command_options(
        boost::program_options::variables_map const &vm) override
    {
        if (vm.count("log-file")) {
            m_log_file_name = vm["log-file"].as<std::string>();
        } else {
            throw argument_error{
                "Missing '--log-file=FILE' or '-f FILE' on command line"};
        }
    }

    std::string m_log_file_name;
}; // class CreateDiffOptions

static void populate_changeset_cache(pqxx::work &txn,
                                     changeset_user_lookup &cucache)
{
    for (auto &c : cucache) {
        pqxx::result const result =
            txn.prepared("changeset_user")(c.first).exec();
        if (result.size() != 1) {
            throw database_error{
                "Expected exactly one result (changeset_user)."};
        }
        c.second.id = result[0][1].as<osmium::user_id_type>();
        c.second.username = result[0][2].c_str();
    }
}

bool app(osmium::VerboseOutput &vout, Config const &config,
         CreateDiffOptions const &options)
{
    changeset_user_lookup cucache;
    PIDFile pid_file{config.run_dir(), "osmdbt-create-diff"};

    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};

    db.prepare("changeset_user",
               "SELECT c.id, c.user_id, u.display_name FROM changesets c, "
               "users u WHERE c.user_id = u.id AND c.id = $1");

    db.prepare(
        "node",
        R"(SELECT node_id, version, changeset_id, visible, to_char(timestamp, 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS timestamp, longitude, latitude FROM nodes WHERE node_id=$1 AND version=$2)");
    db.prepare(
        "way",
        R"(SELECT way_id, version, changeset_id, visible, to_char(timestamp, 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS timestamp FROM ways WHERE way_id=$1 AND version=$2)");
    db.prepare(
        "relation",
        R"(SELECT relation_id, version, changeset_id, visible, to_char(timestamp, 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS timestamp FROM relations WHERE relation_id=$1 AND version=$2)");

    db.prepare("node_tag",
               "SELECT k, v FROM node_tags WHERE node_id=$1 AND version=$2");
    db.prepare("way_tag",
               "SELECT k, v FROM way_tags WHERE way_id=$1 AND version=$2");
    db.prepare("relation_tag", "SELECT k, v FROM relation_tags WHERE "
                               "relation_id=$1 AND version=$2");

    db.prepare("way_nodes", "SELECT node_id FROM way_nodes WHERE way_id=$1 "
                            "AND version=$2 ORDER BY sequence_id");
    db.prepare(
        "members",
        "SELECT member_type, member_id, member_role FROM relation_members "
        "WHERE relation_id=$1 AND version=$2 ORDER BY sequence_id");

    pqxx::work txn{db};
    vout << "Database version: " << get_db_version(txn) << '\n';

    vout << "Reading log file '" << options.log_file_name() << "'...\n";
    auto const objects_todo =
        read_log(config.log_dir(), options.log_file_name(), &cucache);
    vout << "  Got " << objects_todo.size() << " objects.\n";

    vout << "Populating changeset cache...\n";
    populate_changeset_cache(txn, cucache);
    vout << "  Got " << cucache.size() << " changesets.\n";

    auto const osm_data_file_name = replace_suffix(
        config.changes_dir() + "/" + options.log_file_name(), ".osc.gz");
    vout << "Opening output file '" << osm_data_file_name << ".new'...\n";
    osmium::io::File file{osm_data_file_name + ".new", "osc.gz"};

    osmium::io::Header header;
    header.has_multiple_object_versions();
    header.set("generator",
               std::string{"osmdbt-create-diff/"} + get_osmdbt_version());

    osmium::io::Writer writer{file, header, osmium::io::overwrite::no,
                              osmium::io::fsync::yes};

    vout << "Processing " << objects_todo.size() << " objects...\n";
    std::size_t const buffer_size = 1024 * 1024;
    osmium::memory::Buffer buffer{buffer_size};
    std::size_t count = 0;
    for (auto const &obj : objects_todo) {
        obj.get_data(txn, buffer, cucache);
        ++count;
        if (buffer.committed() > buffer_size - 1024) {
            vout << "  " << count << " done\n";
            writer(std::move(buffer));
            buffer = osmium::memory::Buffer{buffer_size};
        }
    }

    if (buffer.committed() > 0) {
        writer(std::move(buffer));
        vout << "  " << count << " done\n";
    }

    txn.commit();
    writer.close();

    rename_file(osm_data_file_name + ".new", osm_data_file_name);
    sync_dir(dirname(osm_data_file_name));
    vout << "Wrote and synced output file.\n";

    vout << "All done.\n";
    txn.commit();

    osmium::MemoryUsage mem;
    vout << "Current memory used: " << mem.current() << " MBytes\n";
    vout << "Peak memory used: " << mem.peak() << " MBytes\n";

    vout << "Done.\n";

    return true;
}

int main(int argc, char *argv[])
{
    CreateDiffOptions options;
    return app_wrapper(options, argc, argv);
}
