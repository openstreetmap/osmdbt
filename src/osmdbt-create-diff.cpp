
#include "config.hpp"
#include "db.hpp"
#include "io.hpp"
#include "options.hpp"
#include "osmobj.hpp"
#include "state.hpp"
#include "util.hpp"
#include "version.hpp"

#include <osmium/io/gzip_compression.hpp>
#include <osmium/io/pbf_output.hpp>
#include <osmium/io/xml_output.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/string.hpp>
#include <osmium/util/verbose_output.hpp>

#include <cstddef>
#include <filesystem>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

class CreateDiffOptions : public Options
{
public:
    CreateDiffOptions()
    : Options("create-diff", "Create replication diff files from log file.")
    {}

    [[nodiscard]] std::vector<std::string> const &
    log_file_names() const noexcept
    {
        return m_log_file_names;
    }

    [[nodiscard]] std::size_t init_state() const noexcept
    {
        return m_init_state;
    }

    [[nodiscard]] uint32_t max_changes() const noexcept
    {
        return m_max_changes;
    }

    [[nodiscard]] bool with_comment() const noexcept { return m_with_comment; }

    [[nodiscard]] bool dry_run() const noexcept { return m_dry_run; }

    [[nodiscard]] bool with_pbf_output() const noexcept
    {
        return m_with_pbf_output;
    }

private:
    void add_command_options(po::options_description &desc) override
    {
        po::options_description opts_cmd{"COMMAND OPTIONS"};

        // clang-format off
        opts_cmd.add_options()
            ("with-comment", "Add comment to state file with current date")
            ("log-file,f", po::value<std::vector<std::string>>(), "Read specified log file")
            ("max-changes,m", po::value<uint32_t>(), "Maximum number of changes (default: no limit)")
            ("dry-run,n", "Dry-run, only create files in tmp dir")
            ("sequence-number,s", po::value<std::size_t>(), "Initialize state with specified value")
            ("with-pbf-output,p", "Also generate change files in PBF format");
        // clang-format on

        desc.add(opts_cmd);
    }

    void check_command_options(po::variables_map const &vm) override
    {
        if (vm.count("log-file")) {
            m_log_file_names = vm["log-file"].as<std::vector<std::string>>();
        }
        if (vm.count("max-changes")) {
            m_max_changes = vm["max-changes"].as<uint32_t>();
        }
        if (vm.count("with-comment")) {
            m_with_comment = true;
        }
        if (vm.count("dry-run")) {
            m_dry_run = true;
        }
        if (vm.count("sequence-number")) {
            m_init_state = vm["sequence-number"].as<std::size_t>();
        }
        if (vm.count("with-pbf-output")) {
            m_with_pbf_output = true;
        }
    }

    std::vector<std::string> m_log_file_names;
    std::size_t m_init_state = 0;
    std::uint32_t m_max_changes = std::numeric_limits<uint32_t>::max();
    bool m_with_comment = false;
    bool m_dry_run = false;
    bool m_with_pbf_output = false;

}; // class CreateDiffOptions

void populate_changeset_cache(pqxx::dbtransaction &txn,
                              changeset_user_lookup &cucache)
{
    assert(!cucache.empty());

    std::string query{
        "SELECT c.id, c.user_id, u.display_name FROM changesets c, users u"
        "  WHERE c.user_id = u.id AND c.id IN ("};

    for (auto const &c : cucache) {
        query += std::to_string(c.first);
        query += ",";
    }

    query.back() = ')';

    pqxx::result const result = txn.exec(query);
    for (auto const &row : result) {
        auto const cid = row[0].as<osmium::changeset_id_type>();
        auto const uid = row[1].as<osmium::user_id_type>();
        auto const *const username = row[2].c_str();
        auto &ui = cucache[cid];
        ui.id = uid;
        ui.username = username;
    }
}

