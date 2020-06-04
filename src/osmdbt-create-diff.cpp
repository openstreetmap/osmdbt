
#include "config.hpp"
#include "db.hpp"
#include "exception.hpp"
#include "io.hpp"
#include "options.hpp"
#include "osmobj.hpp"
#include "state.hpp"
#include "util.hpp"
#include "version.hpp"

#include <osmium/io/gzip_compression.hpp>
#include <osmium/io/xml_output.hpp>
#include <osmium/memory/buffer.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/string.hpp>
#include <osmium/util/verbose_output.hpp>

#include <boost/filesystem.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class CreateDiffOptions : public Options
{
public:
    CreateDiffOptions()
    : Options("create-diff", "Create replication diff files from log file.")
    {}

    std::vector<std::string> const &log_file_names() const noexcept
    {
        return m_log_file_names;
    }

    std::vector<std::string> &log_file_names() noexcept
    {
        return m_log_file_names;
    }

    std::size_t init_state() const noexcept { return m_init_state; }

    bool dry_run() const noexcept { return m_dry_run; }

private:
    void add_command_options(po::options_description &desc) override
    {
        po::options_description opts_cmd{"COMMAND OPTIONS"};

        // clang-format off
        opts_cmd.add_options()
            ("log-file,f", po::value<std::vector<std::string>>(), "Read specified log file")
            ("sequence-number,s", po::value<std::size_t>(), "Initialize state with specified value")
            ("dry-run,n", "Dry-run, only create files in tmp dir");
        // clang-format on

        desc.add(opts_cmd);
    }

    void check_command_options(
        boost::program_options::variables_map const &vm) override
    {
        if (vm.count("log-file")) {
            m_log_file_names = vm["log-file"].as<std::vector<std::string>>();
        }
        if (vm.count("sequence-number")) {
            m_init_state = vm["sequence-number"].as<std::size_t>();
        }
        if (vm.count("dry-run")) {
            m_dry_run = true;
        }
    }

    std::vector<std::string> m_log_file_names;
    std::size_t m_init_state = 0;
    bool m_dry_run = false;
}; // class CreateDiffOptions

static void populate_changeset_cache(pqxx::work &txn,
                                     changeset_user_lookup &cucache)
{
    for (auto &c : cucache) {
        pqxx::result const result =
#if PQXX_VERSION_MAJOR >= 6
            txn.exec_prepared("changeset_user", c.first);
#else
            txn.prepared("changeset_user")(c.first).exec();
#endif
        if (result.size() != 1) {
            throw database_error{
                "Expected exactly one result (changeset_user)."};
        }
        c.second.id = result[0][1].as<osmium::user_id_type>();
        c.second.username = result[0][2].c_str();
    }
}

osmium::Timestamp get_timestamp(std::string const &filename)
{
    auto const ts = filename.substr(9, 20);
    return osmium::Timestamp{ts};
}

static State get_state(Config const &config, CreateDiffOptions const &options,
                       osmium::Timestamp timestamp)
{
    if (options.init_state() != 0) {
        return State{options.init_state(), timestamp};
    }

    boost::filesystem::path state_file{config.changes_dir() + "state.txt"};
    if (!boost::filesystem::exists(state_file)) {
        throw std::runtime_error{"Missing state file: '" + state_file.string() +
                                 "'"};
    }
    State state{state_file.string()};
    return state.next(timestamp);
}

