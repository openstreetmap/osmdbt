
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
        pqxx::result r = txn.exec("SELECT * FROM version();");

        if (r.size() != 1) {
            throw std::runtime_error{
                "Database error: Expected exactly one result\n"};
        }

        if (std::strncmp(r[0][0].as<char const *>(), "PostgreSQL", 10)) {
            throw std::runtime_error{
                "Database error: Expected version string\n"};
        }

        vout << "Database version: " << r[0][0] << '\n';

        txn.commit();

        vout << "Done.\n";
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    return 0;
}