State get_state(Config const &config, CreateDiffOptions const &options,
                osmium::Timestamp timestamp)
{
    if (options.init_state() != 0) {
        return State{options.init_state(), timestamp};
    }

    std::filesystem::path const state_file{config.changes_dir() + "state.txt"};
    if (!std::filesystem::exists(state_file)) {
        throw std::runtime_error{"Missing state file: '" + state_file.string() +
                                 "'"};
    }
    State const state{state_file.string()};
    return state.next(timestamp);
}

void write_lock_file(std::string const &path, State const &state,
                     std::vector<std::string> const &log_files)
{
    int const fd = excl_write_open(path);

    if (fd < 0) {
        if (errno == EEXIST) {
            throw std::runtime_error{"Lock file '" + path +
                                     "' exists. Need sysadmin cleanup."};
        }
        throw std::runtime_error{"Can not create lock file '" + path + "'."};
    }

    std::string output{
        "# If this file is left around osmdbt-create-diff crashed in a "
        "criticial section.\n# Check log, diff, and state files and clean "
        "up.\n"};
    output += "osmdbt-create-diff-pid=";
    output += std::to_string(::getpid());
    output += "\nnew-state=";
    output += std::to_string(state.sequence_number());
    output += "\nlog-files:\n";

    for (auto const &file : log_files) {
        output += file;
        output += '\n';
    }

    osmium::io::detail::reliable_write(fd, output.data(), output.size());
    osmium::io::detail::reliable_close(fd);
}

std::string wanted(std::vector<osmobj> const &objs)
{
    assert(!objs.empty());

    std::string sql{"WITH wanted(id, version) AS (VALUES "};

    for (auto const &obj : objs) {
        sql += '(';
        sql += std::to_string(obj.id());
        sql += ',';
        sql += std::to_string(obj.version());
        sql += "),";
    }
    sql.back() = ')';
    sql += ' ';

    return sql;
}

struct tag
{
    std::string key;
    std::string value;
    osmium::object_id_type id;
    osmium::object_version_type version;

    tag(osmium::object_id_type id_, osmium::object_version_type version_,
        char const *key_, char const *value_)
    : key(key_), value(value_), id(id_), version(version_)
    {}
};

std::vector<tag> get_tags(pqxx::dbtransaction &txn, char const *type,
                          std::string const &wanted)
{
    std::string query = wanted;

    query += "SELECT w.id, w.version, t.k, t.v FROM ";
    query += type;
    query += "_tags t INNER JOIN wanted w ON t.";
    query += type;
    query += "_id = w.id AND t.version = w.version"
             "  ORDER BY w.id, w.version, t.k COLLATE \"C\"";

    std::vector<tag> tags;

    pqxx::result const result = txn.exec(query);
    for (auto const &row : result) {
        tags.emplace_back(row[0].as<osmium::object_id_type>(),
                          row[1].as<osmium::object_version_type>(),
                          row[2].c_str(), row[3].c_str());
    }

    return tags;
}

struct way_node
{
    osmium::object_id_type way_id;
    osmium::object_id_type node_ref;
    osmium::object_version_type version;

    way_node(osmium::object_id_type wid, osmium::object_version_type v,
             osmium::object_id_type nref)
    : way_id(wid), node_ref(nref), version(v)
    {}
};

std::vector<way_node> get_nodes(pqxx::dbtransaction &txn,
                                std::string const &wanted)
{
    std::string query = wanted;

    query +=
        "SELECT wn.way_id, wn.version, wn.node_id FROM way_nodes wn"
        "  INNER JOIN wanted w ON wn.way_id = w.id AND wn.version = w.version"
        "  ORDER BY wn.way_id, wn.version, wn.sequence_id";

    std::vector<way_node> way_nodes;

    pqxx::result const result = txn.exec(query);
    for (auto const &row : result) {
        way_nodes.emplace_back(row[0].as<osmium::object_id_type>(),
                               row[1].as<osmium::object_version_type>(),
                               row[2].as<osmium::object_id_type>());
    }

    return way_nodes;
}

