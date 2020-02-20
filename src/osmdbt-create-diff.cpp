
#include "config.hpp"
#include "db.hpp"
#include "exception.hpp"
#include "io.hpp"
#include "options.hpp"
#include "util.hpp"
#include "version.hpp"

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/io/xml_output.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/string.hpp>
#include <osmium/util/verbose_output.hpp>

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
                "Missing '--log-file FILE' or '-f FILE' on command line"};
        }
    }

    std::string m_log_file_name;
}; // class CreateDiffOptions

struct userinfo
{
    osmium::user_id_type id = 0;
    std::string username;
};

using changeset_user_lookup =
    std::unordered_map<osmium::changeset_id_type, userinfo>;

changeset_user_lookup cucache;

void populate_changeset_cache(pqxx::work &txn)
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

class osmobj
{
public:
    explicit osmobj(std::string const &obj, std::string const &version,
                    std::string const &changeset)
    {
        if (obj.size() < 2 || version.size() < 2 || changeset.size() < 2) {
            throw std::runtime_error{
                "Log file has wrong format: entry too short"};
        }

        m_type = osmium::char_to_item_type(obj[0]);
        if (m_type != osmium::item_type::node &&
            m_type != osmium::item_type::way &&
            m_type != osmium::item_type::relation) {
            throw std::runtime_error{
                "Log file has wrong format: type must be 'n', 'w', or 'r'"};
        }
        m_id = std::strtoll(&obj[1], nullptr, 10);

        if (version[0] != 'v') {
            throw std::runtime_error{
                "Log file has wrong format: expected version"};
        }
        m_version = std::strtoll(&version[1], nullptr, 10);

        if (changeset[0] != 'c') {
            throw std::runtime_error{
                "Log file has wrong format: expected changeset"};
        }
        m_cid = std::strtoll(&changeset[1], nullptr, 10);

        cucache[m_cid] = {};
    }

    template <typename TBuilder>
    void set_attributes(TBuilder &builder, osmium::changeset_id_type const cid,
                        bool const visible, osmium::user_id_type const uid,
                        char const *const timestamp) const
    {
        builder.set_id(m_id)
            .set_version(m_version)
            .set_changeset(cid)
            .set_visible(visible)
            .set_uid(uid)
            .set_timestamp(timestamp);
    }

    void add_nodes(pqxx::work &txn, osmium::builder::WayBuilder &builder) const
    {
        osmium::builder::WayNodeListBuilder wnbuilder{builder};
        pqxx::result const result =
            txn.prepared("way_nodes")(m_id)(m_version).exec();

        for (auto const &row : result) {
            wnbuilder.add_node_ref(row[0].as<osmium::object_id_type>());
        }
    }

    void add_members(pqxx::work &txn,
                     osmium::builder::RelationBuilder &builder) const
    {
        osmium::builder::RelationMemberListBuilder mbuilder{builder};
        pqxx::result const result =
            txn.prepared("members")(m_id)(m_version).exec();

        for (auto const &row : result) {
            osmium::item_type type;
            switch (*row[0].c_str()) {
            case 'N':
                type = osmium::item_type::node;
                break;
            case 'W':
                type = osmium::item_type::way;
                break;
            case 'R':
                type = osmium::item_type::relation;
                break;
            default:
                assert(false);
            }
            mbuilder.add_member(type, row[1].as<osmium::object_id_type>(),
                                row[2].c_str());
        }
    }

    template <typename TBuilder>
    void add_tags(pqxx::work &txn, TBuilder &builder) const
    {
        osmium::builder::TagListBuilder tbuilder{builder};
        pqxx::result const result =
            txn.prepared(osmium::item_type_to_name(m_type) +
                         std::string{"_tag"})(m_id)(m_version)
                .exec();

        for (auto const &row : result) {
            tbuilder.add_tag(row[0].c_str(), row[1].c_str());
        }
    }

    void get_data(pqxx::work &txn, osmium::memory::Buffer &buffer) const
    {
        pqxx::result const result =
            txn.prepared(osmium::item_type_to_name(m_type))(m_id)(m_version)
                .exec();

        assert(result.size() == 1);
        if (result.size() != 1) {
            throw database_error{"Expected exactly one result (get_data)."};
        }
        auto const &row = result[0];

        auto const cid = row["changeset_id"].as<osmium::changeset_id_type>();
        bool const visible = row["visible"].c_str()[0] == 't';
        auto const &user = cucache[cid];
        char const *const timestamp = row["timestamp"].c_str();

        switch (m_type) {
        case osmium::item_type::node: {
            osmium::builder::NodeBuilder builder{buffer};
            set_attributes(builder, cid, visible, user.id, timestamp);
            osmium::Location loc{row["longitude"].as<int64_t>(),
                                 row["latitude"].as<int64_t>()};
            builder.set_location(loc).set_user(user.username);
            add_tags(txn, builder);
        } break;
        case osmium::item_type::way: {
            osmium::builder::WayBuilder builder{buffer};
            set_attributes(builder, cid, visible, user.id, timestamp);
            builder.set_user(user.username);
            add_nodes(txn, builder);
            add_tags(txn, builder);
        } break;
        case osmium::item_type::relation: {
            osmium::builder::RelationBuilder builder{buffer};
            set_attributes(builder, cid, visible, user.id, timestamp);
            builder.set_user(user.username);
            add_members(txn, builder);
            add_tags(txn, builder);
        } break;
        default:
            assert(false);
        }

        buffer.commit();
    }

private:
    osmium::item_type m_type;
    osmium::object_id_type m_id;
    osmium::object_version_type m_version;
    osmium::changeset_id_type m_cid;

}; // class osmobj

static std::vector<osmobj> read_log(std::string const &dir_name,
                                    std::string const &file_name)
{
    std::vector<osmobj> objects_todo;

    std::ifstream logfile{dir_name + "/" + file_name};
    if (!logfile.is_open()) {
        throw std::system_error{errno, std::system_category(),
                                "Could not open log file '" + file_name + "'"};
    }

    for (std::string line; std::getline(logfile, line);) {
        auto const parts = osmium::split_string(line, ' ');
        if (parts.size() < 3) {
            std::cerr << "Warning: Ignored log line due to wrong formatting: "
                      << line << '\n';
            continue;
        }

        if (parts[2] == "N") {
            if (parts.size() != 6) {
                std::cerr
                    << "Warning: Ignored log line due to wrong formatting: "
                    << line << '\n';
                continue;
            }
            objects_todo.emplace_back(parts[3], parts[4], parts[5]);
        } else if (parts[2] == "X") {
            std::cerr << "Error found in logfile: " << line << '\n';
        }
    }

    return objects_todo;
}

bool app(osmium::VerboseOutput &vout, Config const &config,
         CreateDiffOptions const &options)
{
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
        read_log(config.log_dir(), options.log_file_name());
    vout << "  Got " << objects_todo.size() << " objects.\n";

    vout << "Populating changeset cache...\n";
    populate_changeset_cache(txn);
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
        obj.get_data(txn, buffer);
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
