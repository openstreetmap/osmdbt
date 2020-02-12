
#include "config.hpp"
#include "db.hpp"

#include <osmium/io/detail/read_write.hpp>
#include <osmium/util/verbose_output.hpp>

#include <pqxx/pqxx>

#include <iostream>
#include <string>

static Command command_catchup = {"catchup", "[OPTIONS] LSN",
                                  "Advance replication slot to specified LSN."};

int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: osmdbt-catchup [OPTIONS] LSN\n";
        return 2;
    }

    std::string const lsn{argv[1]};
    try {
        Options options{command_catchup};
        options.parse_command_line(argc, argv);
        osmium::VerboseOutput vout{!options.quiet()};
        options.show_version(vout);

        vout << "Reading config from '" << options.config_file() << "'\n";
        Config config{options, vout};

        vout << "Connecting to database...\n";
        pqxx::connection db{config.db_connection()};

        pqxx::work txn{db};
        int const version = get_db_version(txn);
        vout << "Database version: " << version << '\n';

        vout << "Catching up to " << lsn << "...\n";
        catchup_to_lsn(txn, version, config.replication_slot(), lsn);

        txn.commit();

        vout << "Done.\n";
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    return 0;
}
