
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
    db.prepare("disable-replication",
               "SELECT * FROM pg_drop_replication_slot($1);");

    pqxx::work txn{db};
    vout << "Database version: " << get_db_version(txn) << '\n';

    pqxx::result const result =
        txn.prepared("disable-replication")(config.replication_slot()).exec();

    if (result.size() == 1 && result[0][0].c_str()[0] == '\0') {
        vout << "Replication disabled.\n";
    }

    txn.commit();

    vout << "Done.\n";

    return true;
}

int main(int argc, char *argv[])
{
    Options options{"disable-replication",
                    "Disable replication on the database."};

    return app_wrapper(options, argc, argv);
}
