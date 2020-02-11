
#include "config.hpp"
#include "options.hpp"
#include "util.hpp"

#include <osmium/util/verbose_output.hpp>

#include <pqxx/pqxx>

#include <iostream>

static Command command_enable_replication = {
    "enable-replication", "[OPTIONS]", "Enable replication on the database."};

int main(int argc, char *argv[])
{
    try {
        auto const options =
            parse_command_line(argc, argv, command_enable_replication);
        osmium::VerboseOutput vout{!options.quiet};

        vout << "Reading config from '" << options.config_file << "'\n";
        Config config{options, vout};

        vout << "Connecting to database...\n";
        pqxx::connection db{config.db_connection()};
        db.prepare("enable-replication",
                   "SELECT * FROM pg_create_logical_replication_slot($1, "
                   "'osm-logical');");

        pqxx::work txn{db};
        pqxx::result const result =
            txn.prepared("enable-replication")(config.replication_slot())
                .exec();

        if (result.size() == 1 &&
            result[0][0].c_str() == config.replication_slot()) {
            vout << "Replication enabled.\n";
        }

        txn.commit();

        vout << "Done.\n";
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    return 0;
}
