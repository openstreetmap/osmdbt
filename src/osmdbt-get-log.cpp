
#include "config.hpp"
#include "db.hpp"
#include "exception.hpp"
#include "io.hpp"
#include "options.hpp"
#include "util.hpp"

#include <osmium/io/detail/read_write.hpp>
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
}; // class GetLogOptions

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

    rename_file(file_name_new, file_name_final);
    sync_dir(dir_name);
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

bool app(osmium::VerboseOutput &vout, Config const &config,
         GetLogOptions const &options)
{
    PIDFile pid_file{config.run_dir(), "osmdbt-get-log"};

    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};
    db.prepare("peek",
               "SELECT * FROM pg_logical_slot_peek_changes($1, NULL, NULL);");

    pqxx::work txn{db};
    vout << "Database version: " << get_db_version(txn) << '\n';

    vout << "Reading replication log...\n";
    pqxx::result const result =
        txn.prepared("peek")(config.replication_slot()).exec();

    if (result.empty()) {
        vout << "No changes found.\n";
        vout << "Did not write log file.\n";
        txn.commit();
        vout << "Done.\n";
        return false;
    }

    vout << "There are " << result.size() << " entries in replication log.\n";

    std::string data;
    data.reserve(result.size() * 50); // log lines should fit in 50 bytes

    std::string lsn;

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
        std::string lsn_dash;
        std::transform(lsn.cbegin(), lsn.cend(), std::back_inserter(lsn_dash),
                    [](char c) { return c == '/' ? '-' : c; });

        std::string file_name = "/osm-repl-";
        file_name += get_time();
        file_name += '-';
        file_name += lsn_dash;
        file_name += ".log";
        vout << "Writing log to '" << config.log_dir() << file_name << "'...\n";

        write_data_to_file(data, config.log_dir(), file_name);
        vout << "Wrote and synced log.\n";
    } else {
        vout << "No actual changes found.\n";
        vout << "Did not write log file.\n";
    }

    if (options.catchup()) {
        vout << "Catching up to " << lsn << "...\n";
        catchup_to_lsn(txn, config.replication_slot(), lsn);
    } else {
        vout << "Not catching up (use --catchup if you want this).\n";
    }

    txn.commit();

    vout << "Done.\n";

    return has_actual_data;
}

int main(int argc, char *argv[])
{
    GetLogOptions options;
    return app_wrapper(options, argc, argv);
}