struct member
{
    std::string mrole;
    osmium::object_id_type relation_id;
    osmium::object_id_type mref;
    osmium::object_version_type version;
    osmium::item_type mtype;

    member(osmium::object_id_type rid, osmium::object_version_type v,
           osmium::item_type type, osmium::object_id_type ref, char const *role)
    : mrole(role), relation_id(rid), mref(ref), version(v), mtype(type)
    {}
};

osmium::item_type type_from_char(char const *str) noexcept
{
    assert(str);

    switch (*str) { // NOLINT(bugprone-switch-missing-default-case) default is after switch
    case 'N':
        return osmium::item_type::node;
    case 'W':
        return osmium::item_type::way;
    case 'R':
        return osmium::item_type::relation;
    }

    assert(false);
    return osmium::item_type::undefined;
}

std::vector<member> get_members(pqxx::dbtransaction &txn,
                                std::string const &wanted)
{
    std::string query = wanted;

    query += "SELECT m.relation_id, m.version, m.member_type,"
             "    m.member_id, m.member_role FROM relation_members m"
             "  INNER JOIN wanted w"
             "    ON m.relation_id = w.id AND m.version = w.version"
             "  ORDER BY m.relation_id, m.version, m.sequence_id";

    std::vector<member> members;

    pqxx::result const result = txn.exec(query);
    for (auto const &row : result) {
        members.emplace_back(row["relation_id"].as<osmium::object_id_type>(),
                             row["version"].as<osmium::object_version_type>(),
                             type_from_char(row["member_type"].c_str()),
                             row["member_id"].as<osmium::object_id_type>(),
                             row["member_role"].c_str());
    }

    return members;
}

using tags_iterator = std::vector<tag>::const_iterator;
using way_nodes_iterator = std::vector<way_node>::const_iterator;
using members_iterator = std::vector<member>::const_iterator;

tags_iterator add_tags(tags_iterator it, tags_iterator end,
                       osmium::object_id_type id,
                       osmium::object_version_type version,
                       osmium::builder::Builder &builder)
{
    if (it == end || it->id != id || it->version != version) {
        return it;
    }

    osmium::builder::TagListBuilder tbuilder{builder};
    do {
        tbuilder.add_tag(it->key, it->value);
        ++it;
    } while (it != end && it->id == id && it->version == version);

    return it;
}

way_nodes_iterator add_way_nodes(way_nodes_iterator it, way_nodes_iterator end,
                                 osmium::object_id_type id,
                                 osmium::object_version_type version,
                                 osmium::builder::Builder &builder)
{
    if (it == end || it->way_id != id || it->version != version) {
        return it;
    }

    osmium::builder::WayNodeListBuilder wnbuilder{builder};
    do {
        wnbuilder.add_node_ref(it->node_ref);
        ++it;
    } while (it != end && it->way_id == id && it->version == version);

    return it;
}

members_iterator add_members(members_iterator it, members_iterator end,
                             osmium::object_id_type id,
                             osmium::object_version_type version,
                             osmium::builder::Builder &builder)
{
    if (it == end || it->relation_id != id || it->version != version) {
        return it;
    }

    osmium::builder::RelationMemberListBuilder mbuilder{builder};
    do {
        mbuilder.add_member(it->mtype, it->mref, it->mrole);
        ++it;
    } while (it != end && it->relation_id == id && it->version == version);

    return it;
}

char const attr[] =
    R"(, o.version, o.changeset_id, o.visible, to_char(o.timestamp, 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS timestamp, o.redaction_id)";

constexpr std::size_t const buffer_size = 1024UL * 1024UL;

template <typename TBuilder>
void set_attributes(TBuilder &builder, changeset_user_lookup const &cucache,
                    osmium::object_id_type id,
                    osmium::object_version_type version,
                    osmium::Timestamp timestamp,
                    pqxx::result::const_iterator const &row)
{
    auto const cid = row["changeset_id"].as<osmium::changeset_id_type>();
    bool const visible = row["visible"].c_str()[0] == 't';
    auto const &user = cucache.at(cid);

    builder.set_id(id)
        .set_version(version)
        .set_changeset(cid)
        .set_visible(visible)
        .set_uid(user.id)
        .set_timestamp(timestamp)
        .set_user(user.username);
}

