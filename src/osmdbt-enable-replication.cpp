
#include "config.hpp"
#include "db.hpp"
#include "options.hpp"
#include "util.hpp"

#include <osmium/util/verbose_output.hpp>

#include <iostream>

bool app(osmium::VerboseOutput &vout, Config const &config,
         Options const & /*options*/)
{
    vout << "Connecting to database...\n";
    pqxx::connection db{config.db_connection()};
    db.prepare("enable-replication",
               "SELECT * FROM pg_create_logical_replication_slot($1, "
               "'osm-logical');");

    pqxx::work txn{db};
    vout << "Database version: " << get_db_version(txn) << '\n';

    pqxx::result const result =
#if PQXX_VERSION_MAJOR >= 6
        txn.exec_prepared("enable-replication", config.replication_slot());
#else
        txn.prepared("enable-replication")(config.replication_slot()).exec();
#endif

    if (result.size() == 1 &&
        result[0][0].c_str() == config.replication_slot()) {
        vout << "Replication enabled.\n";
    }

    txn.commit();

    vout << "Done.\n";

    return true;
}

int main(int argc, char *argv[])
{
    Options options{"enable-replication",
                    "Enable replication on the database."};

    return app_wrapper(options, argc, argv);
}
