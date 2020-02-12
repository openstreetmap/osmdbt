
#include "config.hpp"
#include "db.hpp"

#include <osmium/io/detail/read_write.hpp>
#include <osmium/util/verbose_output.hpp>

#include <algorithm>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static Command command_peek = {
    "peek", "Write changes from replication slot to log file."};

class PeekOptions : public Options
{
public:
    PeekOptions() : Options(command_peek) {}

    bool catchup() const noexcept { return m_catchup; }

private:
    void add_command_options(po::options_description &desc) override
    {
        po::options_description opts_cmd{"COMMAND OPTIONS"};

        // clang-format off
        opts_cmd.add_options()
            ("catchup", "Commit changes when they have been logged successfully");
        // clang-format on

        desc.add(opts_cmd);
    }

    void check_command_options(
        boost::program_options::variables_map const &vm) override
    {
        if (vm.count("catchup")) {
            m_catchup = true;
        }
    }

    bool m_catchup = false;
}; // class PeekOptions

static void write_data_to_file(std::string const &data,
                               std::string const &dir_name,
                               std::string const &file_name)
{
    std::string const file_name_final{dir_name + file_name};
    std::string const file_name_new{file_name_final + ".new"};

    int const fd = osmium::io::detail::open_for_writing(
        file_name_new, osmium::io::overwrite::no);

    osmium::io::detail::reliable_write(fd, data.data(), data.size());
    osmium::io::detail::reliable_fsync(fd);
    osmium::io::detail::reliable_close(fd);

    if (rename(file_name_new.c_str(), file_name_final.c_str()) != 0) {
        std::string msg{"Rename failed for '"};
        msg += file_name_new;
        msg += "'";
        throw std::system_error{errno, std::system_category(), msg};
    }

    int const dir_fd = open(dir_name.c_str(), O_DIRECTORY);
    if (dir_fd == -1) {
        std::string msg{"Opening output directory failed for '"};
        msg += dir_name;
        msg += "'";
        throw std::system_error{errno, std::system_category(), msg};
    }

    if (fsync(dir_fd) != 0) {
        std::string msg{"Syncing output directory failed for '"};
        msg += dir_name;
        msg += "'";
        throw std::system_error{errno, std::system_category(), msg};
    }

    osmium::io::detail::reliable_close(dir_fd);
}

static std::string get_time()
{
    std::string buffer(20, '\0');

    auto const t = std::time(nullptr);
    auto const num = std::strftime(&buffer[0], buffer.size(), "%Y%m%dT%H%M%S",
                                   std::localtime(&t));

    buffer.resize(num);
    assert(num == 15);

    return buffer;
}

static void run(pqxx::work &txn, osmium::VerboseOutput &vout,
                Config const &config, bool catchup)
{
    pqxx::result const result =
        txn.prepared("peek")(config.replication_slot()).exec();

    if (result.empty()) {
        vout << "No changes found.\n";
        return;
    }

    vout << "There are " << result.size() << " changes.\n";

    std::string data;
    data.reserve(result.size() * 50); // log lines should fit in 50 bytes

    std::string lsn;

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
        }
    }

    vout << "LSN is " << lsn << '\n';

    std::string lsn_dash;
    std::transform(lsn.cbegin(), lsn.cend(), std::back_inserter(lsn_dash),
                   [](char c) { return c == '/' ? '-' : c; });

    std::string file_name = "/osm-repl-";
    file_name += get_time();
    file_name += '-';
    file_name += lsn_dash;
    file_name += ".log";
    vout << "Writing log to '" << config.dir() << file_name << "'...\n";

    write_data_to_file(data, config.dir(), file_name);
    vout << "Wrote and synced log.\n";

    if (catchup) {
        vout << "Catching up to " << lsn << "...\n";
        catchup_to_lsn(txn, config.replication_slot(), lsn);
    } else {
        vout << "Not catching up (use --catchup if you want this).\n";
    }
}

int main(int argc, char *argv[])
{
    try {
        PeekOptions options;
        options.parse_command_line(argc, argv);
        osmium::VerboseOutput vout{!options.quiet()};
        options.show_version(vout);

        vout << "Reading config from '" << options.config_file() << "'\n";
        Config config{options, vout};

        vout << "Connecting to database...\n";
        pqxx::connection db{config.db_connection()};
        db.prepare(
            "peek",
            "SELECT * FROM pg_logical_slot_peek_changes($1, NULL, NULL);");

        pqxx::work txn{db};
        vout << "Database version: " << get_db_version(txn) << '\n';

        vout << "Reading changes...\n";
        run(txn, vout, config, options.catchup());

        txn.commit();

        vout << "Done.\n";
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    return 0;
}