static void write_lock_file(std::string const &path, State const &state,
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

bool app(osmium::VerboseOutput &vout, Config const &config,
         CreateDiffOptions const &options)
{
    changeset_user_lookup cucache;
    PIDFile pid_file{config.run_dir(), "osmdbt-create-diff"};

    std::vector<std::string> log_files = options.log_file_names();
    if (log_files.empty()) {
        vout << "No log files on command line. Looking for log files in log "
                "directory...\n";
        boost::filesystem::path p{config.log_dir()};
        for (auto const &file : boost::filesystem::directory_iterator(p)) {
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

    auto const ts = get_timestamp(log_files.back());
    auto const state = get_state(config, options, ts);

    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};

    db.prepare("changeset_user",
               "SELECT c.id, c.user_id, u.display_name FROM changesets c, "
               "users u WHERE c.user_id = u.id AND c.id = $1");

    std::string const attr =
        R"(, version, changeset_id, visible, to_char(timestamp, 'YYYY-MM-DD"T"HH24:MI:SS"Z"') AS timestamp, redaction_id)";

    db.prepare("node", "SELECT node_id" + attr +
                           ", longitude, latitude FROM nodes WHERE node_id=$1 "
                           "AND version=$2");
    db.prepare("way", "SELECT way_id" + attr +
                          " FROM ways WHERE way_id=$1 AND version=$2");
    db.prepare("relation",
               "SELECT relation_id" + attr +
                   " FROM relations WHERE relation_id=$1 AND version=$2");

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

    std::vector<osmobj> objects_todo;
    for (auto const &log_file : log_files) {
        vout << "Reading log file '" << config.log_dir() << log_file
             << "'...\n";
        read_log(objects_todo, config.log_dir(), log_file, &cucache);
        vout << "  Got " << objects_todo.size() << " objects.\n";
    }

    if (objects_todo.empty()) {
        vout << "No objects found in log files.\n";
        vout << "Done.\n";
        return true;
    }

    std::sort(objects_todo.begin(), objects_todo.end());

    vout << "Populating changeset cache...\n";
    populate_changeset_cache(txn, cucache);
    vout << "  Got " << cucache.size() << " changesets.\n";

    auto const osm_data_file_name = config.tmp_dir() + "new-change.osc.gz";
    vout << "Opening output file '" << osm_data_file_name << "'...\n";

    osmium::io::Header header;
    header.has_multiple_object_versions();
    header.set("generator",
               std::string{"osmdbt-create-diff/"} + get_osmdbt_version());

    osmium::io::Writer writer{osm_data_file_name, header,
                              osmium::io::overwrite::no,
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

    vout << "Wrote and synced output file.\n";

    auto const state_file_name = config.tmp_dir() + "new-state.txt";
    vout << "Writing state file '" << state_file_name << "'...\n";
    state.write(state_file_name);
    state.write(state_file_name + ".copy");
    vout << "Wrote and synced state file.\n";

    osmium::MemoryUsage mem;
    vout << "Current memory used: " << mem.current() << " MBytes\n";
    vout << "Peak memory used: " << mem.peak() << " MBytes\n";

    if (!options.dry_run()) {
        std::string const lock_file_path{config.tmp_dir() +
                                         "osmdbt-create-diff.lock"};
        write_lock_file(lock_file_path, state, log_files);
        sync_dir(config.tmp_dir());

        vout << "Creating directories...\n";
        boost::filesystem::create_directories(config.changes_dir() +
                                              state.dir2_path());

        vout << "Moving files into their final locations...\n";
        boost::filesystem::rename(config.tmp_dir() + "new-change.osc.gz",
                                  config.changes_dir() + state.osc_path());
        boost::filesystem::rename(config.tmp_dir() + "new-state.txt",
                                  config.changes_dir() + state.state_path());
        sync_dir(config.changes_dir() + state.dir2_path());
        sync_dir(config.changes_dir() + state.dir1_path());

        boost::filesystem::rename(config.tmp_dir() + "new-state.txt.copy",
                                  config.changes_dir() + "state.txt");
        sync_dir(config.changes_dir());

        for (auto const &log_file : log_files) {
            vout << "Renaming log file '" << log_file << "'...\n";
            rename_file(config.log_dir() + log_file,
                        config.log_dir() + log_file + ".done");
        }
        sync_dir(config.log_dir());

        ::unlink(lock_file_path.c_str());
        sync_dir(config.tmp_dir());
    }

    vout << "All done.\n";
    txn.commit();

    vout << "Done.\n";

    return true;
}

int main(int argc, char *argv[])
{
    CreateDiffOptions options;
    return app_wrapper(options, argc, argv);
}