osmium::memory::Buffer process_nodes(pqxx::dbtransaction &txn,
                                     changeset_user_lookup const &cucache,
                                     std::vector<osmobj> const &objs,
                                     osmium::Timestamp *max_timestamp)
{
    std::string query = wanted(objs);

    auto const tags = get_tags(txn, "node", query);

    query += "SELECT o.node_id";
    query += attr;
    query += ", o.longitude, o.latitude"
             "  FROM nodes o"
             "    INNER JOIN wanted w"
             "      ON o.node_id = w.id AND o.version = w.version"
             "  ORDER BY w.id, w.version";

    pqxx::result const result = txn.exec(query);

    osmium::memory::Buffer buffer{buffer_size};

    auto it = tags.begin();
    for (auto const &row : result) {
        auto const id = row["node_id"].as<osmium::object_id_type>();
        auto const version = row["version"].as<osmium::object_version_type>();
        auto const timestamp = osmium::Timestamp{row["timestamp"].c_str()};

        if (timestamp > *max_timestamp) {
            *max_timestamp = timestamp;
        }

        if (!row["redaction_id"].is_null()) {
            std::cerr << "Ignored redacted node " << id << " version "
                      << version
                      << " (redaction_id=" << row["redaction_id"].c_str()
                      << ")\n";
            continue;
        }

        osmium::Location const loc{row["longitude"].as<int64_t>(),
                                   row["latitude"].as<int64_t>()};

        {
            osmium::builder::NodeBuilder builder{buffer};
            builder.set_location(loc);
            set_attributes(builder, cucache, id, version, timestamp, row);
            it = add_tags(it, tags.end(), id, version, builder);
        }
        buffer.commit();
    }

    return buffer;
}

osmium::memory::Buffer process_ways(pqxx::dbtransaction &txn,
                                    changeset_user_lookup const &cucache,
                                    std::vector<osmobj> const &objs,
                                    osmium::Timestamp *max_timestamp)
{
    std::string query = wanted(objs);

    auto const tags = get_tags(txn, "way", query);
    auto const way_nodes = get_nodes(txn, query);

    query += "SELECT o.way_id";
    query += attr;
    query += "  FROM ways o"
             "    INNER JOIN wanted w"
             "      ON o.way_id = w.id AND o.version = w.version"
             "  ORDER BY w.id, w.version";

    pqxx::result const result = txn.exec(query);

    osmium::memory::Buffer buffer{buffer_size};

    auto it = tags.begin();
    auto wn_it = way_nodes.begin();
    for (auto const &row : result) {
        auto const id = row["way_id"].as<osmium::object_id_type>();
        auto const version = row["version"].as<osmium::object_version_type>();
        auto const timestamp = osmium::Timestamp{row["timestamp"].c_str()};

        if (timestamp > *max_timestamp) {
            *max_timestamp = timestamp;
        }

        if (!row["redaction_id"].is_null()) {
            std::cerr << "Ignored redacted way " << id << " version " << version
                      << " (redaction_id=" << row["redaction_id"].c_str()
                      << ")\n";
            continue;
        }

        {
            osmium::builder::WayBuilder builder{buffer};
            set_attributes(builder, cucache, id, version, timestamp, row);
            it = add_tags(it, tags.end(), id, version, builder);
            wn_it = add_way_nodes(wn_it, way_nodes.end(), id, version, builder);
        }
        buffer.commit();
    }

    return buffer;
}

