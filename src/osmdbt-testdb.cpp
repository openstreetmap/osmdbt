
#include "config.hpp"
#include "options.hpp"
#include "util.hpp"

#include <osmium/util/verbose_output.hpp>

#include <pqxx/pqxx>

#include <iostream>

static Command command_testdb = {"testdb", "[OPTIONS]",
                                 "Test connection to the database."};

int main(int argc, char *argv[])
{
    try {
        auto const options = parse_command_line(argc, argv, command_testdb);
        osmium::VerboseOutput vout{!options.quiet};

        show_version(vout, command_testdb);

        vout << "Reading config from '" << options.config_file << "'\n";
        Config config{options, vout};

        vout << "Connecting to database...\n";
        pqxx::connection db{config.db_connection()};
        pqxx::work txn{db};

        {
            pqxx::result result = txn.exec("SELECT * FROM version();");

            if (result.size() != 1) {
                throw std::runtime_error{
                    "Database error: Expected exactly one result\n"};
            }

            if (std::strncmp(result[0][0].c_str(), "PostgreSQL", 10)) {
                throw std::runtime_error{
                    "Database error: Expected version string\n"};
            }

            vout << "Database version: " << result[0][0] << '\n';
        }
        pqxx::result result = txn.exec("SELECT slot_name, database, confirmed_flush_lsn FROM pg_replication_slots WHERE slot_type = 'logical' AND plugin = 'osm-logical';");

        if (result.empty()) {
            vout << "Replication not enabled\n";
        } else {
            vout << "Active replication slots:\n";
            for (auto const &row : result) {
                vout << "  name=" << row[0] << " db=" << row[1] << " lsn=" << row[2] << '\n';
            }
        }

        txn.commit();

        vout << "Done.\n";
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    return 0;
}
