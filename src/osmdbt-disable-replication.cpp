
#include "config.hpp"
#include "options.hpp"
#include "util.hpp"

#include <osmium/util/verbose_output.hpp>

#include <pqxx/pqxx>

#include <iostream>

static Command command_disable_replication = {
    "disable-replication", "[OPTIONS]", "Disable replication on the database."};

int main(int argc, char *argv[])
{
    try {
        Options options{command_disable_replication};
        options.parse_command_line(argc, argv);
        osmium::VerboseOutput vout{!options.quiet()};
        options.show_version(vout);

        vout << "Reading config from '" << options.config_file() << "'\n";
        Config config{options, vout};

        vout << "Connecting to database...\n";
        pqxx::connection db{config.db_connection()};
        db.prepare("disable-replication",
                   "SELECT * FROM pg_drop_replication_slot($1);");

        pqxx::work txn{db};
        pqxx::result const result =
            txn.prepared("disable-replication")(config.replication_slot())
                .exec();

        if (result.size() == 1 && result[0][0].c_str()[0] == '\0') {
            vout << "Replication disabled.\n";
        }

        txn.commit();

        vout << "Done.\n";
    } catch (std::exception const &e) {
        std::cerr << e.what() << '\n';
        return 2;
    }

    return 0;
}
