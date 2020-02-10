
#include "config.hpp"
#include "util.hpp"

#include <osmium/builder/osm_object_builder.hpp>
#include <osmium/io/any_output.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>

#include <pqxx/pqxx>

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

static Command command_create_diff = {
    "create-diff", "[OPTIONS] LOG-FILE",
    "Create replication diff files from log file."};

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
        pqxx::result r = txn.prepared("changeset_user")(c.first).exec();
        assert(r.size() == 1);
        c.second.id = r[0][1].as<osmium::user_id_type>();
        c.second.username = r[0][2].c_str();
        //        std::cout << "CS " << c.first << ' ' << c.second.id << ' ' << c.second.username << '\n';
    }
}

class osmobj
{

    osmium::item_type m_type;
    osmium::object_id_type m_id;
    osmium::object_version_type m_version;
    osmium::changeset_id_type m_cid;

public:
    explicit osmobj(char const *change_message)
    {
        assert(change_message[0] == 'N');
        assert(change_message[1] == ' ');

        m_type = osmium::char_to_item_type(change_message[2]);
        assert(m_type == osmium::item_type::node ||
               m_type == osmium::item_type::way ||
               m_type == osmium::item_type::relation);
        char *after_id;
        m_id = std::strtoll(change_message + 3, &after_id, 10);

        assert(after_id[0] == ' ');
        assert(after_id[1] == 'v');
        char *after_version;
        m_version = std::strtoll(after_id + 2, &after_version, 10);
        assert(after_version[0] == ' ');
        assert(after_version[1] == 'c');

        char *end;
        m_cid = std::strtoll(after_version + 2, &end, 10);
        assert(end[0] == '\0');

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
        pqxx::result r = txn.prepared("way_nodes")(m_id)(m_version).exec();

        for (auto const row : r) {
            wnbuilder.add_node_ref(row[0].as<osmium::object_id_type>());
        }
    }

    void add_members(pqxx::work &txn,
                     osmium::builder::RelationBuilder &builder) const
    {
        osmium::builder::RelationMemberListBuilder mbuilder{builder};
        pqxx::result r = txn.prepared("members")(m_id)(m_version).exec();

        for (auto const row : r) {
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
        pqxx::result r = txn.prepared(osmium::item_type_to_name(m_type) +
                                      std::string{"_tag"})(m_id)(m_version)
                             .exec();

        for (auto const row : r) {
            tbuilder.add_tag(row[0].c_str(), row[1].c_str());
        }
    }

    void get_data(pqxx::work &txn, osmium::memory::Buffer &buffer) const
    {
        //pqxx::result r = txn.exec_prepared(osmium::item_type_to_name(m_type), m_id, m_version);
        pqxx::result r =
            txn.prepared(osmium::item_type_to_name(m_type))(m_id)(m_version)
                .exec();
        assert(r.size() == 1);
        auto row = r[0];

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

}; // class osmobj

static std::vector<osmobj> read_log(std::string const &file_name)
{
    std::vector<osmobj> objects_todo;

    std::fstream logfile{file_name};

    // XXX quick hack, should be done more efficiently
    std::string type, obj, version, changeset;
    while (logfile) {
        logfile >> type;

        if (type == "N") {
            logfile >> obj >> version >> changeset;
            std::string x{type + " " + obj + " " + version + " " + changeset};
            objects_todo.emplace_back(x.c_str());
        }
    }

    return objects_todo;
}

static void write_osc(std::string const &file_name,
                      osmium::memory::Buffer &&buffer)
{
    osmium::io::File file{file_name};

    osmium::io::Header header;
    header.has_multiple_object_versions();
    header.set("generator", "osmdbt/0.1");

    osmium::io::Writer writer{file, header};
    writer(std::move(buffer));
    writer.close();
}

static void run(pqxx::work &txn, osmium::VerboseOutput &vout,
                Config const & /*config*/, std::string const &log_file_name)
{
    vout << "Reading log file '" << log_file_name << "'...\n";
    auto const objects_todo = read_log(log_file_name);

    vout << "Populating changeset cache...\n";
    populate_changeset_cache(txn);

    vout << "Processing objects...\n";
    // XXX the size should probably depend on the number of objects or so
    osmium::memory::Buffer buffer{1024 * 1024};
    for (auto const &obj : objects_todo) {
        obj.get_data(txn, buffer);
    }

    txn.commit();

    auto const osm_data_file_name = replace_suffix(log_file_name, ".osc.gz");

    vout << "Writing replication diff file '" << osm_data_file_name << "'...\n";
    write_osc(osm_data_file_name, std::move(buffer));
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: osmdbt-create-diff [OPTIONS] LOG-FILE-NAME\n";
        return 2;
    }
    std::string const log_file_name{argv[1]};
    try {
        auto const options =
            parse_command_line(argc, argv, command_create_diff);
        osmium::VerboseOutput vout{!options.quiet};

        vout << "Reading config from '" << options.config_file << "'\n";
        Config config{options, vout};

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

        db.prepare(
            "node_tag",
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
        run(txn, vout, config, log_file_name);
        txn.commit();

        osmium::MemoryUsage mem;
        vout << "Current memory used: " << mem.current() << " MBytes\n";
        vout << "Peak memory used: " << mem.peak() << " MBytes\n";

        vout << "Done.\n";
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    return 0;
}
