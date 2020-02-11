
#include "config.hpp"

#include <osmium/io/detail/read_write.hpp>
#include <osmium/util/verbose_output.hpp>

#include <pqxx/pqxx>

#include <iostream>
#include <string>

static Command command_peek = {
    "peek", "[OPTIONS]", "Write changes from replication slot to log file."};

static void write_data_to_file(std::string const &data,
                               std::string const &file_name)
{
    std::string file_name_new{file_name};
    file_name_new.append(".new");

    int const fd = osmium::io::detail::open_for_writing(
        file_name_new, osmium::io::overwrite::no);

    osmium::io::detail::reliable_write(fd, data.data(), data.size());
    osmium::io::detail::reliable_fsync(fd);
    osmium::io::detail::reliable_close(fd);

    if (rename(file_name_new.c_str(), file_name.c_str()) != 0) {
        throw std::system_error{errno, std::system_category(),
                                std::string{"Rename failed for '"} + file_name +
                                    "'"};
    }
}

static void run(pqxx::work &txn, osmium::VerboseOutput &vout,
                Config const &config)
{
    std::string data;
    std::string lsn;

    pqxx::result result =
        txn.prepared("peek")(config.replication_slot()).exec();

    if (result.empty()) {
        vout << "No changes found.\n";
        return;
    }

    vout << "There are " << result.size() << " changes.\n";

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

    auto const pos = lsn.find_first_of('/');
    if (pos != std::string::npos) {
        lsn[pos] = '-';
    }

    std::string file_name = "/tmp/xx-repldiff-" + lsn + ".log";
    vout << "Writing log to '" << file_name << "'\n";

    write_data_to_file(data, file_name);
}

int main(int argc, char *argv[])
{
    try {
        auto const options = parse_command_line(argc, argv, command_peek);
        osmium::VerboseOutput vout{!options.quiet};

        vout << "Reading config from '" << options.config_file << "'\n";
        Config config{options, vout};

        vout << "Connecting to database...\n";
        pqxx::connection db{config.db_connection()};
        db.prepare(
            "peek",
            "SELECT * FROM pg_logical_slot_peek_changes($1, NULL, NULL);");

        pqxx::work txn{db};

        vout << "Reading changes...\n";
        run(txn, vout, config);

        txn.commit();

        vout << "Done.\n";
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    return 0;
}