osmium::memory::Buffer process_relations(pqxx::dbtransaction &txn,
                                         changeset_user_lookup const &cucache,
                                         std::vector<osmobj> const &objs,
                                         osmium::Timestamp *max_timestamp)
{
    std::string query = wanted(objs);

    auto const tags = get_tags(txn, "relation", query);
    auto const members = get_members(txn, query);

    query += "SELECT o.relation_id";
    query += attr;
    query += "  FROM relations o"
             "    INNER JOIN wanted w"
             "      ON o.relation_id = w.id AND o.version = w.version"
             "  ORDER BY w.id, w.version";

    pqxx::result const result = txn.exec(query);

    osmium::memory::Buffer buffer{buffer_size};

    auto it = tags.begin();
    auto member_it = members.begin();
    for (auto const &row : result) {
        auto const id = row["relation_id"].as<osmium::object_id_type>();
        auto const version = row["version"].as<osmium::object_version_type>();
        auto const timestamp = osmium::Timestamp{row["timestamp"].c_str()};

        if (timestamp > *max_timestamp) {
            *max_timestamp = timestamp;
        }

        if (!row["redaction_id"].is_null()) {
            std::cerr << "Ignored redacted relation " << id << " version "
                      << version
                      << " (redaction_id=" << row["redaction_id"].c_str()
                      << ")\n";
            continue;
        }

        {
            osmium::builder::RelationBuilder builder{buffer};
            set_attributes(builder, cucache, id, version, timestamp, row);
            it = add_tags(it, tags.end(), id, version, builder);
            member_it =
                add_members(member_it, members.end(), id, version, builder);
        }
        buffer.commit();
    }

    return buffer;
}

void write_to(osmium::memory::Buffer &buffer, osmium::io::Writer &w1,
              osmium::io::Writer &w2)
{
    osmium::memory::Buffer buffer_copy{buffer.committed()};
    buffer_copy.add_buffer(buffer);
    buffer_copy.commit();

    w1(std::move(buffer_copy));
    w2(std::move(buffer));
}

