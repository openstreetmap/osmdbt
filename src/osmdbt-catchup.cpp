
#include "config.hpp"

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
        auto const options = parse_command_line(argc, argv, command_catchup);
        osmium::VerboseOutput vout{!options.quiet};

        vout << "Reading config from '" << options.config_file << "'\n";
        Config config{options, vout};

        vout << "Connecting to database...\n";
        pqxx::connection db{config.db_connection()};

        // the pg_replication_slot_advance() function is only available in PostgreSQL 11 and beyond.
        //db.prepare("advance", "SELECT * FROM pg_replication_slot_advance($1, CAST ($2 AS pg_lsn));");
        db.prepare("advance", "SELECT * FROM pg_logical_slot_get_changes($1, "
                              "CAST ($2 AS pg_lsn), NULL);");

        pqxx::work txn{db};

        vout << "Catching up...\n";
        pqxx::result result =
            txn.prepared("advance")(config.replication_slot())(lsn).exec();
        /*
        if (r.size() != 1) {
            vout << "Error.\n";
            return 1;
        }

        vout << "Result: " << r[0][0].c_str() << ' '
                           << r[0][1].c_str() << ' '
                           << r[0][2].c_str() << '\n';
*/
        txn.commit();

        vout << "Done.\n";
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    return 0;
}
