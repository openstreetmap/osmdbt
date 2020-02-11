
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

        pqxx::work txn{db};
        int version = 0;
        {
            pqxx::result const result = txn.exec("SELECT * FROM version();");
            if (result.size() != 1) {
                throw std::runtime_error{
                    "Database error (version): Expected exactly one result"};
            }

            if (std::strncmp(result[0][0].c_str(), "PostgreSQL", 10)) {
                throw std::runtime_error{
                    "Database error: Expected version string"};
            }

            version = std::atoi(result[0][0].c_str() + 11);
            vout << "Database version: " << version << '\n';
        }

        if (version >= 11) {
            db.prepare("advance",
                       "SELECT * FROM pg_replication_slot_advance($1, CAST ($2 "
                       "AS pg_lsn));");
        } else {
            db.prepare("advance",
                       "SELECT * FROM pg_logical_slot_get_changes($1, "
                       "CAST ($2 AS pg_lsn), NULL);");
        }

        vout << "Catching up...\n";
        pqxx::result const result =
            txn.prepared("advance")(config.replication_slot())(lsn).exec();

        vout << "Result size=" << result.size() << ": " << result[0][0].c_str()
             << ' ' << result[0][1].c_str() << '\n';

        txn.commit();

        vout << "Done.\n";
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    return 0;
}