bool app(osmium::VerboseOutput &vout, Config const &config,
         CreateDiffOptions const &options)
{
    changeset_user_lookup cucache;
    PIDFile const pid_file{config.run_dir(), "osmdbt-create-diff"};

    std::vector<std::string> log_files = options.log_file_names();
    if (log_files.empty()) {
        vout << "No log files on command line. Looking for log files in log "
                "directory...\n";
        std::filesystem::path const p{config.log_dir()};
        for (auto const &file : std::filesystem::directory_iterator(p)) {
            if (file.path().extension() == ".log") {
                log_files.push_back(file.path().filename().string());
            }
        }
    }

    if (log_files.empty()) {
        vout << "No log files found.\n";
        vout << "Done.\n";
        return true;
    }

    vout << log_files.size() << " log files to read.\n";

    // Read log files in order
    std::sort(log_files.begin(), log_files.end());

    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};

    pqxx::read_transaction txn{db};
    vout << "Database version: " << get_db_version(txn) << '\n';

    osmobjects objects_todo;

    std::vector<std::string> read_log_files;
    for (auto const &log_file : log_files) {
        vout << "Reading log file '" << config.log_dir() << log_file
             << "'...\n";
        read_log(objects_todo, config.log_dir(), log_file, &cucache);
        vout << "  Got " << objects_todo.nodes().size() << " nodes, "
             << objects_todo.ways().size() << " ways, "
             << objects_todo.relations().size() << " relations.\n";
        read_log_files.push_back(log_file);
        if (objects_todo.size() > options.max_changes()) {
            vout << "  Reached limit of " << options.max_changes()
                 << " objects.\n";
            break;
        }
    }

    if (objects_todo.empty()) {
        vout << "No objects found in log files.\n";
        vout << "Done.\n";
        return true;
    }

    objects_todo.sort();

    vout << "Populating changeset cache...\n";
    populate_changeset_cache(txn, cucache);
    vout << "  Got " << cucache.size() << " changesets.\n";

    auto const new_change_file_name = config.tmp_dir() + "new-change.osc";
    vout << "Opening output file '" << new_change_file_name << ".gz'...\n";
    vout << "Opening output file '" << new_change_file_name << ".pbf'...\n";

    osmium::io::Header header;
    header.set_has_multiple_object_versions(true);
    header.set("generator", "osmdbt-create-diff/" + get_osmdbt_version());

    osmium::io::Writer writer_xml{new_change_file_name + ".gz", header,
                                  osmium::io::overwrite::allow,
                                  osmium::io::fsync::yes};

    osmium::io::Writer writer_pbf{new_change_file_name + ".pbf", header,
                                  osmium::io::overwrite::allow,
                                  osmium::io::fsync::yes};

    vout << "Processing " << objects_todo.nodes().size() << " nodes, "
         << objects_todo.ways().size() << " ways, "
         << objects_todo.relations().size() << " relations...\n";

    // In this variable we'll remember the last OSM object timestamp that
    // we have seen. This will later end up in the state file.
    osmium::Timestamp max_timestamp{};

    if (!objects_todo.nodes().empty()) {
        auto buffer =
            process_nodes(txn, cucache, objects_todo.nodes(), &max_timestamp);

        write_to(buffer, writer_pbf, writer_xml);
    }
    if (!objects_todo.ways().empty()) {
        auto buffer =
            process_ways(txn, cucache, objects_todo.ways(), &max_timestamp);

        write_to(buffer, writer_pbf, writer_xml);
    }
    if (!objects_todo.relations().empty()) {
        auto buffer = process_relations(txn, cucache, objects_todo.relations(),
                                        &max_timestamp);

        write_to(buffer, writer_pbf, writer_xml);
    }

    txn.commit();
    writer_xml.close();
    writer_pbf.close();

    vout << "Wrote and synced output file.\n";

    auto const state_file_name = config.tmp_dir() + "new-state.txt";
    vout << "Writing state file '" << state_file_name << "'...\n";
    auto const state = get_state(config, options, max_timestamp);

    const std::time_t now = options.with_comment() ? std::time(nullptr) : 0;
    state.write(state_file_name, now);
    state.write(state_file_name + ".copy", now);

    vout << "Wrote and synced state file.\n";

    osmium::MemoryUsage const mem;
    vout << "Current memory used: " << mem.current() << " MBytes\n";
    vout << "Peak memory used: " << mem.peak() << " MBytes\n";

    if (!options.dry_run()) {
        std::string const lock_file_path{config.tmp_dir() +
                                         "osmdbt-create-diff.lock"};
        write_lock_file(lock_file_path, state, read_log_files);
        sync_dir(config.tmp_dir());

        vout << "Creating directories...\n";
        std::filesystem::create_directories(config.changes_dir() +
                                            state.dir2_path());

        vout << "Moving files into their final locations...\n";
        std::filesystem::rename(new_change_file_name + ".gz",
                                config.changes_dir() + state.osc_path() +
                                    ".gz");

        if (options.with_pbf_output()) {
            std::filesystem::rename(new_change_file_name + ".pbf",
                                    config.changes_dir() + state.osc_path() +
                                        ".pbf");
        }

        std::filesystem::rename(config.tmp_dir() + "new-state.txt",
                                config.changes_dir() + state.state_path());
        sync_dir(config.changes_dir() + state.dir2_path());
        sync_dir(config.changes_dir() + state.dir1_path());

        std::filesystem::rename(config.tmp_dir() + "new-state.txt.copy",
                                config.changes_dir() + "state.txt");
        sync_dir(config.changes_dir());

        for (auto const &log_file : read_log_files) {
            vout << "Renaming log file '" << log_file << "'...\n";
            rename_file(config.log_dir() + log_file,
                        config.log_dir() + log_file + ".done");
        }
        sync_dir(config.log_dir());

        ::unlink(lock_file_path.c_str());
        sync_dir(config.tmp_dir());
    }

    vout << "All done.\n";

    vout << "Done.\n";

    return true;
}

} // anonymous namespace

int main(int argc, char *argv[])
{
    CreateDiffOptions options;
    return app_wrapper(options, argc, argv);
}
